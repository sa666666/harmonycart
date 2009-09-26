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

#include <QApplication>
#include <QProgressDialog>
#include <cstring>
#include <fstream>

#include "bspf.hxx"

#include "BSType.hxx"
#include "Cart.hxx"
#include "CartDetector.hxx"
#include "SerialPort.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cart::Cart()
  : myDetectedDevice(0),
    myOscillator("10000"),
    myBinaryContent(NULL),
    myBinaryLength(0)
//    myCurrentSector(0),
//    myNumSectors(0),
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cart::~Cart()
{
  delete[] myBinaryContent;
  myBinaryContent = NULL;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Cart::autodetectHarmony(SerialPort& port)
{
  resetTarget(port, PROGRAM_MODE);
  port.clearBuffers();

  // Get the version #, if any
  const char* result = lpc_PhilipsChipVersion(port);
  if (strncmp(result, "ERROR:", 6) == 0)
  {
    cerr << result << endl;
    return result;
  }
#if 0
  else if (1)//myStartAddress == 0)
  {
    /* Only reset target if startaddress = 0
     * Otherwise stay with the running program as started in Download()
     */
    resetTarget(port, RUN_MODE);
  }
#endif
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Cart::downloadBIOS(SerialPort& port, const string& filename)
{
  string result = "BIOS downloaded.";

  // Read the file into a buffer
  uInt32 size = 0;
  uInt8* bios = readFile(filename, size);
  if(size > 0)
  {
    // Actually write the data to the cart/serial port, and
    // use a progressbar to show progress
    lpc_PhilipsDownload(port, bios, size, true);
  }
  else
    result = "Couldn't open BIOS file.";

  delete[] bios;
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Cart::downloadROM(SerialPort& port, const string& filename, BSType type)
{
  string result = "Cartridge ROM downloaded.";

  uInt32 romsize = 0, armsize = 0;
  uInt8 *rombuf, *armbuf;
  string armfile = "";

  // Read the ROM file into a buffer
  rombuf = readFile(filename, romsize);
  if(romsize == 0)
  {
    result = "Couldn't open ROM file.";
    goto cleanup;
  }

  // Determine the bankswitch type
  if(type == CartDetector::autodetectType(rombuf, romsize))
  {
    cout << "Bankswitch type: " << Bankswitch::typeToName(type)
         << " (auto-detected)" << endl;
  }
  else
  {
    cout << "Bankswitch type: " << Bankswitch::typeToName(type)
         << " (WARNING: overriding auto-detection)" << endl;
  }

  // First determine the name of the bankswitch file to use
  switch(type)
  {
    case BS_0840:
      armfile = "0840.arm";
      break;
    case BS_4K:
      armfile = "2k4k.arm";
      break;
    case BS_3E:
      armfile = "3E.arm";
      break;
    case BS_3F:
      armfile = "3F.arm";
      break;
    case BS_CV:
      armfile = "CV.arm";
      break;
    case BS_DPC:
      armfile = "DPC.arm";
      break;
    case BS_E0:
      armfile = "E0.arm";
      break;
    case BS_E7:
      armfile = "E7.arm";
      break;
    case BS_FA:
      armfile = "FA.arm";
      break;
    case BS_F4:
      armfile = "F4.arm";
      break;
    case BS_F6:
      armfile = "F6.arm";
      break;
    case BS_F8:
      armfile = "F8.arm";
      break;
    case BS_F4SC:
      armfile = "F4SC.arm";
      break;
    case BS_F6SC:
      armfile = "F6SC.arm";
      break;
    case BS_F8SC:
      armfile = "F8SC.arm";
      break;
    case BS_FE:
      armfile = "FE.arm";
      break;
    case BS_AR:
      armfile = "SC.arm";
      break;
    case BS_UA:
      armfile = "UA.arm";
      break;
    default:
      cout << "Warning - The Harmony Cartridge does not support \'"
           << Bankswitch::typeToName(type) << "\' bankswitching" << endl;
      result = "Bankswitch type not supported.";
      goto cleanup;
      break;
  }

  // Now load the proper bankswitch file
  // FIXME - abstract for different OS filesystems
  armfile = "arm/" + armfile;
  armbuf = readFile(armfile, armsize);
  if(armsize == 0)
  {
    result = "Couldn't open bankswitch ARM file.";
    goto cleanup;
  }

  // Now we have to combine the rom and ARM code



cleanup:
  delete[] rombuf;
  delete[] armbuf;
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 Cart::initSectors()
{
#if 0
  myCurrentSector = 0;

  if(myIsValid)
  {
    myNumSectors = myCartSize / 256;        // the number of 256 byte sectors
    if(myType == BS_3F || myType == BS_3E)  // 3F and 3E add 8 more (2040 - 2047)
      myNumSectors += 8;
  }
  else
    myNumSectors = 0;

  return myNumSectors;
#endif
return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 Cart::writeNextSector(SerialPort& port)
{
port.isOpen();
#if 0
  if(!myIsValid)
    throw "write: Invalid cart";
  else if(myCurrentSector == myNumSectors)
    throw "write: All sectors already written";

  // Handle 3F and 3E carts, which are a little different from the rest
  // There are two ranges of sectors; the second starts once we past the
  // cart size
  if((myType == BS_3F || myType == BS_3E) &&
      myCurrentSector == myCartSize / 256)
    myCurrentSector = 2040;

  uInt16 sector = myCurrentSector;
  uInt32 retry = 0;
  bool status;
  while(!(status = downloadSector(sector, port)) && retry++ < myRetry)
    cout << "Write transmission of sector " <<  sector << " failed, retry " << retry << endl;
  if(!status)
    throw "write: failed max retries";

  myCurrentSector++;
  return sector;
#endif
return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt16 Cart::verifyNextSector(SerialPort& port)
{
port.isOpen();
#if 0
  if(!myIsValid)
    throw "verify: Invalid cart";
  else if(myCurrentSector == myNumSectors)
    throw "verify: All sectors already verified";

  // Handle 3F and 3E carts, which are a little different from the rest
  // There are two ranges of sectors; the second starts once we past the
  // cart size
  if((myType == BS_3F || myType == BS_3E) &&
      myCurrentSector == myCartSize / 256)
    myCurrentSector = 2040;

  uInt16 sector = myCurrentSector;
  uInt32 retry = 0;
  bool status;
  while(!(status = verifySector(sector, port)) && retry++ < myRetry)
    cout << "Read transmission of sector " <<  sector << " failed, retry " << retry << endl;
  if(!status)
    throw "verify: failed max retries";

  myCurrentSector++;
  return sector;
#endif
return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8* Cart::readFile(const string& filename, uInt32& size)
{
  uInt8* buffer = (uInt8*) NULL;
  size = 0;

  cout << "Reading from file: \'" << filename << "\'" << endl;

  // Read file into buffer
  ifstream in(filename.c_str(), ios::binary);
  if(!in)
  {
    cerr << "ERROR: file not found\n";
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
  cout << "Read in " << length << " bytes" << endl;
  in.close();

  size = length;
  return buffer;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cart::downloadSector(uInt32 sector, SerialPort& port)
{
cerr << sector << port.isOpen();
#if 0
  uInt8 buffer[262];

  buffer[0] = 1;                             // Mark start of command
  buffer[1] = 0;                             // Command # for 'Download Sector'
  buffer[2] = (uInt8)((sector >> 8) & 0xff); // Sector # Hi-Byte
  buffer[3] = (uInt8)sector;                 // Sector # Lo-Byte
  buffer[4] = (uInt8)myType;                 // Bankswitching mode

  uInt8 chksum = 0;
  for(int i = 0; i < 256; i++)
    buffer[5+i] = myCart[(sector*256) + i];
  for(int i = 2; i < 261; i++)
    chksum ^= buffer[i];
  buffer[261] = chksum;

  // Write sector to serial port
  if(port.writeBytes(buffer, 262) != 262)
  {
    cout << "Transmission error in downloadSector" << endl;
    return false;
  }

  // Check return code of sector write
  uInt8 result = port.waitForAck();

  // Check return code
  if(result == 0x7c)
  {
    cout << "Checksum Error for sector " << sector << endl;
    return false;
  }
  else if(result == 0xff)
  {
    return true;
  }
  else
  {
    cout << "Undefined response " << (int)result << " for sector " << sector << endl;
    return false;
  }
#endif
return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cart::verifySector(uInt32 sector, SerialPort& port)
{
cerr << sector << port.isOpen();
#if 0
  uInt8 buffer[257];

  uInt8 chksum = 0;
  buffer[0] = 1;                             // Mark start of command
  buffer[1] = 1;                             // Command # for 'Read Sector'
  buffer[2] = (uInt8)((sector >> 8) & 0xff); // Sector # Hi-Byte
  buffer[3] = (uInt8)sector;                 // Sector # Lo-Byte
  chksum ^= buffer[2];
  chksum ^= buffer[3];
  buffer[4] = chksum;                        // Chksum

  // Write command to serial port
  if(port.writeBytes(buffer, 5) != 5)
  {
    cout << "Write transmission error of command in verifySector" << endl;
    return false;
  }

  // Check return code of command write
  uInt8 result = port.waitForAck();

  // Check return code
  if(result == 0x00)
  {
    cout << "Checksum Error for verify sector " << sector << endl;
    return false;
  }
  else if(result != 0xfe)
  {
    cout << "Undefined response " << (int)result << " for sector " << sector << endl;
    return false;
  }

  // Now it's safe to read the sector (256 data bytes + 1 chksum)
  int BytesRead = 0;
  do
  {
    uInt8 data = 0;
    if(port.readBytes(&data, 1) == 1)
      buffer[BytesRead++] = data;
  }
  while(BytesRead < 257);
  port.writeBytes(buffer, 1);  // Send an Ack

  // Make sure the data chksum matches
  chksum = 0;
  for(int i = 0; i < 256; ++i)
    chksum ^= buffer[i];
  if(chksum != buffer[256])
    return false;

  // Now that we have a valid sector read back from the device,
  // compare to the actual data to make sure they match
  return memcmp(myCart + sector*256, &buffer, 256) == 0;
#endif
return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cart::resetTarget(SerialPort& port, TARGET_MODE mode)
{
  switch (mode)
  {
    // Reset and jump to boot loader
    case PROGRAM_MODE:
      port.controlModemLines(1, 1);
      port.Sleep(100);
      port.clearBuffers();
      port.Sleep(100);
      port.controlModemLines(0, 1);
      // Longer delay is the Reset signal is conected to an external rest controller
      port.Sleep(500);
      // Clear the RTS line after having reset the micro
      // Needed for the "GO <Address> <Mode>" ISP command to work
      port.controlModemLines(0, 0);
      break;

    // Reset and start uploaded program
    case RUN_MODE:
      port.controlModemLines(1, 0);
      port.Sleep(100);
      port.clearBuffers();
      port.Sleep(100);
      port.controlModemLines(0, 0);
      port.Sleep(100);
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* Cart::lpc_PhilipsChipVersion(SerialPort& port)
{
  int nQuestionMarks;
  int found, i;
  int  strippedsize;
  char Answer[128];
//  char tmp_string[64];
  char temp[128];
  char *strippedAnswer, *endPtr;
  const char* cmdstr;
  time_t tStartUpload = 0;//, tDoneUpload = 0;

  static char version[1024] = { 0 };

  /* SA_CHANGED: '100' to '3' to shorten autodetection */
  for (nQuestionMarks = found = 0; !found && nQuestionMarks < 3; nQuestionMarks++)
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

#if 0
    if (strcmp(strippedAnswer, "Bootloader\r\n") == 0)
    {
      long chars, xtal;
      uInt32 ticks;
      chars = (17 * 0/*myBinaryLength*/ + 1) / 10;
      int WatchDogSeconds = (10 * chars + 5) / port.getBaud() + 10;
      xtal = atol(oscillator.c_str()) * 1000;
      ticks = (uInt32)WatchDogSeconds * ((xtal + 15) / 16);
//      DebugPrintf(2, "Entering ISP; re-synchronizing (watchdog = %ld seconds)\n", WatchDogSeconds);
      sprintf(temp, "T %lu\r\n", ticks);
      port.send(temp);
      port.receive(Answer, sizeof(Answer)-1, &realsize, 1,100);
      if (strcmp(Answer, "OK\r\n") != 0)
      {
        strcpy(version, "ERROR: No answer on \'watchdog timer set\'");
        return version;
      }
      port.send("G 10356\r\n");
      port.Sleep(200);
      nQuestionMarks = 0;
//      WaitForWatchDog = 1;
      continue;
    }
#endif

    tStartUpload = time(NULL);
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
    sprintf(version, "LPC%d, %d kiB ROM / %d kiB SRAM",
            LPCtypes[myDetectedDevice].Product,
            LPCtypes[myDetectedDevice].FlashSize,
            LPCtypes[myDetectedDevice].RAMSize);
  }

  return version;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Cart::lpc_PhilipsDownload(SerialPort& port, uInt8* data, uInt32 size,
                              bool showdialog)
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
  long WatchDogSeconds = 0;
  int WaitForWatchDog = 0;
  const char* cmdstr;
  int repeat = 0;

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
  delete[] myBinaryContent;  myBinaryContent = NULL;
  uInt32 myBinaryOffset = 0, myStartAddress = 0, myBinaryLength = size;
  if(myBinaryLength % 4 != 0)
  {
    uInt32 newBinaryLength = ((myBinaryLength + 3)/4) * 4;
    cerr << "Warning:  data not aligned to 32 bits, padded (length was "
         << myBinaryLength << ", now " << newBinaryLength << ")\n";
    myBinaryLength = newBinaryLength;
  }
  myBinaryContent = new uInt8[myBinaryLength];
  memcpy(myBinaryContent, data, size);

  QProgressDialog* progress;
  uInt32 progressStep = 0, progressSize = myBinaryLength/45 + 20;

  if(LPCtypes[myDetectedDevice].ChipVariant == CHIP_VARIANT_LPC2XXX)
  {
    // Patch 0x14, otherwise it is not running and jumps to boot mode
    ivt_CRC = 0;

    // Clear the vector at 0x14 so it doesn't affect the checksum:
    for(int i = 0; i < 4; i++)
      myBinaryContent[i + 0x14] = 0;

    // Calculate a native checksum of the little endian vector table:
    for(int i = 0; i < (4 * 8);)
    {
      ivt_CRC += myBinaryContent[i++];
      ivt_CRC += myBinaryContent[i++] << 8;
      ivt_CRC += myBinaryContent[i++] << 16;
      ivt_CRC += myBinaryContent[i++] << 24;
    }

    /* Negate the result and place in the vector at 0x14 as little endian
     * again. The resulting vector table should checksum to 0. */
    ivt_CRC = (uInt32) (0 - ivt_CRC);
    for (int i = 0; i < 4; i++)
      myBinaryContent[i + 0x14] = (unsigned char)(ivt_CRC >> (8 * i));
  }
  else if(LPCtypes[myDetectedDevice].ChipVariant == CHIP_VARIANT_LPC17XX)
  {
    // Patch 0x1C, otherwise it is not running and jumps to boot mode
    ivt_CRC = 0;

    // Clear the vector at 0x1C so it doesn't affect the checksum:
    for (int i = 0; i < 4; i++)
      myBinaryContent[i + 0x1C] = 0;

    // Calculate a native checksum of the little endian vector table:
    for (int i = 0; i < (4 * 8);)
    {
      ivt_CRC += myBinaryContent[i++];
      ivt_CRC += myBinaryContent[i++] << 8;
      ivt_CRC += myBinaryContent[i++] << 16;
      ivt_CRC += myBinaryContent[i++] << 24;
    }

    /* Negate the result and place in the vector at 0x1C as little endian
     * again. The resulting vector table should checksum to 0. */
    ivt_CRC = (uInt32) (0 - ivt_CRC);
    for (int i = 0; i < 4; i++)
      myBinaryContent[i + 0x1C] = (unsigned char)(ivt_CRC >> (8 * i));
  }

  cout << "Synchronizing (ESC to abort)";

  /* SA_CHANGED: '100' to '3' to shorten autodetection */
  for (nQuestionMarks = found = 0; !found && nQuestionMarks < 3; nQuestionMarks++)
  {
    cout << ".";
    port.send("?");

    memset(Answer, 0, sizeof(Answer));
    strippedsize = port.receive(Answer, sizeof(Answer)-1, 1, 100);
    strippedAnswer = Answer;

    while ((strippedsize > 0) && ((*strippedAnswer == '?') || (*strippedAnswer == 0)))
    {
      strippedAnswer++;
      strippedsize--;
    }

    if (strcmp(strippedAnswer, "Bootloader\r\n") == 0)
    {
      int chars, xtal;
      uInt32 ticks;
      chars = (17 * 0/*myBinaryLength*/ + 1) / 10;
      int WatchDogSeconds = (10 * chars + 5) / port.getBaud() + 10;
      xtal = atol(myOscillator.c_str()) * 1000;
      ticks = (uInt32)WatchDogSeconds * ((xtal + 15) / 16);
      cout << "Entering ISP; re-synchronizing (watchdog = " << WatchDogSeconds << " seconds)\n";
      sprintf(temp, "T %u\r\n", ticks);
      port.send(temp);
      port.receive(Answer, sizeof(Answer)-1, 1, 100);
      if (strcmp(Answer, "OK\r\n") != 0)
      {
        cerr << "No answer on 'watchdog timer set'\n";
        return (NO_ANSWER_WDT);
      }
      port.send("G 10356\r\n");
      port.Sleep(200);
      nQuestionMarks = 0;
      WaitForWatchDog = 1;
      continue;
    }

    tStartUpload = time(NULL);
    if (strcmp(strippedAnswer, "Synchronized\r\n") == 0)
      found = 1;
    else
      resetTarget(port, PROGRAM_MODE);
  } // end for

  if(!found)
  {
    cerr << "ERROR: no answer on '?'\n";
    return (NO_ANSWER_QM);
  }

  cout << " OK\n";

  port.send("Synchronized\n");
  port.receive(Answer, sizeof(Answer) - 1, 2, 1000);
  if ((strcmp(Answer, "Synchronized\r\nOK\r\n") != 0) && (strcmp(Answer, "Synchronized\rOK\r\n") != 0) &&
      (strcmp(Answer, "Synchronized\nOK\r\n") != 0))
  {
    cerr << "No answer on 'Synchronized'\n";
    return (NO_ANSWER_SYNC);
  }

  sprintf(temp, "%s\n", myOscillator.c_str());
  port.send(temp);
  port.receive(Answer, sizeof(Answer)-1, 2, 1000);

  sprintf(temp, "%s\nOK\r\n", myOscillator.c_str());
  if (strcmp(Answer, temp) != 0)
  {
    cerr << "No answer on Oscillator-Command\n";
    return (NO_ANSWER_OSC);
  }

  cmdstr = "U 23130\n";
  if (!lpc_SendAndVerify(port, cmdstr, Answer, sizeof Answer))
  {
    cerr << "Unlock-Command:\n";
    return (UNLOCK_ERROR + lpc_GetAndReportErrorNumber(Answer));
  }

  cout << "Read bootcode version: ";

  cmdstr = "K\n";
  port.send(cmdstr);
  port.receive(Answer, sizeof(Answer)-1, 4, 5000);

  if (strncmp(Answer, cmdstr, strlen(cmdstr)) != 0)
  {
    cerr << "no answer on Read Boot Code Version\n";
    return (NO_ANSWER_RBV);
  }

  if (strncmp(Answer + strlen(cmdstr), "0\r\n", 3) == 0)
  {
    strippedAnswer = Answer + strlen(cmdstr) + 3;
    cout << strippedAnswer;
  }
  else
    cout << "unknown\n";

  cout << "Read part ID: ";

  cmdstr = "J\n";
  port.send(cmdstr);
  port.receive(Answer, sizeof(Answer)-1, 3, 5000);

  if (strncmp(Answer, cmdstr, strlen(cmdstr)) != 0)
  {
    cerr << "no answer on Read Part Id\n";
    return (NO_ANSWER_RPID);
  }

  strippedAnswer = (strncmp(Answer, "J\n0\r\n", 5) == 0) ? Answer + 5 : Answer;
  Pos = strtoul(strippedAnswer, &endPtr, 10);
  *endPtr = '\0'; /* delete \r\n */
  for (i = sizeof LPCtypes / sizeof LPCtypes[0] - 1; i > 0 && LPCtypes[i].id != Pos; i--)
    /* nothing */;

  myDetectedDevice = i;
  if (myDetectedDevice == 0)
  {
    cout << "unknown";
  }
  else
  {
    char version[100];
    sprintf(version, "LPC%d, %d kiB ROM / %d kiB SRAM",
            LPCtypes[myDetectedDevice].Product,
            LPCtypes[myDetectedDevice].FlashSize,
            LPCtypes[myDetectedDevice].RAMSize);
    cout << version;
  }
  cout << " (" << hex << Pos << dec << ")\n";

  /* In case of a download to RAM, use full RAM for downloading
   * set the flash parameters to full RAM also.
   * This makes sure that all code is downloaded as one big sector
   */
  if (myBinaryOffset >= lpc_ReturnValueLpcRamStart())
  {
    LPCtypes[myDetectedDevice].FlashSectors = 1;
    LPCtypes[myDetectedDevice].MaxCopySize  = LPCtypes[myDetectedDevice].RAMSize*1024 - (lpc_ReturnValueLpcRamBase() - lpc_ReturnValueLpcRamStart());
    LPCtypes[myDetectedDevice].SectorTable  = SectorTable_RAM;
    SectorTable_RAM[0] = LPCtypes[myDetectedDevice].MaxCopySize;
  }

  // Start with sector 1 and go upward... Sector 0 containing the interrupt vectors
  // will be loaded last, since it contains a checksum and device will re-enter
  // bootloader mode as long as this checksum is invalid.
  cout << "Will start programming at Sector 1 if possible, and conclude with Sector 0 to ensure that checksum is written last.\n";
  if (LPCtypes[myDetectedDevice].SectorTable[0] >= myBinaryLength)
  {
    Sector = 0;
    SectorStart = 0;
  }
  else
  {
    SectorStart = LPCtypes[myDetectedDevice].SectorTable[0];
    Sector = 1;
  }


char BUF[200];
sprintf(BUF, " ==> offset = %d, start addr = %d, length = %d\n", myBinaryOffset, myStartAddress, myBinaryLength);
cerr <<  BUF << endl;
sprintf(BUF, " ==> Start @ sector = %d\n", Sector);
cerr <<  BUF << endl;








  // Erasing sector 0 first
  cout << "Erasing sector 0 first, to invalidate checksum. ";

  sprintf(tmpString, "P %d %d\n", 0, 0);
  if (!lpc_SendAndVerify(port, tmpString, Answer, sizeof Answer))
  {
    cerr << "Wrong answer on Prepare-Command\n";
    return (WRONG_ANSWER_PREP + lpc_GetAndReportErrorNumber(Answer));
  }

  sprintf(tmpString, "E %d %d\n", 0, 0);
  if (!lpc_SendAndVerify(port, tmpString, Answer, sizeof Answer))
  {
    cerr << "Wrong answer on Erase-Command\n";
    return (WRONG_ANSWER_ERAS + lpc_GetAndReportErrorNumber(Answer));
  }
  cout << "OK \n";

  if(showdialog)
  {
    progress = new QProgressDialog("Downloading data ...", QString(), 0, progressSize);
    progress->setWindowModality(Qt::WindowModal);
    progress->setMinimumDuration(0);
    progressStep = 0;
  }

  // OK, the main loop where we start writing to the cart
  while (1)
  {
    if (Sector >= LPCtypes[myDetectedDevice].FlashSectors)
    {
      cerr << "Program too large; running out of Flash sectors.\n";
      return (PROGRAM_TOO_LARGE);
    }

    cout << "Sector " << Sector << flush;

    if (myBinaryOffset < lpc_ReturnValueLpcRamStart()) // Skip Erase when running from RAM
    {
      sprintf(tmpString, "P %d %d\n", Sector, Sector);
      if (!lpc_SendAndVerify(port, tmpString, Answer, sizeof Answer))
      {
        cerr << "Wrong answer on Prepare-Command (1) (Sector " << Sector << ")\n";
        return (WRONG_ANSWER_PREP + lpc_GetAndReportErrorNumber(Answer));
      }

      cout << "." << flush;

      if (Sector != 0) // Sector 0 already erased
      {
        sprintf(tmpString, "E %d %d\n", Sector, Sector);
        if (!lpc_SendAndVerify(port, tmpString, Answer, sizeof Answer))
        {
          cerr << "Wrong answer on Erase-Command (Sector " << Sector << ")\n";
          return (WRONG_ANSWER_ERAS + lpc_GetAndReportErrorNumber(Answer));
        }

        cout << "." << flush;
      }
    }

    SectorLength = LPCtypes[myDetectedDevice].SectorTable[Sector];
    if (SectorLength > myBinaryLength - SectorStart)
      SectorLength = myBinaryLength - SectorStart;

    for (SectorOffset = 0; SectorOffset < SectorLength; SectorOffset += SectorChunk)
    {
      if (SectorOffset > 0)
        cout << "|" << flush;

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
        cerr << "Wrong answer on Write-Command\n";
        return (WRONG_ANSWER_WRIT + lpc_GetAndReportErrorNumber(Answer));
      }

      cout << "." << flush;

      block_CRC = 0;
      Line = 0;

      // Transfer blocks of 45 * 4 bytes to RAM
      for (Pos = SectorStart + SectorOffset; (Pos < SectorStart + SectorOffset + CopyLength) && (Pos < myBinaryLength); Pos += (45 * 4))
      {
        for (Block = 0; Block < 4; Block++)  // Each block 45 bytes
        {
          cout << "." << flush;

          // Inform the calling application about having written another chuck of data
          if(showdialog)
            progress->setValue(++progressStep);

          // Uuencode one 45 byte block
          tmpStringPos = 0;

          sendbuf[Line][tmpStringPos++] = (char)(' ' + 45);    // Encode Length of block

          for (BlockOffset = 0; BlockOffset < 45; BlockOffset++)
          {
            if (myBinaryOffset < lpc_ReturnValueLpcRamStart())
            { // Flash: use full memory
              c = myBinaryContent[Pos + Block * 45 + BlockOffset];
            }
            else
            { // RAM: Skip first 0x200 bytes, these are used by the download program in LPC21xx
              c = myBinaryContent[Pos + Block * 45 + BlockOffset + 0x200];
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
            for (repeat = 0; repeat < 3; repeat++)
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

            if (repeat >= 3)
            {
              cerr << "Error on writing block_CRC (1)\n";
              return (ERROR_WRITE_CRC);
            }
            Line = 0;
            block_CRC = 0;
          }
        }
      }

      if (Line != 0)
      {
        for (repeat = 0; repeat < 3; repeat++)
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

        if (repeat >= 3)
        {
          cerr << "Error on writing block_CRC (3)\n";
          return (ERROR_WRITE_CRC2);
        }
      }

      if (myBinaryOffset < lpc_ReturnValueLpcRamStart())
      {
        // Prepare command must be repeated before every write
        sprintf(tmpString, "P %d %d\n", Sector, Sector);

        if (!lpc_SendAndVerify(port, tmpString, Answer, sizeof Answer))
        {
          cerr << "Wrong answer on Prepare-Command (2) (Sector " << Sector << ")\n";
          return (WRONG_ANSWER_PREP2 + lpc_GetAndReportErrorNumber(Answer));
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
          cerr << "Wrong answer on Copy-Command\n";
          return (WRONG_ANSWER_COPY + lpc_GetAndReportErrorNumber(Answer));
        }

        if (0)// FIXME IspEnvironment->Verify)
        {
          //Avoid compare first 64 bytes.
          //Because first 64 bytes are re-mapped to flash boot sector,
          //and the compare result may not be correct.
          if (SectorStart + SectorOffset<64)
            sprintf(tmpString, "M %d %d %d\n", 64, lpc_ReturnValueLpcRamBase() + (64 - SectorStart - SectorOffset), CopyLength-(64 - SectorStart - SectorOffset));
          else
           sprintf(tmpString, "M %d %d %d\n", SectorStart + SectorOffset, lpc_ReturnValueLpcRamBase(), CopyLength);

          if (!lpc_SendAndVerify(port, tmpString, Answer, sizeof Answer))
          {
            cerr << "Wrong answer on Compare-Command\n";
            return (WRONG_ANSWER_COPY + lpc_GetAndReportErrorNumber(Answer));
          }
        }
      }
    }

    cout << "\n" << flush;

    if ((SectorStart + SectorLength) >= myBinaryLength && Sector!=0)
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
  if (0)// FIXMEIspEnvironment->Verify)
    cout << "Download Finished and Verified correct... taking " << int(tDoneUpload - tStartUpload) << " seconds\n";
  else
    cout << "Download Finished... taking " << int(tDoneUpload - tStartUpload) << " seconds\n";

  if (WaitForWatchDog)
  {
    cerr << "Wait for restart, in " << (WatchDogSeconds - (tDoneUpload - tStartUpload)) << " seconds from now\n";
  }
  else
  {
    cout << "Now launching the brand new code\n" << flush;

    if(LPCtypes[myDetectedDevice].ChipVariant == CHIP_VARIANT_LPC2XXX)
      sprintf(tmpString, "G %d A\n", myStartAddress);
    else if(LPCtypes[myDetectedDevice].ChipVariant == CHIP_VARIANT_LPC17XX)
      sprintf(tmpString, "G %d T\n", myStartAddress & ~1);

    port.send(tmpString);  //goto 0 : run this fresh new downloaded code code
    if (myBinaryOffset < lpc_ReturnValueLpcRamStart())
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
        sprintf(ExpectedAnswer, "G %d A\n0", myStartAddress);
      else if(LPCtypes[myDetectedDevice].ChipVariant == CHIP_VARIANT_LPC17XX)
        sprintf(ExpectedAnswer, "G %d T\n0", myStartAddress & ~1);

      if (realsize == 0 || strncmp((const char *)Answer, /*cmdstr*/ExpectedAnswer, strlen(/*cmdstr*/ExpectedAnswer)) != 0)
      {
        cerr << "Failed to run the new downloaded code: ";
        return (FAILED_RUN + lpc_GetAndReportErrorNumber(Answer));
      }
    }

    cout << flush;
  }
cleanup:
  if(showdialog)
    progress->setValue(progressSize);

  return (0);
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

//  PhilipsOutputErrorMessage(Result);

  return Result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 Cart::lpc_ReturnValueLpcRamStart()
{
  if(LPCtypes[myDetectedDevice].ChipVariant == CHIP_VARIANT_LPC2XXX)
    return LPC_RAMSTART_LPC2XXX;
  else if(LPCtypes[myDetectedDevice].ChipVariant == CHIP_VARIANT_LPC17XX)
    return LPC_RAMSTART_LPC17XX;

  return 0; // FIXME
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 Cart::lpc_ReturnValueLpcRamBase()
{
  if(LPCtypes[myDetectedDevice].ChipVariant == CHIP_VARIANT_LPC2XXX)
    return LPC_RAMBASE_LPC2XXX;
  else if(LPCtypes[myDetectedDevice].ChipVariant == CHIP_VARIANT_LPC17XX)
    return LPC_RAMBASE_LPC17XX;

  return 0; // FIXME
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
  8192, 8192, 8192, 8192, 8192, 8192, 8192,
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uInt32 Cart::SectorTable_212x[] = {
  8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192,
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
// Used for LPC17xx devices
const uInt32 Cart::SectorTable_17xx[] = {
   4096,  4096,  4096,  4096,  4096,  4096,  4096,  4096,
   4096,  4096,  4096,  4096,  4096,  4096,  4096,  4096,
  32768, 32768, 32768, 32768, 32768, 32768, 32768, 32768,
  32768, 32768, 32768, 32768, 32768, 32768
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 Cart::SectorTable_RAM[] = { 65000 };

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cart::LPC_DEVICE_TYPE Cart::LPCtypes[] = {
  { 0, 0, 0, 0, 0, 0, 0, CHIP_VARIANT_UNKNOWN },  /* unknown */

  { 0x00001110, 1751,  32,  8,  8, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
  { 0x00001121, 1752,  64, 16, 16, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
  { 0x00011722, 1754, 128, 32, 18, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
  { 0x00011723, 1756, 256, 32, 22, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
  { 0x00013F34, 1758, 512, 64, 30, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
  { 0x00011922, 1764, 128, 32, 18, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
  { 0x00013733, 1765, 256, 64, 22, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
  { 0x00013F33, 1766, 256, 64, 22, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
  { 0x26013F33, 1766, 256, 64, 22, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
  { 0x00013F37, 1768, 512, 64, 30, 4096, SectorTable_17xx, CHIP_VARIANT_LPC17XX },
  { 0x0004FF11, 2103,  32,  8,  8, 4096, SectorTable_2103, CHIP_VARIANT_LPC2XXX },
  { 0xFFF0FF12, 2104, 128, 16, 15, 8192, SectorTable_210x, CHIP_VARIANT_LPC2XXX },
  { 0xFFF0FF22, 2105, 128, 32, 15, 8192, SectorTable_210x, CHIP_VARIANT_LPC2XXX },
  { 0xFFF0FF32, 2106, 128, 64, 15, 8192, SectorTable_210x, CHIP_VARIANT_LPC2XXX },
  { 0x0201FF01, 2109,  64,  8,  8, 4096, SectorTable_2109, CHIP_VARIANT_LPC2XXX },
  { 0x0101FF12, 2114, 128, 16, 15, 8192, SectorTable_211x, CHIP_VARIANT_LPC2XXX },
  { 0x0201FF12, 2119, 128, 16, 15, 8192, SectorTable_211x, CHIP_VARIANT_LPC2XXX },
  { 0x0101FF13, 2124, 256, 16, 17, 8192, SectorTable_212x, CHIP_VARIANT_LPC2XXX },
  { 0x0201FF13, 2129, 256, 16, 17, 8192, SectorTable_212x, CHIP_VARIANT_LPC2XXX },
  { 0x0002FF01, 2131,  32,  8,  8, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
  { 0x0002FF11, 2132,  64, 16,  9, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
  { 0x0002FF12, 2134, 128, 16, 11, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
  { 0x0002FF23, 2136, 256, 32, 15, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
  { 0x0002FF25, 2138, 512, 32, 27, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
  { 0x0402FF01, 2141,  32,  8,  8, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
  { 0x0402FF11, 2142,  64, 16,  9, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
  { 0x0402FF12, 2144, 128, 16, 11, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
  { 0x0402FF23, 2146, 256, 40, 15, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
  { 0x0402FF25, 2148, 512, 40, 27, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
  { 0x0301FF13, 2194, 256, 16, 17, 8192, SectorTable_212x, CHIP_VARIANT_LPC2XXX },
  { 0x0301FF12, 2210,   0, 16,  0, 8192, SectorTable_211x, CHIP_VARIANT_LPC2XXX }, /* table is a "don't care" */
  { 0x0401FF12, 2212, 128, 16, 15, 8192, SectorTable_211x, CHIP_VARIANT_LPC2XXX },
  { 0x0601FF13, 2214, 256, 16, 17, 8192, SectorTable_212x, CHIP_VARIANT_LPC2XXX },
  /*            2290; same id as the LPC2210 */
  { 0x0401FF13, 2292, 256, 16, 17, 8192, SectorTable_212x, CHIP_VARIANT_LPC2XXX },
  { 0x0501FF13, 2294, 256, 16, 17, 8192, SectorTable_212x, CHIP_VARIANT_LPC2XXX },
  { 0x00000000, 2361, 128, 34, 11, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
  { 0x00000000, 2362, 128, 34, 11, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
  { 0x1600F902, 2364, 128, 34, 11, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
  { 0x1600E823, 2365, 256, 58, 15, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
  { 0x1600F923, 2366, 256, 58, 15, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
  { 0x1600E825, 2367, 512, 58, 15, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
  { 0x1600F925, 2368, 512, 58, 28, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
  { 0x1700E825, 2377, 512, 58, 28, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
  { 0x1700FD25, 2378, 512, 58, 28, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
  { 0x1800F935, 2387, 512, 98, 28, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
  { 0x1800FF35, 2388, 512, 98, 28, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
  { 0x1500FF35, 2458, 512, 98, 28, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
  { 0x1600FF30, 2460,   0, 98,  0, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
  { 0x1600FF35, 2468, 512, 98, 28, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
  { 0x1701FF30, 2470,   0, 98,  0, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX },
  { 0x1701FF35, 2478, 512, 98, 28, 4096, SectorTable_213x, CHIP_VARIANT_LPC2XXX }
};
