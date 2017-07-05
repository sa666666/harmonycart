//=========================================================================
//
//  H   H    A    RRRR   M   M   OOO   N   N  Y   Y
//  H   H   A A   R   R  MM MM  O   O  NN  N   Y Y
//  HHHHH  AAAAA  RRRR   M M M  O   O  N N N    Y   "Harmony Cart software"
//  H   H  A   A  R R    M   M  O   O  N  NN    Y
//  H   H  A   A  R  R   M   M   OOO   N   N    Y
//
// Copyright (c) 2009-2017 by Stephen Anthony <sa666666@gmail.com>
//
// See the file "License.txt" for information on usage and redistribution
// of this file, and for a DISCLAIMER OF ALL WARRANTIES.
//=========================================================================

#ifndef __OSYSTEM_UNIX_HXX
#define __OSYSTEM_UNIX_HXX

#include "OSystem.hxx"

/**
  This class defines UNIX-like OS's (MacOS X) system specific settings.

  @author  Stephen Anthony
*/
class OSystemMACOSX : public OSystem
{
  public:
    /**
      Create a new MACOSX-specific operating system object
    */
    OSystemMACOSX();

    /**
      Destructor
    */
    virtual ~OSystemMACOSX() = default;

  private:
    // Following constructors and assignment operators not supported
    OSystemMACOSX(const OSystemMACOSX&) = delete;
    OSystemMACOSX(OSystemMACOSX&&) = delete;
    OSystemMACOSX& operator=(const OSystemMACOSX&) = delete;
    OSystemMACOSX& operator=(OSystemMACOSX&&) = delete;
};

#endif
