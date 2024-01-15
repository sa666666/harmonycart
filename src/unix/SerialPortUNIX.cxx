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

#include "FSNode.hxx"
#include "SerialPortUNIX.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SerialPortUNIX::SerialPortUNIX()
  : SerialPort()
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
      cerr << "ERROR: unknown baudrate " << myBaud << '\n';
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
size_t SerialPortUNIX::receiveBlock(void* answer, size_t max_size)
{
  size_t result = 0;
  if(myHandle)
  {
    result = read(myHandle, answer, max_size);
    if(result == 0)
      timeoutTick();
  }
  return result;
#if 0
// FIXME: from lpc21isp 1.93 (not sure if this will be used yet)
// This replaces the read call above
    {
        fd_set
            readSet;
        struct timeval
            timeVal;

        FD_ZERO(&readSet);                             // clear the set
        FD_SET(IspEnvironment->fdCom,&readSet);        // add this descriptor to the set
        timeVal.tv_sec=0;                              // set up the timeout waiting for one to come ready (500ms)
        timeVal.tv_usec=500*1000;
        if(select(FD_SETSIZE,&readSet,NULL,NULL,&timeVal)==1)    // wait up to 500 ms or until our data is ready
        {
            *real_size=read(IspEnvironment->fdCom, answer, max_size);
        }
        else
        {
            // timed out, show no characters received and timer expired
            *real_size=0;
            IspEnvironment->serial_timeout_count=0;
        }
    }
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t SerialPortUNIX::sendBlock(const void* data, size_t size)
{
  return myHandle ? write(myHandle, data, size) : 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SerialPortUNIX::setTimeout(uInt32 timeout_milliseconds)
{
  mySerialTimeoutCount = timeout_milliseconds / 100;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SerialPortUNIX::timeoutCheck()
{
  return mySerialTimeoutCount == 0;
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
    cerr << "ioctl get failed, status = " << status << '\n';

  if (DTR) status |=  TIOCM_DTR;
  else     status &= ~TIOCM_DTR;
  if (RTS) status |=  TIOCM_RTS;
  else     status &= ~TIOCM_RTS;

  if (ioctl(myHandle, TIOCMSET, &status) != 0)
    cerr << "ioctl set failed, status = " << status << '\n';
  if (ioctl(myHandle, TIOCMGET, &status) != 0)
    cerr << "ioctl get failed, status = " << status << '\n';
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SerialPortUNIX::controlXonXoff(bool XonXoff)
{
  tcgetattr(myHandle, &myNewtio);

  if(XonXoff)
  {
    myNewtio.c_iflag |= IXON;
    myNewtio.c_iflag |= IXOFF;
  }
  else
  {
    myNewtio.c_iflag &= ~IXON;
    myNewtio.c_iflag &= ~IXOFF;
  }

  tcsetattr(myHandle, TCSANOW, &myNewtio);
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

  // Check if port is valid; for now that means if it can be opened
  auto isPortValid = [](const string& port) {
    int handle = open(port.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if(handle > 0)  close(handle);
    return handle > 0;
  };

  // Get all possible devices in the '/dev' directory
  FSNode::NameFilter filter = [](const FSNode& node) {
    return BSPF::startsWithIgnoreCase(node.getPath(), "/dev/ttyS") ||
           BSPF::startsWithIgnoreCase(node.getPath(), "/dev/ttyUSB");
  };
  FSList portList;
  portList.reserve(8);

  FSNode dev("/dev/");
  dev.getChildren(portList, FSNode::ListMode::All, filter, false);

  // Add only those that can be opened
  for(const auto& port: portList)
    if(isPortValid(port.getPath()))
      myPortNames.emplace_back(port.getPath());

  return myPortNames;
}
