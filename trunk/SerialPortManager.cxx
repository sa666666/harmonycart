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

#include "Cart.hxx"
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
void SerialPortManager::connectHarmonyCart(Cart& cart)
{
  myFoundHarmonyCart = false;

  // First try the port that was successful the last time
  if(myPortName != "" && connect(myPortName, cart))
  {
    // myPortName already contains the correct name
  }
  else  // Search through all ports
  {
    const StringList& ports = myPort.getPortNames();
    for(uInt32 i = 0; i < ports.size(); ++i)
    {
      if(connect(ports[i], cart))
      {
        break;
      }
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SerialPortManager::connect(const string& device, Cart& cart)
{
  myPort.closePort();
  myFoundHarmonyCart = false;

  if(myPort.openPort(device))
  {
    string version = cart.autodetectHarmony(myPort);
    if(strncmp(version.c_str(), "ERROR:", 6) != 0)
    {
      myFoundHarmonyCart = true;
      myPortName = device;
      myVersionID = version;
    }
  }
  myPort.closePort();
  return myFoundHarmonyCart;
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
