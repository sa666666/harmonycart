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

#include <QApplication>
#include <cstring>

#include "bspf.hxx"
#include "Cart.hxx"
#include "SerialPort.hxx"
#include "SerialPortManager.hxx"
#include "HarmonyCartWindow.hxx"

void runCommandlineApp(HarmonyCartWindow& win, int ac, char* av[])
{
  string bstype = "", tvformat = "", romfile = "";

  // Parse commandline args
  for(int i = 1; i < ac; ++i)
  {
    if(strstr(av[i], "-bs=") == av[i])
      bstype = av[i]+4;
//    else if(...)         // add more options here
    else
      romfile = av[i];
  }

  SerialPortManager& manager = win.portManager();
  if(manager.harmonyCartAvailable())
  {
    cout << "Harmony \'" << manager.versionID() << "\' @ \'" << manager.portName() << "\'" << endl;
  }
  else
  {
    cout << "HarmonyCart not detected" << endl;
    return;
  }

  // Create a new cart for writing
  Cart cart;

  // Create a new single-load cart
  cart.create(romfile, bstype);

  // Write to serial port
  if(cart.isValid())
  {
    try
    {
      uInt16 numSectors = cart.initSectors();
      for(uInt16 sector = 0; sector < numSectors; ++sector)
      {
        uInt16 s = cart.writeNextSector(manager.port());
        cout << "Sector " << setw(4) << s << " successfully sent : "
             << setw(3) << (100*(sector+1)/numSectors) << "% complete" << endl;
      }
    }
    catch(const char* msg)
    {
    }
  }
  else
    cout << "ERROR: Invalid cartridge, not written" << endl;
}


int main(int ac, char* av[])
{
  if(ac == 2 && !strcmp(av[1], "-help"))
  {
    cerr << "TODO: help" << endl;
    return 0;
  }

  // The application and window needs to be created even if we're using
  // commandline mode, since the settings are controlled by a QSettings
  // object which needs a Qt context.
  QApplication app(ac, av);
  HarmonyCartWindow win;

  if(ac == 1)  // Launch GUI
  {
    win.show();
    return app.exec();
  }
  else  // Assume we're working from the commandline
  {
    runCommandlineApp(win, ac, av);
  }

  return 0;
}
