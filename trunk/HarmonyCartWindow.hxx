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

#ifndef __HARMONYCART_WINDOW_HXX
#define __HARMONYCART_WINDOW_HXX

#include <QMainWindow>
#include <QAction>
#include <QLabel>
#include <QProgressBar>
#include <QThread>
#include <QDir>
#include <sstream>

#include "Cart.hxx"
#include "SerialPortManager.hxx"
#include "FindHarmonyThread.hxx"
#include "ui_harmonycartwindow.h"

#include "OSystem.hxx"
#if defined(BSPF_UNIX)
  #include "OSystemUNIX.hxx"
#elif defined(BSPF_WIN32)
  #include "OSystemWin32.hxx"
#elif defined(BSPF_MAC_OSX)
  #include "OSystemMACOSX.hxx"
#endif

namespace Ui
{
  class HarmonyCartWindow;
}

class HarmonyCartWindow : public QMainWindow
{
Q_OBJECT
  public:
    HarmonyCartWindow(QWidget* parent = 0);
    ~HarmonyCartWindow();

  public:
    SerialPortManager& portManager() { return myManager; }
    void connectHarmonyCart() { slotConnectHarmonyCart(); }
    string armPath() { return ui->armpathFileEdit->text().toStdString(); }
    bool verifyDownload() { return ui->actAutoVerifyDownload->isChecked(); }

  protected:
    void closeEvent(QCloseEvent* event);
    bool eventFilter(QObject* object, QEvent* event);

  private:
    void setupConnections();
    void readSettings();
    void loadROM(const QString& file);
    void assignToQPButton(QPushButton* button, int id);
    void assignToQPButton(QPushButton* button, int id, const QString& file, bool save);
    QString getOpenROMName(const QString& path);

    void statusMessage(const QString& msg);

  private slots:
    void slotConnectHarmonyCart();
    void slotUpdateFindHarmonyStatus();

    void slotDownloadBIOS();
    void slotDownloadROM();
    void slotCopyHBIOS();

    void slotOpenROM();
    void slotRetry(QAction* action);
    void slotAbout();
    void slotQPButtonClicked(QAbstractButton* b);
    void slotBSTypeChanged(int id);
    void slotShowLog(bool checked);
    void slotShowDefaultMsg();

    void slotSelectEEPROM();
    void slotSelectHBIOS();
    void slotSelectSDMount();
    void slotSelectARMPath();

  private:
    Ui::HarmonyCartWindow* ui;
    FindHarmonyThread* myFindHarmonyThread;
    QButtonGroup* myQPGroup;

    Cart myCart;
    ostringstream myLog;
    SerialPortManager myManager;

    QLabel* myStatus;
    QLabel* myLED;
    QDir myLastDir;

    QString myHarmonyCartMessage;
    bool myDownloadInProgress;

  #if defined(BSPF_UNIX)
    OSystemUNIX myOSystem;
  #elif defined(BSPF_WIN32)
    OSystemWin32 myOSystem;
  #elif defined(BSPF_MAC_OSX)
    OSystemMACOSX myOSystem;
  #else
    OSystem myOSystem;
  #endif
};

#endif
