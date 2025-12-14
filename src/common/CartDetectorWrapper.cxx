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

#include "CartDetectorWrapper.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Bankswitch::Type CartDetectorHC::autodetectType(
    const string& rom, const ByteBuffer& image, size_t size)
{
  // First attempt to detect by filename extension
  Bankswitch::Type type = autodetectTypeByExtension(rom);
  if(type != Bankswitch::Type::_AUTO)
    return type;

  return autodetectTypeByContent(image, size);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Bankswitch::Type CartDetectorHC::autodetectType(const string& rom)
{
  // First attempt to detect by filename extension
  Bankswitch::Type type = autodetectTypeByExtension(rom);
  if(type != Bankswitch::Type::_AUTO)
    return type;

  // Read file into buffer
  std::ifstream in(rom, std::ios::binary);
  if(in)
  {
    // Figure out how much data we should read
    in.seekg(0, std::ios::end);
    uInt32 length = uInt32(in.tellg());
    in.seekg(0, std::ios::beg);
    if(length > 0)
    {
      ByteBuffer buffer = make_unique<uInt8[]>(length);
      in.read((char*)buffer.get(), length);
      in.close();
      type = autodetectTypeByContent(buffer, length);
    }
    else
      type = Bankswitch::Type::_CUSTOM;
  }

  return type;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Bankswitch::Type CartDetectorHC::autodetectTypeByExtension(const string& rom)
{
  FSNode file(rom);
  return Bankswitch::typeFromExtension(file);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Bankswitch::Type CartDetectorHC::autodetectTypeByContent(
    const ByteBuffer& image, size_t size)
{
//  Bankswitch::Type type = Bankswitch::Type::_CUSTOM;

  Bankswitch::Type type = CartDetector::autodetectType(image, size);
  return type == Bankswitch::Type::_2K ? Bankswitch::Type::_4K : type;
}
