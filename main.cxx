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
//
// $Id$
//=========================================================================

#include <QApplication>
#include <QFile>
#include "ui_harmonycartwindow.h"

#include "bspf_harmony.hxx"
#include "BSType.hxx"
#include "Cart.hxx"
#include "SerialPort.hxx"
#include "SerialPortManager.hxx"
#include "HarmonyCartWindow.hxx"
#include "Version.hxx"

void usage()
{
  cout << "Harmony Programming Tool version " << HARMONY_VERSION << endl
       << endl
       << "Usage: harmonycart [options ...] datafile" << endl
       << "       Run without any options or datafile to use the graphical frontend" << endl
       << "       Consult the manual for more in-depth information" << endl
       << endl
       << "Valid options are:" << endl
       << endl
       << "  -bios       Treat the specified datafile as an EEPROM loader BIOS image" << endl
       << "              Otherwise, the datafile is treated as a ROM image instead" << endl
       << "  -bs=[type]  Specify the bankswitching scheme for a ROM image" << endl
       << "              (default is 'auto')" << endl
       << "  -help       Displays the message you're now reading" << endl
       << endl
       << "This software is Copyright (c) 2009-2017 Stephen Anthony, and is released" << endl
       << "under the GNU GPL version 3." << endl
       << endl;
}

void runCommandlineApp(HarmonyCartWindow& win, int ac, char* av[])
{
  string datafile = "";
  BSType bstype = BS_AUTO;
  bool biosupdate = false;

  // Parse commandline args
  for(int i = 1; i < ac; ++i)
  {
    if(BSPF_startsWithIgnoreCase(av[i], "-bs="))
      bstype = Bankswitch::nameToType(av[i]+4);
    else if(BSPF_equalsIgnoreCase(av[i], "-bios"))
      biosupdate = true;
    else if(BSPF_equalsIgnoreCase(av[i], "-help"))
    {
      usage();
      return;
    }
    else if(BSPF_startsWithIgnoreCase(av[i], "-"))
    {
      // Unknown argument
      cout << "Unknown argument \'" << av[i] << "\'" << endl << endl;
      usage();
      return;
    }
//    else if(...)         // add more options here
    else
      datafile = av[i];
  }

  Cart cart;
  SerialPortManager& manager = win.portManager();

  manager.connectHarmonyCart(cart);
  if(manager.harmonyCartAvailable())
  {
    cout << "Harmony \'" << manager.versionID().c_str() << "\' @ \'" << manager.portName().c_str() << "\'" << endl;
  }
  else
  {
    cout << "Harmony Cart not detected" << endl;
    return;
  }

  // Are we updating the BIOS or a single-load ROM?
  if(biosupdate)
  {
    cout << "Downloading BIOS file..." << endl;
    if(datafile == "" || !QFile::exists(QString(datafile.c_str())))
    {
      cout << "Couldn't find BIOS file \'" << datafile.c_str() << "\'" << endl;
      return;
    }

    if(manager.openCartPort())
    {
      // Download the BIOS, but don't show a graphical progress indicator
      cart.downloadBIOS(manager.port(), datafile, win.verifyDownload(), false);
      manager.closeCartPort();
    }
    else
      cout << "Couldn't open Harmony Cart" << endl;
  }
  else  // Single-load ROM image
  {
    cout << "Downloading single-load ROM file..." << endl;
    if(datafile == "" || !QFile::exists(QString(datafile.c_str())))
    {
      cout << "Couldn't find ROM file \'" << datafile.c_str() << "\'" << endl;
      return;
    }

    if(manager.openCartPort())
    {
      // Download the ROM, but don't show a graphical progress indicator
      cart.downloadROM(manager.port(), win.armPath(), datafile,
                       bstype, win.verifyDownload(), false);
      manager.closeCartPort();
    }
    else
      cout << "Couldn't open Harmony Cart" << endl;
  }
}


int main(int ac, char* av[])
{
  // The application and window needs to be created even if we're using
  // commandline mode, since the settings are controlled by a QSettings
  // object which needs a Qt context.
  QApplication app(ac, av);
  HarmonyCartWindow win;

  if(ac == 1)  // Launch GUI
  {
    // Only start a 'connect' thread if we're in UI mode
    win.show();
    win.connectHarmonyCart();
    return app.exec();
  }
  else  // Assume we're working from the commandline
  {
    runCommandlineApp(win, ac, av);
  }

  return 0;
}
