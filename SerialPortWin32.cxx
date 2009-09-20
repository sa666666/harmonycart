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

#if defined(BSPF_WIN32)

#undef UNICODE
#include <windows.h>

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
  closePort();  // paranoia: make sure port is in consistent state

  if(!myHandle)
  {
    myHandle = CreateFile(device.c_str(), GENERIC_READ|GENERIC_WRITE, 0,
                          NULL, OPEN_EXISTING, 0, NULL);

    if(myHandle)
    {
      DCB dcb;

      FillMemory(&dcb, sizeof(dcb), 0);
      dcb.DCBlength = sizeof(dcb);
      if(!BuildCommDCB("115200,n,8,1", &dcb))
        return false;

      memset(&dcb, 0, sizeof(DCB));
      dcb.BaudRate = CBR_19200;
      dcb.ByteSize = 8;
      dcb.Parity = NOPARITY;
      dcb.StopBits = ONESTOPBIT;
      SetCommState(myHandle, &dcb);
    }
    else 
      return false;
  }
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
int SerialPortWin32::readBytes(uInt8* data, uInt32 size)
{
return 0;
//  return myHandle ? read(myHandle, data, size) : -1;
// TODO - implement this
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int SerialPortWin32::writeBytes(const uInt8* data, uInt32 size)
{
//  return myHandle ? write(myHandle, data, size) : -1;
// TODO - implement this
  if(myHandle)
  {
    DWORD written;
    return WriteFile(myHandle, data, 1, &written, 0) == TRUE;
  }
  return false;

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
uInt8 SerialPortWin32::waitForAck(uInt32 wait)
{
return 0;
// TODO - implement this
/*
  uInt8 result = 0;
  for(int pass = 0; pass < 100; ++pass)
  {
    if(readBytes(&result, 1) == 1)
      break;
    usleep(wait);
  }
  return result;
*/
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
