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

#include "lpc21isp.h"
#include "SerialPortManager.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SerialPortManager::SerialPortManager()
  : myFoundHarmonyCart(false),
    myPortName(""),
    myVersionID("")
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SerialPortManager::~SerialPortManager()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SerialPortManager::setDefaultPort(const string& port)
{
  myPortName = port;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SerialPortManager::connectHarmonyCart()
{
  myFoundHarmonyCart = false;

  // First try the port that was successful the last time
  if(myPortName != "" && connect(myPortName))
  {
    // myPortName already contains the correct name
  }
  else  // Search through all ports
  {
    const StringList& ports = myPort.getPortNames();
    for(uInt32 i = 0; i < ports.size(); ++i)
    {
      if(connect(ports[i]))
      {
        break;
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SerialPortManager::connect(const string& device)
{
  const char* version = lpc_AutoDetect(device.c_str());
  if(strncmp(version, "ERROR:", 6) != 0)
  {
    myFoundHarmonyCart = true;
    myPortName = device;
    myVersionID = version;
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SerialPortManager::harmonyCartAvailable()
{
  return myFoundHarmonyCart;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SerialPort& SerialPortManager::port()
{
  return myPort;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& SerialPortManager::portName()
{
  return myPortName;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const string& SerialPortManager::versionID()
{
  return myVersionID;
}
