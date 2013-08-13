//=========================================================================
//
//  H   H    A    RRRR   M   M   OOO   N   N  Y   Y
//  H   H   A A   R   R  MM MM  O   O  NN  N   Y Y
//  HHHHH  AAAAA  RRRR   M M M  O   O  N N N    Y   "Harmony Cart software"
//  H   H  A   A  R R    M   M  O   O  N  NN    Y
//  H   H  A   A  R  R   M   M   OOO   N   N    Y
//
// Copyright (c) 2009-2013 by Stephen Anthony <stephena@users.sf.net>
//
// See the file "License.txt" for information on usage and redistribution
// of this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//=========================================================================

/**
  Auto-detect cart type.
  Based on auto-detection code from Stella (http://stella.sf.net)

  @author  Stephen Anthony
*/

#include <cstring>
#include <fstream>

#include "bspf_harmony.hxx"
#include "CartDetector.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BSType CartDetector::autodetectType(const string& rom,
                                    const uInt8* image, uInt32 size)
{
  // First attempt to detect by filename extension
  BSType type = autodetectTypeByExtension(rom);
  if(type != BS_AUTO)
    return type;

  // Then attempt to detect by actual ROM contents
  if(!image || size == 0)
  {
    // Read file into buffer
    ifstream in(rom.c_str(), ios::binary);
    if(in)
    {
      // Figure out how much data we should read
      in.seekg(0, ios::end);
      uInt32 length = (uInt32)in.tellg();
      in.seekg(0, ios::beg);
      if(length > 0)
      {
        uInt8* buffer = new uInt8[length];
        in.read((char*)buffer, length);
        in.close();
        type = autodetectTypeByContent(buffer, length);
        delete[] buffer;
      }
      else
        type = BS_CUSTOM;
    }
  }
  else
    type = autodetectTypeByContent(image, size);

  return type;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BSType CartDetector::autodetectTypeByExtension(const string& rom)
{
  // Extensions defined in Harmony manual and corresponding
  // bankswitch types
  static const char* exts[20] = {
    "2K", "4K", "F4", "F4S", "F6", "F6S", "F8", "F8S",
    "FA", "FE", "3F", "3E", "E0", "E7", "CV", "UA",
    "AR", "DPC", "084", "CU"
   };
  static BSType types[20] = {
    BS_4K, BS_4K, BS_F4, BS_F4SC, BS_F6, BS_F6SC, BS_F8, BS_F8SC,
    BS_FA, BS_FE, BS_3F, BS_3E, BS_E0, BS_E7, BS_CV, BS_UA,
    BS_AR, BS_DPC, BS_0840, BS_CUSTOM
   };

  string::size_type idx = rom.rfind('.');
  const string& ext = idx != string::npos ? rom.substr(idx+1) : EmptyString;
  for(int i = 0; i < 20; ++i)
    if(BSPF_equalsIgnoreCase(exts[i], ext))
      return types[i];

  return BS_AUTO;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BSType CartDetector::autodetectTypeByContent(const uInt8* image, uInt32 size)
{
  BSType type = BS_CUSTOM;

  if((size % 8448) == 0 || size == 6144)
  {
    type = BS_AR;
  }
  else if(size < 2048)  // Sub2K images
  {
    type = BS_4K;     // 2K is automatically converted to 4K
  }
  else if((size == 2048) ||
          (size == 4096 && memcmp(image, image + 2048, 2048) == 0))
  {
    if(isProbablyCV(image, size))
      type = BS_CV;
    else
      type = BS_4K;   // 2K is automatically converted to 4K
  }
  else if(size == 4096)
  {
    if(isProbablyCV(image, size))
      type = BS_CV;
    else
      type = BS_4K;
  }
  else if(size == 8*1024)  // 8K
  {
    if(isProbablySC(image, size))
      type = BS_F8SC;
    else if(memcmp(image, image + 4096, 4096) == 0)
      type = BS_4K;
    else if(isProbablyE0(image, size))
      type = BS_E0;
    else if(isProbably3E(image, size))
      type = BS_3E;
    else if(isProbably3F(image, size))
      type = BS_3F;
    else if(isProbablyUA(image, size))
      type = BS_UA;
    else if(isProbablyFE(image, size))
      type = BS_FE;
    else if(isProbably0840(image, size))
      type = BS_0840;
    else
      type = BS_F8;
  }
  else if(size >= 10240 && size <= 10496)  // ~10K - Pitfall2
  {
    type = BS_DPC;
  }
  else if(size == 12*1024)  // 12K
  {
    type = BS_FA;
  }
  else if(size == 16*1024)  // 16K
  {
    if(isProbablySC(image, size))
      type = BS_F6SC;
    else if(isProbablyE7(image, size))
      type = BS_E7;
    else if(isProbably3E(image, size))
      type = BS_3E;
    else if(isProbably3F(image, size))
      type = BS_3F;
    else
      type = BS_F6;
  }
  else if(size == 24*1024 || size == 28*1024)  // 24K & 28K
  {
    type = BS_FA2;
  }
  else if(size == 29*1024)  // 29K
  {
    if(isProbablyARM(image, size))
      type = BS_FA2;
    else /*if(isProbablyDPCplus(image, size))*/
      type = BS_DPCP;
  }
  else if(size == 32*1024)  // 32K
  {
    if(isProbablySC(image, size))
      type = BS_F4SC;
    else if(isProbably3E(image, size))
      type = BS_3E;
    else if(isProbably3F(image, size))
      type = BS_3F;
    else if(isProbablyDPCplus(image, size))
      type = BS_DPCP;
    else if(isProbablyCTY(image, size))
      type = BS_CTY;
    else if(isProbablyFA2(image, size))
      type = BS_FA2;
    else
      type = BS_F4;
  }
  else if(size == 64*1024)  // 64K
  {
    if(isProbably3E(image, size))
      type = BS_3E;
    else if(isProbably3F(image, size))
      type = BS_3F;
    else if(isProbably4A50(image, size))
      type = BS_4A50;
    else if(isProbablyEF(image, size, type))
      ; // type has been set directly in the function
    else if(isProbablyX07(image, size))
      type = BS_X07;
    else
      type = BS_F0;
  }
  else if(size == 128*1024)  // 128K
  {
    if(isProbably3E(image, size))
      type = BS_3E;
    else if(isProbably3F(image, size))
      type = BS_3F;
    else if(isProbably4A50(image, size))
      type = BS_4A50;
    else if(isProbablySB(image, size))
      type = BS_SB;
    else
      type = BS_MC;
  }
  else if(size == 256*1024)  // 256K
  {
    if(isProbably3E(image, size))
      type = BS_3E;
    else if(isProbably3F(image, size))
      type = BS_3F;
    else /*if(isProbablySB(image, size))*/
      type = BS_SB; // Not supported
  }
  else  // what else can we do?
  {
    if(isProbably3E(image, size))
      type = BS_3E;
    else if(isProbably3F(image, size))
      type = BS_3F;
  }

  return type;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::searchForBytes(const uInt8* image, uInt32 imagesize,
                                  const uInt8* signature, uInt32 sigsize,
                                  uInt32 minhits)
{
  uInt32 count = 0;
  for(uInt32 i = 0; i < imagesize - sigsize; ++i)
  {
    uInt32 matches = 0;
    for(uInt32 j = 0; j < sigsize; ++j)
    {
      if(image[i+j] == signature[j])
        ++matches;
      else
        break;
    }
    if(matches == sigsize)
    {
      ++count;
      i += sigsize;  // skip past this signature 'window' entirely
    }
    if(count >= minhits)
      break;
  }

  return (count >= minhits);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablySC(const uInt8* image, uInt32 size)
{
  // We assume a Superchip cart contains the same bytes for its entire
  // RAM area; obviously this test will fail if it doesn't
  // The RAM area will be the first 256 bytes of each 4K bank
  uInt32 banks = size / 4096;
  for(uInt32 i = 0; i < banks; ++i)
  {
    uInt8 first = image[i*4096];
    for(uInt32 j = 0; j < 256; ++j)
    {
      if(image[i*4096+j] != first)
        return false;
    }
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyARM(const uInt8* image, uInt32)
{
  // ARM code contains the following 'loader' patterns in the first 1K
  // Thanks to Thomas Jentzsch of AtariAge for this advice
  uInt8 signature[2][4] = {
    { 0xA0, 0xC1, 0x1F, 0xE0 },
    { 0x00, 0x80, 0x02, 0xE0 }
  };
  if(searchForBytes(image, 1024, signature[0], 4, 1))
    return true;
  else
    return searchForBytes(image, 1024, signature[1], 4, 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbably0840(const uInt8* image, uInt32 size)
{
  // 0840 cart bankswitching is triggered by accessing addresses 0x0800
  // or 0x0840 at least twice
  uInt8 signature1[3][3] = {
    { 0xAD, 0x00, 0x08 },  // LDA $0800
    { 0xAD, 0x40, 0x08 },  // LDA $0840
    { 0x2C, 0x00, 0x08 }   // BIT $0800
  };
  for(uInt32 i = 0; i < 3; ++i)
    if(searchForBytes(image, size, signature1[i], 3, 2))
      return true;

  uInt8 signature2[2][4] = {
    { 0x0C, 0x00, 0x08, 0x4C },  // NOP $0800; JMP ...
    { 0x0C, 0xFF, 0x0F, 0x4C }   // NOP $0FFF; JMP ...
  };
  for(uInt32 i = 0; i < 2; ++i)
    if(searchForBytes(image, size, signature2[i], 4, 2))
      return true;

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbably3E(const uInt8* image, uInt32 size)
{
  // 3E cart bankswitching is triggered by storing the bank number
  // in address 3E using 'STA $3E', commonly followed by an
  // immediate mode LDA
  uInt8 signature[] = { 0x85, 0x3E, 0xA9, 0x00 };  // STA $3E; LDA #$00
  return searchForBytes(image, size, signature, 4, 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbably3F(const uInt8* image, uInt32 size)
{
  // 3F cart bankswitching is triggered by storing the bank number
  // in address 3F using 'STA $3F'
  // We expect it will be present at least 2 times, since there are
  // at least two banks
  uInt8 signature[] = { 0x85, 0x3F };  // STA $3F
  return searchForBytes(image, size, signature, 2, 2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbably4A50(const uInt8* image, uInt32 size)
{
  // 4A50 carts store address $4A50 at the NMI vector, which
  // in this scheme is always in the last page of ROM at
  // $1FFA - $1FFB (at least this is true in rev 1 of the format)
  if(image[size-6] == 0x50 && image[size-5] == 0x4A)
    return true;

  // Program starts at $1Fxx with NOP $6Exx or NOP $6Fxx?
  if(((image[0xfffd] & 0x1f) == 0x1f) &&
      (image[image[0xfffd] * 256 + image[0xfffc]] == 0x0c) &&
      ((image[image[0xfffd] * 256 + image[0xfffc] + 2] & 0xfe) == 0x6e))
    return true;

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyCV(const uInt8* image, uInt32 size)
{
  // CV RAM access occurs at addresses $f3ff and $f400
  // These signatures are attributed to the MESS project
  uInt8 signature[2][3] = {
    { 0x9D, 0xFF, 0xF3 },  // STA $F3FF.X
    { 0x99, 0x00, 0xF4 }   // STA $F400.Y
  };
  if(searchForBytes(image, size, signature[0], 3, 1))
    return true;
  else
    return searchForBytes(image, size, signature[1], 3, 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyCTY(const uInt8*, uInt32)
{
  return false;  // TODO - add autodetection
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyDPCplus(const uInt8* image, uInt32 size)
{
  // DPC+ ARM code has 2 occurrences of the string DPC+
  uInt8 signature[] = { 'D', 'P', 'C', '+' };
  return searchForBytes(image, size, signature, 4, 2);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyE0(const uInt8* image, uInt32 size)
{
  // E0 cart bankswitching is triggered by accessing addresses
  // $FE0 to $FF9 using absolute non-indexed addressing
  // To eliminate false positives (and speed up processing), we
  // search for only certain known signatures
  // Thanks to "stella@casperkitty.com" for this advice
  // These signatures are attributed to the MESS project
  uInt8 signature[8][3] = {
   { 0x8D, 0xE0, 0x1F },  // STA $1FE0
   { 0x8D, 0xE0, 0x5F },  // STA $5FE0
   { 0x8D, 0xE9, 0xFF },  // STA $FFE9
   { 0x0C, 0xE0, 0x1F },  // NOP $1FE0
   { 0xAD, 0xE0, 0x1F },  // LDA $1FE0
   { 0xAD, 0xE9, 0xFF },  // LDA $FFE9
   { 0xAD, 0xED, 0xFF },  // LDA $FFED
   { 0xAD, 0xF3, 0xBF }   // LDA $BFF3
  };
  for(uInt32 i = 0; i < 8; ++i)
    if(searchForBytes(image, size, signature[i], 3, 1))
      return true;

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyE7(const uInt8* image, uInt32 size)
{
  // E7 cart bankswitching is triggered by accessing addresses
  // $FE0 to $FE6 using absolute non-indexed addressing
  // To eliminate false positives (and speed up processing), we
  // search for only certain known signatures
  // Thanks to "stella@casperkitty.com" for this advice
  // These signatures are attributed to the MESS project
  uInt8 signature[7][3] = {
   { 0xAD, 0xE2, 0xFF },  // LDA $FFE2
   { 0xAD, 0xE5, 0xFF },  // LDA $FFE5
   { 0xAD, 0xE5, 0x1F },  // LDA $1FE5
   { 0xAD, 0xE7, 0x1F },  // LDA $1FE7
   { 0x0C, 0xE7, 0x1F },  // NOP $1FE7
   { 0x8D, 0xE7, 0xFF },  // STA $FFE7
   { 0x8D, 0xE7, 0x1F }   // STA $1FE7
  };
  for(uInt32 i = 0; i < 7; ++i)
    if(searchForBytes(image, size, signature[i], 3, 1))
      return true;

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyEF(const uInt8* image, uInt32 size, BSType& type)
{
  // Newer EF carts store strings 'EFEF' and 'EFSC' starting at address $FFF8
  // This signature is attributed to "RevEng" of AtariAge
  uInt8 efef[] = { 'E', 'F', 'E', 'F' };
  uInt8 efsc[] = { 'E', 'F', 'S', 'C' };
  if(searchForBytes(image+size-8, 8, efef, 4, 1))
  {
    type = BS_EF;
    return true;
  }
  else if(searchForBytes(image+size-8, 8, efsc, 4, 1))
  {
    type = BS_EFSC;
    return true;
  }

  // Otherwise, EF cart bankswitching switches banks by accessing addresses
  // 0xFE0 to 0xFEF, usually with either a NOP or LDA
  // It's likely that the code will switch to bank 0, so that's what is tested
  bool isEF = false;
  uInt8 signature[4][3] = {
    { 0x0C, 0xE0, 0xFF },  // NOP $FFE0
    { 0xAD, 0xE0, 0xFF },  // LDA $FFE0
    { 0x0C, 0xE0, 0x1F },  // NOP $1FE0
    { 0xAD, 0xE0, 0x1F }   // LDA $1FE0
  };
  for(uInt32 i = 0; i < 4; ++i)
  {
    if(searchForBytes(image, size, signature[i], 3, 1))
    {
      isEF = true;
      break;
    }
  }

  // Now that we know that the ROM is EF, we need to check if it's
  // the SC variant
  if(isEF)
  {
    type = isProbablySC(image, size) ? BS_EFSC : BS_EF;
    return true;
  }

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyFA2(const uInt8* image, uInt32)
{
  // This currently tests only the 32K version of FA2; the 24 and 28K
  // versions are easy, in that they're the only possibility with those
  // file sizes

  // 32K version has all zeros in 29K-32K area
  for(uInt32 i = 29*1024; i < 32*1024; ++i)
    if(image[i] != 0)
      return false;

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyFE(const uInt8* image, uInt32 size)
{
  // FE bankswitching is very weird, but always seems to include a
  // 'JSR $xxxx'
  // These signatures are attributed to the MESS project
  uInt8 signature[4][5] = {
    { 0x20, 0x00, 0xD0, 0xC6, 0xC5 },  // JSR $D000; DEC $C5
    { 0x20, 0xC3, 0xF8, 0xA5, 0x82 },  // JSR $F8C3; LDA $82
    { 0xD0, 0xFB, 0x20, 0x73, 0xFE },  // BNE $FB; JSR $FE73
    { 0x20, 0x00, 0xF0, 0x84, 0xD6 }   // JSR $F000; STY $D6
  };
  for(uInt32 i = 0; i < 4; ++i)
    if(searchForBytes(image, size, signature[i], 5, 1))
      return true;

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablySB(const uInt8* image, uInt32 size)
{
  // SB cart bankswitching switches banks by accessing address 0x0800
  uInt8 signature[2][3] = {
    { 0xBD, 0x00, 0x08 },  // LDA $0800,x
    { 0xAD, 0x00, 0x08 }   // LDA $0800
  };
  if(searchForBytes(image, size, signature[0], 3, 1))
    return true;
  else
    return searchForBytes(image, size, signature[1], 3, 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyUA(const uInt8* image, uInt32 size)
{
  // UA cart bankswitching switches to bank 1 by accessing address 0x240
  // using 'STA $240' or 'LDA $240'
  uInt8 signature[3][3] = {
    { 0x8D, 0x40, 0x02 },  // STA $240
    { 0xAD, 0x40, 0x02 },  // LDA $240
    { 0xBD, 0x1F, 0x02 }   // LDA $21F,X
  };
  for(uInt32 i = 0; i < 3; ++i)
    if(searchForBytes(image, size, signature[i], 3, 1))
      return true;

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyX07(const uInt8* image, uInt32 size)
{
  // X07 bankswitching switches to bank 0, 1, 2, etc by accessing address 0x08xd
  uInt8 signature[6][3] = {
    { 0xAD, 0x0D, 0x08 },  // LDA $080D
    { 0xAD, 0x1D, 0x08 },  // LDA $081D
    { 0xAD, 0x2D, 0x08 },  // LDA $082D
    { 0x0C, 0x0D, 0x08 },  // NOP $080D
    { 0x0C, 0x1D, 0x08 },  // NOP $081D
    { 0x0C, 0x2D, 0x08 }   // NOP $082D
  };
  for(uInt32 i = 0; i < 6; ++i)
    if(searchForBytes(image, size, signature[i], 3, 1))
      return true;

  return false;
}
