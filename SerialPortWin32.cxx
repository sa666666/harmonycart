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

//#define BSPF_WIN32
//#define COMPILE_FOR_WINDOWS

#if defined(BSPF_WIN32)

#undef UNICODE
#include <windows.h>

#include <sstream>
#include "SerialPortWin32.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SerialPortWin32::SerialPortWin32()
  : SerialPort(),
    myHandle(NULL)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SerialPortWin32::~SerialPortWin32()
{
  closePort();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SerialPortWin32::openPort(const string& device)
{
  if(!myHandle)
    closePort();

  DCB dcb;
  COMMTIMEOUTS commtimeouts;

  myHandle = CreateFileA(device.c_str(), GENERIC_READ|GENERIC_WRITE,
                         0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if(myHandle == INVALID_HANDLE_VALUE)
  {
    myHandle = NULL;
    return false;
  }

  GetCommState(myHandle, &dcb);
  dcb.BaudRate    = myBaud;
  dcb.ByteSize    = 8;
  dcb.StopBits    = ONESTOPBIT;
  dcb.Parity      = NOPARITY;
  dcb.fDtrControl = DTR_CONTROL_DISABLE;
  dcb.fOutX       = 0;
  dcb.fInX        = 0;
  dcb.fNull       = 0;
  dcb.fRtsControl = RTS_CONTROL_DISABLE;

  // added by Herbert Demmel - iF CTS line has the wrong state, we would never send anything!
  dcb.fOutxCtsFlow = 0;
  dcb.fOutxDsrFlow = 0;

  if(SetCommState(myHandle, &dcb) == 0)
  {
    cerr << "ERROR: Can't set baudrate " << myBaud << " " << GetLastError() << endl;
    return false;
  }

  /*
   *  Peter Hayward 02 July 2008
   *
   *  The following call is only needed if the WaitCommEvent
   *  or possibly the GetCommMask functions are used.  They are
   *  *not* in this implimentation.  However, under Windows XP SP2
   *  on my laptop the use of this call causes XP to freeze (crash) while
   *  this program is running, e.g. in section 5/6/7 ... of a largish
   *  download.  Removing this *unnecessary* call fixed the problem.
   *  At the same time I've added a call to SetupComm to request
   *  (not necessarity honoured) the operating system to provide
   *  large I/O buffers for high speed I/O without handshaking.
   *
   *   SetCommMask(myHandle, EV_RXCHAR | EV_TXEMPTY);
   */
  SetupComm(myHandle, 32000, 32000);
  SetCommMask(myHandle, EV_RXCHAR | EV_TXEMPTY);

  commtimeouts.ReadIntervalTimeout         = MAXDWORD;
  commtimeouts.ReadTotalTimeoutMultiplier  =    0;
  commtimeouts.ReadTotalTimeoutConstant    =    1;
  commtimeouts.WriteTotalTimeoutMultiplier =    0;
  commtimeouts.WriteTotalTimeoutConstant   =    0;
  SetCommTimeouts(myHandle, &commtimeouts);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SerialPortWin32::closePort()
{
  if(myHandle)
  {
    CloseHandle(myHandle);
    myHandle = NULL;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SerialPortWin32::isOpen()
{
  return myHandle != NULL;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int SerialPortWin32::receiveBlock(void* answer, uInt32 max_size, uInt32* real_size)
{
  int result = -1;
  if(myHandle)
  {
    ReadFile(myHandle, answer, max_size, &result, NULL);
    if(result == 0)
      timeoutTick(IspEnvironment);
  }
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int SerialPortWin32::sendBlock(const void* data, uInt32 size)
{
  unsigned long realsize;
  size_t m;
  unsigned long rxsize;
  char* pch;
  char* rxpch;

  if(myHandle)
  {
    WriteFile(myHandle, data, size, &realsize, NULL);
    return realsize;
  }
  else
    return -1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SerialPortWin32::setTimeout(uInt32 timeout_milliseconds)
{
  mySerialTimeoutCount = timeout_milliseconds;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SerialPortWin32::clearBuffers()
{
  PurgeComm(myHandle, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SerialPortWin32::controlModemLines(bool DTR, bool RTS)
{
  // Handle whether to swap the control lines
  if(myControlLinesSwapped)
  {
    bool tempRTS;
    tempRTS = RTS;
    RTS = DTR;
    DTR = tempRTS;
  }

  if (DTR) EscapeCommFunction(myHandle, SETDTR);
  else     EscapeCommFunction(myHandle, CLRDTR);
  if (RTS) EscapeCommFunction(myHandle, SETRTS);
  else     EscapeCommFunction(myHandle, CLRRTS);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const StringList& SerialPortWin32::getPortNames()
{
  myPortNames.clear();

  // For now, we just append all 256 possible COM ports
  // In the future, we should only include the ones that actually exist
  // TODO - figure out how to determine if a port exists

  for(int i = 1; i <= 256; ++i)
  {
    ostringstream port;
    port << "COM" << i;
    myPortNames.push_back(port.str());
  }
  return myPortNames;
}

#endif // BSPF_WIN32
