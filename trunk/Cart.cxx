//=========================================================================
//
//  H   H    A    RRRR   M   M   OOO   N   N  Y   Y
//  H   H   A A   R   R  MM MM  O   O  NN  N   Y Y
//  HHHHH  AAAAA  RRRR   M M M  O   O  N N N    Y   "Harmony Cart software"
//  H   H  A   A  R R    M   M  O   O  N  NN    Y
//  H   H  A   A  R  R   M   M   OOO   N   N    Y
//
// Copyright (c) 2009-2015 by Stephen Anthony <stephena@users.sf.net>
//
// See the file "License.txt" for information on usage and redistribution
// of this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//=========================================================================

#include <QApplication>
#include <QPixmap>
#include <QIcon>
#include <QString>
#include <QDir>
#include <cstring>
#include <fstream>
#include <sstream>
#include <ostream>
#include <time.h>

#include "bspf_harmony.hxx"

#include "BSType.hxx"
#include "Cart.hxx"
#include "CartDetector.hxx"
#include "SerialPort.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cart::Cart()
  : myDetectedDevice(0),
    myRetry(1),
    myOscillator("10000"),
    myLog(&cout)
{
  myProgress.setWindowModality(Qt::WindowModal);
  myProgress.setWindowIcon(QPixmap(":icons/pics/appicon.png"));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cart::~Cart()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cart::setLogger(ostream* out)
{
  myLog = out;
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
    *myLog << result.c_str() << endl;
    return result;
  }

 return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Cart::downloadBIOS(SerialPort& port, const string& filename,
                          bool verify, bool showprogress)
{
  string result = "";

  // Read the file into a buffer
  uInt32 size = 0;
  uInt8* bios = readFile(filename, size);
  if(size > 0)
    result = lpc_NxpDownload(port, bios, size, verify, showprogress);
  else
    result = "Couldn't open BIOS file";

  delete[] bios;
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Cart::downloadROM(SerialPort& port, const string& armpath,
                         const string& filename, BSType type,
                         bool verify, bool showprogress)
{
  string result = "";
  bool autodetect = type == BS_AUTO;
  uInt32 romsize = 0, armsize = 0, size = 0;
  uInt8 *rombuf = NULL, *armbuf = NULL;
  uInt8 binary[512*1024], *binary_ptr = binary;
  string armfile = "";

  // Read the ROM file into a buffer
  rombuf = readFile(filename, romsize);
  if(romsize == 0)
  {
    result = "Couldn't open ROM file.";
    goto cleanup;
  }

  // Determine the bankswitch type
  if(autodetect)
    type = CartDetector::autodetectType(filename, rombuf, romsize);
  if(autodetect || type == CartDetector::autodetectType(filename, rombuf, romsize))
  {
    *myLog << "Bankswitch type: " << Bankswitch::typeToName(type).c_str()
          << " (auto-detected)" << endl;
  }
  else
  {
    *myLog << "Bankswitch type: " << Bankswitch::typeToName(type).c_str()
          << " (WARNING: overriding auto-detection)" << endl;
  }

  // First determine the name of the bankswitch file to use
  switch(type)
  {
    case BS_0840:   armfile = "0840.arm";   break;
    case BS_4K:     armfile = "2K4K.arm";   break;
    case BS_3E:     armfile = "3E.arm";     break;
    case BS_3F:     armfile = "3F.arm";     break;
    case BS_CV:     armfile = "CV.arm";     break;
    case BS_DPC:    armfile = "DPC.arm";    break;
    case BS_E0:     armfile = "E0.arm";     break;
    case BS_E7:     armfile = "E7.arm";     break;
    case BS_FA:     armfile = "FA.arm";     break;
    case BS_FA2:    armfile = "FA2.arm";    break;
    case BS_F4:     armfile = "F4.arm";     break;
    case BS_F6:     armfile = "F6.arm";     break;
    case BS_F8:     armfile = "F8.arm";     break;
    case BS_F4SC:   armfile = "F4SC.arm";   break;
    case BS_F6SC:   armfile = "F6SC.arm";   break;
    case BS_F8SC:   armfile = "F8SC.arm";   break;
    case BS_FE:     armfile = "FE.arm";     break;
    case BS_AR:     armfile = "SC.arm";     break;
    case BS_UA:     armfile = "UA.arm";     break;
    case BS_DPCP:   armfile = "DPC+.arm";   break;
    case BS_CUSTOM:                         break;
    default:
      result = "Bankswitch type \'" + Bankswitch::typeToName(type) + "\' not supported.";
      *myLog << "ERROR: " << result.c_str() << endl;
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
    case BS_F4SC:
    {
      // Copy ROM data
      memcpy(binary_ptr, rombuf, romsize);

      // ARM code in first "RAM" area
      memcpy(binary_ptr, armbuf, 256);

      // ARM code in second "RAM" area
      if(armsize > 4096)
        memcpy(binary_ptr+4096, armbuf+4096, armsize-4096);

      size = romsize;  // No further processing required
      break;
    }

    case BS_F4:
    {
      // Copy ARM data to determine remaining size
      // Leave space for 8 bytes, to indicate the bank configuration
      memcpy(binary_ptr, armbuf, armsize);
      binary_ptr += armsize + 8;

      // Reorganize bin for best compression
      // Possible bank organizations:
      //   12345670  02345671  01345672  01245673
      //   01235674  01234675  01234576  01234567

      uInt32 i;
      for(i = 0; i < 8; ++i) // i = bank to go last
      {
        uInt8* ptr = binary_ptr;
        for(uInt32 h = 0; h < 8; ++h)
        {
          if (h != i)
          {
            memcpy(ptr, rombuf+4096*h, 4096);
            ptr += 4096;
          }
        }

        memcpy(ptr, rombuf+4096*i, 4096);
        ptr += 4096;

        // Compress last bank
        try
        {
          romsize = 28672 + compressLastBank(binary_ptr);
        }
        catch(const char* msg)
        {
          result = msg;
          *myLog << "ERROR: " << result.c_str() << endl;
          goto cleanup;
        }
        if(romsize+armsize < 32760)
          break; // one of the banks fits
      }

      if(i == 8)
      {
        result = "Cannot compress F4 binary";
        *myLog << "ERROR: " << result.c_str() << endl;
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

    case BS_4K:  // 2K & 'Sub2K'
    {
      if(romsize < 4096)
      {
        // All ROMs 2K or less should be mirrored into the 4K address space
        uInt32 power2 = 1;
        while(power2 < romsize)
          power2 <<= 1;

        // Create a 4K buffer and reassign to rombuf
        uInt8* tmp = new uInt8[4096];
        uInt8* tmp_ptr = tmp;
        for(uInt32 i = 0; i < 4096/power2; ++i, tmp_ptr += power2)
          memcpy(tmp_ptr, rombuf, romsize);

        delete[] rombuf;
        rombuf = tmp;
        romsize = 4096;
      }
      break;
    }

    case BS_AR:  // Supercharger
    {
      // Take care of special AR ROM which are only 6K
      if(romsize == 6144)
      {
        // Minimum buffer size is 6K + 256 bytes
        uInt8* tmp = new uInt8[6144+256];
        memcpy(tmp, rombuf, 6144);          // copy ROM
        memcpy(tmp+6144, ourARHeader, 256); // copy missing header

        delete[] rombuf;
        rombuf = tmp;
        romsize = 6144 + 256;
      }
      else  // size is multiple of 8448
      {
        // To save space, we skip 2K in each Supercharger load
        uInt32 numLoads = romsize / 8448;
        uInt8* tmp = new uInt8[numLoads*(6144+256)];
        uInt8 *tmp_ptr = tmp, *rom_ptr = rombuf;
        for(uInt32 i = 0; i < numLoads; ++i, tmp_ptr += 6144+256, rom_ptr += 8448)
        {
          memcpy(tmp_ptr, rom_ptr, 6144);                // 6KB  @ pos 0K
          memcpy(tmp_ptr+6144, rom_ptr+6144+2048, 256);  // 256b @ pos 8K
        }

        delete[] rombuf;
        rombuf = tmp;
        romsize = numLoads * (6144+256);
      }
      break;
    }

    case BS_DPCP:
    {
      // There are two variants of DPC+; one with the ARM code
      // already added (32KB), and the other without (29KB)

      // The one with the ARM code will be processed here
      // The one without the ARM code will be processed below
      if(romsize == 32 * 1024)
      {
        // Copy ROM data; no further processing required
        size = romsize;
        memcpy(binary_ptr, rombuf, size);
      }
      break;
    }

    case BS_FA2:
    {
      // There are two variants of FA2; one with the ARM code
      // already added and padded (32KB), and the other without (28KB)
    
      if(romsize == 32 * 1024)
      {
        // Copy ROM data; no further processing required
        size = romsize;
        memcpy(binary_ptr, rombuf, size);
      }
      else if(romsize == 28 * 1024)
      {
        // Copy ARM data
        memcpy(binary_ptr, armbuf, armsize);
        binary_ptr += 1024;

        // Copy ROM data
        memcpy(binary_ptr, rombuf, romsize);

        size = 32 * 1024;
      }
      break;
    }

    case BS_CUSTOM:
    {
      // Copy ROM data; no further processing required
      size = romsize;
      memcpy(binary_ptr, rombuf, size);
      break;
    }

    default:  break;
  }

  // Do we need combine ARM and ROM data?
  if(size == 0)
  {
    // Copy ARM data
    memcpy(binary_ptr, armbuf, armsize);
    binary_ptr += armsize;

    // Copy ROM data
    memcpy(binary_ptr, rombuf, romsize);

    size = romsize + armsize;
  }

  result = lpc_NxpDownload(port, binary, size, verify, showprogress);

cleanup:
  delete[] rombuf;
  delete[] armbuf;
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8* Cart::readFile(const string& filename, uInt32& size)
{
  uInt8* buffer = (uInt8*) NULL;
  size = 0;

  *myLog << "Reading from file: \'" << filename.c_str() << "\' ... ";

  // Read file into buffer
  ifstream in(filename.c_str(), ios::binary);
  if(!in)
  {
    *myLog << "ERROR: file not found\n";
    return buffer;
  }
  // Figure out how much data we should read
  in.seekg(0, ios::end);
  streampos length = in.tellg();
  in.seekg(0, ios::beg);
  if(length <= 0)
    return buffer;

  buffer = new uInt8[length];
  in.read((char*)buffer, length);
  *myLog << "read in " << length << " bytes" << endl;
  in.close();

  size = length;
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
  uInt32 a, b, cb, j, k, r, x, y, len, mode_count;
  uInt8 buflast[4096], buf[40000], bc[10000], dec[10000], bufliteral[200];

  a = b = cb = j = k = r = x = len = mode_count = 0;
  y = 32768-4096;

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
      mode_count++;
    }
    else
    {
      mode_count++;
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
    b=0;
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

  /* SA_CHANGED: '100' to '3' to shorten autodetection */
  for (int nQuestionMarks = found = 0; !found && nQuestionMarks < 3; nQuestionMarks++)
  {
    port.send("?");

    memset(Answer, 0, sizeof(Answer));
    strippedsize = port.receive(Answer, sizeof(Answer)-1, 1, 100);
    strippedAnswer = Answer;

    while ((strippedsize > 0) && ((*strippedAnswer == '?') || (*strippedAnswer == 0)))
    {
      strippedAnswer++;
      strippedsize--;
    }

    if (strcmp(strippedAnswer, "Synchronized\r\n") == 0)
      found = 1;
    else
      resetTarget(port, PROGRAM_MODE);
  } // end for

  if (!found)
  {
    strcpy(version, "ERROR: no answer on \'?\'");
    return version;
  }

  port.send("Synchronized\n");
  port.receive(Answer, sizeof(Answer) - 1, 2, 1000);

  if ((strcmp(Answer, "Synchronized\r\nOK\r\n") != 0) && (strcmp(Answer, "Synchronized\rOK\r\n") != 0) &&
      (strcmp(Answer, "Synchronized\nOK\r\n") != 0))
  {
    strcpy(version, "ERROR: No answer on \'Synchronized\'");
    return version;
  }

  sprintf(temp, "%s\n", myOscillator.c_str());
  port.send(temp);
  port.receive(Answer, sizeof(Answer)-1, 2, 1000);

  sprintf(temp, "%s\nOK\r\n", myOscillator.c_str());
  if (strcmp(Answer, temp) != 0)
  {
    strcpy(version, "ERROR: No answer on Oscillator-Command");
    return version;
  }

  cmdstr = "U 23130\n";
  if (!lpc_SendAndVerify(port, cmdstr, Answer, sizeof Answer))
  {
    sprintf(version, "ERROR: Unlock-Command: %d", (UNLOCK_ERROR + lpc_GetAndReportErrorNumber(Answer)));
    return version;
  }

  cmdstr = "K\n";
  port.send(cmdstr);
  port.receive(Answer, sizeof(Answer)-1, 4, 5000);
  if (strncmp(Answer, cmdstr, strlen(cmdstr)) != 0)
  {
    strcpy(version, "ERROR: no answer on Read Boot Code Version");
    return version;
  }

  if (strncmp(Answer + strlen(cmdstr), "0\r\n", 3) == 0)
    strippedAnswer = Answer + strlen(cmdstr) + 3;

  cmdstr = "J\n";
  port.send(cmdstr);
  port.receive(Answer, sizeof(Answer)-1, 3, 5000);
  if (strncmp(Answer, cmdstr, strlen(cmdstr)) != 0)
  {
    strcpy(version, "ERROR: no answer on Read Part Id");
    return version;
  }

  strippedAnswer = (strncmp(Answer, "J\n0\r\n", 5) == 0) ? Answer + 5 : Answer;
  uInt32 Pos = strtoul(strippedAnswer, &endPtr, 10);
  *endPtr = '\0'; /* delete \r\n */
  for (i = sizeof LPCtypes / sizeof LPCtypes[0] - 1; i > 0 && LPCtypes[i].id != Pos; i--)
    /* nothing */;

  myDetectedDevice = i;
  if(myDetectedDevice != 0)
  {
    sprintf(version, "LPC%s, %d kiB ROM / %d kiB SRAM",
            LPCtypes[myDetectedDevice].Product,
            LPCtypes[myDetectedDevice].FlashSize,
            LPCtypes[myDetectedDevice].RAMSize);
  }

  return version;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Cart::lpc_NxpDownload(SerialPort& port, uInt8* data, uInt32 size,
                             bool verify, bool showprogress)
{
  char Answer[128], ExpectedAnswer[128], temp[128];
  char *strippedAnswer, *endPtr;
  int strippedsize, nQuestionMarks;
  int found;
  uInt32 Sector;
  uInt32 SectorLength;
  uInt32 SectorStart, SectorOffset, SectorChunk;
  char tmpString[128];
  char uuencode_table[64];
  int Line;
  uInt32 tmpStringPos;
  uInt32 BlockOffset;
  uInt32 Block;
  uInt32 Pos;
  uInt32 CopyLength;
  int c,k=0,i;
  uInt32 ivt_CRC;          // CRC over interrupt vector table
  uInt32 block_CRC;
  time_t tStartUpload=0, tDoneUpload=0;
  const char* cmdstr;
  uInt32 repeat = 0;
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

  // Build up uuencode table
  uuencode_table[0] = 0x60;           // 0x20 is translated to 0x60 !

  for (int i = 1; i < 64; i++)
    uuencode_table[i] = (char)(0x20 + i);

  // Make sure the data is aligned to 32-bits, and copy to internal buffer
  uInt32 BinaryOffset = 0, StartAddress = 0, BinaryLength = size;
  if(BinaryLength % 4 != 0)
  {
    uInt32 newBinaryLength = ((BinaryLength + 3)/4) * 4;
    *myLog << "Warning:  data not aligned to 32 bits, padded (length was "
          << BinaryLength << ", now " << newBinaryLength << ")\n";
    BinaryLength = newBinaryLength;
  }
  uInt8* binaryContent = new uInt8[BinaryLength];
  memcpy(binaryContent, data, size);
  uInt32 progressStep = 0;
  if(showprogress)
    initializeProgress("Updating Flash", 0, BinaryLength/45 + 20);

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
  else if(LPCtypes[myDetectedDevice].ChipVariant == CHIP_VARIANT_LPC17XX ||
          LPCtypes[myDetectedDevice].ChipVariant == CHIP_VARIANT_LPC13XX ||
          LPCtypes[myDetectedDevice].ChipVariant == CHIP_VARIANT_LPC11XX)
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

  *myLog << "Synchronizing";

  /* SA_CHANGED: '100' to '3' to shorten autodetection */
  for (nQuestionMarks = found = 0; !found && nQuestionMarks < 3; nQuestionMarks++)
  {
    *myLog << ".";
    port.send("?");

    memset(Answer, 0, sizeof(Answer));
    strippedsize = port.receive(Answer, sizeof(Answer)-1, 1, 100);
    strippedAnswer = Answer;

    while ((strippedsize > 0) && ((*strippedAnswer == '?') || (*strippedAnswer == 0)))
    {
      strippedAnswer++;
      strippedsize--;
    }

    tStartUpload = time(NULL);
    if (strcmp(strippedAnswer, "Synchronized\r\n") == 0)
      found = 1;
    else
      resetTarget(port, PROGRAM_MODE);
  } // end for

  if(!found)
  {
    result << "ERROR: no answer on '?'";
    goto cleanup;
  }

  *myLog << " OK\n";

  port.send("Synchronized\n");
  port.receive(Answer, sizeof(Answer) - 1, 2, 1000);
  if ((strcmp(Answer, "Synchronized\r\nOK\r\n") != 0) && (strcmp(Answer, "Synchronized\rOK\r\n") != 0) &&
      (strcmp(Answer, "Synchronized\nOK\r\n") != 0))
  {
    result << "ERROR: No answer on 'Synchronized'";
    goto cleanup;
  }

  sprintf(temp, "%s\n", myOscillator.c_str());
  port.send(temp);
  port.receive(Answer, sizeof(Answer)-1, 2, 1000);

  sprintf(temp, "%s\nOK\r\n", myOscillator.c_str());
  if (strcmp(Answer, temp) != 0)
  {
    result << "ERROR: No answer on Oscillator-Command";
    goto cleanup;
  }

  cmdstr = "U 23130\n";
  if (!lpc_SendAndVerify(port, cmdstr, Answer, sizeof Answer))
  {
    result << "ERROR: Unlock-Command: " + lpc_GetAndReportErrorNumber(Answer);
    goto cleanup;
  }

  *myLog << "Read bootcode version: ";

  cmdstr = "K\n";
  port.send(cmdstr);
  port.receive(Answer, sizeof(Answer)-1, 4, 5000);

  if (strncmp(Answer, cmdstr, strlen(cmdstr)) != 0)
  {
    result << "ERROR: no answer on Read Boot Code Version";
    goto cleanup;
  }

  if (strncmp(Answer + strlen(cmdstr), "0\r\n", 3) == 0)
  {
    strippedAnswer = Answer + strlen(cmdstr) + 3;
    *myLog << strippedAnswer;
  }
  else
    *myLog << "unknown\n";

  *myLog << "Read part ID: ";

  cmdstr = "J\n";
  port.send(cmdstr);
  port.receive(Answer, sizeof(Answer)-1, 3, 5000);

  if (strncmp(Answer, cmdstr, strlen(cmdstr)) != 0)
  {
    result << "ERROR: no answer on Read Part Id";
    goto cleanup;
  }

  strippedAnswer = (strncmp(Answer, "J\n0\r\n", 5) == 0) ? Answer + 5 : Answer;
  Pos = strtoul(strippedAnswer, &endPtr, 10);
  *endPtr = '\0'; /* delete \r\n */
  for (i = sizeof LPCtypes / sizeof LPCtypes[0] - 1; i > 0 && LPCtypes[i].id != Pos; i--)
    /* nothing */;

  myDetectedDevice = i;
  if (myDetectedDevice == 0)
  {
    *myLog << "unknown";
  }
  else
  {
    char version[100];
    sprintf(version, "LPC%s, %d kiB ROM / %d kiB SRAM",
            LPCtypes[myDetectedDevice].Product,
            LPCtypes[myDetectedDevice].FlashSize,
            LPCtypes[myDetectedDevice].RAMSize);
    *myLog << version;
  }
  *myLog << " (" << hex << Pos << dec << ")\n";

  // Make sure the data can fit in the flash we have available
  if(size > LPCtypes[myDetectedDevice].FlashSize * 1024)
  {
    result << "ERROR: Data to large for available flash";
    goto cleanup;
  }

  /* In case of a download to RAM, use full RAM for downloading
   * set the flash parameters to full RAM also.
   * This makes sure that all code is downloaded as one big sector
   */
  if (BinaryOffset >= lpc_ReturnValueLpcRamStart())
  {
    LPCtypes[myDetectedDevice].FlashSectors = 1;
    LPCtypes[myDetectedDevice].MaxCopySize  = LPCtypes[myDetectedDevice].RAMSize*1024 - (lpc_ReturnValueLpcRamBase() - lpc_ReturnValueLpcRamStart());
    LPCtypes[myDetectedDevice].SectorTable  = SectorTable_RAM;
    SectorTable_RAM[0] = LPCtypes[myDetectedDevice].MaxCopySize;
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

  sprintf(tmpString, "P %d %d\n", 0, 0);
  if (!lpc_SendAndVerify(port, tmpString, Answer, sizeof Answer))
  {
    result << "ERROR: Wrong answer on Prepare-Command " << lpc_GetAndReportErrorNumber(Answer);
    goto cleanup;
  }

  sprintf(tmpString, "E %d %d\n", 0, 0);
  if (!lpc_SendAndVerify(port, tmpString, Answer, sizeof Answer))
  {
    result << "ERROR: Wrong answer on Erase-Command " << lpc_GetAndReportErrorNumber(Answer);
    goto cleanup;
  }
  *myLog << "OK \n";

  // OK, the main loop where we start writing to the cart
  while (1)
  {
    if (Sector >= LPCtypes[myDetectedDevice].FlashSectors)
    {
      result << "ERROR: Program too large; running out of Flash sectors";
      goto cleanup;
    }

    *myLog << "Sector " << Sector << flush;
    if(showprogress)
      updateProgressText("Downloading sector " + QString::number(Sector) + " ...                  ");

    if (BinaryOffset < lpc_ReturnValueLpcRamStart()) // Skip Erase when running from RAM
    {
      sprintf(tmpString, "P %d %d\n", Sector, Sector);
      if (!lpc_SendAndVerify(port, tmpString, Answer, sizeof Answer))
      {
        result << "ERROR: Wrong answer on Prepare-Command (1) (Sector " << Sector << ") "
               << lpc_GetAndReportErrorNumber(Answer);
        goto cleanup;
      }

      *myLog << "." << flush;

      if (Sector != 0) // Sector 0 already erased
      {
        sprintf(tmpString, "E %d %d\n", Sector, Sector);
        if (!lpc_SendAndVerify(port, tmpString, Answer, sizeof Answer))
        {
          result << "ERROR: Wrong answer on Erase-Command (Sector " << Sector << ") "
                 << lpc_GetAndReportErrorNumber(Answer);
          goto cleanup;
        }

        *myLog << "." << flush;
      }
    }

    SectorLength = LPCtypes[myDetectedDevice].SectorTable[Sector];
    if (SectorLength > BinaryLength - SectorStart)
      SectorLength = BinaryLength - SectorStart;

    for (SectorOffset = 0; SectorOffset < SectorLength; SectorOffset += SectorChunk)
    {
      if (SectorOffset > 0)
        *myLog << "|" << flush;

      // If the Flash ROM sector size is bigger than the number of bytes
      // we can copy from RAM to Flash, we must "chop up" the sector and
      // copy these individually.
      // This is especially needed in the case where a Flash sector is
      // bigger than the amount of SRAM.
      SectorChunk = SectorLength - SectorOffset;
      if (SectorChunk > (unsigned)LPCtypes[myDetectedDevice].MaxCopySize)
        SectorChunk = LPCtypes[myDetectedDevice].MaxCopySize;

      // Write multiple of 45 * 4 Byte blocks to RAM, but copy maximum of on sector to Flash
      // In worst case we transfer up to 180 byte to much to RAM
      // but then we can always use full 45 byte blocks and length is multiple of 4
      CopyLength = SectorChunk;
      if ((CopyLength % (45 * 4)) != 0)
        CopyLength += ((45 * 4) - (CopyLength % (45 * 4)));

      sprintf(tmpString, "W %d %d\n", lpc_ReturnValueLpcRamBase(), CopyLength);
      if (!lpc_SendAndVerify(port, tmpString, Answer, sizeof Answer))
      {
        *myLog << "ERROR: Wrong answer on Write-Command " << lpc_GetAndReportErrorNumber(Answer);
        goto cleanup;
      }

      *myLog << "." << flush;

      block_CRC = 0;
      Line = 0;

      // Transfer blocks of 45 * 4 bytes to RAM
      for (Pos = SectorStart + SectorOffset; (Pos < SectorStart + SectorOffset + CopyLength) && (Pos < BinaryLength); Pos += (45 * 4))
      {
        for (Block = 0; Block < 4; Block++)  // Each block 45 bytes
        {
          *myLog << "." << flush;

          // Inform the calling application about having written another chuck of data
          if(showprogress)
          {
            if(!updateProgressValue(++progressStep))
            {
              result << "Cancelled download";
              goto cleanup;
            }
          }

          // Uuencode one 45 byte block
          tmpStringPos = 0;

          sendbuf[Line][tmpStringPos++] = (char)(' ' + 45);    // Encode Length of block

          for (BlockOffset = 0; BlockOffset < 45; BlockOffset++)
          {
            if (BinaryOffset < lpc_ReturnValueLpcRamStart())
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
          sendbuf[Line][tmpStringPos++] = '\n';
          sendbuf[Line][tmpStringPos++] = 0;
          port.send(sendbuf[Line]);

          // receive only for debug proposes
          port.receive(Answer, sizeof(Answer)-1, 1, 5000);

          Line++;
          if (Line == 20)
          {
            for (repeat = 0; repeat < myRetry; repeat++)
            {
              // printf("block_CRC = %d\n", block_CRC);
              sprintf(tmpString, "%d\n", block_CRC);
              port.send(tmpString);
              port.receive(Answer, sizeof(Answer)-1, 2, 5000);

              sprintf(tmpString, "%d\nOK\r\n", block_CRC);
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
              goto cleanup;
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
          sprintf(tmpString, "%d\n", block_CRC);
          port.send(tmpString);
          port.receive(Answer, sizeof(Answer)-1, 2, 5000);

          sprintf(tmpString, "%d\nOK\r\n", block_CRC);
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
          goto cleanup;
        }
      }

      if (BinaryOffset < lpc_ReturnValueLpcRamStart())
      {
        // Prepare command must be repeated before every write
        sprintf(tmpString, "P %d %d\n", Sector, Sector);

        if (!lpc_SendAndVerify(port, tmpString, Answer, sizeof Answer))
        {
          result << "ERROR: Wrong answer on Prepare-Command (2) (Sector " << Sector << ") "
                 << lpc_GetAndReportErrorNumber(Answer);
          goto cleanup;
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

        sprintf(tmpString, "C %d %d %d\n", SectorStart + SectorOffset, lpc_ReturnValueLpcRamBase(), CopyLength);
        if (!lpc_SendAndVerify(port, tmpString, Answer, sizeof Answer))
        {
          result << "ERROR: Wrong answer on Copy-Command " << lpc_GetAndReportErrorNumber(Answer);
          goto cleanup;
        }

        if (verify)
        {
          // Avoid compare first 64 bytes.
          // Because first 64 bytes are re-mapped to flash boot sector,
          // and the compare result may not be correct.
          if (SectorStart + SectorOffset<64)
            sprintf(tmpString, "M %d %d %d\n", 64, lpc_ReturnValueLpcRamBase() + (64 - SectorStart - SectorOffset), CopyLength-(64 - SectorStart - SectorOffset));
          else
           sprintf(tmpString, "M %d %d %d\n", SectorStart + SectorOffset, lpc_ReturnValueLpcRamBase(), CopyLength);

          if (!lpc_SendAndVerify(port, tmpString, Answer, sizeof Answer))
          {
            result << "ERROR: Wrong answer on Compare-Command " << lpc_GetAndReportErrorNumber(Answer);
            goto cleanup;
          }
        }
      }
    }

    *myLog << "\n" << flush;

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

  tDoneUpload = time(NULL);
  if (verify)
    result << "Download Finished and Verified correct... taking " << int(tDoneUpload - tStartUpload) << " seconds";
  else
    result << "Download Finished... taking " << int(tDoneUpload - tStartUpload) << " seconds";

  if (1 /* IspEnvironment->DoNotStart == 0*/)
  {
    *myLog << "Now launching the brand new code\n" << flush;

    if(LPCtypes[myDetectedDevice].ChipVariant == CHIP_VARIANT_LPC2XXX)
      sprintf(tmpString, "G %d A\n", StartAddress);
    else if(LPCtypes[myDetectedDevice].ChipVariant == CHIP_VARIANT_LPC17XX ||
            LPCtypes[myDetectedDevice].ChipVariant == CHIP_VARIANT_LPC13XX ||
            LPCtypes[myDetectedDevice].ChipVariant == CHIP_VARIANT_LPC11XX)
      sprintf(tmpString, "G %d T\n", StartAddress & ~1);

    port.send(tmpString);  //goto 0 : run this fresh new downloaded code code
    if (BinaryOffset < lpc_ReturnValueLpcRamStart())
    { // Skip response on G command - show response on Terminal instead
      uInt32 realsize = port.receive(Answer, sizeof(Answer)-1, 2, 5000);
      /* the reply string is frequently terminated with a -1 (EOF) because the
       * connection gets broken; zero-terminate the string ourselves
       */
      while (realsize > 0 && ((signed char) Answer[(int)realsize - 1]) < 0)
        realsize--;

      Answer[(int)realsize] = '\0';
      /* Better to check only the first 9 chars instead of complete receive buffer,
       * because the answer can contain the output by the started programm
       */
      if(LPCtypes[myDetectedDevice].ChipVariant == CHIP_VARIANT_LPC2XXX)
        sprintf(ExpectedAnswer, "G %d A\n0", StartAddress);
      else if(LPCtypes[myDetectedDevice].ChipVariant == CHIP_VARIANT_LPC17XX)
        sprintf(ExpectedAnswer, "G %d T\n0", StartAddress & ~1);

      if (realsize == 0 || strncmp((const char *)Answer, /*cmdstr*/ExpectedAnswer, strlen(/*cmdstr*/ExpectedAnswer)) != 0)
      {
        result << "Failed to run the new downloaded code: " << lpc_GetAndReportErrorNumber(Answer);
        goto cleanup;
      }
    }

    *myLog << flush;
  }

cleanup:
  delete[] binaryContent;  binaryContent = NULL;
  if(showprogress)
    finalizeProgress();

  *myLog << result.str().c_str() << endl;
  return result.str();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Cart::lpc_SendAndVerify(SerialPort& port, const char* Command,
                            char* AnswerBuffer, int AnswerLength)
{
  int cmdlen;

  port.send(Command);
  port.receive(AnswerBuffer, AnswerLength - 1, 2, 5000);
  cmdlen = strlen(Command);

  return (strncmp(AnswerBuffer, Command, cmdlen) == 0 &&
          strcmp(AnswerBuffer + cmdlen, "0\r\n") == 0);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
unsigned char Cart::lpc_GetAndReportErrorNumber(const char* Answer)
{
  unsigned char Result = 0xFF;    // Error !!!
  unsigned int i = 0;

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
uInt32 Cart::lpc_ReturnValueLpcRamStart()
{
  if(LPCtypes[myDetectedDevice].ChipVariant == CHIP_VARIANT_LPC2XXX)
    return LPC_RAMSTART_LPC2XXX;
  else if(LPCtypes[myDetectedDevice].ChipVariant == CHIP_VARIANT_LPC17XX)
    return LPC_RAMSTART_LPC17XX;
  else if(LPCtypes[myDetectedDevice].ChipVariant == CHIP_VARIANT_LPC13XX)
    return LPC_RAMSTART_LPC13XX;
  else if(LPCtypes[myDetectedDevice].ChipVariant == CHIP_VARIANT_LPC11XX)
    return LPC_RAMSTART_LPC11XX;

  return 0;  // TODO - more properly handle this
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 Cart::lpc_ReturnValueLpcRamBase()
{
  if(LPCtypes[myDetectedDevice].ChipVariant == CHIP_VARIANT_LPC2XXX)
    return LPC_RAMBASE_LPC2XXX;
  else if(LPCtypes[myDetectedDevice].ChipVariant == CHIP_VARIANT_LPC17XX)
    return LPC_RAMBASE_LPC17XX;
  else if(LPCtypes[myDetectedDevice].ChipVariant == CHIP_VARIANT_LPC13XX)
    return LPC_RAMBASE_LPC13XX;
  else if(LPCtypes[myDetectedDevice].ChipVariant == CHIP_VARIANT_LPC11XX)
    return LPC_RAMBASE_LPC11XX;

  return 0;  // TODO - more properly handle this
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
