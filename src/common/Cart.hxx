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

#ifndef CART_HXX
#define CART_HXX

class SerialPort;

#include "bspf.hxx"
#include "Bankswitch.hxx"
#include "CartProgrammer.hxx"
#include "Progress.hxx"

/**
  Create a new Cart object, which can be used to create single
  cartridges to upload BIOS files.

  @author Stephen Anthony
*/
class Cart
{
  public:
    Cart() = default;
    ~Cart() = default;

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
                        bool verify, bool showprogress, bool continueOnError);

    /**
      Loads ROM cartridge data from the given filename, creating a cart.
      The bankswitch type is autodetected if type is "".
      The filename should exist and be readable.
    */
    string downloadROM(SerialPort& port, const string& armpath,
                       const string& filename, Bankswitch::Type type,
                       bool verify, bool showprogress, bool continueOnError);

    /** Set number of write retries before bailing out. */
    void setConnectionAttempts(uInt32 attempt);

    /** Set number of write retries before bailing out. */
    void setRetry(uInt32 retry);

    /**
      On F4 (32K) bankswitching, when the first bank is compressed, the
      cartridge starts in bank 1. This can cause problems with some ROMs.
      This option will allow to leave out bank 0 when searching for the
      first compressable bank.
    */
    void skipF4CompressionOnBank0(bool skip) {
      myF4FirstCompressionBank = skip ? 1 : 0;
    }

    /**
      Log all output to the given stream.
    */
    void setLogger(ostream* out);

  private:
    /**
      Read data from given file and return it in a buffer, along
      with the allocated size.
    */
    ByteBuffer readFile(const string& filename, size_t &size);

    /**
      Compress the last bank of the given ROM image.  For now, it always
      assumes a ROM size of 32K.

      @param binary  The ROM data (assumed to be 32K)
      @return  The compressed size of the last bank (originally 4K)
    */
    uInt32 compressLastBank(uInt8* binary);

  private:
    Progress myProgress;
    CartProgrammer myProgrammer;

    ostream* myLog{&cout};
    uInt32   myF4FirstCompressionBank{0};

    // Supercharger/Arcadia ROM header
    static uInt8 ourARHeader[256];

  private:
    // Following constructors and assignment operators not supported
    Cart(const Cart&) = delete;
    Cart(Cart&&) = delete;
    Cart& operator=(const Cart&) = delete;
    Cart& operator=(Cart&&) = delete;
};

#endif
