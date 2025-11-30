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

#include <QApplication>
#include <QPixmap>
#include <QIcon>
#include <QString>
#include <QDir>
#include <cstring>
#include <ostream>
#include <time.h>

#include "Bankswitch.hxx"
#include "Cart.hxx"
#include "CartDetectorWrapper.hxx"
#include "SerialPort.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cart::Cart()
{
  myProgress.setWindowModality(Qt::WindowModal);
  myProgress.setWindowIcon(QPixmap(":icons/pics/appicon.png"));
  myProgress.reset();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cart::setLogger(ostream* out)
{
  myLog = out;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cart::skipF4CompressionOnBank0(bool skip)
{
   myF4FirstCompressionBank = skip ? 1 : 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Cart::autodetectHarmony(SerialPort& port)
{
  resetTarget(port, PROGRAM_MODE);
  port.clearBuffers();

  // Get the version #, if any
  string result = lpc_NxpChipVersion(port);
  if (strncmp(result.c_str(), "ERROR:", 6) == 0)
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
      result = lpc_NxpDownload(port, bios.get(), static_cast<uInt32>(size),
                               verify, showprogress, continueOnError);
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
    result = lpc_NxpDownload(port, binary, static_cast<uInt32>(size),
                             verify, showprogress, continueOnError);
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
  *myLog << "Reading from file: \'" << filename.c_str() << "\' ... ";

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
void Cart::resetTarget(SerialPort& port, TARGET_MODE mode)
{
  switch (mode)
  {
    // Reset and jump to boot loader
    case PROGRAM_MODE:
      port.controlModemLines(1, 1);
      port.sleepMillis(100);
      port.clearBuffers();
      port.sleepMillis(100);
      port.controlModemLines(0, 1);
      // Longer delay is the Reset signal is conected to an external rest controller
      port.sleepMillis(500);
      // Clear the RTS line after having reset the micro
      // Needed for the "GO <Address> <Mode>" ISP command to work
      port.controlModemLines(0, 0);
      break;

    // Reset and start uploaded program
    case RUN_MODE:
      port.controlModemLines(1, 0);
      port.sleepMillis(100);
      port.clearBuffers();
      port.sleepMillis(100);
      port.controlModemLines(0, 0);
      port.sleepMillis(100);
      break;
  }
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
void Cart::initializeProgress(const QString& title, int minimum, int maximum)
{
  myProgress.setWindowTitle(title);
  myProgress.setRange(minimum, maximum);
  myProgress.setMinimumDuration(0);
  myProgress.setValue(0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cart::updateProgressText(const QString& text)
{
  myProgress.setLabelText(text);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cart::updateProgressValue(int step)
{
  myProgress.setValue(step);
  return !myProgress.wasCanceled();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cart::finalizeProgress()
{
  myProgress.setValue(myProgress.maximum());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Cart::lpc_NxpChipVersion(SerialPort& port)
{
  int found, i;
  int strippedsize;
  char Answer[128], temp[128];
  char *strippedAnswer, *endPtr;
  const char* cmdstr;

  static char version[1024] = { 0 };

  for (uInt32 nQuestionMarks = found = 0; !found && nQuestionMarks < myConnectionAttempts; nQuestionMarks++)
  {
    port.send("?");

    memset(Answer, 0, sizeof(Answer));
    strippedsize = static_cast<int>(port.receive(Answer, sizeof(Answer)-1, 1, 100));
    strippedAnswer = Answer;

    while ((strippedsize > 0) && ((*strippedAnswer == '?') || (*strippedAnswer == 0)))
    {
      strippedAnswer++;
      strippedsize--;
    }

    lpc_FormatCommand(strippedAnswer, strippedAnswer);
    if (strcmp(strippedAnswer, "Synchronized\n") == 0)
      found = 1;
    else
      resetTarget(port, PROGRAM_MODE);
  } // end for

  if (!found)
  {
    strcpy(version, "ERROR: no answer on \'?\'");
    return version;
  }

  port.send("Synchronized\r\n");
  port.receive(Answer, sizeof(Answer) - 1, 2, 1000);

  lpc_FormatCommand(Answer, Answer);
  if (strcmp(Answer, "Synchronized\nOK\n") != 0)
  {
    strcpy(version, "ERROR: No answer on \'Synchronized\'");
    return version;
  }

  sprintf(temp, "%s\r\n", myOscillator.c_str());
  port.send(temp);
  port.receive(Answer, sizeof(Answer)-1, 2, 1000);

  sprintf(temp, "%s\nOK\n", myOscillator.c_str());
  lpc_FormatCommand(Answer, Answer);
  if (strcmp(Answer, temp) != 0)
  {
    strcpy(version, "ERROR: No answer on Oscillator-Command");
    return version;
  }

  cmdstr = "U 23130\r\n";
  if (!lpc_SendAndVerify(port, cmdstr, Answer, sizeof Answer))
  {
    sprintf(version, "ERROR: Unlock-Command: %d", (UNLOCK_ERROR + lpc_GetAndReportErrorNumber(Answer)));
    return version;
  }

  cmdstr = "K\r\n";
  port.send(cmdstr);
  port.receive(Answer, sizeof(Answer)-1, 4, 5000);

  lpc_FormatCommand(cmdstr, temp);
  lpc_FormatCommand(Answer, Answer);
  if (strncmp(Answer, temp, strlen(temp)) != 0)
  {
    strcpy(version, "ERROR: no answer on Read Boot Code Version");
    return version;
  }

  if (strncmp(Answer + strlen(temp), "0\n", 2) == 0)
    strippedAnswer = Answer + strlen(temp) + 2;

  cmdstr = "J\r\n";
  port.send(cmdstr);
  port.receive(Answer, sizeof(Answer)-1, 3, 5000);

  lpc_FormatCommand(cmdstr, temp);
  lpc_FormatCommand(Answer, Answer);
  if (strncmp(Answer, temp, strlen(temp)) != 0)
  {
    strcpy(version, "ERROR: no answer on Read Part Id");
    return version;
  }

  strippedAnswer = (strncmp(Answer, "J\n0\n", 4) == 0) ? Answer + 4 : Answer;

  uInt32 Id[2];
  Id[0] = strtoul(strippedAnswer, &endPtr, 10);
  Id[1] = 0UL;
  *endPtr = '\0'; /* delete \r\n */
  for (i = sizeof LPCtypes / sizeof LPCtypes[0] - 1; i > 0 && LPCtypes[i].id != Id[0]; i--)
    /* nothing */;

  myDetectedDevice = i;

  if (LPCtypes[myDetectedDevice].EvalId2 != 0)
  {
    /* Read out the second configuration word and run the search again */
    *endPtr = '\n';
    endPtr++;
    if ((endPtr[0] == '\0') || (endPtr[strlen(endPtr)-1] != '\n'))
    {
      /* No or incomplete word 2 */
      port.receive(endPtr, sizeof(Answer)-(endPtr-Answer)-1, 1, 100);
    }

    lpc_FormatCommand(endPtr, endPtr);
    if ((*endPtr == '\0') || (*endPtr == '\n'))
      return "";

    Id[1] = strtoul(endPtr, &endPtr, 10);
    *endPtr = '\0'; /* delete \r\n */

    uInt32 Id1Masked = Id[1] & 0xFF;

    /* now search the table again */
    for (i = sizeof LPCtypes / sizeof LPCtypes[0] - 1; i > 0 &&
        (LPCtypes[i].id != Id[0] || LPCtypes[i].id2 != Id1Masked); i--)
      /* nothing */;
    myDetectedDevice = i;
  }
  if (myDetectedDevice != 0)
  {
    char version[100];
    sprintf(version, "LPC%s, %d kiB FLASH / %d kiB SRAM",
            LPCtypes[myDetectedDevice].Product,
            LPCtypes[myDetectedDevice].FlashSize,
            LPCtypes[myDetectedDevice].RAMSize);
    return version;
  }

  return "unknown";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Cart::lpc_NxpDownload(SerialPort& port, uInt8* data, uInt32 size,
                             bool verify, bool showprogress, bool continueOnError)
{
  auto handleError = [&](const string& result, bool fatalError = false)
  {
    if(!continueOnError || fatalError)
    {
      if(showprogress)
        finalizeProgress();

      throw std::runtime_error(result);
    }
    else
      *myLog << result << '\n';
  };

  char Answer[128], ExpectedAnswer[128], temp[128];
  char *strippedAnswer{nullptr}, *endPtr{nullptr};
  int strippedsize{0};
  int found{0};
  uInt32 Sector{0};
  uInt32 SectorLength{0};
  uInt32 SectorStart{0}, SectorOffset{0}, SectorChunk{0};
  char tmpString[128];
  char uuencode_table[64];
  int Line{0};
  uInt32 tmpStringPos{0};
  uInt32 BlockOffset{0};
  uInt32 Block{0};
  uInt32 Pos{0};
  uInt32 Id[2];
  uInt32 Id1Masked{0};
  uInt32 CopyLength{0};
  int c{0},k{0},i{0};
  uInt32 ivt_CRC{0};          // CRC over interrupt vector table
  uInt32 block_CRC{0};
  time_t tStartUpload{0}, tDoneUpload{0};
  const char* cmdstr{nullptr};
  uInt32 repeat{0};
  ostringstream result;

  // Puffer for data to resend after "RESEND\r\n" Target responce
  char sendbuf0[128], sendbuf1[128], sendbuf2[128], sendbuf3[128], sendbuf4[128],
       sendbuf5[128], sendbuf6[128], sendbuf7[128], sendbuf8[128], sendbuf9[128],
       sendbuf10[128],sendbuf11[128],sendbuf12[128],sendbuf13[128],sendbuf14[128],
       sendbuf15[128],sendbuf16[128],sendbuf17[128],sendbuf18[128],sendbuf19[128];

  char* sendbuf[20] = {
    sendbuf0,  sendbuf1,  sendbuf2,  sendbuf3,  sendbuf4,
    sendbuf5,  sendbuf6,  sendbuf7,  sendbuf8,  sendbuf9,
    sendbuf10, sendbuf11, sendbuf12, sendbuf13, sendbuf14,
    sendbuf15, sendbuf16, sendbuf17, sendbuf18, sendbuf19
  };

  // Make sure the data is aligned to 32-bits, and copy to internal buffer
  uInt32 BinaryOffset = 0, StartAddress = 0, BinaryLength = size;
  if(BinaryLength % 4 != 0)
  {
    uInt32 newBinaryLength = ((BinaryLength + 3)/4) * 4;
    *myLog << "Warning:  data not aligned to 32 bits, padded (length was "
          << BinaryLength << ", now " << newBinaryLength << ")\n";
    BinaryLength = newBinaryLength;
  }
  ByteBuffer binaryContent = make_unique<uInt8[]>(BinaryLength);
  memcpy(binaryContent.get(), data, size);
  uInt32 progressStep = 0;
  if(showprogress)
    initializeProgress("Updating Flash", 0, BinaryLength/45 + 20);

  *myLog << "Synchronizing";

  for (uInt32 nQuestionMarks = found = 0; !found && nQuestionMarks < myConnectionAttempts; nQuestionMarks++)
  {
    *myLog << ".";
    port.send("?");

    memset(Answer, 0, sizeof(Answer));
    strippedsize = static_cast<int>(port.receive(Answer, sizeof(Answer)-1, 1, 100));
    strippedAnswer = Answer;

    while ((strippedsize > 0) && ((*strippedAnswer == '?') || (*strippedAnswer == 0)))
    {
      strippedAnswer++;
      strippedsize--;
    }

    tStartUpload = time(NULL);

    lpc_FormatCommand(strippedAnswer, strippedAnswer);
    if (strcmp(strippedAnswer, "Synchronized\n") == 0)
      found = 1;
    else
      resetTarget(port, PROGRAM_MODE);
  } // end for

  if(!found)
    handleError("ERROR: no answer on '?'", true);

  *myLog << " OK\n";

  port.send("Synchronized\r\n");
  port.receive(Answer, sizeof(Answer) - 1, 2, 1000);

  lpc_FormatCommand(Answer, Answer);
  if (strcmp(Answer, "Synchronized\nOK\n") != 0)
    handleError("ERROR: No answer on 'Synchronized'", true);

  sprintf(temp, "%s\r\n", myOscillator.c_str());
  port.send(temp);
  port.receive(Answer, sizeof(Answer)-1, 2, 1000);

  sprintf(temp, "%s\nOK\n", myOscillator.c_str());
  lpc_FormatCommand(Answer, Answer);
  if (strcmp(Answer, temp) != 0)
    handleError("ERROR: No answer on Oscillator-Command", true);

  cmdstr = "U 23130\r\n";
  if (!lpc_SendAndVerify(port, cmdstr, Answer, sizeof Answer))
  {
    result << "ERROR: Unlock-Command: " << lpc_GetAndReportErrorNumber(Answer);
    handleError(result.str(), true);
  }

  *myLog << "Read bootcode version: ";

  cmdstr = "K\r\n";
  port.send(cmdstr);
  port.receive(Answer, sizeof(Answer)-1, 4, 5000);

  lpc_FormatCommand(cmdstr, temp);
  lpc_FormatCommand(Answer, Answer);
  if (strncmp(Answer, temp, strlen(temp)) != 0)
    handleError("ERROR: no answer on Read Boot Code Version", true);

  if (strncmp(Answer + strlen(temp), "0\n", 2) == 0)
  {
    strippedAnswer = Answer + strlen(temp) + 2;
    *myLog << strippedAnswer;
  }
  else
    *myLog << "unknown\n";

  *myLog << "Read part ID: ";

  cmdstr = "J\r\n";
  port.send(cmdstr);
  port.receive(Answer, sizeof(Answer)-1, 3, 5000);

  lpc_FormatCommand(cmdstr, temp);
  lpc_FormatCommand(Answer, Answer);
  if (strncmp(Answer, temp, strlen(temp)) != 0)
    handleError("ERROR: no answer on Read Part Id", true);

  strippedAnswer = (strncmp(Answer, "J\n0\n", 4) == 0) ? Answer + 4 : Answer;

  Id[0] = strtoul(strippedAnswer, &endPtr, 10);
  Id[1] = 0UL;
  *endPtr = '\0'; /* delete \r\n */
  for (i = sizeof LPCtypes / sizeof LPCtypes[0] - 1; i > 0 && LPCtypes[i].id != Id[0]; i--)
    /* nothing */;

  myDetectedDevice = i;

  if (LPCtypes[myDetectedDevice].EvalId2 != 0)
  {
    /* Read out the second configuration word and run the search again */
    *endPtr = '\n';
    endPtr++;
    if ((endPtr[0] == '\0') || (endPtr[strlen(endPtr)-1] != '\n'))
    {
      /* No or incomplete word 2 */
      port.receive(endPtr, sizeof(Answer)-(endPtr-Answer)-1, 1, 100);
    }

    lpc_FormatCommand(endPtr, endPtr);
    if ((*endPtr == '\0') || (*endPtr == '\n'))
      handleError("ERROR: incomplete answer on Read Part Id (second configuration word missing)", true);

    Id[1] = strtoul(endPtr, &endPtr, 10);
    *endPtr = '\0'; /* delete \r\n */

    Id1Masked = Id[1] & 0xFF;

    /* now search the table again */
    for (i = sizeof LPCtypes / sizeof LPCtypes[0] - 1; i > 0 &&
        (LPCtypes[i].id != Id[0] || LPCtypes[i].id2 != Id1Masked); i--)
      /* nothing */;
    myDetectedDevice = i;
  }
  if (myDetectedDevice == 0)
  {
    *myLog << "unknown";
  }
  else
  {
    char version[100];
    sprintf(version, "LPC%s, %d kiB FLASH / %d kiB SRAM",
            LPCtypes[myDetectedDevice].Product,
            LPCtypes[myDetectedDevice].FlashSize,
            LPCtypes[myDetectedDevice].RAMSize);
    *myLog << version;
  }
  if (LPCtypes[myDetectedDevice].EvalId2 != 0)
    *myLog << " (" << std::hex << Id[0] << "/" << Id[1] << " -> " << Id1Masked << std::dec << ")\n";
  else
    *myLog << " (" << std::hex << Id[0] << std::dec << ")\n";

  // Make sure the data can fit in the flash we have available
  if(size > LPCtypes[myDetectedDevice].FlashSize * 1024)
    handleError("ERROR: Data to large for available flash", true);

  if (true /*!IspEnvironment->DetectOnly*/)
  {
    // Build up uuencode table
    uuencode_table[0] = 0x60;           // 0x20 is translated to 0x60 !

    for (int i = 1; i < 64; i++)
      uuencode_table[i] = (char)(0x20 + i);

    if(LPCtypes[myDetectedDevice].ChipVariant == CHIP_VARIANT_LPC2XXX)
    {
      // Patch 0x14, otherwise it is not running and jumps to boot mode
      ivt_CRC = 0;

      // Clear the vector at 0x14 so it doesn't affect the checksum:
      for(int i = 0; i < 4; i++)
        binaryContent[i + 0x14] = 0;

      // Calculate a native checksum of the little endian vector table:
      for(int i = 0; i < (4 * 8);)
      {
        ivt_CRC += binaryContent[i++];
        ivt_CRC += binaryContent[i++] << 8;
        ivt_CRC += binaryContent[i++] << 16;
        ivt_CRC += binaryContent[i++] << 24;
      }

      /* Negate the result and place in the vector at 0x14 as little endian
       * again. The resulting vector table should checksum to 0. */
      ivt_CRC = (uInt32) (0 - ivt_CRC);
      for (int i = 0; i < 4; i++)
        binaryContent[i + 0x14] = (unsigned char)(ivt_CRC >> (8 * i));
    }
    else if(auto type = LPCtypes[myDetectedDevice].ChipVariant;
            type == CHIP_VARIANT_LPC43XX || type == CHIP_VARIANT_LPC18XX || type == CHIP_VARIANT_LPC17XX ||
            type == CHIP_VARIANT_LPC13XX || type == CHIP_VARIANT_LPC11XX || type == CHIP_VARIANT_LPC8XX)
    {
      // Patch 0x1C, otherwise it is not running and jumps to boot mode
      ivt_CRC = 0;

      // Clear the vector at 0x1C so it doesn't affect the checksum:
      for (int i = 0; i < 4; i++)
        binaryContent[i + 0x1C] = 0;

      // Calculate a native checksum of the little endian vector table:
      for (int i = 0; i < (4 * 8);)
      {
        ivt_CRC += binaryContent[i++];
        ivt_CRC += binaryContent[i++] << 8;
        ivt_CRC += binaryContent[i++] << 16;
        ivt_CRC += binaryContent[i++] << 24;
      }

      /* Negate the result and place in the vector at 0x1C as little endian
       * again. The resulting vector table should checksum to 0. */
      ivt_CRC = (uInt32) (0 - ivt_CRC);
      for (int i = 0; i < 4; i++)
        binaryContent[i + 0x1C] = (unsigned char)(ivt_CRC >> (8 * i));
    }
    else
    {
      result << "ERROR: wrong chip variant " << LPCtypes[myDetectedDevice].ChipVariant
             << " (detected device "<< myDetectedDevice << ")\n";
      handleError(result.str(), true);
    }
  }

  /* In case of a download to RAM, use full RAM for downloading
   * set the flash parameters to full RAM also.
   * This makes sure that all code is downloaded as one big sector
   */
  if ( (BinaryOffset >= lpc_ReturnValueLpcRamStart()) &&
       (BinaryOffset + BinaryLength <= lpc_ReturnValueLpcRamStart() +
                                       (LPCtypes[myDetectedDevice].RAMSize*1024)))
  {
    LPCtypes[myDetectedDevice].FlashSectors = 1;
    LPCtypes[myDetectedDevice].MaxCopySize  = LPCtypes[myDetectedDevice].RAMSize*1024 - (lpc_ReturnValueLpcRamBase() - lpc_ReturnValueLpcRamStart());
    LPCtypes[myDetectedDevice].SectorTable  = SectorTable_RAM;
    SectorTable_RAM[0] = LPCtypes[myDetectedDevice].MaxCopySize;
  }

  if(LPCtypes[myDetectedDevice].ChipVariant == CHIP_VARIANT_LPC8XX)
  {
    // XON/XOFF must be switched off for LPC8XX
    // otherwise problem during binary transmission of data to LPC8XX
    *myLog << "Switch off XON/XOFF !!!";
    port.controlXonXoff(0);
  }

  // Start with sector 1 and go upward... Sector 0 containing the interrupt vectors
  // will be loaded last, since it contains a checksum and device will re-enter
  // bootloader mode as long as this checksum is invalid.
  *myLog << "Will start programming at Sector 1 if possible, and conclude with Sector 0 to ensure that checksum is written last.\n";
  if (LPCtypes[myDetectedDevice].SectorTable[0] >= BinaryLength)
  {
    Sector = 0;
    SectorStart = 0;
  }
  else
  {
    SectorStart = LPCtypes[myDetectedDevice].SectorTable[0];
    Sector = 1;
  }

  // Erasing sector 0 first
  *myLog << "Erasing sector 0 first, to invalidate checksum. ";

  if (auto type = LPCtypes[myDetectedDevice].ChipVariant;
      type == CHIP_VARIANT_LPC43XX || type == CHIP_VARIANT_LPC18XX)
  {
    // TODO: Quick and dirty hack to address bank 0
    sprintf(tmpString, "P %d %d 0\r\n", 0, 0);
  }
  else
    sprintf(tmpString, "P %d %d\r\n", 0, 0);

  if (!lpc_SendAndVerify(port, tmpString, Answer, sizeof Answer))
  {
    result << "ERROR: Wrong answer on Prepare-Command " << lpc_GetAndReportErrorNumber(Answer);
    handleError(result.str());
  }

  if (auto type = LPCtypes[myDetectedDevice].ChipVariant;
      type == CHIP_VARIANT_LPC43XX || type == CHIP_VARIANT_LPC18XX)
  {
    // TODO: Quick and dirty hack to address bank 0
    sprintf(tmpString, "E %d %d 0\r\n", 0, 0);
  }
  else
    sprintf(tmpString, "E %d %d\r\n", 0, 0);

  if (!lpc_SendAndVerify(port, tmpString, Answer, sizeof Answer))
  {
    result << "ERROR: Wrong answer on Erase-Command " << lpc_GetAndReportErrorNumber(Answer);
    handleError(result.str());
  }

  *myLog << "OK \n";

  // OK, the main loop where we start writing to the cart
  while (1)
  {
    if (Sector >= LPCtypes[myDetectedDevice].FlashSectors)
      handleError("ERROR: Program too large; running out of Flash sectors", true);

    *myLog << "Sector " << Sector << std::flush;
    if(showprogress)
      updateProgressText("Downloading sector " + QString::number(Sector) + " ...                  ");

    if ( BinaryOffset < lpc_ReturnValueLpcRamStart()  // Skip Erase when running from RAM
         || (BinaryOffset >= lpc_ReturnValueLpcRamStart() + (LPCtypes[myDetectedDevice].RAMSize*1024)))
    {
      if (auto type = LPCtypes[myDetectedDevice].ChipVariant;
        type == CHIP_VARIANT_LPC43XX || type == CHIP_VARIANT_LPC18XX)
      {
        // TODO: Quick and dirty hack to address bank 0
        sprintf(tmpString, "P %d %d 0\r\n", Sector, Sector);
      }
      else
        sprintf(tmpString, "P %d %d\r\n", Sector, Sector);

      if (!lpc_SendAndVerify(port, tmpString, Answer, sizeof Answer))
      {
        result << "ERROR: Wrong answer on Prepare-Command (1) (Sector " << Sector << ") "
               << lpc_GetAndReportErrorNumber(Answer);
        handleError(result.str());
      }

      *myLog << "." << std::flush;

      if (Sector != 0) // Sector 0 already erased
      {
        if (auto type = LPCtypes[myDetectedDevice].ChipVariant;
          type == CHIP_VARIANT_LPC43XX || type == CHIP_VARIANT_LPC18XX)
        {
          // TODO: Quick and dirty hack to address bank 0
          sprintf(tmpString, "E %d %d 0\r\n", Sector, Sector);
        }
        else
          sprintf(tmpString, "E %d %d\r\n", Sector, Sector);

        if (!lpc_SendAndVerify(port, tmpString, Answer, sizeof Answer))
        {
          result << "ERROR: Wrong answer on Erase-Command (Sector " << Sector << ") "
                 << lpc_GetAndReportErrorNumber(Answer);
          handleError(result.str());
        }

        *myLog << "." << std::flush;
      }
    }

    SectorLength = LPCtypes[myDetectedDevice].SectorTable[Sector];
    if (SectorLength > BinaryLength - SectorStart)
      SectorLength = BinaryLength - SectorStart;

    for (SectorOffset = 0; SectorOffset < SectorLength; SectorOffset += SectorChunk)
    {
      // Check if we are to write only 0xFFs - it would be just a waste of time..
      if (SectorOffset == 0)
      {
        for (SectorOffset = 0; SectorOffset < SectorLength; ++SectorOffset)
          if (binaryContent[SectorStart + SectorOffset] != 0xFF)
            break;

        if (SectorOffset == SectorLength) // all data contents were 0xFFs
        {
          *myLog << "Whole sector contents is 0xFFs, skipping programming." << std::flush;
          break;
        }
        SectorOffset = 0; // re-set otherwise
      }

      if (SectorOffset > 0)
        *myLog << "|" << std::flush;

      // If the Flash ROM sector size is bigger than the number of bytes
      // we can copy from RAM to Flash, we must "chop up" the sector and
      // copy these individually.
      // This is especially needed in the case where a Flash sector is
      // bigger than the amount of SRAM.
      SectorChunk = SectorLength - SectorOffset;
      if (SectorChunk > (unsigned)LPCtypes[myDetectedDevice].MaxCopySize)
        SectorChunk = LPCtypes[myDetectedDevice].MaxCopySize;

      // Write multiple of 45 * 4 Byte blocks to RAM, but copy maximum of on sector to Flash
      // In worst case we transfer up to 180 byte too much to RAM
      // but then we can always use full 45 byte blocks and length is multiple of 4
      CopyLength = SectorChunk;

      if(auto type = LPCtypes[myDetectedDevice].ChipVariant;
          type == CHIP_VARIANT_LPC2XXX || type == CHIP_VARIANT_LPC17XX || type == CHIP_VARIANT_LPC13XX ||
          type == CHIP_VARIANT_LPC11XX || type == CHIP_VARIANT_LPC18XX || type == CHIP_VARIANT_LPC43XX)
      {
        if ((CopyLength % (45 * 4)) != 0)
          CopyLength += ((45 * 4) - (CopyLength % (45 * 4)));
      }

      sprintf(tmpString, "W %d %d\r\n", lpc_ReturnValueLpcRamBase(), CopyLength);
      if (!lpc_SendAndVerify(port, tmpString, Answer, sizeof Answer))
      {
        result << "ERROR: Wrong answer on Write-Command " << lpc_GetAndReportErrorNumber(Answer);
        handleError(result.str());
      }

      *myLog << "." << std::flush;

      if(auto type = LPCtypes[myDetectedDevice].ChipVariant;
          type == CHIP_VARIANT_LPC2XXX || type == CHIP_VARIANT_LPC17XX || type == CHIP_VARIANT_LPC13XX ||
          type == CHIP_VARIANT_LPC11XX || type == CHIP_VARIANT_LPC18XX || type == CHIP_VARIANT_LPC43XX)
      {
        block_CRC = 0;
        Line = 0;

        // Transfer blocks of 45 * 4 bytes to RAM
        for (Pos = SectorStart + SectorOffset; (Pos < SectorStart + SectorOffset + CopyLength) && (Pos < BinaryLength); Pos += (45 * 4))
        {
          for (Block = 0; Block < 4; Block++)  // Each block 45 bytes
          {
            *myLog << "." << std::flush;

            // Inform the calling application about having written another chuck of data
            if(showprogress)
            {
              if(!updateProgressValue(++progressStep))
                handleError("Cancelled download", true);
            }

            // Uuencode one 45 byte block
            tmpStringPos = 0;

            sendbuf[Line][tmpStringPos++] = (char)(' ' + 45);    // Encode Length of block

            for (BlockOffset = 0; BlockOffset < 45; BlockOffset++)
            {
              if (BinaryOffset < lpc_ReturnValueLpcRamStart() ||
                 (BinaryOffset >= lpc_ReturnValueLpcRamStart()+(LPCtypes[myDetectedDevice].RAMSize*1024)))
              { // Flash: use full memory
                c = binaryContent[Pos + Block * 45 + BlockOffset];
              }
              else
              { // RAM: Skip first 0x200 bytes, these are used by the download program in LPC21xx
                c = binaryContent[Pos + Block * 45 + BlockOffset + 0x200];
              }

              block_CRC += c;
              k = (k << 8) + (c & 255);

              if ((BlockOffset % 3) == 2)   // Collecting always 3 Bytes, then do processing in 4 Bytes
              {
                sendbuf[Line][tmpStringPos++] = uuencode_table[(k >> 18) & 63];
                sendbuf[Line][tmpStringPos++] = uuencode_table[(k >> 12) & 63];
                sendbuf[Line][tmpStringPos++] = uuencode_table[(k >>  6) & 63];
                sendbuf[Line][tmpStringPos++] = uuencode_table[ k        & 63];
              }
            }
            sendbuf[Line][tmpStringPos++] = '\r';
            sendbuf[Line][tmpStringPos++] = '\n';
            sendbuf[Line][tmpStringPos++] = 0;
            port.send(sendbuf[Line]);

            // receive only for debug purposes
            port.receive(Answer, sizeof(Answer)-1, 1, 5000);
            lpc_FormatCommand(sendbuf[Line], tmpString);
            lpc_FormatCommand(Answer, Answer);
            if (strncmp(Answer, tmpString, strlen(tmpString)) != 0)
              handleError("Error on writing data (1)");

            Line++;
            if (Line == 20)
            {
              for (repeat = 0; repeat < myRetry; repeat++)
              {
                sprintf(tmpString, "%d\r\n", block_CRC);
                port.send(tmpString);
                port.receive(Answer, sizeof(Answer)-1, 2, 5000);

                sprintf(tmpString, "%d\nOK\n", block_CRC);
                lpc_FormatCommand(tmpString, tmpString);
                lpc_FormatCommand(Answer, Answer);
                if (strcmp(Answer, tmpString) != 0)
                {
                  for (i = 0; i < Line; i++)
                  {
                    port.send(sendbuf[i]);
                    port.receive(Answer, sizeof(Answer)-1, 1, 5000);
                  }
                }
                else
                  break;
              }

              if (repeat >= myRetry)
              {
                result << "ERROR: writing block_CRC (1), retries = " << repeat;
                handleError(result.str(), true);
              }

              Line = 0;
              block_CRC = 0;
            }
          }
        }

        if (Line != 0)
        {
          for (repeat = 0; repeat < myRetry; repeat++)
          {
            sprintf(tmpString, "%d\r\n", block_CRC);
            port.send(tmpString);
            port.receive(Answer, sizeof(Answer)-1, 2, 5000);

            sprintf(tmpString, "%d\nOK\n", block_CRC);
            lpc_FormatCommand(tmpString, tmpString);
            lpc_FormatCommand(Answer, Answer);
            if (strcmp(Answer, tmpString) != 0)
            {
              for (i = 0; i < Line; i++)
              {
                port.send(sendbuf[i]);
                port.receive(Answer, sizeof(Answer)-1, 1, 5000);
              }
            }
            else
              break;
          }

          if (repeat >= myRetry)
          {
            result << "ERROR: writing block_CRC (3), retries = " << repeat;
            handleError(result.str(), true);
          }
        }
      }
      else if (LPCtypes[myDetectedDevice].ChipVariant == CHIP_VARIANT_LPC8XX)
      {
        uInt8 BigAnswer[4096];
        uInt32 CopyLengthPartialOffset = 0;
        uInt32 CopyLengthPartialRemainingBytes;

        while (CopyLengthPartialOffset < CopyLength)
        {
          CopyLengthPartialRemainingBytes = CopyLength - CopyLengthPartialOffset;
          if (CopyLengthPartialRemainingBytes > 256)
          {
            // There seems to be an error in LPC812:
            // When too much bytes are written at high speed,
            // bytes get lost
            // Workaround: Use smaller blocks
            CopyLengthPartialRemainingBytes = 256;
          }

          const void* data = binaryContent.get() + (SectorStart + SectorOffset + CopyLengthPartialOffset);
          port.send(data, CopyLengthPartialRemainingBytes);

          if (port.receiveCompleteBlock(&BigAnswer, CopyLengthPartialRemainingBytes, 10000) != 0)
            handleError("ERROR_WRITE_DATA");

          if(std::memcmp(binaryContent.get() + (SectorStart + SectorOffset + CopyLengthPartialOffset), BigAnswer, CopyLengthPartialRemainingBytes))
            handleError("ERROR_WRITE_DATA");

          CopyLengthPartialOffset += CopyLengthPartialRemainingBytes;
        }
      }

      if (BinaryOffset < lpc_ReturnValueLpcRamStart() ||
          BinaryOffset >= lpc_ReturnValueLpcRamStart() + (LPCtypes[myDetectedDevice].RAMSize*1024))
      {
        // Prepare command must be repeated before every write
        if (auto type = LPCtypes[myDetectedDevice].ChipVariant;
            type == CHIP_VARIANT_LPC43XX || type == CHIP_VARIANT_LPC18XX)
        {
          // TODO: Quick and dirty hack to address bank 0
          sprintf(tmpString, "P %d %d 0\r\n", Sector, Sector);
        }
        else
          sprintf(tmpString, "P %d %d\r\n", Sector, Sector);

        if (!lpc_SendAndVerify(port, tmpString, Answer, sizeof Answer))
        {
          result << "ERROR: Wrong answer on Prepare-Command (2) (Sector "
                 << Sector << ") " << lpc_GetAndReportErrorNumber(Answer);
          handleError(result.str());
        }

        // Round CopyLength up to one of the following values: 512, 1024,
        // 4096, 8192; but do not exceed the maximum copy size (usually
        // 8192, but chip-dependent)
        if (CopyLength < 512)
          CopyLength = 512;
        else if (SectorLength < 1024)
          CopyLength = 1024;
        else if (SectorLength < 4096)
          CopyLength = 4096;
        else
          CopyLength = 8192;

        if (CopyLength > (unsigned)LPCtypes[myDetectedDevice].MaxCopySize)
          CopyLength = LPCtypes[myDetectedDevice].MaxCopySize;

        sprintf(tmpString, "C %d %d %d\r\n", BinaryOffset + SectorStart + SectorOffset,
                lpc_ReturnValueLpcRamBase(), CopyLength);
        if (!lpc_SendAndVerify(port, tmpString, Answer, sizeof Answer))
        {
          result << "ERROR: Wrong answer on Copy-Command " << lpc_GetAndReportErrorNumber(Answer);
          handleError(result.str());
        }

        if (verify)
        {
          // Avoid compare first 64 bytes.
          // Because first 64 bytes are re-mapped to flash boot sector,
          // and the compare result may not be correct.
          if (SectorStart + SectorOffset<64)
            sprintf(tmpString, "M %d %d %d\r\n", 64, lpc_ReturnValueLpcRamBase() +
                (64 - SectorStart - SectorOffset), CopyLength-(64 - SectorStart - SectorOffset));
          else
            sprintf(tmpString, "M %d %d %d\r\n", SectorStart + SectorOffset,
                lpc_ReturnValueLpcRamBase(), CopyLength);

          if (!lpc_SendAndVerify(port, tmpString, Answer, sizeof Answer))
          {
            result << "ERROR: Wrong answer on Compare-Command " << lpc_GetAndReportErrorNumber(Answer);
            handleError(result.str());
          }
        }
      }
    }

    *myLog << "\n" << std::flush;

    if ((SectorStart + SectorLength) >= BinaryLength && Sector!=0)
    {
      Sector = 0;
      SectorStart = 0;
    }
    else if (Sector == 0)
    {
      break;
    }
    else
    {
      SectorStart += LPCtypes[myDetectedDevice].SectorTable[Sector];
      Sector++;
    }
  }

  ostringstream returnVal;
  tDoneUpload = time(NULL);
  if (verify)
    returnVal << "Download Finished and Verified correct... taking " << int(tDoneUpload - tStartUpload) << " seconds";
  else
    returnVal << "Download Finished... taking " << int(tDoneUpload - tStartUpload) << " seconds";

  // For LPC18xx set boot bank to 0
  if (auto type = LPCtypes[myDetectedDevice].ChipVariant;
      type == CHIP_VARIANT_LPC43XX || type == CHIP_VARIANT_LPC18XX)
  {
    if (!lpc_SendAndVerify(port, "S 0\r\n", Answer, sizeof Answer))
    {
      result << "Wrong answer on SetActiveBootFlashBank-Command " << lpc_GetAndReportErrorNumber(Answer);
      handleError(result.str());
    }
  }

  if (1 /* IspEnvironment->DoNotStart == 0*/)
  {
    *myLog << "Now launching the brand new code\n" << std::flush;

    auto type = LPCtypes[myDetectedDevice].ChipVariant;
    if(type == CHIP_VARIANT_LPC2XXX)
      sprintf(tmpString, "G %d A\r\n", StartAddress);
    else if(type == CHIP_VARIANT_LPC43XX || type == CHIP_VARIANT_LPC18XX || type == CHIP_VARIANT_LPC17XX ||
            type == CHIP_VARIANT_LPC13XX || type == CHIP_VARIANT_LPC11XX)
      sprintf(tmpString, "G %d T\r\n", StartAddress & ~1);
    else if(type == CHIP_VARIANT_LPC8XX)
      sprintf(tmpString, "G 0 T\r\n");
    else
      handleError("Internal Error", true);

    port.send(tmpString);  //goto 0 : run this fresh new downloaded code code
    if (BinaryOffset < lpc_ReturnValueLpcRamStart() ||
        BinaryOffset >= lpc_ReturnValueLpcRamStart() + (LPCtypes[myDetectedDevice].RAMSize*1024))
    { // Skip response on G command - show response on Terminal instead
      size_t realsize = port.receive(Answer, sizeof(Answer)-1, 2, 5000);
      /* the reply string is frequently terminated with a -1 (EOF) because the
       * connection gets broken; zero-terminate the string ourselves
       */
      while (realsize > 0 && ((signed char) Answer[(int)realsize - 1]) < 0)
        realsize--;

      Answer[(int)realsize] = '\0';
      /* Better to check only the first 9 chars instead of complete receive buffer,
       * because the answer can contain the output by the started programm
       */
      if(type == CHIP_VARIANT_LPC2XXX)
        sprintf(ExpectedAnswer, "G %d A\n0", StartAddress);
      else if(type == CHIP_VARIANT_LPC43XX || type == CHIP_VARIANT_LPC18XX || type == CHIP_VARIANT_LPC17XX ||
              type == CHIP_VARIANT_LPC13XX || type == CHIP_VARIANT_LPC11XX)
        sprintf(ExpectedAnswer, "G %d T\n0", StartAddress & ~1);
      else if(type == CHIP_VARIANT_LPC8XX)
        sprintf(ExpectedAnswer, "G 0 T\n0");
      else
        handleError("Internal Error", true);

      lpc_FormatCommand(Answer, Answer);
      if (realsize == 0 || strncmp((const char *)Answer, /*cmdstr*/ExpectedAnswer, strlen(/*cmdstr*/ExpectedAnswer)) != 0)
      {
        result << "Failed to run the new downloaded code: " << lpc_GetAndReportErrorNumber(Answer);
        handleError(result.str());
      }
    }

    *myLog << std::flush;
  }

  if(showprogress)
    finalizeProgress();

  *myLog << returnVal.str() << '\n';
  return returnVal.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Cart::lpc_SendAndVerify(SerialPort& port, const char* Command,
                            char* AnswerBuffer, int AnswerLength)
{
  port.send(Command);
  port.receive(AnswerBuffer, AnswerLength - 1, 2, 5000);
  size_t cmdlen = strlen(Command);

  char* FormattedCommand = (char*) alloca(cmdlen+1);
  lpc_FormatCommand(Command, FormattedCommand);
  lpc_FormatCommand(AnswerBuffer, AnswerBuffer);
  cmdlen = strlen(FormattedCommand);
  return (strncmp(AnswerBuffer, FormattedCommand, cmdlen) == 0 &&
          strcmp(AnswerBuffer + cmdlen, "0\n") == 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 Cart::lpc_GetAndReportErrorNumber(const char* Answer)
{
  uInt8 Result = 0xFF;    // Error !!!
  uInt32 i = 0;

  while (1)
  {
    if (Answer[i] == 0x00)
      break;

    if (Answer[i] == 0x0a)
    {
      i++;

      if (Answer[i] < '0' || Answer[i] > '9')
        break;

      Result = (unsigned char) (atoi(&Answer[i]));
      break;
    }
    i++;
  }

  return Result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cart::lpc_FormatCommand(const char* In, char* Out)
{
  size_t i, j;
  for (i = 0, j = 0; In[j] != '\0'; i++, j++)
  {
    if ((In[j] == '\r') || (In[j] == '\n'))
    {
      if (i > 0) // Ignore leading line breaks (they must be leftovers from a previous answer)
      {
        Out[i] = '\n';
      }
      else
      {
        i--;
      }
      while ((In[j+1] == '\r') || (In[j+1] == '\n'))
      {
        j++;
      }
    }
    else
    {
      Out[i] = In[j];
    }
  }
  Out[i] = '\0';
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 Cart::lpc_ReturnValueLpcRamStart()
{
  switch(LPCtypes[myDetectedDevice].ChipVariant)
  {
    case CHIP_VARIANT_LPC43XX:  return LPC_RAMSTART_LPC43XX;
    case CHIP_VARIANT_LPC2XXX:  return LPC_RAMSTART_LPC2XXX;
    case CHIP_VARIANT_LPC18XX:  return LPC_RAMSTART_LPC18XX;
    case CHIP_VARIANT_LPC17XX:  return LPC_RAMSTART_LPC17XX;
    case CHIP_VARIANT_LPC13XX:  return LPC_RAMSTART_LPC13XX;
    case CHIP_VARIANT_LPC11XX:  return LPC_RAMSTART_LPC11XX;
    case CHIP_VARIANT_LPC8XX:   return LPC_RAMSTART_LPC8XX;

    default:  return 0;  // TODO - more properly handle this
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 Cart::lpc_ReturnValueLpcRamBase()
{
  switch(LPCtypes[myDetectedDevice].ChipVariant)
  {
    case CHIP_VARIANT_LPC43XX:  return LPC_RAMBASE_LPC43XX;
    case CHIP_VARIANT_LPC2XXX:  return LPC_RAMBASE_LPC2XXX;
    case CHIP_VARIANT_LPC18XX:  return LPC_RAMBASE_LPC18XX;
    case CHIP_VARIANT_LPC17XX:  return LPC_RAMBASE_LPC17XX;
    case CHIP_VARIANT_LPC13XX:  return LPC_RAMBASE_LPC13XX;
    case CHIP_VARIANT_LPC11XX:  return LPC_RAMBASE_LPC11XX;
    case CHIP_VARIANT_LPC8XX:   return LPC_RAMBASE_LPC8XX;

    default:  return 0;  // TODO - more properly handle this
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt32 Cart::SectorTable_210x[] = {
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt32 Cart::SectorTable_2103[] = {
  4096, 4096, 4096, 4096, 4096, 4096, 4096, 4096
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt32 Cart::SectorTable_2109[] = {
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt32 Cart::SectorTable_211x[] = {
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
  8192, 8192, 8192, 8192, 8192, 8192, 8192
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt32 Cart::SectorTable_212x[] = {
   8192,  8192, 8192, 8192, 8192, 8192, 8192, 8192,
  65536, 65536, 8192, 8192, 8192, 8192, 8192, 8192, 8192
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Used for devices with 500K (LPC2138 and LPC2148) and
// for devices with 504K (1 extra 4k block at the end)
const uInt32 Cart::SectorTable_213x[] = {
   4096,  4096,  4096,  4096,  4096,  4096,  4096,  4096,
  32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768,
  32768, 32768, 32768, 32768, 32768, 32768,  4096,  4096,
   4096,  4096,  4096,  4096
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Used for LPC11xx devices
const uInt32 Cart::SectorTable_11xx[] = {
  4096, 4096, 4096, 4096, 4096, 4096, 4096, 4096,
  4096, 4096, 4096, 4096, 4096, 4096, 4096, 4096,
  4096, 4096, 4096, 4096, 4096, 4096, 4096, 4096,
  4096, 4096, 4096, 4096, 4096, 4096, 4096, 4096
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Used for LPC17xx devices
const uInt32 Cart::SectorTable_17xx[] = {
   4096,  4096,  4096,  4096,  4096,  4096,  4096,  4096,
   4096,  4096,  4096,  4096,  4096,  4096,  4096,  4096,
  32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768,
  32768, 32768, 32768, 32768, 32768, 32768
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Used for LPC18xx devices
const uInt32 Cart::SectorTable_18xx[] = {
   8192,  8192,  8192,  8192,  8192,  8192,  8192,  8192,
  65536, 65536, 65536, 65536, 65536, 65536, 65536
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Used for LPC43xx devices
const uInt32 Cart::SectorTable_43xx[] = {
   8192,  8192,  8192,  8192,  8192,  8192,  8192,  8192,
  65536, 65536, 65536, 65536, 65536, 65536, 65536
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Used for LPC8xx devices
const uInt32 Cart::SectorTable_8xx[] = {
  1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024,
  1024, 1024, 1024, 1024, 1024, 1024, 1024, 1024
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 Cart::SectorTable_RAM[] = { 65000 };

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cart::LPC_DEVICE_TYPE Cart::LPCtypes[] = {
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, CHIP_VARIANT_NONE },  /* unknown */

   // id,        id2,  use id2, name of product,          flash size, ram size, total number of sector, max copy size, sector table, chip variant

   { 0x00008100, 0x00000000, 0, "810M021FN8",                      4,   1,  4,  256, SectorTable_8xx,  CHIP_VARIANT_LPC8XX  },
   { 0x00008110, 0x00000000, 0, "811M001FDH16",                    8,   2,  8, 1024, SectorTable_8xx,  CHIP_VARIANT_LPC8XX  },
   { 0x00008120, 0x00000000, 0, "812M101FDH16",                   16,   4, 16, 1024, SectorTable_8xx,  CHIP_VARIANT_LPC8XX  },
   { 0x00008121, 0x00000000, 0, "812M101FD20",                    16,   4, 16, 1024, SectorTable_8xx,  CHIP_VARIANT_LPC8XX  },
   { 0x00008122, 0x00000000, 0, "812M101FDH20",                   16,   4, 16, 1024, SectorTable_8xx,  CHIP_VARIANT_LPC8XX  },

   { 0x2500102B, 0x00000000, 0, "1102",                           32,   8,  8, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX },

   { 0x0A07102B, 0x00000000, 0, "1110.../002",                     4,   1,  1, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x1A07102B, 0x00000000, 0, "1110.../002",                     4,   1,  1, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x0A16D02B, 0x00000000, 0, "1111.../002",                     8,   2,  2, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x1A16D02B, 0x00000000, 0, "1111.../002",                     8,   2,  2, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x041E502B, 0x00000000, 0, "1111.../101",                     8,   2,  2, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x2516D02B, 0x00000000, 0, "1111.../102",                     8,   2,  2, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x00010013, 0x00000000, 0, "1111.../103",                     8,   2,  2, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x0416502B, 0x00000000, 0, "1111.../201",                     8,   4,  2, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x2516902B, 0x00000000, 0, "1111.../202",                     8,   4,  2, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x00010012, 0x00000000, 0, "1111.../203",                     8,   4,  2, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x042D502B, 0x00000000, 0, "1112.../101",                    16,   2,  4, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x2524D02B, 0x00000000, 0, "1112.../102",                    16,   2,  4, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x0A24902B, 0x00000000, 0, "1112.../102",                    16,   4,  4, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x1A24902B, 0x00000000, 0, "1112.../102",                    16,   4,  4, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x00020023, 0x00000000, 0, "1112.../103",                    16,   2,  4, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x0425502B, 0x00000000, 0, "1112.../201",                    16,   4,  4, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x2524902B, 0x00000000, 0, "1112.../202",                    16,   4,  4, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x00020022, 0x00000000, 0, "1112.../203",                    16,   4,  4, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x0434502B, 0x00000000, 0, "1113.../201",                    24,   4,  6, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x2532902B, 0x00000000, 0, "1113.../202",                    24,   4,  6, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x00030032, 0x00000000, 0, "1113.../203",                    24,   4,  6, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x0434102B, 0x00000000, 0, "1113.../301",                    24,   8,  6, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x2532102B, 0x00000000, 0, "1113.../302",                    24,   8,  6, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x00030030, 0x00000000, 0, "1113.../303",                    24,   8,  6, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x0A40902B, 0x00000000, 0, "1114.../102",                    32,   4,  8, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x1A40902B, 0x00000000, 0, "1114.../102",                    32,   4,  8, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x0444502B, 0x00000000, 0, "1114.../201",                    32,   4,  8, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x2540902B, 0x00000000, 0, "1114.../202",                    32,   4,  8, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x00040042, 0x00000000, 0, "1114.../203",                    32,   8,  8, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x0444102B, 0x00000000, 0, "1114.../301",                    32,   8,  8, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x2540102B, 0x00000000, 0, "1114.../302",                    32,   8,  8, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x00040040, 0x00000000, 0, "1114.../303",                    32,   8,  8, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x00040060, 0x00000000, 0, "1114.../323",                    32,   8, 12, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x00040070, 0x00000000, 0, "1114.../333",                    32,   8, 14, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x00050080, 0x00000000, 0, "1115.../303",                    64,   8, 16, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX },

   { 0x1421102B, 0x00000000, 0, "11C12.../301",                   16,   8,  4, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x1440102B, 0x00000000, 0, "11C14.../301",                   32,   8,  8, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x1431102B, 0x00000000, 0, "11C22.../301",                   16,   8,  4, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX },
   { 0x1430102B, 0x00000000, 0, "11C24.../301",                   32,   8,  8, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX },

   { 0x293E902B, 0x00000000, 0, "11E11FHN33/101",                  8,   4,  2, 1024, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10518 Rev. 3 -- 25 Nov 2013 */
   { 0x2954502B, 0x00000000, 0, "11E12FBD48/201",                 16,   6,  4, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10518 Rev. 3 -- 25 Nov 2013 */
   { 0x296A102B, 0x00000000, 0, "11E13FBD48/301",                 24,   8,  6, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10518 Rev. 3 -- 25 Nov 2013 */
   { 0x2980102B, 0x00000000, 0, "11E14(FHN33,FBD48,FBD64)/401",   32,  10,  8, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10518 Rev. 3 -- 25 Nov 2013 */
   { 0x00009C41, 0x00000000, 0, "11E36(FBD64,FHN33)/501",         96,  12, 24, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10518 Rev. 3 -- 25 Nov 2013 */
   { 0x00007C45, 0x00000000, 0, "11E37HFBD64/401",               128,  10, 32, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10518 Rev. 3 -- 25 Nov 2013 */
   { 0x00007C41, 0x00000000, 0, "11E37(FBD48,FBD64)/501",        128,  12, 32, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10518 Rev. 3 -- 25 Nov 2013 */

   { 0x095C802B, 0x00000000, 0, "11U12(FHN33,FBD48)/201",         16,   6,  4, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10462 Rev. 5 -- 20 Nov 2013 */
   { 0x295C802B, 0x00000000, 0, "11U12(FHN33,FBD48)/201",         16,   6,  4, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10462 Rev. 5 -- 20 Nov 2013 */
   { 0x097A802B, 0x00000000, 0, "11U13FBD48/201",                 24,   6,  6, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10462 Rev. 5 -- 20 Nov 2013 */
   { 0x297A802B, 0x00000000, 0, "11U13FBD48/201",                 24,   6,  6, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10462 Rev. 5 -- 20 Nov 2013 */
   { 0x0998802B, 0x00000000, 0, "11U14FHN33/201",                 32,   6,  8, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10462 Rev. 5 -- 20 Nov 2013 */
   { 0x2998802B, 0x00000000, 0, "11U14(FHN,FHI)33/201",           32,   6,  8, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10462 Rev. 5 -- 20 Nov 2013 */
   { 0x0998802B, 0x00000000, 0, "11U14(FBD,FET)48/201",           32,   6,  8, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10462 Rev. 5 -- 20 Nov 2013 */
   { 0x2998802B, 0x00000000, 0, "11U14(FBD,FET)48/201",           32,   6,  8, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10462 Rev. 5 -- 20 Nov 2013 */
   { 0x2972402B, 0x00000000, 0, "11U23FBD48/301",                 24,   8,  6, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10462 Rev. 5 -- 20 Nov 2013 */
   { 0x2988402B, 0x00000000, 0, "11U24(FHI33,FBD48,FET48)/301",   32,   8,  8, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10462 Rev. 5 -- 20 Nov 2013 */
   { 0x2980002B, 0x00000000, 0, "11U24(FHN33,FBD48,FBD64)/401",   32,  10,  8, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10462 Rev. 5 -- 20 Nov 2013 */
   { 0x0003D440, 0x00000000, 0, "11U34(FHN33,FBD48)/311",         40,   8, 10, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10462 Rev. 5 -- 20 Nov 2013 */
   { 0x0001CC40, 0x00000000, 0, "11U34(FHN33,FBD48)/421",         48,  10, 12, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10462 Rev. 5 -- 20 Nov 2013 */
   { 0x0001BC40, 0x00000000, 0, "11U35(FHN33,FBD48,FBD64)/401",   64,  10, 16, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10462 Rev. 5 -- 20 Nov 2013 */
   { 0x0000BC40, 0x00000000, 0, "11U35(FHI33,FET48)/501",         64,  12, 16, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10462 Rev. 5 -- 20 Nov 2013 */
   { 0x00019C40, 0x00000000, 0, "11U36(FBD48,FBD64)/401",         96,  10, 24, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10462 Rev. 5 -- 20 Nov 2013 */
   { 0x00017C40, 0x00000000, 0, "11U37FBD48/401",                128,  10, 32, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10462 Rev. 5 -- 20 Nov 2013 */
   { 0x00007C44, 0x00000000, 0, "11U37HFBD64/401",               128,  10, 32, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10462 Rev. 5 -- 20 Nov 2013 */
   { 0x00007C40, 0x00000000, 0, "11U37FBD64/501",                128,  12, 32, 4096, SectorTable_11xx, CHIP_VARIANT_LPC11XX }, /* From UM10462 Rev. 5 -- 20 Nov 2013 */

   { 0x3640C02B, 0x00000000, 0, "1224.../101",                    32,   8,  4, 2048, SectorTable_17xx, CHIP_VARIANT_LPC11XX },
   { 0x3642C02B, 0x00000000, 0, "1224.../121",                    48,  12, 32, 4096, SectorTable_17xx, CHIP_VARIANT_LPC11XX },
   { 0x3650002B, 0x00000000, 0, "1225.../301",                    64,  16, 32, 4096, SectorTable_17xx, CHIP_VARIANT_LPC11XX },
   { 0x3652002B, 0x00000000, 0, "1225.../321",                    80,  20, 32, 4096, SectorTable_17xx, CHIP_VARIANT_LPC11XX },
   { 0x3660002B, 0x00000000, 0, "1226",                           96,  24, 32, 4096, SectorTable_17xx, CHIP_VARIANT_LPC11XX },
   { 0x3670002B, 0x00000000, 0, "1227",                          128,  32, 32, 4096, SectorTable_17xx, CHIP_VARIANT_LPC11XX },

   { 0x2C42502B, 0x00000000, 0, "1311",                            8,   4,  2, 1024, SectorTable_17xx, CHIP_VARIANT_LPC13XX },
   { 0x1816902B, 0x00000000, 0, "1311/01",                         8,   4,  2, 1024, SectorTable_17xx, CHIP_VARIANT_LPC13XX },
   { 0x2C40102B, 0x00000000, 0, "1313",                           32,   8,  8, 4096, SectorTable_17xx, CHIP_VARIANT_LPC13XX },
   { 0x1830102B, 0x00000000, 0, "1313/01",                        32,   8,  8, 4096, SectorTable_17xx, CHIP_VARIANT_LPC13XX },
   { 0x3A010523, 0x00000000, 0, "1315",                           32,   8,  8, 4096, SectorTable_17xx, CHIP_VARIANT_LPC13XX },
   { 0x1A018524, 0x00000000, 0, "1316",                           48,   8, 12, 4096, SectorTable_17xx, CHIP_VARIANT_LPC13XX },
   { 0x1A020525, 0x00000000, 0, "1317",                           64,   8, 16, 4096, SectorTable_17xx, CHIP_VARIANT_LPC13XX },
   { 0x3D01402B, 0x00000000, 0, "1342",                           16,   4,  4, 1024, SectorTable_17xx, CHIP_VARIANT_LPC13XX },
   { 0x3D00002B, 0x00000000, 0, "1343",                           32,   8,  8, 4096, SectorTable_17xx, CHIP_VARIANT_LPC13XX },
   { 0x28010541, 0x00000000, 0, "1345",                           32,   8,  8, 4096, SectorTable_17xx, CHIP_VARIANT_LPC13XX },
   { 0x08018542, 0x00000000, 0, "1346",                           48,   8, 12, 4096, SectorTable_17xx, CHIP_VARIANT_LPC13XX },
   { 0x08020543, 0x00000000, 0, "1347",                           64,   8, 16, 4096, SectorTable_17xx, CHIP_VARIANT_LPC13XX },

   { 0x25001118, 0x00000000, 0, "1751",                           32,   8,  8, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
   { 0x25001121, 0x00000000, 0, "1752",                           64,  16, 16, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
   { 0x25011722, 0x00000000, 0, "1754",                          128,  32, 18, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
   { 0x25011723, 0x00000000, 0, "1756",                          256,  32, 22, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
   { 0x25013F37, 0x00000000, 0, "1758",                          512,  64, 30, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
   { 0x25113737, 0x00000000, 0, "1759",                          512,  64, 30, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
   { 0x26012033, 0x00000000, 0, "1763",                          256,  64, 22, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
   { 0x26011922, 0x00000000, 0, "1764",                          128,  32, 18, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
   { 0x26013733, 0x00000000, 0, "1765",                          256,  64, 22, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
   { 0x26013F33, 0x00000000, 0, "1766",                          256,  64, 22, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
   { 0x26012837, 0x00000000, 0, "1767",                          512,  64, 30, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
   { 0x26013F37, 0x00000000, 0, "1768",                          512,  64, 30, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
   { 0x26113F37, 0x00000000, 0, "1769",                          512,  64, 30, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },

   { 0x27011132, 0x00000000, 0, "1774",                          128,  40, 18, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
   { 0x27191F43, 0x00000000, 0, "1776",                          256,  80, 22, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
   { 0x27193747, 0x00000000, 0, "1777",                          512,  96, 30, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
   { 0x27193F47, 0x00000000, 0, "1778",                          512,  96, 30, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
   { 0x281D1743, 0x00000000, 0, "1785",                          256,  80, 22, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
   { 0x281D1F43, 0x00000000, 0, "1786",                          256,  80, 22, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
   { 0x281D3747, 0x00000000, 0, "1787",                          512,  96, 30, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
   { 0x281D3F47, 0x00000000, 0, "1788",                          512,  96, 30, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },

   // LPC18xx
   { 0xF00B1B3F, 0x00000000, 1, "1810",                            0,  32,  0, 8192, SectorTable_18xx, CHIP_VARIANT_LPC18XX }, // Flashless
   { 0xF001D830, 0x00000000, 1, "1812",                          512,  32, 15, 8192, SectorTable_18xx, CHIP_VARIANT_LPC18XX },
   { 0xF001D830, 0x00000000, 1, "1813",                          512,  32, 11, 8192, SectorTable_18xx, CHIP_VARIANT_LPC18XX },
   { 0xF001D830, 0x00000000, 1, "1815",                          768,  32, 13, 8192, SectorTable_18xx, CHIP_VARIANT_LPC18XX },
   { 0xF001D830, 0x00000000, 1, "1817",                         1024,  32, 15, 8192, SectorTable_18xx, CHIP_VARIANT_LPC18XX },
   { 0xF00A9B3C, 0x00000000, 1, "1820",                            0,  32,  0, 8192, SectorTable_18xx, CHIP_VARIANT_LPC18XX }, // Flashless
   { 0xF001D830, 0x00000000, 1, "1822",                          512,  32, 15, 8192, SectorTable_18xx, CHIP_VARIANT_LPC18XX },
   { 0xF001D830, 0x00000000, 1, "1823",                          512,  32, 11, 8192, SectorTable_18xx, CHIP_VARIANT_LPC18XX },
   { 0xF001D830, 0x00000000, 1, "1825",                          768,  32, 13, 8192, SectorTable_18xx, CHIP_VARIANT_LPC18XX },
   { 0xF001D830, 0x00000000, 1, "1827",                         1024,  32, 15, 8192, SectorTable_18xx, CHIP_VARIANT_LPC18XX },
   { 0xF0009A30, 0x00000000, 1, "1830",                            0,  32,  0, 8192, SectorTable_18xx, CHIP_VARIANT_LPC18XX }, // Flashless
   { 0xF001DA30, 0x00000044, 1, "1833",                          512,  32, 11, 8192, SectorTable_18xx, CHIP_VARIANT_LPC18XX },
   { 0xF001DA30, 0x00000000, 1, "1837",                         1024,  32, 15, 8192, SectorTable_18xx, CHIP_VARIANT_LPC18XX },
   { 0xF0009830, 0x00000000, 1, "1850",                            0,  32,  0, 8192, SectorTable_18xx, CHIP_VARIANT_LPC18XX }, // Flashless
   { 0xF001D830, 0x00000044, 1, "1853",                          512,  32, 11, 8192, SectorTable_18xx, CHIP_VARIANT_LPC18XX },
   { 0xF001D830, 0x00000000, 1, "1857",                         1024,  32, 15, 8192, SectorTable_18xx, CHIP_VARIANT_LPC18XX },

   { 0x0004FF11, 0x00000000, 0, "2103",                           32,   8,  8, 4096, SectorTable_2103, CHIP_VARIANT_LPC2XXX },
   { 0xFFF0FF12, 0x00000000, 0, "2104",                          128,  16, 15, 8192, SectorTable_210x, CHIP_VARIANT_LPC2XXX },
   { 0xFFF0FF22, 0x00000000, 0, "2105",                          128,  32, 15, 8192, SectorTable_210x, CHIP_VARIANT_LPC2XXX },
   { 0xFFF0FF32, 0x00000000, 0, "2106",                          128,  64, 15, 8192, SectorTable_210x, CHIP_VARIANT_LPC2XXX },
   { 0x0201FF01, 0x00000000, 0, "2109",                           64,   8,  8, 4096, SectorTable_2109, CHIP_VARIANT_LPC2XXX },
   { 0x0101FF12, 0x00000000, 0, "2114",                          128,  16, 15, 8192, SectorTable_211x, CHIP_VARIANT_LPC2XXX },
   { 0x0201FF12, 0x00000000, 0, "2119",                          128,  16, 15, 8192, SectorTable_211x, CHIP_VARIANT_LPC2XXX },
   { 0x0101FF13, 0x00000000, 0, "2124",                          256,  16, 17, 8192, SectorTable_212x, CHIP_VARIANT_LPC2XXX },
   { 0x0201FF13, 0x00000000, 0, "2129",                          256,  16, 17, 8192, SectorTable_212x, CHIP_VARIANT_LPC2XXX },
   { 0x0002FF01, 0x00000000, 0, "2131",                           32,   8,  8, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x0002FF11, 0x00000000, 0, "2132",                           64,  16,  9, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x0002FF12, 0x00000000, 0, "2134",                          128,  16, 11, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x0002FF23, 0x00000000, 0, "2136",                          256,  32, 15, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x0002FF25, 0x00000000, 0, "2138",                          512,  32, 27, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x0402FF01, 0x00000000, 0, "2141",                           32,   8,  8, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x0402FF11, 0x00000000, 0, "2142",                           64,  16,  9, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x0402FF12, 0x00000000, 0, "2144",                          128,  16, 11, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x0402FF23, 0x00000000, 0, "2146",                          256,  40, 15, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x0402FF25, 0x00000000, 0, "2148",                          512,  40, 27, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x0301FF13, 0x00000000, 0, "2194",                          256,  16, 17, 8192, SectorTable_212x, CHIP_VARIANT_LPC2XXX },
   { 0x0301FF12, 0x00000000, 0, "2210",                            0,  16,  0, 8192, SectorTable_211x, CHIP_VARIANT_LPC2XXX }, /* table is a "don't care" */
   { 0x0401FF12, 0x00000000, 0, "2212",                          128,  16, 15, 8192, SectorTable_211x, CHIP_VARIANT_LPC2XXX },
   { 0x0601FF13, 0x00000000, 0, "2214",                          256,  16, 17, 8192, SectorTable_212x, CHIP_VARIANT_LPC2XXX },
   /*                           "2290"; same id as the LPC2210 */
   { 0x0401FF13, 0x00000000, 0, "2292",                          256,  16, 17, 8192, SectorTable_212x, CHIP_VARIANT_LPC2XXX },
   { 0x0501FF13, 0x00000000, 0, "2294",                          256,  16, 17, 8192, SectorTable_212x, CHIP_VARIANT_LPC2XXX },
   { 0x1600F701, 0x00000000, 0, "2361",                          128,  34, 11, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX }, /* From UM10211 Rev. 4.1 -- 5 Sep 2012 */
   { 0x1600FF22, 0x00000000, 0, "2362",                          128,  34, 11, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX }, /* From UM10211 Rev. 4.1 -- 5 Sep 2012 */
   { 0x0603FB02, 0x00000000, 0, "2364",                          128,  34, 11, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX }, /* From UM10211 Rev. 01 -- 6 July 2007 */
   { 0x1600F902, 0x00000000, 0, "2364",                          128,  34, 11, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x1600E823, 0x00000000, 0, "2365",                          256,  58, 15, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x0603FB23, 0x00000000, 0, "2366",                          256,  58, 15, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX }, /* From UM10211 Rev. 01 -- 6 July 2007 */
   { 0x1600F923, 0x00000000, 0, "2366",                          256,  58, 15, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x1600E825, 0x00000000, 0, "2367",                          512,  58, 15, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x0603FB25, 0x00000000, 0, "2368",                          512,  58, 28, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX }, /* From UM10211 Rev. 01 -- 6 July 2007 */
   { 0x1600F925, 0x00000000, 0, "2368",                          512,  58, 28, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x1700E825, 0x00000000, 0, "2377",                          512,  58, 28, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x0703FF25, 0x00000000, 0, "2378",                          512,  58, 28, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX }, /* From UM10211 Rev. 01 -- 6 July 2007 */
   { 0x1600FD25, 0x00000000, 0, "2378",                          512,  58, 28, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX }, /* From UM10211 Rev. 01 -- 29 October 2007 */
   { 0x1700FD25, 0x00000000, 0, "2378",                          512,  58, 28, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x1700FF35, 0x00000000, 0, "2387",                          512,  98, 28, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX }, /* From UM10211 Rev. 03 -- 25 August 2008 */
   { 0x1800F935, 0x00000000, 0, "2387",                          512,  98, 28, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x1800FF35, 0x00000000, 0, "2388",                          512,  98, 28, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x1500FF35, 0x00000000, 0, "2458",                          512,  98, 28, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x1600FF30, 0x00000000, 0, "2460",                            0,  98,  0, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x1600FF35, 0x00000000, 0, "2468",                          512,  98, 28, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x1701FF30, 0x00000000, 0, "2470",                            0,  98,  0, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
   { 0x1701FF35, 0x00000000, 0, "2478",                          512,  98, 28, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },

   { 0xA00A8B3F, 0x00000000, 1, "4310",                            0, 168,  0, 4096, SectorTable_43xx, CHIP_VARIANT_LPC43XX }, /* From UM10503 Rev. 1.4 -- 3 Sep 2012 */
   { 0xA00BCB3F, 0x00000080, 1, "4312",                          512, 104, 15, 4096, SectorTable_43xx, CHIP_VARIANT_LPC43XX }, /* info not yet available */
   { 0xA00BCB3F, 0x00000044, 1, "4313",                          512, 104, 11, 4096, SectorTable_43xx, CHIP_VARIANT_LPC43XX }, /* info not yet available */
   { 0xA001CB3F, 0x00000022, 1, "4315",                          768, 136, 13, 4096, SectorTable_43xx, CHIP_VARIANT_LPC43XX }, /* info not yet available */
   { 0xA001CB3F, 0x00000000, 1, "4317",                         1024, 136, 15, 4096, SectorTable_43xx, CHIP_VARIANT_LPC43XX }, /* info not yet available */
   { 0xA0008B3C, 0x00000000, 1, "4320",                            0, 200,  0, 4096, SectorTable_43xx, CHIP_VARIANT_LPC43XX }, /* From UM10503 Rev. 1.4 -- 3 Sep 2012 */
   { 0xA00BCB3C, 0x00000080, 1, "4322",                          512, 104, 15, 4096, SectorTable_43xx, CHIP_VARIANT_LPC43XX }, /* info not yet available */
   { 0xA00BCB3C, 0x00000044, 1, "4323",                          512, 104, 11, 4096, SectorTable_43xx, CHIP_VARIANT_LPC43XX }, /* info not yet available */
   { 0xA001CB3C, 0x00000022, 1, "4325",                          768, 136, 13, 4096, SectorTable_43xx, CHIP_VARIANT_LPC43XX }, /* info not yet available */
   { 0xA001CB3C, 0x00000000, 1, "4327",                         1024, 136, 15, 4096, SectorTable_43xx, CHIP_VARIANT_LPC43XX }, /* info not yet available */
   { 0xA0000A30, 0x00000000, 1, "4330",                            0, 264,  0, 4096, SectorTable_43xx, CHIP_VARIANT_LPC43XX }, /* From UM10503 Rev. 1.4 -- 3 Sep 2012 */
   { 0xA001CA30, 0x00000044, 1, "4333",                          512, 512, 11, 4096, SectorTable_43xx, CHIP_VARIANT_LPC43XX }, /* info not yet available */
   { 0xA001CA30, 0x00000000, 1, "4337",                         1024, 512, 15, 4096, SectorTable_43xx, CHIP_VARIANT_LPC43XX }, /* info not yet available */
   { 0xA0000830, 0x00000000, 1, "4350",                            0, 264,  0, 4096, SectorTable_43xx, CHIP_VARIANT_LPC43XX }, /* From UM10503 Rev. 1.4 -- 3 Sep 2012 */
   { 0xA001C830, 0x00000044, 1, "4353",                          512, 512, 11, 4096, SectorTable_43xx, CHIP_VARIANT_LPC43XX }, /* From UM10503 Rev. 1.4 -- 3 Sep 2012 */
   { 0xA001C830, 0x00000000, 1, "4357",                         1024, 512, 15, 4096, SectorTable_43xx, CHIP_VARIANT_LPC43XX }  /* From UM10503 Rev. 1.4 -- 3 Sep 2012 */
};

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
