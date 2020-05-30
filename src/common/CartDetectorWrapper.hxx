//=========================================================================
//
//  H   H    A    RRRR   M   M   OOO   N   N  Y   Y
//  H   H   A A   R   R  MM MM  O   O  NN  N   Y Y
//  HHHHH  AAAAA  RRRR   M M M  O   O  N N N    Y   "Harmony Cart software"
//  H   H  A   A  R R    M   M  O   O  N  NN    Y
//  H   H  A   A  R  R   M   M   OOO   N   N    Y
//
// Copyright (c) 2009-2020 by Stephen Anthony <sa666666@gmail.com>
//
// See the file "License.txt" for information on usage and redistribution
// of this file, and for a DISCLAIMER OF ALL WARRANTIES.
//=========================================================================

#ifndef CARTRIDGE_DETECTOR_WRAPPER_HXX
#define CARTRIDGE_DETECTOR_WRAPPER_HXX

#include "CartDetector.hxx"

/**
  A wrapper class so that we can use CartDetector class from Stella directly.

  @author  Stephen Anthony
*/
class CartDetectorHC
{
  public:
    /**
      Try to auto-detect the bankswitching type of the cartridge,
      based first on filename, then on actual file content

      @param rom    The file containing the ROM image
      @param image  A pointer to the ROM image (may be null)
      @param size   The size of the ROM image (may be 0)
      @return  The "best guess" for the cartridge type
    */
    static Bankswitch::Type autodetectType(
        const string& rom, const ByteBuffer& image, uInt32 size);
    static Bankswitch::Type autodetectType(const string& rom);

  private:
    /**
      Try to auto-detect the bankswitching type of the cartridge
      based on the filename extensions as defined in the Harmony
      manual

      @param rom  The file containing the ROM image
      @return  The "best guess" for the cartridge type
    */
    static Bankswitch::Type autodetectTypeByExtension(const string& rom);

    /**
      Try to auto-detect the bankswitching type of the cartridge
      based on an analysis of the ROM data (from Stella)

      @param rom  The file containing the ROM image
      @return  The "best guess" for the cartridge type
    */
    static Bankswitch::Type autodetectTypeByContent(const ByteBuffer& image, uInt32 size);

  private:
    // Following constructors and assignment operators not supported
    CartDetectorHC() = delete;
    CartDetectorHC(const CartDetectorHC&) = delete;
    CartDetectorHC(CartDetectorHC&&) = delete;
    CartDetectorHC& operator=(const CartDetectorHC&) = delete;
    CartDetectorHC& operator=(CartDetectorHC&&) = delete;
};

#endif
