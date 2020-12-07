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

#undef UNICODE
#include <windows.h>
#include <sstream>
#include <cstdio>

#include "SerialPortWINDOWS.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SerialPortWINDOWS::SerialPortWINDOWS()
  : SerialPort()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SerialPortWINDOWS::~SerialPortWINDOWS()
{
  closePort();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SerialPortWINDOWS::openPort(const string& device)
{
  if(myHandle == INVALID_HANDLE_VALUE)
    closePort();

  DCB dcb;
  COMMTIMEOUTS commtimeouts;

  /* Torsten Lang 2013-05-06 Switch to higher timer resolution (we want to use
     1ms timeouts in the serial device driver!) */
  timeBeginPeriod(1UL);

  const string& portname = string("\\\\.\\") + device;
  myHandle = CreateFile(portname.c_str(), GENERIC_READ|GENERIC_WRITE,
                        0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if(myHandle == INVALID_HANDLE_VALUE)
    return false;

  GetCommState(myHandle, &dcb);
  dcb.BaudRate    = myBaud;
  dcb.ByteSize    = 8;
  dcb.StopBits    = ONESTOPBIT;
  dcb.Parity      = NOPARITY;
  dcb.fDtrControl = DTR_CONTROL_DISABLE;
  dcb.fOutX       = TRUE; // TL TODO - according to LPC manual! FALSE;
  dcb.fInX        = TRUE; // TL TODO - according to LPC manual! FALSE;
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
  commtimeouts.ReadTotalTimeoutMultiplier  = MAXDWORD;
  commtimeouts.ReadTotalTimeoutConstant    =    1;
  commtimeouts.WriteTotalTimeoutMultiplier =    0;
  commtimeouts.WriteTotalTimeoutConstant   =    0;
  SetCommTimeouts(myHandle, &commtimeouts);

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SerialPortWINDOWS::closePort()
{
  if(myHandle != INVALID_HANDLE_VALUE)
  {
    CloseHandle(myHandle);
    myHandle = INVALID_HANDLE_VALUE;

    /* Torsten Lang 2013-05-06 Switch back timer resolution */
    timeEndPeriod(1UL);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SerialPortWINDOWS::isOpen()
{
  return myHandle != INVALID_HANDLE_VALUE;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t SerialPortWINDOWS::receiveBlock(void* answer, size_t max_size)
{
  DWORD result = 0;
  if(myHandle != INVALID_HANDLE_VALUE)
  {
    ReadFile(myHandle, answer, static_cast<DWORD>(max_size), &result, NULL);
    if(result == 0)
      timeoutTick();
  }
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
size_t SerialPortWINDOWS::sendBlock(const void* data, size_t size)
{
  DWORD result = 0;
  if(myHandle != INVALID_HANDLE_VALUE)
    WriteFile(myHandle, data, static_cast<DWORD>(size), &result, NULL);

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SerialPortWINDOWS::setTimeout(uInt32 timeout_milliseconds)
{
  mySerialTimeoutCount = timeGetTime() + timeout_milliseconds;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SerialPortWINDOWS::timeoutCheck()
{
  return static_cast<Int32>(mySerialTimeoutCount - timeGetTime()) < 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SerialPortWINDOWS::clearBuffers()
{
  PurgeComm(myHandle, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SerialPortWINDOWS::controlModemLines(bool DTR, bool RTS)
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
void SerialPortWINDOWS::controlXonXoff(bool XonXoff)
{
  DCB dcb;
  GetCommState(myHandle, &dcb);

  if(XonXoff)
  {
    dcb.fOutX = TRUE;
    dcb.fInX  = TRUE;
  }
  else
  {
    dcb.fOutX = FALSE;
    dcb.fInX  = FALSE;
  }

  SetCommState(myHandle, &dcb);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SerialPortWINDOWS::sleepMillis(uInt32 milliseconds)
{
  Sleep(DWORD(milliseconds));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const StringList& SerialPortWINDOWS::getPortNames()
{
  myPortNames.clear();

  HKEY hKey = NULL;
  LSTATUS result = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
      L"HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_READ, &hKey);
  if(result == ERROR_SUCCESS)
  {
    TCHAR deviceName[2048], friendlyName[32];
    DWORD numValues = 0;

    result = RegQueryInfoKey(hKey, NULL, NULL, NULL, NULL, NULL, NULL,
        &numValues, NULL, NULL, NULL, NULL);
    if(result == ERROR_SUCCESS)
    {
      DWORD type = 0;
      DWORD deviceNameLen = 2047;
      DWORD friendlyNameLen = 31;

      for(DWORD i = 0; i < numValues; ++i)
      {
        result = RegEnumValue(hKey, i, deviceName, &deviceNameLen,
            NULL, &type, (LPBYTE)friendlyName, &friendlyNameLen);

        if(result == ERROR_SUCCESS && type == REG_SZ)
          myPortNames.emplace_back(friendlyName);
      }
    }
  }
  RegCloseKey(hKey);

  return myPortNames;
}
