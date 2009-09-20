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

/**
  Auto-detect cart type.
  Based on auto-detection code from Stella (http://stella.sf.net)

  @author  Stephen Anthony
*/

#include <cstring>

#include "bspf.hxx"
#include "CartDetector.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BSType CartDetector::autodetectType(const uInt8* image, uInt32 size)
{
  BSType type = BS_NONE;

  if((size % 8448) == 0)
  {
    type = BS_AR;
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
  else if(size == 8192)  // 8K
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
  else if((size == 10495) || (size == 10496) || (size == 10240))  // 10K - Pitfall2
  {
    type = BS_NONE;  // Not supported
  }
  else if(size == 12288)  // 12K
  {
    // TODO - this should really be in a method that checks the first
    // 512 bytes of ROM and finds if either the lower 256 bytes or
    // higher 256 bytes are all the same.  For now, we assume that
    // all carts of 12K are CBS RAM Plus/FASC.
    type = BS_FA;
  }
  else if(size == 16384)  // 16K
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
  else if(size == 32768)  // 32K
  {
    if(isProbablySC(image, size))
      type = BS_F4SC;
    else if(isProbably3E(image, size))
      type = BS_3E;
    else if(isProbably3F(image, size))
      type = BS_3F;
    else
      type = BS_F4;
  }
  else if(size == 65536)  // 64K
  {
    if(isProbably3E(image, size))
      type = BS_3E;
    else if(isProbably3F(image, size))
      type = BS_3F;
    else if(isProbably4A50(image, size))
      type = BS_NONE;  // Not supported
    else if(isProbablyEF(image, size))
    {
      type = BS_EF;
      if(isProbablySC(image, size))
        type = BS_EFSC;
    }
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
      type = BS_NONE;  // Not supported
    else if(isProbablySB(image, size))
      type = BS_NONE;  // Not supported
    else
      type = BS_NONE;  // Not supported
  }
  else if(size == 256*1024)  // 256K
  {
    if(isProbably3E(image, size))
      type = BS_3E;
    else if(isProbably3F(image, size))
      type = BS_3F;
    else /*if(isProbablySB(image, size))*/
      type = BS_NONE; // Not supported
  }
  else  // what else can we do?
  {
    if(isProbably3E(image, size))
      type = BS_3E;
    else if(isProbably3F(image, size))
      type = BS_3F;
    else
      type = BS_4K;  // Most common bankswitching type
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
bool CartDetector::isProbably3E(const uInt8* image, uInt32 size)
{
  // 3E cart bankswitching is triggered by storing the bank number
  // in address 3E using 'STA $3E', commonly followed by an
  // immediate mode LDA
  uInt8 signature[] = { 0x85, 0x3E, 0xA9, 0x00 };  // STA $3E; LDA #$00
  return searchForBytes(image, size, signature, 4, 1);
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
  {
    if(searchForBytes(image, size, signature[i], 3, 1))
      return true;
  }
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
  uInt8 signature[5][3] = {
   { 0xAD, 0xE5, 0xFF },  // LDA $FFE5
   { 0xAD, 0xE5, 0x1F },  // LDA $1FE5
   { 0x0C, 0xE7, 0x1F },  // NOP $1FE7
   { 0x8D, 0xE7, 0xFF },  // STA $FFE7
   { 0x8D, 0xE7, 0x1F }   // STA $1FE7
  };
  for(uInt32 i = 0; i < 5; ++i)
  {
    if(searchForBytes(image, size, signature[i], 3, 1))
      return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyEF(const uInt8* image, uInt32 size)
{
  // EF cart bankswitching switches banks by accessing addresses 0xFE0
  // to 0xFEF, usually with either a NOP or LDA
  // It's likely that the code will switch to bank 0, so that's what is tested
  uInt8 signature[2][3] = {
    { 0x0C, 0xE0, 0xFF },  // NOP $FFE0
    { 0xAD, 0xE0, 0xFF }   // LDA $FFE0
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
  uInt8 signature[2][3] = {
    { 0x8D, 0x40, 0x02 },  // STA $240
    { 0xAD, 0x40, 0x02 }   // LDA $240
  };
  if(searchForBytes(image, size, signature[0], 3, 1))
    return true;
  else
    return searchForBytes(image, size, signature[1], 3, 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbably4A50(const uInt8* image, uInt32 size)
{
  // 4A50 carts store address $4A50 at the NMI vector, which
  // in this scheme is always in the last page of ROM at
  // $1FFA - $1FFB (at least this is true in rev 1 of the format)
  int idx = size - 6;  // $1FFA
  return (image[idx] == 0x50 && image[idx+1] == 0x4A);
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
bool CartDetector::isProbably0840(const uInt8* image, uInt32 size)
{
  // 0840 cart bankswitching is triggered by accessing addresses 0x0800
  // or 0x0840
  uInt8 signature[2][3] = {
    { 0xAD, 0x00, 0x08 },  // LDA $0800
    { 0xAD, 0x40, 0x08 }   // LDA $0840
  };
  if(searchForBytes(image, size, signature[0], 3, 1))
    return true;
  else
    return searchForBytes(image, size, signature[1], 3, 1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool CartDetector::isProbablyCV(const uInt8* image, uInt32 size)
{
  // CV RAM access occurs at addresses $f3ff and $f400
  // These signatures are attributed to the MESS project
  uInt8 signature[2][3] = {
    { 0x9D, 0xFF, 0xF3 },  // STA $F3FF
    { 0x99, 0x00, 0xF4 }   // STA $F400
  };
  if(searchForBytes(image, size, signature[0], 3, 1))
    return true;
  else
    return searchForBytes(image, size, signature[1], 3, 1);
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
  {
    if(searchForBytes(image, size, signature[i], 5, 1))
      return true;
  }
  return false;
}