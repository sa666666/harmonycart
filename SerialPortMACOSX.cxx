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

#include "bspf.hxx"

#if defined(BSPF_MAC_OSX)

#include <cstdio>
#include <fcntl.h>
#include <sys/errno.h>
#include <sys/termios.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <cstring>

#include "SerialPortMACOSX.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SerialPortMACOSX::SerialPortMACOSX()
  : SerialPort(),
    myHandle(0)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SerialPortMACOSX::~SerialPortMACOSX()
{
  closePort();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SerialPortMACOSX::openPort(const string& device)
{
  closePort();  // paranoia: make sure port is in consistent state

  myHandle = open(device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
  if(myHandle <= 0)
    return false;

  struct termios termios;
  tcgetattr(myHandle, &termios);
  memset(&termios, 0, sizeof(struct termios));
  cfmakeraw(&termios);
  cfsetspeed(&termios, 115200);
  termios.c_cflag = CREAD | CLOCAL;
  termios.c_cflag |= CS8;
  termios.c_cflag |= CDTR_IFLOW;
  tcsetattr(myHandle, TCSANOW, &termios);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SerialPortMACOSX::closePort()
{
  if(myHandle)
    close(myHandle);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SerialPortMACOSX::isOpen()
{
  return myHandle > 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int SerialPortMACOSX::readBytes(uInt8* data, uInt32 size)
{
  return myHandle ? read(myHandle, data, size) : -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int SerialPortMACOSX::writeBytes(const uInt8* data, uInt32 size)
{
  return myHandle ? write(myHandle, data, size) : -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 SerialPortMACOSX::waitForAck(uInt32 wait)
{
  uInt8 result = 0;
  for(int pass = 0; pass < 100; ++pass)
  {
    if(readBytes(&result, 1) == 1)
      break;
    usleep(wait);
  }
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const StringList& SerialPortMACOSX::getPortNames()
{
  myPortNames.clear();

  io_iterator_t theSerialIterator;
  io_object_t theObject;
  char dialInDevice[1024];
  if(createSerialIterator(&theSerialIterator) == KERN_SUCCESS)
  {
    while(theObject = IOIteratorNext(theSerialIterator))
    {
      strcpy(dialInDevice, getRegistryString(theObject, kIODialinDeviceKey));
      myPortNames.push_back(dialInDevice);
    }
  }

  return myPortNames;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
kern_return_t SerialPortMACOSX::createSerialIterator(io_iterator_t* serialIterator)
{
  kern_return_t kernResult;
  mach_port_t masterPort;
  CFMutableDictionaryRef classesToMatch;
  if((kernResult = IOMasterPort(NULL, &masterPort)) != KERN_SUCCESS)
  {
    cerr << "IOMasterPort returned " << kernResult << endl;
    return kernResult;
  }
  if((classesToMatch = IOServiceMatching(kIOSerialBSDServiceValue)) == NULL)
  {
    cerr << "IOServiceMatching returned NULL\n";
    return kernResult;
  }
  CFDictionarySetValue(classesToMatch, CFSTR(kIOSerialBSDTypeKey), CFSTR(kIOSerialBSDRS232Type));
  kernResult = IOServiceGetMatchingServices(masterPort, classesToMatch, serialIterator);
  if(kernResult != KERN_SUCCESS)
  {
    cerr << "IOServiceGetMatchingServices returned " << kernResult << endl;
  }
  return kernResult;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
char* SerialPortMACOSX::getRegistryString(io_object_t sObj, char* propName)
{
  static char resultStr[256];
  CFTypeRef nameCFstring;
  resultStr[0] = 0;
  nameCFstring = IORegistryEntryCreateCFProperty(sObj,
      CFStringCreateWithCString(kCFAllocatorDefault, propName, kCFStringEncodingASCII),
      kCFAllocatorDefault, 0);
  if(nameCFstring)
  {
    CFStringGetCString((CFStringRef)nameCFstring, resultStr, sizeof(resultStr), kCFStringEncodingASCII);
    CFRelease(nameCFstring);
  }
  return resultStr;
}

#endif // BSPF_MAC_OSX
