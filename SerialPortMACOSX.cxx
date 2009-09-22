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
  myHandle = open(device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
  if(myHandle < 0)
    return false;

  // clear input & output buffers, then switch to "blocking mode"
  tcflush(myHandle, TCOFLUSH);
  tcflush(myHandle, TCIFLUSH);
  fcntl(myHandle, F_SETFL, fcntl(myHandle, F_GETFL) & ~O_NONBLOCK);

  tcgetattr(myHandle, &myOldtio); // save current port settings

  bzero(&myNewtio, sizeof(myNewtio));
  myNewtio.c_cflag = CS8 | CLOCAL | CREAD;

  #define NEWTERMIOS_SETBAUDRATE(bps) myNewtio.c_ispeed = myNewtio.c_ospeed = bps;

  switch (myBaud)
  {
//    case 1152000: NEWTERMIOS_SETBAUDRATE(B1152000); break;
//    case  576000: NEWTERMIOS_SETBAUDRATE(B576000);  break;
    case  230400: NEWTERMIOS_SETBAUDRATE(B230400);  break;
    case  115200: NEWTERMIOS_SETBAUDRATE(B115200);  break;
    case   57600: NEWTERMIOS_SETBAUDRATE(B57600);   break;
    case   38400: NEWTERMIOS_SETBAUDRATE(B38400);   break;
    case   19200: NEWTERMIOS_SETBAUDRATE(B19200);   break;
    case    9600: NEWTERMIOS_SETBAUDRATE(B9600);    break;
    default:
    {
      cerr << "ERROR: unknown baudrate " << myBaud << endl;
      return false;
    }
  }

  myNewtio.c_iflag = IGNPAR | IGNBRK | IXON | IXOFF;
  myNewtio.c_oflag = 0;

  // set input mode (non-canonical, no echo,...)
  myNewtio.c_lflag = 0;

  cfmakeraw(&myNewtio);
  myNewtio.c_cc[VTIME] = 1;   /* inter-character timer used */
  myNewtio.c_cc[VMIN]  = 0;   /* blocking read until 0 chars received */

  tcflush(myHandle, TCIFLUSH);
  if(tcsetattr(myHandle, TCSANOW, &myNewtio))
  {
    cerr << "Could not change serial port behaviour (wrong baudrate?)\n";
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SerialPortMACOSX::closePort()
{
  if(myHandle)
  {
    tcflush(myHandle, TCOFLUSH);
    tcflush(myHandle, TCIFLUSH);
    tcsetattr(myHandle, TCSANOW, &myOldtio);

    close(myHandle);
    myHandle = 0;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SerialPortMACOSX::isOpen()
{
  return myHandle > 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int SerialPortMACOSX::ReceiveComPortBlock(void* answer, uInt32 max_size, uInt32* real_size)
{
  int result = -1;
  if(myHandle)
  {
    result = read(myHandle, answer, max_size);
    if(result == 0)
      SerialTimeoutTick();
  }
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int SerialPortMACOSX::SendComPortBlock(const void* data, uInt32 size)
{
  return myHandle ? write(myHandle, data, size) : -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SerialPortMACOSX::SerialTimeoutSet(uInt32 timeout_milliseconds)
{
  mySerialTimeoutCount = timeout_milliseconds / 100;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SerialPortMACOSX::ClearSerialPortBuffers()
{
  // Variables to store the current tty state, create a new one
  struct termios origtty, tty;

  // Store the current tty settings
  tcgetattr(myHandle, &origtty);

  // Flush input and output buffers
  tty = origtty;
  tcsetattr(myHandle, TCSAFLUSH, &tty);

  // Reset the tty to its original settings
  tcsetattr(myHandle, TCSADRAIN, &origtty);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SerialPortMACOSX::ControlModemLines(bool DTR, bool RTS)
{
  // Handle whether to swap the control lines
  if(myControlLinesSwapped)
  {
    bool tempRTS;
    tempRTS = RTS;
    RTS = DTR;
    DTR = tempRTS;
  }

  int status;
  if(ioctl(myHandle, TIOCMGET, &status) != 0)
    cerr << "ioctl get failed, status = " << status << endl;

  if (DTR) status |=  TIOCM_DTR;
  else     status &= ~TIOCM_DTR;
  if (RTS) status |=  TIOCM_RTS;
  else     status &= ~TIOCM_RTS;

  if (ioctl(myHandle, TIOCMSET, &status) != 0)
    cerr << "ioctl set failed, status = " << status << endl;
  if (ioctl(myHandle, TIOCMGET, &status) != 0)
    cerr << "ioctl get failed, status = " << status << endl;
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
