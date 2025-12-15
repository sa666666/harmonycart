//=========================================================================
//
//  H   H    A    RRRR   M   M   OOO   N   N  Y   Y
//  H   H   A A   R   R  MM MM  O   O  NN  N   Y Y
//  HHHHH  AAAAA  RRRR   M M M  O   O  N N N    Y   "Harmony Cart software"
//  H   H  A   A  R R    M   M  O   O  N  NN    Y
//  H   H  A   A  R  R   M   M   OOO   N   N    Y
//
// Copyright (c) 2009-2026 by Stephen Anthony <sa666666@gmail.com>
//
// See the file "License.txt" for information on usage and redistribution
// of this file, and for a DISCLAIMER OF ALL WARRANTIES.
//=========================================================================

#include <QApplication>
#include <QFile>

#include "bspf.hxx"
#include "Bankswitch.hxx"
#include "Cart.hxx"
#include "SerialPortManager.hxx"
#include "HarmonyCartWindow.hxx"
#include "Version.hxx"

void usage()
{
  cout << "Harmony Programming Tool version " << HARMONY_VERSION << '\n'
       << '\n'
       << "Usage: harmonycart [options ...] datafile\n"
       << "       Run without any options or datafile to use the graphical frontend\n"
       << "       Consult the manual for more in-depth information\n"
       << '\n'
       << "Valid options are:\n"
       << '\n'
       << "  -bios       Treat the specified datafile as an EEPROM loader BIOS image\n"
       << "              Otherwise, the datafile is treated as a ROM image instead\n"
       << "  -bs=[type]  Specify the bankswitching scheme for a ROM image\n"
       << "              (default is 'auto')\n"
       << "  -help       Displays the message you're now reading\n"
       << '\n'
       << "This software is Copyright (c) 2009-2026 Stephen Anthony, and is released\n"
       << "under the GNU GPL version 3.\n"
       << '\n';
}

void runCommandlineApp(HarmonyCartWindow& win, int ac, char* av[])
{
  string datafile = "";
  Bankswitch::Type bstype = Bankswitch::Type::_AUTO;
  bool biosupdate = false;

  // Parse commandline args
  for(int i = 1; i < ac; ++i)
  {
    if(BSPF::startsWithIgnoreCase(av[i], "-bs="))
      bstype = Bankswitch::nameToType(av[i]+4);
    else if(BSPF::equalsIgnoreCase(av[i], "-bios"))
      biosupdate = true;
    else if(BSPF::equalsIgnoreCase(av[i], "-help"))
    {
      usage();
      return;
    }
    else if(BSPF::startsWithIgnoreCase(av[i], "-"))
    {
      // Unknown argument
      cout << "Unknown argument \'" << av[i] << "\'\n\n";
      usage();
      return;
    }
//    else if(...)         // add more options here
    else
      datafile = av[i];
  }

  Cart& cart = win.cart();
  cart.setLogger(&cout);
  SerialPortManager& manager = win.portManager();

  manager.connectHarmonyCart(cart);
  if(manager.harmonyCartAvailable())
  {
    cout << "Harmony \'" << manager.versionID() << "\' @ \'" << manager.portName() << "\'\n";
  }
  else
  {
    cout << "Harmony Cart not detected\n";
    return;
  }

  // Are we updating the BIOS or a single-load ROM?
  if(biosupdate)
  {
    cout << "Downloading BIOS file...\n";
    if(datafile == "" || !QFile::exists(QString(datafile.c_str())))
    {
      cout << "Couldn't find BIOS file \'" << datafile << "\'\n";
      return;
    }

    if(manager.openCartPort())
    {
      // Download the BIOS, but don't show a graphical progress indicator
      cart.downloadBIOS(manager.port(), datafile, win.verifyDownload(),
                        false, win.continueOnErrors());
      manager.closeCartPort();
    }
    else
      cout << "Couldn't open Harmony Cart\n";
  }
  else  // Single-load ROM image
  {
    cout << "Downloading single-load ROM file...\n";
    if(datafile == "" || !QFile::exists(QString(datafile.c_str())))
    {
      cout << "Couldn't find ROM file \'" << datafile << "\'\n";
      return;
    }

    if(manager.openCartPort())
    {
      // Download the ROM, but don't show a graphical progress indicator
      cart.downloadROM(manager.port(), win.armPath(), datafile,
                       bstype, win.verifyDownload(),
                       false, win.continueOnErrors());
      manager.closeCartPort();
    }
    else
      cout << "Couldn't open Harmony Cart\n";
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
