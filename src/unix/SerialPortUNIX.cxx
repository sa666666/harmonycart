//=========================================================================
//
//  H   H    A    RRRR   M   M   OOO   N   N  Y   Y
//  H   H   A A   R   R  MM MM  O   O  NN  N   Y Y
//  HHHHH  AAAAA  RRRR   M M M  O   O  N N N    Y   "Harmony Cart software"
//  H   H  A   A  R R    M   M  O   O  N  NN    Y
//  H   H  A   A  R  R   M   M   OOO   N   N    Y
//
// Copyright (c) 2009-2020 by Stephen Anthony <sa666666@gmail.com>
//
// See the file "License.txt" for information on usage and redistribution
// of this file, and for a DISCLAIMER OF ALL WARRANTIES.
//=========================================================================

#if defined(__OpenBSD__)
  #include <errno.h>
#else
  #include <sys/errno.h>
#endif

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

#if defined(__FreeBSD__) || defined(__OpenBSD__)
  if(cfsetspeed(&myNewtio, (speed_t)myBaud)
  {
    cerr << "ERROR: baudrate " << myBaud << " not supported" << endl;
    return false;
  }
#else
  #define NEWTERMIOS_SETBAUDRATE(bps) myNewtio.c_cflag |= bps;

  switch (myBaud)
  {
#if defined (B1152000)
    case 1152000: NEWTERMIOS_SETBAUDRATE(B1152000); break;
#endif
#if defined (B576000)
    case  576000: NEWTERMIOS_SETBAUDRATE(B576000);  break;
#endif
#if defined (B230400)
    case  230400: NEWTERMIOS_SETBAUDRATE(B230400);  break;
#endif
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
#endif

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
void SerialPortUNIX::closePort()
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
bool SerialPortUNIX::isOpen()
{
  return myHandle > 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 SerialPortUNIX::receiveBlock(void* answer, uInt32 max_size)
{
  uInt32 result = 0;
  if(myHandle)
  {
    result = read(myHandle, answer, max_size);
    if(result == 0)
      timeoutTick();
  }
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 SerialPortUNIX::sendBlock(const void* data, uInt32 size)
{
  return myHandle ? write(myHandle, data, size) : 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SerialPortUNIX::setTimeout(uInt32 timeout_milliseconds)
{
  mySerialTimeoutCount = timeout_milliseconds / 100;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SerialPortUNIX::clearBuffers()
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
void SerialPortUNIX::controlModemLines(bool DTR, bool RTS)
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
void SerialPortUNIX::sleepMillis(uInt32 milliseconds)
{
  usleep(milliseconds*1000); // convert to microseconds
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
          int n = receiveBlock(&c, 1);
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
