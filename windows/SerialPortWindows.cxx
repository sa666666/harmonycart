//=========================================================================
//
//  H   H    A    RRRR   M   M   OOO   N   N  Y   Y
//  H   H   A A   R   R  MM MM  O   O  NN  N   Y Y
//  HHHHH  AAAAA  RRRR   M M M  O   O  N N N    Y   "Harmony Cart software"
//  H   H  A   A  R R    M   M  O   O  N  NN    Y
//  H   H  A   A  R  R   M   M   OOO   N   N    Y
//
// Copyright (c) 2009-2017 by Stephen Anthony <sa666666@gmail.com>
//
// See the file "License.txt" for information on usage and redistribution
// of this file, and for a DISCLAIMER OF ALL WARRANTIES.
//=========================================================================

#undef UNICODE
#include <windows.h>
#include <sstream>
#include <cstdio>

#include "SerialPortWindows.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SerialPortWindows::SerialPortWindows()
  : SerialPort(),
    myHandle(INVALID_HANDLE_VALUE)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SerialPortWindows::~SerialPortWindows()
{
  closePort();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SerialPortWindows::openPort(const string& device)
{
  if(!myHandle)
    closePort();

  DCB dcb;
  COMMTIMEOUTS commtimeouts;

  const string& portname = string("\\\\.\\") + device;
  myHandle = CreateFileA(portname.c_str(), GENERIC_READ|GENERIC_WRITE,
                         0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if(myHandle == INVALID_HANDLE_VALUE)
    return false;

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
void SerialPortWindows::closePort()
{
  if(myHandle != INVALID_HANDLE_VALUE)
  {
    CloseHandle(myHandle);
    myHandle = INVALID_HANDLE_VALUE;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SerialPortWindows::isOpen()
{
  return myHandle != INVALID_HANDLE_VALUE;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 SerialPortWindows::receiveBlock(void* answer, uInt32 max_size)
{
  DWORD result = 0;
  if(myHandle != INVALID_HANDLE_VALUE)
  {
    ReadFile(myHandle, answer, max_size, &result, NULL);
    if(result == 0)
      timeoutTick();
  }
  return (uInt32)result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt32 SerialPortWindows::sendBlock(const void* data, uInt32 size)
{
  DWORD result = 0;
  if(myHandle != INVALID_HANDLE_VALUE)
    WriteFile(myHandle, data, size, &result, NULL);

  return (uInt32)result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SerialPortWindows::setTimeout(uInt32 timeout_milliseconds)
{
  mySerialTimeoutCount = timeout_milliseconds;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SerialPortWindows::clearBuffers()
{
  PurgeComm(myHandle, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SerialPortWindows::controlModemLines(bool DTR, bool RTS)
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
void SerialPortWindows::sleepMillis(uInt32 milliseconds)
{
  Sleep(DWORD(milliseconds));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const StringList& SerialPortWindows::getPortNames()
{
  myPortNames.clear();

  for(int i = 1; i <= 256; ++i)
  {
    TCHAR strPort[32] = {0};
    sprintf(strPort, "COM%d", i);

    if(openPort(strPort))
    {
      uInt8 c;
      int n = receiveBlock(&c, 1);
      if(n >= 0)
        myPortNames.push_back(strPort);
    }
    closePort();
  }

  return myPortNames;
}
