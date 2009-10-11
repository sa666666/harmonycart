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

#ifndef __HARMONYCART_WINDOW_HXX
#define __HARMONYCART_WINDOW_HXX

#include <QMainWindow>
#include <QAction>
#include <QLabel>
#include <QProgressBar>
#include <QThread>
#include <sstream>

#include "Cart.hxx"
#include "SerialPortManager.hxx"
#include "FindHarmonyThread.hxx"
#include "ui_harmonycartwindow.h"

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

  private slots:
    void slotConnectHarmonyCart();
    void slotUpdateFindHarmonyStatus();

    void slotDownloadBIOS();
    void slotDownloadROM();
    void slotCopyHBIOS();

    void slotOpenROM();
    void slotRetry(QAction* action);
    void slotAbout();
    void slotQPButtonClicked(int id);
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

    Cart myCart;
    ostringstream myLog;
    SerialPortManager myManager;

    QLabel* myStatus;
    QLabel* myLED;

    QString myHarmonyCartMessage;
    bool myDownloadInProgress;
};

#endif
