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

#ifndef FINDHARMONYTHREAD_HXX
#define FINDHARMONYTHREAD_HXX

#include <QThread>

#include "SerialPortManager.hxx"
#include "Cart.hxx"

/**
  This class is a wrapper thread around the 'find Harmony' functionality.
  Searching through all available serial ports and querying them can be a
  time-consuming operation, during which the UI would be unresponsive.
  Using a thread eliminates this UI lockup.

  @author  Stephen Anthony
*/
class FindHarmonyThread: public QThread
{
Q_OBJECT
  public:
    FindHarmonyThread(SerialPortManager& manager, Cart& cart)
      : QThread(),
        myManager{manager},
        myCart{cart}
    { }
    virtual ~FindHarmonyThread() = default;

  protected:
    void run() { myManager.connectHarmonyCart(myCart); }

  private:
    SerialPortManager& myManager;
    Cart& myCart;

    // Following constructors and assignment operators not supported
    FindHarmonyThread() = delete;
    FindHarmonyThread(const FindHarmonyThread&) = delete;
    FindHarmonyThread(FindHarmonyThread&&) = delete;
    FindHarmonyThread& operator=(const FindHarmonyThread&) = delete;
    FindHarmonyThread& operator=(FindHarmonyThread&&) = delete;
};

#endif // FINDHARMONYTHREAD_HXX
