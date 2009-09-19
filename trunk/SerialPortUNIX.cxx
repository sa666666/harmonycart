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

#if defined(BSPF_UNIX)

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/termios.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <dirent.h>
#include <cstring>

#include "SerialPortUNIX.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SerialPortUNIX::SerialPortUNIX()
  : SerialPort(),
    myHandle(0)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SerialPortUNIX::~SerialPortUNIX()
{
  closePort();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SerialPortUNIX::openPort(const string& device)
{
  closePort();  // paranoia: make sure port is in consistent state

  myHandle = open(device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
  if(myHandle <= 0)
    return false;

  struct termios termios;
  memset(&termios, 0, sizeof(struct termios));

  termios.c_cflag = CREAD | CLOCAL;
  termios.c_cflag |= B115200;
  termios.c_cflag |= CS8;
  tcflush(myHandle, TCIFLUSH);
  tcsetattr(myHandle, TCSANOW, &termios);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SerialPortUNIX::closePort()
{
  if(myHandle)
  {
    close(myHandle);
    myHandle = 0;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SerialPortUNIX::isOpen()
{
  return myHandle > 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int SerialPortUNIX::readBytes(uInt8* data, uInt32 size)
{
  return myHandle ? read(myHandle, data, size) : -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int SerialPortUNIX::writeBytes(const uInt8* data, uInt32 size)
{
  return myHandle ? write(myHandle, data, size) : -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 SerialPortUNIX::waitForAck(uInt32 wait)
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
const StringList& SerialPortUNIX::getPortNames()
{
  myPortNames.clear();

  // First get all possible devices in the '/dev' directory
  DIR* dirp = opendir("/dev");
  if(dirp != NULL)
  {
    // Search for files matching common serial port device names
    struct dirent* dp;
    while ((dp = readdir(dirp)) != NULL)
    {
      const char* ptr = dp->d_name;

      if((strstr(ptr, "ttyS") == ptr) ||  // linux Serial Ports
         (strstr(ptr, "ttyUSB") == ptr))  // for USB frobs
      {
        string device = "/dev/";
        device += ptr;

        if(openPort(device))
        {
          uInt8 c;
          int n = readBytes(&c, 1);
          if(n >= 0)
            myPortNames.push_back(device);
        }
        closePort();
      }
    }
    closedir(dirp);
  }

  return myPortNames;
}

#endif // BSPF_UNIX
