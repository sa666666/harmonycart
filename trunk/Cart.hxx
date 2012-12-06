//=========================================================================
//
//  H   H    A    RRRR   M   M   OOO   N   N  Y   Y
//  H   H   A A   R   R  MM MM  O   O  NN  N   Y Y
//  HHHHH  AAAAA  RRRR   M M M  O   O  N N N    Y   "Harmony Cart software"
//  H   H  A   A  R R    M   M  O   O  N  NN    Y
//  H   H  A   A  R  R   M   M   OOO   N   N    Y
//
// Copyright (c) 2009-2012 by Stephen Anthony <stephena@users.sf.net>
//
// See the file "License.txt" for information on usage and redistribution
// of this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//=========================================================================

#ifndef __CART_HXX
#define __CART_HXX

#define MAXCARTSIZE 32*1024

class QProgressDialog;

#include <vector>

#include "bspf_harmony.hxx"

#include "BSType.hxx"
#include "SerialPort.hxx"

/**
 *
 * @author stephena
 */
class Cart
{
  public:
    /**
      Create a new Cart object, which can be used to create single
      cartridges to upload BIOS files.
    */
    Cart();
    ~Cart();

    /**
      Log all output to the given stream.
    */
    void setLogger(ostream* out);

  public:
    /**
      Attempt to locate the Harmony cart on the given serial port.  If
      found, return the version info, otherwise return the empty string.
    */
    string autodetectHarmony(SerialPort& port);

    /**
      Loads EEPROM loader BIOS data from the given filename.
      The filename should exist and be readable.
    */
    string downloadBIOS(SerialPort& port, const string& filename,
                        bool verify, bool showprogress = true);

    /**
      Loads ROM cartridge data from the given filename, creating a cart.
      The bankswitch type is autodetected if type is "".
      The filename should exist and be readable.
    */
    string downloadROM(SerialPort& port, const string& armpath,
                       const string& filename, BSType type,
                       bool verify, bool showprogress = true);

    /** Set number of write retries before bailing out. */
    void setRetry(int retry) { myRetry = retry + 1; }

  private:
    enum TARGET           { NXP_ARM, ANALOG_DEVICES_ARM };
    enum TARGET_MODE      { PROGRAM_MODE, RUN_MODE      };
    enum FILE_FORMAT_TYPE { FORMAT_BINARY, FORMAT_HEX   };

    /**
      Read data from given file and return it in a buffer, along
      with the allocated size.
    */
    uInt8* readFile(const string& filename, uInt32& size);

    /**
      Resets the target leaving it in either download (program) mode or
      run mode.

      @param mode  The mode to leave the target in
    */
    void resetTarget(SerialPort& port, TARGET_MODE mode);

    /**
      Compress the last bank of the given ROM image.  For now, it always
      assumes a ROM size of 32K.

      @param binary  The ROM data (assumed to be 32K)
      @return  The compressed size of the last bank (originally 4K)
    */
    uInt32 compressLastBank(uInt8* binary);

    /**
      The following functions originally come from the lpc21isp utilties,
      specifically lpc21isp.c and lpcprog.c.  See the functions themselves
      for more detailed commments.  All such functions are named starting
      with 'lpc_'.
    */
    string lpc_NxpChipVersion(SerialPort& port);
    string lpc_NxpDownload(SerialPort& port, uInt8* data, uInt32 size,
                           bool verify = false, QProgressDialog* progress = 0);
    uInt32 lpc_ReturnValueLpcRamStart();
    uInt32 lpc_ReturnValueLpcRamBase();

    /**
      Download the file from the internal memory image to the philips
      microcontroller.
    */
    int lpc_SendAndVerify(SerialPort& port, const char* Command,
                          char* AnswerBuffer, int AnswerLength);

    /**
      Find error number in string.  This will normally be the string
      returned from the microcontroller.

      @param Answer  The buffer to search for the error number
      @return   The error number found, if no linefeed found before the end
                of the string an error value of 255 is returned.  If a
                non-numeric value is found then it is printed to stdout and
                an error value of 255 is returned.
    */
    unsigned char lpc_GetAndReportErrorNumber(const char* Answer);

  private:
    uInt32   myDetectedDevice;
    uInt32   myRetry;
    string   myOscillator;
    ostream* myLog;

    /* LPC_RAMSTART, LPC_RAMBASE
     *
     * Used in PhilipsDownload() to decide whether to Flash code or just place in in RAM
     * (works for .hex files only)
     *
     * LPC_RAMSTART - the Physical start address of the SRAM
     * LPC_RAMBASE  - the base address where downloading starts.
     *                Note that any code in the .hex file that resides in 0x4000,0000 ~ 0x4000,0200
     *                will _not_ be written to the LPCs SRAM.
     *                This is due to the fact that 0x4000,0040 - 0x4000,0200 is used by the bootrom.
     *                Any interrupt vectors must be copied to 0x4000,0000 and remapped to 0x0000,0000
     *                by the startup code.
     */
    enum {
      LPC_RAMSTART_LPC2XXX = 0x40000000L,
      LPC_RAMBASE_LPC2XXX  = 0x40000200L,

      LPC_RAMSTART_LPC17XX = 0x10000000L,
      LPC_RAMBASE_LPC17XX  = 0x10000200L,

      LPC_RAMSTART_LPC13XX = 0x10000000L,
      LPC_RAMBASE_LPC13XX  = 0x10000300L,

      LPC_RAMSTART_LPC11XX = 0x10000000L,
      LPC_RAMBASE_LPC11XX  = 0x10000300L
    };

    /* Return values used by PhilipsDownload(): reserving all values from 0x1000 to 0x1FFF */
    enum {
      NO_ANSWER_WDT     = 0x1000,
      NO_ANSWER_QM      = 0x1001,
      NO_ANSWER_SYNC    = 0x1002,
      NO_ANSWER_OSC     = 0x1003,
      NO_ANSWER_RBV     = 0x1004,
      NO_ANSWER_RPID    = 0x1005,
      ERROR_WRITE_DATA  = 0x1006,
      ERROR_WRITE_CRC   = 0x1007,
      ERROR_WRITE_CRC2  = 0x1008,
      PROGRAM_TOO_LARGE = 0x1009
    };

    enum {
      USER_ABORT_SYNC    = 0x100A,   /* User aborted synchronisation process */
      UNLOCK_ERROR       = 0x1100,   /* return value is 0x1100 + NXP ISP returned value (0 to 255) */
      WRONG_ANSWER_PREP  = 0x1200,   /* return value is 0x1200 + NXP ISP returned value (0 to 255) */
      WRONG_ANSWER_ERAS  = 0x1300,   /* return value is 0x1300 + NXP ISP returned value (0 to 255) */
      WRONG_ANSWER_WRIT  = 0x1400,   /* return value is 0x1400 + NXP ISP returned value (0 to 255) */
      WRONG_ANSWER_PREP2 = 0x1500,   /* return value is 0x1500 + NXP ISP returned value (0 to 255) */
      WRONG_ANSWER_COPY  = 0x1600,   /* return value is 0x1600 + NXP ISP returned value (0 to 255) */
      FAILED_RUN         = 0x1700    /* return value is 0x1700 + NXP ISP returned value (0 to 255) */
    };

    /* LPC_FLASHMASK
     *
     * LPC_FLASHMASK - bitmask to define the maximum size of the Filesize to download.
     *                 LoadFile() will check any new segment address record (03) or extended linear
     *                 address record (04) to see if the addressed 64 kByte data block still falls
     *                 in the max. flash size.
     *                 LoadFile() will not load any files that are larger than this size.
     */
    enum { LPC_FLASHMASK =  0xFFC00000 /* 22 bits = 4 MB */ };

    enum CHIP_VARIANT {
      CHIP_VARIANT_NONE,
      CHIP_VARIANT_LPC2XXX, CHIP_VARIANT_LPC17XX,
      CHIP_VARIANT_LPC13XX, CHIP_VARIANT_LPC11XX
    };

    struct LPC_DEVICE_TYPE {
      const uInt32 id;
      const char*  Product;
      const uInt32 FlashSize;      /* in kiB, for informational purposes only */
      const uInt32 RAMSize;        /* in kiB, for informational purposes only */
            uInt32 FlashSectors;   /* total number of sectors */
            uInt32 MaxCopySize;    /* maximum size that can be copied to Flash in a single command */
      const uInt32* SectorTable;   /* pointer to a sector table with constant the sector sizes */
      const CHIP_VARIANT ChipVariant;
    };

    static const uInt32 SectorTable_210x[15];
    static const uInt32 SectorTable_2103[8];
    static const uInt32 SectorTable_2109[8];
    static const uInt32 SectorTable_211x[15];
    static const uInt32 SectorTable_212x[17];
    // Used for devices with 500K (LPC2138 and LPC2148) and
    // for devices with 504K (1 extra 4k block at the end)
    static const uInt32 SectorTable_213x[28];
    // Used for LPC17xx devices
    static const uInt32 SectorTable_17xx[30];
    static uInt32 SectorTable_RAM[1];
    static LPC_DEVICE_TYPE LPCtypes[101];

    static uInt8 ourARHeader[256];
};

#endif
