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

#ifndef __SERIALPORTMANAGER_HXX
#define __SERIALPORTMANAGER_HXX

#include "bspf_harmony.hxx"
#include "Cart.hxx"

#if defined(BSPF_WIN32)
  #include "SerialPortWin32.hxx"
#elif defined(BSPF_MAC_OSX)
  #include "SerialPortMACOSX.hxx"
#elif defined(BSPF_UNIX)
  #include "SerialPortUNIX.hxx"
#else
  #error Unsupported platform!
#endif

class SerialPortManager
{
  public:
    SerialPortManager();
    ~SerialPortManager();

    void setDefaultPort(const string& port);
    void connectHarmonyCart(Cart& cart);
    bool harmonyCartAvailable();

    SerialPort& port();
    const string& portName();
    const string& versionID();

    bool openCartPort();
    void closeCartPort();

  private:
    bool detect(const string& device, Cart& cart);

  private:
  #if defined(BSPF_WIN32)
    SerialPortWin32 myPort;
  #elif defined(BSPF_MAC_OSX)
    SerialPortMACOSX myPort;
  #elif defined(BSPF_UNIX)
    SerialPortUNIX myPort;
  #endif

    bool myFoundHarmonyCart;
    string myPortName;
    string myVersionID;
};

#endif // __SERIALPORTMANAGER_HXX
