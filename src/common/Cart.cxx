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

#include <QDir>
#include <QString>

#include "bspf.hxx"
#include "Bankswitch.hxx"
#include "Cart.hxx"
#include "CartDetectorWrapper.hxx"
#include "SerialPort.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Cart::autodetectHarmony(SerialPort& port)
{
  myProgrammer.reset(port);
  port.clearBuffers();

  // Get the version #, if any
  string result = myProgrammer.chipVersion(port);
  if(result.starts_with("ERROR:"))
  {
    *myLog << result.c_str() << '\n';
    return result;
  }

 return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Cart::downloadBIOS(SerialPort& port, const string& filename,
                          bool verify, bool showprogress, bool continueOnError)
{
  string result = "";

  // Read the file into a buffer
  size_t size = 0;
  ByteBuffer bios = readFile(filename, size);
  try
  {
    if(size > 0)
    {
      myProgress.setEnabled(showprogress);
      result = myProgrammer.download(port, bios.get(), static_cast<uInt32>(size),
                                     myProgress, verify, continueOnError);
    }
    else
      result = "Couldn't open BIOS file";
  }
  catch(const runtime_error& e)
  {
    result = e.what();
  }

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Cart::downloadROM(SerialPort& port, const string& armpath,
                         const string& filename, Bankswitch::Type type,
                         bool verify, bool showprogress, bool continueOnError)
{
  string result = "";
  bool autodetect = type == Bankswitch::Type::_AUTO;
  size_t romsize = 0, armsize = 0, size = 0;
  ByteBuffer armbuf;
  uInt8 binary[512*1024], *binary_ptr = binary;
  string armfile = "";

  // Read the ROM file into a buffer
  ByteBuffer rombuf = readFile(filename, romsize);
  if(romsize == 0)
  {
    result = "Couldn't open ROM file.";
    goto cleanup;
  }

  // Determine the bankswitch type
  if(autodetect)
    type = CartDetectorHC::autodetectType(filename, rombuf, romsize);
  if(autodetect || type == CartDetectorHC::autodetectType(filename, rombuf, romsize))
  {
    *myLog << "Bankswitch type: " << Bankswitch::typeToName(type).c_str()
           << " (auto-detected)\n";
  }
  else
  {
    *myLog << "Bankswitch type: " << Bankswitch::typeToName(type).c_str()
           << " (WARNING: overriding auto-detection)\n";
  }

  // First determine the name of the bankswitch file to use
  switch(type)
  {
    case Bankswitch::Type::_0840:   armfile = "0840.arm";   break;
    case Bankswitch::Type::_3E:     armfile = "3E.arm";     break;
    case Bankswitch::Type::_3F:     armfile = "3F.arm";     break;
    case Bankswitch::Type::_4K:     armfile = "2K4K.arm";   break;
    case Bankswitch::Type::_AR:     armfile = "SC.arm";     break;
    case Bankswitch::Type::_CV:     armfile = "CV.arm";     break;
    case Bankswitch::Type::_DPC:    armfile = "DPC.arm";    break;
    case Bankswitch::Type::_DPCP:   armfile = "DPC+.arm";   break;
    case Bankswitch::Type::_E0:     armfile = "E0.arm";     break;
    case Bankswitch::Type::_E7:     armfile = "E7.arm";     break;
    case Bankswitch::Type::_F4:     armfile = "F4.arm";     break;
    case Bankswitch::Type::_F4SC:   armfile = "F4SC.arm";   break;
    case Bankswitch::Type::_F6:     armfile = "F6.arm";     break;
    case Bankswitch::Type::_F6SC:   armfile = "F6SC.arm";   break;
    case Bankswitch::Type::_F8:     armfile = "F8.arm";     break;
    case Bankswitch::Type::_F8SC:   armfile = "F8SC.arm";   break;
    case Bankswitch::Type::_FA:     armfile = "FA.arm";     break;
    case Bankswitch::Type::_FA2:    armfile = "FA2.arm";    break;
    case Bankswitch::Type::_FE:     armfile = "FE.arm";     break;
    case Bankswitch::Type::_UA:     armfile = "UA.arm";     break;
    case Bankswitch::Type::_CUSTOM:                         break;
    default:
      result = "Bankswitch type \'" + Bankswitch::typeToName(type) + "\' not supported.";
      *myLog << "ERROR: " << result.c_str() << '\n';
      goto cleanup;
  }

  // Now load the proper bankswitch file
  // This incredibly ugly line makes sure that the path separator works on all ports
  // It seems that '/' is used even on Windows, but all separators must be converted
  // to '\' before passing it to C++ streams
  // Damn Windows for being the only OS that uses '\'
  if(armfile != "")
  {
    armfile = QDir(QString(armpath.c_str()) + "/" + QString(armfile.c_str())).canonicalPath().toStdString();
    armbuf = readFile(armfile, armsize);
    if(armsize == 0)
    {
      result = "Couldn't open bankswitch ARM file.";
      goto cleanup;
    }
  }

  // Now we have to combine the ROM and ARM code
  //   Certain cases below will take care of combining the ARM and ROM data
  //   in a specific fashion; in those cases, 'size' will be non-zero,
  //   indicating that no further processing is required.
  //
  //   Otherwise, normal cases will have 'size' as zero, indicating that ARM
  //   and ROM data is to be combined later in the method.

  memset(binary_ptr, 0, 512*1024);
  switch(type)
  {
    case Bankswitch::Type::_F4SC:
    {
      // Copy ROM data
      memcpy(binary_ptr, rombuf.get(), romsize);

      // ARM code in first "RAM" area
      memcpy(binary_ptr, armbuf.get(), 256);

      // ARM code in second "RAM" area
      if(armsize > 4096)
        memcpy(binary_ptr+4096, armbuf.get()+4096, armsize-4096);

      size = romsize;  // No further processing required
      break;
    }

    case Bankswitch::Type::_F4:
    {
      // Copy ARM data to determine remaining size
      // Leave space for 8 bytes, to indicate the bank configuration
      memcpy(binary_ptr, armbuf.get(), armsize);
      binary_ptr += armsize + 8;

      // Reorganize bin for best compression
      // Possible bank organizations:
      //   12345670  02345671  01345672  01245673
      //   01235674  01234675  01234576  01234567

      uInt32 i;
      for(i = myF4FirstCompressionBank; i < 8; ++i) // i = bank to go last
      {
        uInt8* ptr = binary_ptr;
        for(uInt32 h = 0; h < 8; ++h)
        {
          if (h != i)
          {
            memcpy(ptr, rombuf.get()+4096*h, 4096);
            ptr += 4096;
          }
        }

        memcpy(ptr, rombuf.get()+4096*i, 4096);
        //ptr += 4096;

        // Compress last bank
        try
        {
          romsize = 28672 + compressLastBank(binary_ptr);
        }
        catch(const char* msg)
        {
          result = msg;
          *myLog << "ERROR: " << result.c_str() << '\n';
          goto cleanup;
        }
        if(romsize+armsize < 32760)
          break; // one of the banks fits
      }

      if(i == 8)
      {
        result = "Cannot compress F4 binary";
        *myLog << "ERROR: " << result.c_str() << '\n';
        goto cleanup;
      }

      // Output bank index:
      //   70123456  07123456  01723456  01273456
      //   01237456  01234756  01234576  01234567
      uInt32 f = 0;
      uInt8* ptr = binary_ptr - 8;
      for(uInt32 h = 0; h < 8; ++h)
      {
        if(h != i)  *ptr++ = f++;
        else        *ptr++ = 7;
      }

      size = armsize + 8 + romsize;  // No further processing required
      break;
    }

    case Bankswitch::Type::_4K:  // 2K & 'Sub2K'
    {
      if(romsize < 4096)
      {
        // All ROMs 2K or less should be mirrored into the 4K address space
        uInt32 power2 = 1;
        while(power2 < romsize)
          power2 <<= 1;

        // Create a 4K buffer and reassign to rombuf
        ByteBuffer tmp = make_unique<uInt8[]>(4096);
        uInt8* tmp_ptr = tmp.get();
        for(uInt32 i = 0; i < 4096/power2; ++i, tmp_ptr += power2)
          memcpy(tmp_ptr, rombuf.get(), romsize);

        rombuf = std::move(tmp);
        romsize = 4096;
      }
      break;
    }

    case Bankswitch::Type::_AR:  // Supercharger
    {
      // Take care of special AR ROM which are only 6K
      if(romsize == 6144)
      {
        // Minimum buffer size is 6K + 256 bytes
        ByteBuffer tmp = make_unique<uInt8[]>(6144+256);
        memcpy(tmp.get(), rombuf.get(), 6144);     // copy ROM
        memcpy(tmp.get()+6144, ourARHeader, 256);  // copy missing header

        rombuf = std::move(tmp);
        romsize = 6144 + 256;
      }
      else  // size is multiple of 8448
      {
        // To save space, we skip 2K in each Supercharger load
        size_t numLoads = romsize / 8448;
        ByteBuffer tmp = make_unique<uInt8[]>(numLoads*(6144+256));
        uInt8 *tmp_ptr = tmp.get(), *rom_ptr = rombuf.get();
        for(size_t i = 0; i < numLoads; ++i, tmp_ptr += 6144+256, rom_ptr += 8448)
        {
          memcpy(tmp_ptr, rom_ptr, 6144);                // 6KB  @ pos 0K
          memcpy(tmp_ptr+6144, rom_ptr+6144+2048, 256);  // 256b @ pos 8K
        }

        rombuf = std::move(tmp);
        romsize = numLoads * (6144+256);
      }
      break;
    }

    case Bankswitch::Type::_DPCP:
    {
      // There are two variants of DPC+; one with the ARM code
      // already added (32KB), and the other without (29KB)

      // The one with the ARM code will be processed here
      // The one without the ARM code will be processed below
      if(romsize == 32 * 1024)
      {
        // Copy ROM data; no further processing required
        size = romsize;
        memcpy(binary_ptr, rombuf.get(), size);
      }
      break;
    }

    case Bankswitch::Type::_FA2:
    {
      // There are two variants of FA2; one with the ARM code
      // already added and padded (32KB), and the other without (28KB)

      if(romsize == 32 * 1024)
      {
        // Copy ROM data; no further processing required
        size = romsize;
        memcpy(binary_ptr, rombuf.get(), size);
      }
      else if(romsize == 28 * 1024)
      {
        // Copy ARM data
        memcpy(binary_ptr, armbuf.get(), armsize);
        binary_ptr += 1024;

        // Copy ROM data
        memcpy(binary_ptr, rombuf.get(), romsize);

        size = 32 * 1024;
      }
      break;
    }

    case Bankswitch::Type::_CUSTOM:
    {
      // Copy ROM data; no further processing required
      size = romsize;
      memcpy(binary_ptr, rombuf.get(), size);
      break;
    }

    default:  break;
  }

  // Do we need combine ARM and ROM data?
  if(size == 0)
  {
    // Copy ARM data
    memcpy(binary_ptr, armbuf.get(), armsize);
    binary_ptr += armsize;

    // Copy ROM data
    memcpy(binary_ptr, rombuf.get(), romsize);

    size = romsize + armsize;
  }

  try
  {
    myProgress.setEnabled(showprogress);
    result = myProgrammer.download(port, binary, static_cast<uInt32>(size),
                                   myProgress, verify, continueOnError);
  }
  catch(const runtime_error& e)
  {
    result = e.what();
  }

cleanup:
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ByteBuffer Cart::readFile(const string& filename, size_t& size)
{
  *myLog << "Reading from file: \'" << filename << "\' ... ";

  // Read file into buffer
  FSNode file(filename);
  ByteBuffer buffer;  size = 0;

  if(filename == "" || !file.exists())
    *myLog << "ERROR: file not found\n";
  else if((size = file.read(buffer)) == 0)
    *myLog << "ERROR: file not found\n";
  else
    *myLog << "read in " << size << " bytes\n";

  return buffer;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 Cart::compressLastBank(uInt8* binary)
{
  uInt32 a{0}, b{0}, cb{0}, j{0}, k{0}, r{0}, x{0}, y{32768-4096}, len{0};
  uInt8 buflast[4096], buf[40000], bc[10000], dec[10000], bufliteral[200];

  memcpy(buf, binary, 32768);

  while (y < 32768)
  {
    x = k = 0;
    while (x < 32768-4096)
    {
      while (buf[y] != buf[x])
        x++;
      if (x == 32768-4096)
        break;

      // match
      len=0;
      while ((buf[y+len]==buf[x+len]) && (len<131) && (x+len<32768-4096))
        len++;

      if (k<len)
      {
        k=len;
        j=x;
      }
      x++;
    }

    // changed from k>4
    if (k >= 4)
    {
      if (b)
      {
        bc[cb++] = b;
        for(a = 0; a < b; ++a)
          bc[cb++] = bufliteral[a];
        b = 0;
      }

      bc[cb++] = k-4+128;
      bc[cb++] = j/256;
      bc[cb++] = j%256;
    }
    else
    {
      if (!k)
      {
        k = 1;
        // printf("y %X x %X\n",y,x);
      } // byte not found in entire binary ???

      for (a = 0; a < k; ++a)
        bufliteral[a+b] = buf[y+a];

      b += k;
      if (b > 127)
      {
        bc[cb++] = 127;
        for(a = 0; a < 127; ++a)
          bc[cb++] = bufliteral[a];
        b = b-127;
        if (b)
          for (a = 0; a < b; ++a)
            bufliteral[a] = bufliteral[a+127];
      }
    }
    y += k;
  }

  if (b)
  {
    bc[cb++] = b;
    for (a = 0; a < b; ++a)
      bc[cb++] = bufliteral[a];
    //b=0;
  }

#if 0
  if (cb>3424) // subject to change...
  {
    char buf[100];
    sprintf(buf, "Unable to compress: file is %d bytes", cb+28672);
    throw buf;
  }
#endif

  // decompression test
  for (a = 0; a < 4096; ++a)
  {
    buflast[a] = buf[32768-4096 + a];
    buf[32768-4096 + a] = 0;
  }

  x = y = 0;
  while (x < cb)
  {
    if (bc[x] > 127) // retrieve from memory
    {
      r = bc[x] - 124; // number
      b = bc[x+1]*256 + bc[x+2]; // address
      for (a = b; a < b + r; ++a)
        dec[y++] = buf[a];
      x += 3;
    }
    else // string of literals
    {
      r = bc[x]; // number
      for (a = x + 1; a < x + r + 1; ++a)
        dec[y++] = bc[a];
      x += r + 1;
    }
  }
  for (a = 0; a < 4096; ++a)
    if(dec[a] != buflast[a])
      break;

  if (a >= 4096)
  {
    for (a = 0; a < cb; ++a)
      binary[28672+a] = bc[a];
  }
  else
  {
    char buf[100];
    sprintf(buf, "Unknown compression error");
    throw buf;
  }

  return cb;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cart::setLogger(ostream* out)
{
  myLog = out;
  myProgrammer.setLogger(out);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cart::setConnectionAttempts(uInt32 attempt)
{
  myProgrammer.setConnectionAttempts(attempt);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cart::setRetry(uInt32 retry)
{
  myProgrammer.setRetry(retry);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 Cart::ourARHeader[256] = {
  0xac, 0xfa, 0x0f, 0x18, 0x62, 0x00, 0x24, 0x02,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0x00, 0x04, 0x08, 0x0c, 0x10, 0x14, 0x18, 0x1c,
  0x01, 0x05, 0x09, 0x0d, 0x11, 0x15, 0x19, 0x1d,
  0x02, 0x06, 0x0a, 0x0e, 0x12, 0x16, 0x1a, 0x1e,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00
};
