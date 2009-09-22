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

#include <cstring>
#include <fstream>

#include "bspf.hxx"

#include "BSType.hxx"
#include "Cart.hxx"
#include "CartDetector.hxx"
#include "SerialPort.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Cart::Cart()
  : myCartSize(0),
    myRetry(0),
    myType(BS_NONE),
    myCurrentSector(0),
    myNumSectors(0),
    myIsValid(false)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
string Cart::autodetectHarmony(SerialPort& port)
{
  ISP_ENVIRONMENT IspEnvironment;
  const char* result;

  // Initialize ISP Environment
  //   SA: These values are hard-coded for the Harmony cart
  memset(&IspEnvironment, 0, sizeof(IspEnvironment));
  IspEnvironment.micro               = PHILIPS_ARM;
  IspEnvironment.FileFormat          = FORMAT_BINARY;  // -bin
  IspEnvironment.ProgramChip         = 0;          // -detectonly
  IspEnvironment.DetectOnly          = 1;           //
//  IspEnvironment.ControlLines        = 1;           // -control
//  IspEnvironment.ControlLinesSwapped = 1;           // -controlswap
//  IspEnvironment.serial_port         = (char*)device;
//  IspEnvironment.baud_rate           = "38400";        // HC baud is always 38400
//  strcpy(IspEnvironment.StringOscillator, "10000");    // HC oscillator is always 10000

  resetTarget(port, PROGRAM_MODE);
  port.ClearSerialPortBuffers();

  // Get the version #, if any
  result = lpc_PhilipsChipVersion(port);
  if (strncmp(result, "ERROR:", 6) == 0)
    return result;
  else if (IspEnvironment.StartAddress == 0)
  {
    /* Only reset target if startaddress = 0
     * Otherwise stay with the running program as started in Download()
     */
    resetTarget(port, RUN_MODE);
  }

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Cart::resetTarget(SerialPort& port, TARGET_MODE mode)
{
  switch (mode)
  {
    // Reset and jump to boot loader
    case PROGRAM_MODE:
      port.ControlModemLines(1, 1);
      port.Sleep(100);
      port.ClearSerialPortBuffers();
      port.Sleep(100);
      port.ControlModemLines(0, 1);
      // Longer delay is the Reset signal is conected to an external rest controller
      port.Sleep(500);
      // Clear the RTS line after having reset the micro
      // Needed for the "GO <Address> <Mode>" ISP command to work
      port.ControlModemLines(0, 0);
      break;

    // Reset and start uploaded program
    case RUN_MODE:
      port.ControlModemLines(1, 0);
      port.Sleep(100);
      port.ClearSerialPortBuffers();
      port.Sleep(100);
      port.ControlModemLines(0, 0);
      port.Sleep(100);
      break;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cart::updateBIOS(const string& filename)
{
cerr << "update BIOS from " << filename << endl;
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cart::create(const string& filename, const string& type)
{
#if 0
  memset(myCart, 0, MAXCARTSIZE);
  myCartSize = readFile(filename, myCart, MAXCARTSIZE, type);
  myIsValid = myCartSize > 0;
  return myIsValid;
#endif
return false;
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
BSType Cart::autodetectType(uInt8* data, uInt32 size)
{
  return CartDetector::autodetectType(data, size);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int Cart::readFile(const string& filename, uInt8* cartridge, uInt32 maxSize,
                   const string& type)
{
#if 0
  cout << "Reading from file: \'" << filename << "\'" << endl;

  uInt32 minsize = (maxSize != MAXCARTSIZE) ? maxSize : 4096;

  // Read file into buffer
  ifstream in(filename.c_str(), ios::binary);
  if(!in)
    return 0;

  // Figure out how much data we should read
  in.seekg(0, ios::end);
  streampos length = in.tellg();
  in.seekg(0, ios::beg);
  uInt32 cartsize = length > maxSize ? maxSize : (uInt32)length;

  in.read((char*)cartridge, cartsize);
  cout << "Read in " << cartsize << " bytes" << endl;
  in.close();

  // Auto-detect the bankswitch type
  /* TODO - do we really need to consult stella.pro anymore??
  {   // find MD5 value
    MD5_CTX context;
    unsigned char digest[16];
    MD5Init (&context);
    MD5Update (&context, cartridge, cartsize);
    MD5Final (digest, &context);
    printf ("MD5 = ");
    MDPrint (digest);
  }
  */
  if(type == "")
  {
    myType = autodetectType(cartridge, cartsize);
    cout << "Bankswitch type: " << Bankswitch::typeToName(myType)
         << " (auto-detected)" << endl;
  }
  else
  {
    myType = Bankswitch::nameToType(type);
    cout << "Bankswitch type: " << Bankswitch::typeToName(myType)
         << " (WARNING: overriding auto-detection)" << endl;
  }
  switch(myType)
  {
    case BS_F0:
    case BS_E0:
    case BS_FE:
    case BS_AR:
    case BS_NONE:
      cout << "Warning - The Krokodile Cartridge does not support this type of bank switching" << endl;
      break;
    default:
      break;
  }

  // Pad buffer to minimum size
  if(cartsize < minsize)
  {
    cout << "  Converting to " << (minsize/1024) << "K." << endl;
    int i = 0;
    while(cartsize < minsize)
      cartridge[cartsize++] = cartridge[i++];
  }
  cout << endl;

  // 3F and 3E carts need the upper bank in uppermost part of the ROM
  if(myType == BS_3F || myType == BS_3E)
    for(int i = 0; i < 2048; i++)
      myCart[MAXCARTSIZE - 2048 + i] = myCart[cartsize - 2048 + i];

  return cartsize;
#endif
return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Cart::downloadSector(uInt32 sector, SerialPort& port)
{
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
const char* Cart::lpc_PhilipsChipVersion(SerialPort& port)
{
  unsigned int realsize;
  int nQuestionMarks;
  int found, i;
  int  strippedsize;
  char Answer[128];
  char tmp_string[64];
  char temp[128];
  char *strippedAnswer, *endPtr, *cmdstr;

  static char version[1024] = { 0 };

  /* SA_CHANGED: '100' to '3' to shorten autodetection */
  for (nQuestionMarks = found = 0; !found && nQuestionMarks < 3; nQuestionMarks++)
  {
    port.SendComPort("?");

    memset(Answer, 0, sizeof(Answer));
    port.ReceiveComPort(Answer, sizeof(Answer)-1, &realsize, 1, 100);

    strippedAnswer = Answer;
    strippedsize = realsize;
    while ((strippedsize > 0) && ((*strippedAnswer == '?') || (*strippedAnswer == 0)))
    {
      strippedAnswer++;
      strippedsize--;
    }

    if (strcmp(strippedAnswer, "Bootloader\r\n") == 0)
    {
      long chars, xtal;
      unsigned long ticks;
      chars = (17 * IspEnvironment->BinaryLength + 1) / 10;
      WatchDogSeconds = (10 * chars + 5) / atol(IspEnvironment->baud_rate) + 10;
      xtal = atol(IspEnvironment->StringOscillator) * 1000;
      ticks = (unsigned long)WatchDogSeconds * ((xtal + 15) / 16);
      DebugPrintf(2, "Entering ISP; re-synchronizing (watchdog = %ld seconds)\n", WatchDogSeconds);
      sprintf(temp, "T %lu\r\n", ticks);
      port.SendComPort(IspEnvironment, temp);
      port.ReceiveComPort(IspEnvironment, Answer, sizeof(Answer)-1, &realsize, 1,100);
      if (strcmp(Answer, "OK\r\n") != 0)
      {
        strcpy(version, "ERROR: No answer on \'watchdog timer set\'");
        return version;
      }
      port.SendComPort(IspEnvironment, "G 10356\r\n");
      port.Sleep(200);
      nQuestionMarks = 0;
      WaitForWatchDog = 1;
      continue;
    }

    tStartUpload = time(NULL);
    if (strcmp(strippedAnswer, "Synchronized\r\n") == 0)
      found = 1;
    else
      ResetTarget(IspEnvironment, PROGRAM_MODE);
  } // end for

  if (!found)
  {
    strcpy(version, "ERROR: no answer on \'?\'");
    return version;
  }

  port.SendComPort(IspEnvironment, "Synchronized\n");
  port.ReceiveComPort(IspEnvironment, Answer, sizeof(Answer) - 1, &realsize, 2, 1000);

  if ((strcmp(Answer, "Synchronized\r\nOK\r\n") != 0) && (strcmp(Answer, "Synchronized\rOK\r\n") != 0) &&
      (strcmp(Answer, "Synchronized\nOK\r\n") != 0))
  {
    strcpy(version, "ERROR: No answer on \'Synchronized\'");
    return version;
  }

  sprintf(temp, "%s\n", IspEnvironment->StringOscillator);
  port.SendComPort(IspEnvironment, temp);
  port.ReceiveComPort(IspEnvironment, Answer, sizeof(Answer)-1, &realsize, 2, 1000);

  sprintf(temp, "%s\nOK\r\n", IspEnvironment->StringOscillator);
  if (strcmp(Answer, temp) != 0)
  {
    strcpy(version, "ERROR: No answer on Oscillator-Command");
    return version;
  }

  cmdstr = "U 23130\n";
  if (!SendAndVerify(IspEnvironment, cmdstr, Answer, sizeof Answer))
  {
    sprintf(version, "ERROR: Unlock-Command: %d", (UNLOCK_ERROR + GetAndReportErrorNumber(Answer)));
    return version;
  }

  cmdstr = "K\n";
  port.SendComPort(IspEnvironment, cmdstr);
  port.ReceiveComPort(IspEnvironment, Answer, sizeof(Answer)-1, &realsize, 4,5000);
  if (strncmp(Answer, cmdstr, strlen(cmdstr)) != 0)
  {
    strcpy(version, "ERROR: no answer on Read Boot Code Version");
    return version;
  }

  if (strncmp(Answer + strlen(cmdstr), "0\r\n", 3) == 0)
    strippedAnswer = Answer + strlen(cmdstr) + 3;

  cmdstr = "J\n";
  port.SendComPort(IspEnvironment, cmdstr);
  port.ReceiveComPort(IspEnvironment, Answer, sizeof(Answer)-1, &realsize, 3,5000);
  if (strncmp(Answer, cmdstr, strlen(cmdstr)) != 0)
  {
    strcpy(version, "ERROR: no answer on Read Part Id");
    return version;
  }

  strippedAnswer = (strncmp(Answer, "J\n0\r\n", 5) == 0) ? Answer + 5 : Answer;
  Pos = strtoul(strippedAnswer, &endPtr, 10);
  *endPtr = '\0'; /* delete \r\n */
  for (i = sizeof LPCtypes / sizeof LPCtypes[0] - 1; i > 0 && LPCtypes[i].id != Pos; i--)
    /* nothing */;

  IspEnvironment->DetectedDevice = i;
  if (IspEnvironment->DetectedDevice != 0)
  {
    sprintf(version, "LPC%d, %d kiB ROM / %d kiB SRAM",
            LPCtypes[IspEnvironment->DetectedDevice].Product,
            LPCtypes[IspEnvironment->DetectedDevice].FlashSize,
            LPCtypes[IspEnvironment->DetectedDevice].RAMSize);
  }

  return version;
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
