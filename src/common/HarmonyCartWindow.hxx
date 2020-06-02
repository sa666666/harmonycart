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

#ifndef HARMONYCART_WINDOW_HXX
#define HARMONYCART_WINDOW_HXX

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
#elif defined(BSPF_WINDOWS)
  #include "OSystemWINDOWS.hxx"
#elif defined(BSPF_MACOS)
  #include "OSystemMACOS.hxx"
#endif

namespace Ui
{
  class HarmonyCartWindow;
}

class HarmonyCartWindow : public QMainWindow
{
Q_OBJECT
  public:
    HarmonyCartWindow(QWidget* parent = nullptr);
    virtual ~HarmonyCartWindow();

  public:
    SerialPortManager& portManager() { return myManager; }
    void connectHarmonyCart() { slotConnectHarmonyCart(); }
    string armPath() { return ui->armpathFileEdit->text().toStdString(); }
    bool verifyDownload() { return ui->actAutoVerifyDownload->isChecked(); }

  protected:
    void closeEvent(QCloseEvent* event);

  private:
    void setupConnections();
    void readSettings();
    void loadROM(const QString& file);
    void qpButtonClicked(QAbstractButton* button, size_t id);
    void assignToQPButton(QAbstractButton* button, size_t id);
    void assignToQPButton(QAbstractButton* button, size_t id, const QString& file, bool save);
    QString getOpenROMName(const QString& path);

    void showLog(bool checked);
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
    void slotShowDefaultMsg();

    void slotSelectEEPROM();
    void slotSelectHBIOS();
    void slotSelectSDMount();
    void slotSelectARMPath();

  private:
    Ui::HarmonyCartWindow* ui{nullptr};
    FindHarmonyThread* myFindHarmonyThread{nullptr};
    QButtonGroup* myQPGroup{nullptr};

    Cart myCart;
    ostringstream myLog;
    SerialPortManager myManager;

    QLabel* myStatus{nullptr};
    QLabel* myLED{nullptr};
    QDir myLastDir;

    QString myHarmonyCartMessage;
    bool myDownloadInProgress{false};

  #if defined(BSPF_UNIX)
    OSystemUNIX myOSystem;
  #elif defined(BSPF_WINDOWS)
    OSystemWindows myOSystem;
  #elif defined(BSPF_MACOS)
    OSystemMACOS myOSystem;
  #else
    OSystem myOSystem;
  #endif

  private:
    // Following constructors and assignment operators not supported
    HarmonyCartWindow(const HarmonyCartWindow&) = delete;
    HarmonyCartWindow(HarmonyCartWindow&&) = delete;
    HarmonyCartWindow& operator=(const HarmonyCartWindow&) = delete;
    HarmonyCartWindow& operator=(HarmonyCartWindow&&) = delete;
};

#endif
