//=========================================================================
//
//  H   H    A    RRRR   M   M   OOO   N   N  Y   Y
//  H   H   A A   R   R  MM MM  O   O  NN  N   Y Y
//  HHHHH  AAAAA  RRRR   M M M  O   O  N N N    Y   "Harmony Cart software"
//  H   H  A   A  R R    M   M  O   O  N  NN    Y
//  H   H  A   A  R  R   M   M   OOO   N   N    Y
//
// Copyright (c) 2009-2024 by Stephen Anthony <sa666666@gmail.com>
//
// See the file "License.txt" for information on usage and redistribution
// of this file, and for a DISCLAIMER OF ALL WARRANTIES.
//=========================================================================

#ifndef OSYSTEM_UNIX_HXX
#define OSYSTEM_UNIX_HXX

#include "OSystem.hxx"

class OSystemUNIX : public OSystem
{
  public:
    OSystemUNIX();
    ~OSystemUNIX() override = default;

  private:
    // Following constructors and assignment operators not supported
    OSystemUNIX(const OSystemUNIX&) = delete;
    OSystemUNIX(OSystemUNIX&&) = delete;
    OSystemUNIX& operator=(const OSystemUNIX&) = delete;
    OSystemUNIX& operator=(OSystemUNIX&&) = delete;
};

#endif
