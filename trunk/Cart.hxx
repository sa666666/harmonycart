//=========================================================================
//
//  H   H    A    RRRR   M   M   OOO   N   N  Y   Y
//  H   H   A A   R   R  MM MM  O   O  NN  N   Y Y
//  HHHHH  AAAAA  RRRR   M M M  O   O  N N N    Y   "Harmony Cart software"
//  H   H  A   A  R R    M   M  O   O  N  NN    Y
//  H   H  A   A  R  R   M   M   OOO   N   N    Y
//
// Copyright (c) 2009 by Stephen Anthony <stephena@users.sourceforge.net>
//
// See the file "License.txt" for information on usage and redistribution
// of this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//=========================================================================

#ifndef __CART_HXX
#define __CART_HXX

#define MAXCARTSIZE 32*1024

#include <vector>

#include "bspf.hxx"

#include "BSType.hxx"
#include "SerialPort.hxx"

enum TARGET           { PHILIPS_ARM, ANALOG_DEVICES_ARM };
enum TARGET_MODE      { PROGRAM_MODE, RUN_MODE          };
enum FILE_FORMAT_TYPE { FORMAT_BINARY, FORMAT_HEX       };

    typedef struct {
      TARGET micro;                                // The type of micro that will be programmed.
      FILE_FORMAT_TYPE FileFormat;
      unsigned char ProgramChip;                // Normally set

      int debug_level;
      unsigned char ControlLines;
      unsigned char ControlLinesSwapped;
      unsigned char ControlLinesInverted;
      unsigned char LogFile;
      char *input_file;                   // The name of the file to get input from.
      char *serial_port;                  // Name of the serial port to use to
                                          // communicate with the microcontroller.
                                          // Read from the command line.

      unsigned char TerminalOnly;         // Declared here for lazyness saves ifdef's

      unsigned char HalfDuplex;           // Only used for LPC Programming
      unsigned char DetectOnly;
      unsigned char WipeDevice;
      unsigned char Verify;
      int           DetectedDevice;       /* index in LPCtypes[] array */
      char *baud_rate;                    /**< Baud rate to use on the serial
                                             * port communicating with the
                                             * microcontroller. Read from the
                                             * command line.                        */

      char StringOscillator[6];           /**< Holds representation of oscillator
                                             * speed from the command line.         */

      uInt8* FileContent;
      uInt8* BinaryContent;              /**< Binary image of the                  */
                                          /* microcontroller's memory.            */
      unsigned long BinaryLength;
      unsigned long BinaryOffset;
      unsigned long StartAddress;
      unsigned long BinaryMemSize;
    } ISP_ENVIRONMENT;


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

  public:
    /**
      Attempt to locate the Harmony cart on the given serial port.  If
      found, return the version info, otherwise return the empty string.
    */
    string autodetectHarmony(SerialPort& port);

    /**
      Resets the target leaving it in either download (program) mode or
      run mode.

      @param mode  The mode to leave the target in
    */
    void resetTarget(SerialPort& port, TARGET_MODE mode);

    /**
      Loads EEPROM loader BIOS data from the given filename.
      The filename should exist and be readable.
    */
    bool updateBIOS(const string& filename);


    /**
      Loads cartridge data from the given filename, creating a cart.
      The bankswitch type is autodetected if type is "".
      The filename should exist and be readable.
    */
    bool create(const string& filename, const string& type = "");

    //////////////////////////////////////////////////////////////////
    //  The following two methods act as an iterator through all the
    //  sectors making up the current Cart.
    //////////////////////////////////////////////////////////////////
    /**
      Initializes the sector iterator to the beginning of the list,
      in preparation for multiple calls to writeNextSector() or
      verifyNextSector().

      NOTE: After calling initSectors(), DO NOT mix calls to
            writeNextSector() and verifyNextSector().

      @return  The number of sectors that need to be accessed
    */
    uInt16 initSectors();

    /**
      Write the next sector in the iterator to the serial port,
      returning the actual sector number that was written.

      NOTE: After calling initSectors(), DO NOT mix calls to
            writeNextSector() and verifyNextSector().

      @return  The sector number written; an exception is thrown
               on any errors    */
    uInt16 writeNextSector(SerialPort& port);

    /**
      Read and verify the next sector in the iterator from the serial port,
      returning the actual sector number that was verified.

      NOTE: After calling initSectors(), DO NOT mix calls to
            writeNextSector() and verifyNextSector().

      @return  The sector number verified; an exception is thrown
               on any errors
    */
    uInt16 verifyNextSector(SerialPort& port);

    /** Accessor and mutator for bankswitch type. */
    BSType getBSType()            { return myType; }
    void   setBSType(BSType type) { myType = type; }

    /** Set number of write retries before bailing out. */
    void setRetry(int retry) { myRetry = retry; }

    /** Get the current cart size. */
    uInt32 getSize() { return myCartSize; }

    /** Was the ROM loaded correctly? */
    bool isValid() { return myIsValid; }

    /** Auxiliary method to autodetect the bankswitch type. */
    static BSType autodetectType(uInt8* data, uInt32 size);

  private:
    /**
      Read data from given file and place it in the given buffer.
      The bankswitch type is also autodetected here.
    */
    int readFile(const string& filename, uInt8* cartridge, uInt32 maxSize,
                 const string& type);

    /**
      Write the given sector to the serial port.
    */
    bool downloadSector(uInt32 sector, SerialPort& port);

    /**
      Read and verify the given sector from the serial port.
    */
    bool verifySector(uInt32 sector, SerialPort& port);

  private:
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
      LPC_RAMSTART_LPC2XXX = 0x40000000,
      LPC_RAMBASE_LPC2XXX  = 0x40000200,
      LPC_RAMSTART_LPC17XX = 0x10000000,
      LPC_RAMBASE_LPC17XX  = 0x10000200
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
      UNLOCK_ERROR       = 0x1100,   /* return value is 0x1100 + philips ISP returned value (0 to 255) */
      WRONG_ANSWER_PREP  = 0x1200,   /* return value is 0x1200 + philips ISP returned value (0 to 255) */
      WRONG_ANSWER_ERAS  = 0x1300,   /* return value is 0x1300 + philips ISP returned value (0 to 255) */
      WRONG_ANSWER_WRIT  = 0x1400,   /* return value is 0x1400 + philips ISP returned value (0 to 255) */
      WRONG_ANSWER_PREP2 = 0x1500,   /* return value is 0x1500 + philips ISP returned value (0 to 255) */
      WRONG_ANSWER_COPY  = 0x1600,   /* return value is 0x1600 + philips ISP returned value (0 to 255) */
      FAILED_RUN         = 0x1700    /* return value is 0x1700 + philips ISP returned value (0 to 255) */
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

    enum CHIP_VARIANT { CHIP_VARIANT_LPC2XXX, CHIP_VARIANT_LPC17XX, CHIP_VARIANT_UNKNOWN };

    struct LPC_DEVICE_TYPE {
      uInt32 id;
      uInt32 Product;
      uInt32 FlashSize;          /* in kiB, for informational purposes only */
      uInt32 RAMSize;            /* in kiB, for informational purposes only */
      uInt32 FlashSectors;       /* total number of sectors */
      uInt32 MaxCopySize;        /* maximum size that can be copied to Flash in a single command */
      const uInt32* SectorTable; /* pointer to a sector table with constant the sector sizes */
      CHIP_VARIANT ChipVariant;
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
    static LPC_DEVICE_TYPE LPCtypes[52];

    const char* lpc_PhilipsChipVersion(SerialPort& port, const string& oscillator);
    int lpc_PhilipsDownload(SerialPort& port);
    unsigned long ReturnValueLpcRamStart(ISP_ENVIRONMENT *IspEnvironment);
    unsigned long ReturnValueLpcRamBase(ISP_ENVIRONMENT *IspEnvironment);

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
    uInt8  myCart[MAXCARTSIZE];
    uInt32 myCartSize;
    uInt32 myRetry;
    BSType myType;

    // The following keep track of progress of sector writes
    uInt16 myCurrentSector;
    uInt16 myNumSectors;

    bool myIsValid;
};

#endif
