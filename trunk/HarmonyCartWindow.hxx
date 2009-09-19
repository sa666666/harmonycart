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

#include "Cart.hxx"
#include "SerialPortManager.hxx"
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

  protected:
    void closeEvent(QCloseEvent* event);
    bool eventFilter(QObject* object, QEvent* event);

  private:
    void setupConnections();
    void readSettings();
    void loadROM(const QString& file);
    void assignToQPButton(QPushButton* button, int id);
    void assignToQPButton(QPushButton* button, int id, const QString& file, bool save);

    static void downloadInterrupt();

  private slots:
    void slotConnectHarmonyCart();

    void slotUpdateBIOS();

    void slotOpenROM();
    void slotDownloadROM();
    void slotVerifyROM();
    void slotRetry(QAction* action);
    void slotSetBSType(const QString& text);
    void slotAbout();
    void slotQPButtonClicked(int id);
    void slotShowDefaultMsg();

  private:
    Ui::HarmonyCartWindow* ui;

    Cart myCart;
    SerialPortManager myManager;
    BSType myDetectedBSType;

    QLabel* myStatus;
    QLabel* myLED;
    QProgressBar* myProgress;

    QString myHarmonyCartMessage;
};

#endif
