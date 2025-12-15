//=========================================================================
//
//  H   H    A    RRRR   M   M   OOO   N   N  Y   Y
//  H   H   A A   R   R  MM MM  O   O  NN  N   Y Y
//  HHHHH  AAAAA  RRRR   M M M  O   O  N N N    Y   "Harmony Cart software"
//  H   H  A   A  R R    M   M  O   O  N  NN    Y
//  H   H  A   A  R  R   M   M   OOO   N   N    Y
//
// Copyright (c) 2009-2026 by Stephen Anthony <sa666666@gmail.com>
//
// See the file "License.txt" for information on usage and redistribution
// of this file, and for a DISCLAIMER OF ALL WARRANTIES.
//=========================================================================

#ifndef CART_PROGRAMMER_HXX
#define CART_PROGRAMMER_HXX

class SerialPort;

#include "bspf.hxx"
#include "Progress.hxx"

/**
  Cart programming routines for the Harmony cart.

  For now, this uses functionality from the lpc21isp utilities
    https://github.com/capiman/lpc21isp

  @author Stephen Anthony for the wrapper, lpc21isp utilities for actual
          functionality.
*/
class CartProgrammer
{
  public:
    CartProgrammer() = default;
    ~CartProgrammer() = default;

    /**
      Resets the target to program/download mode.
    */
    void reset(SerialPort& port);

    /** Set number of write retries before bailing out. */
    void setConnectionAttempts(uInt32 attempt) { myConnectionAttempts = attempt; }

    /** Set number of write retries before bailing out. */
    void setRetry(uInt32 retry) { myRetry = retry; }

    /**
      Log all output to the given stream.
    */
    void setLogger(ostream* out) { myLog = out; }

  public:
    string chipVersion(SerialPort& port);
    string download(SerialPort& port, uInt8* data, uInt32 size,
                    Progress& progress, bool verify, bool continueOnError);

  private:
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
    uInt8 lpc_GetAndReportErrorNumber(const char* Answer);

    /**
      Deal with commands that are variously terminated with either <CR><LF>
      or only <LF>.

      @param In   Pointer to input buffer
      @param Out  Pointer to output buffer
    */
    void lpc_FormatCommand(const char* In, char* Out);

    uInt32 lpc_ReturnValueLpcRamStart() const;
    uInt32 lpc_ReturnValueLpcRamBase() const;

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
      LPC_RAMSTART_LPC43XX = 0x10000000L,
      LPC_RAMBASE_LPC43XX  = 0x10000200L,

      LPC_RAMSTART_LPC2XXX = 0x40000000L,
      LPC_RAMBASE_LPC2XXX  = 0x40000200L,

      LPC_RAMSTART_LPC18XX = 0x10000000L,
      LPC_RAMBASE_LPC18XX  = 0x10000200L,

      LPC_RAMSTART_LPC17XX = 0x10000000L,
      LPC_RAMBASE_LPC17XX  = 0x10000200L,

      LPC_RAMSTART_LPC13XX = 0x10000000L,
      LPC_RAMBASE_LPC13XX  = 0x10000300L,

      LPC_RAMSTART_LPC11XX = 0x10000000L,
      LPC_RAMBASE_LPC11XX  = 0x10000300L,

      LPC_RAMSTART_LPC8XX  = 0x10000000L,
      LPC_RAMBASE_LPC8XX   = 0x10000270L
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
      UNKNOWN_LPC        = 0x100B,   /* Unknown LPC detected */
      UNLOCK_ERROR       = 0x1100,   /* return value is 0x1100 + NXP ISP returned value (0 to 255) */
      WRONG_ANSWER_PREP  = 0x1200,   /* return value is 0x1200 + NXP ISP returned value (0 to 255) */
      WRONG_ANSWER_ERAS  = 0x1300,   /* return value is 0x1300 + NXP ISP returned value (0 to 255) */
      WRONG_ANSWER_WRIT  = 0x1400,   /* return value is 0x1400 + NXP ISP returned value (0 to 255) */
      WRONG_ANSWER_PREP2 = 0x1500,   /* return value is 0x1500 + NXP ISP returned value (0 to 255) */
      WRONG_ANSWER_COPY  = 0x1600,   /* return value is 0x1600 + NXP ISP returned value (0 to 255) */
      FAILED_RUN         = 0x1700,   /* return value is 0x1700 + NXP ISP returned value (0 to 255) */
      WRONG_ANSWER_BTBNK = 0x1800    /* return value is 0x1800 + NXP ISP returned value (0 to 255) */
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
      CHIP_VARIANT_LPC43XX,
      CHIP_VARIANT_LPC2XXX,
      CHIP_VARIANT_LPC18XX,
      CHIP_VARIANT_LPC17XX,
      CHIP_VARIANT_LPC13XX,
      CHIP_VARIANT_LPC11XX,
      CHIP_VARIANT_LPC8XX
    };

    struct LPC_DEVICE_TYPE {
      const uInt32 id;
      const uInt32 id2;
      const uInt32 EvalId2;
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
    // Used for LPC11xx devices
    static const uInt32 SectorTable_11xx[32];
    // Used for LPC17xx devices
    static const uInt32 SectorTable_17xx[30];
    // Used for LPC18xx devices
    static const uInt32 SectorTable_18xx[15];
    // Used for LPC43xx devices
    static const uInt32 SectorTable_43xx[15];
    // Used for LPC8xx devices
    static const uInt32 SectorTable_8xx[16];
    static uInt32 SectorTable_RAM[1];
    static LPC_DEVICE_TYPE LPCtypes[190];

  private:
    uInt32 myDetectedDevice{0};
    uInt32 myConnectionAttempts{0};
    uInt32 myRetry{0};
    string myOscillator{"10000"};

    ostream* myLog{&cout};

  private:
    // Following constructors and assignment operators not supported
    CartProgrammer(const CartProgrammer&) = delete;
    CartProgrammer(CartProgrammer&&) = delete;
    CartProgrammer& operator=(const CartProgrammer&) = delete;
    CartProgrammer& operator=(CartProgrammer&&) = delete;
};

#endif
