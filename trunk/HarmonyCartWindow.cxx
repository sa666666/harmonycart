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

#include <QFileDialog>
#include <QLabel>
#include <QPixmap>
#include <QString>
#include <QStatusBar>
#include <QTextEdit>
#include <QAction>
#include <QActionGroup>
#include <QButtonGroup>
#include <QRegExp>
#include <QSettings>
#include <QCloseEvent>
#include <QMessageBox>
#include <QTimer>
#include <QFile>

#include <iostream>
#include <sstream>
#include <sstream>
using namespace std;

#include "CartDetector.hxx"
#include "HarmonyCartWindow.hxx"
#include "FindHarmonyThread.hxx"
#include "ui_harmonycartwindow.h"
#include "Version.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HarmonyCartWindow::HarmonyCartWindow(QWidget* parent)
  : QMainWindow(parent),
    ui(new Ui::HarmonyCartWindow),
    myDownloadInProgress(false)
{
  // Create GUI
  ui->setupUi(this);

  // Create thread to find Harmony cart
  // We use a thread so the UI isn't blocked
  myFindHarmonyThread = new FindHarmonyThread(myManager, myCart);

  // Set up signal/slot connections
  setupConnections();

  // The text part of the status bar
  myStatus = new QLabel();
  myStatus->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
  statusBar()->addPermanentWidget(myStatus, 1000);

  // The LED part of the status bar
  myLED = new QLabel();
  statusBar()->addPermanentWidget(myLED);

  // Deactivate bankswitch and download until it makes sense to use them
  ui->romBSType->setDisabled(true);
  ui->downloadButton->setDisabled(true);  ui->actDownloadROM->setDisabled(true);

  // Initialize settings
  QCoreApplication::setOrganizationName("HarmonyCart");
  QCoreApplication::setApplicationName("Harmony Programming Tool");
  readSettings();

  // By default, start looking for ROMs in the users' home directory
  myLastDir.setPath(QDir::home().absolutePath());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HarmonyCartWindow::~HarmonyCartWindow()
{
  if(myFindHarmonyThread)
  {
    myFindHarmonyThread->quit();
    delete myFindHarmonyThread;
  }
  delete ui;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HarmonyCartWindow::setupConnections()
{
  // File menu
  connect(ui->actSelectROM, SIGNAL(triggered()), this, SLOT(slotOpenROM()));
  connect(ui->actDownloadROM, SIGNAL(triggered()), this, SLOT(slotDownloadROM()));
  connect(ui->actDownloadBIOS, SIGNAL(triggered()), this, SLOT(slotDownloadBIOS()));
  connect(ui->actQuit, SIGNAL(triggered()), this, SLOT(close()));

  // Device menu
  connect(ui->actConnectHarmonyCart, SIGNAL(triggered()), this, SLOT(slotConnectHarmonyCart()));

  // Options menu
  QActionGroup* group = new QActionGroup(this);
  group->setExclusive(true);
  group->addAction(ui->actRetry0);
  group->addAction(ui->actRetry1);
  group->addAction(ui->actRetry2);
  group->addAction(ui->actRetry3);
  connect(group, SIGNAL(triggered(QAction*)), this, SLOT(slotRetry(QAction*)));
  connect(ui->actShowLogAfterDownload, SIGNAL(toggled(bool)), this, SLOT(slotShowLog(bool)));

  // Help menu
  connect(ui->actAbout, SIGNAL(triggered()), this, SLOT(slotAbout()));
  connect(ui->actAboutQt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

  ///////////////////////////////////////////////////////////
  // 'BIOS Update' tab
  ///////////////////////////////////////////////////////////
  // Buttons
  connect(ui->updateBIOSButton, SIGNAL(clicked()), this, SLOT(slotDownloadBIOS()));
  connect(ui->copyHBIOSButton, SIGNAL(clicked()), this, SLOT(slotCopyHBIOS()));
  connect(ui->openEEPROMButton, SIGNAL(clicked()), this, SLOT(slotSelectEEPROM()));
  connect(ui->openHBIOSButton, SIGNAL(clicked()), this, SLOT(slotSelectHBIOS()));
  connect(ui->openSDMountButton, SIGNAL(clicked()), this, SLOT(slotSelectSDMount()));

  ///////////////////////////////////////////////////////////
  // 'Development' tab
  ///////////////////////////////////////////////////////////
  // Buttons
  connect(ui->openRomButton, SIGNAL(clicked()), this, SLOT(slotOpenROM()));
  connect(ui->downloadButton, SIGNAL(clicked()), this, SLOT(slotDownloadROM()));
  connect(ui->openARMPathButton, SIGNAL(clicked()), this, SLOT(slotSelectARMPath()));

  // Quick-select buttons
  myQPGroup = new QButtonGroup(this);
  myQPGroup->setExclusive(false);
  myQPGroup->addButton(ui->qp1Button, 1);   ui->qp1Button->installEventFilter(this);
  myQPGroup->addButton(ui->qp2Button, 2);   ui->qp2Button->installEventFilter(this);
  myQPGroup->addButton(ui->qp3Button, 3);   ui->qp3Button->installEventFilter(this);
  myQPGroup->addButton(ui->qp4Button, 4);   ui->qp4Button->installEventFilter(this);
  myQPGroup->addButton(ui->qp5Button, 5);   ui->qp5Button->installEventFilter(this);
  myQPGroup->addButton(ui->qp6Button, 6);   ui->qp6Button->installEventFilter(this);
  myQPGroup->addButton(ui->qp7Button, 7);   ui->qp7Button->installEventFilter(this);
  myQPGroup->addButton(ui->qp8Button, 8);   ui->qp8Button->installEventFilter(this);
  myQPGroup->addButton(ui->qp9Button, 9);   ui->qp9Button->installEventFilter(this);
  myQPGroup->addButton(ui->qp10Button, 10); ui->qp10Button->installEventFilter(this);
  myQPGroup->addButton(ui->qp11Button, 11); ui->qp11Button->installEventFilter(this);
  myQPGroup->addButton(ui->qp12Button, 12); ui->qp12Button->installEventFilter(this);
  myQPGroup->addButton(ui->qp13Button, 13); ui->qp13Button->installEventFilter(this);
  myQPGroup->addButton(ui->qp14Button, 14); ui->qp14Button->installEventFilter(this);
  myQPGroup->addButton(ui->qp15Button, 15); ui->qp15Button->installEventFilter(this);
  myQPGroup->addButton(ui->qp16Button, 16); ui->qp16Button->installEventFilter(this);
  connect(myQPGroup, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(slotQPButtonClicked(QAbstractButton*)));

  // Other
  connect(ui->romBSType, SIGNAL(activated(int)), this, SLOT(slotBSTypeChanged(int)));

  connect(myFindHarmonyThread, SIGNAL(finished()), this, SLOT(slotUpdateFindHarmonyStatus()));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HarmonyCartWindow::readSettings()
{
  // Load settings
  QSettings s;

  s.beginGroup("MainWindow");
    myManager.setDefaultPort(s.value("harmonyport", "").toString().toStdString());
    int retrycount = s.value("retrycount", 0).toInt();
    if(retrycount == 0)       ui->actRetry0->setChecked(true);
    else if(retrycount == 1)  ui->actRetry1->setChecked(true);
    else if(retrycount == 2)  ui->actRetry2->setChecked(true);
    else if(retrycount == 3)  ui->actRetry3->setChecked(true);
    ui->actShowLogAfterDownload->setChecked(s.value("showlog", false).toBool());
    ui->actAutoDownFileSelect->setChecked(s.value("autodownload", false).toBool());
    ui->actAutoVerifyDownload->setChecked(s.value("autoverify", false).toBool());
  s.endGroup();

  slotShowLog(ui->actShowLogAfterDownload->isChecked());
  myCart.setRetry(retrycount);

  s.beginGroup("QPButtons");
    assignToQPButton(ui->qp1Button, 1, s.value("button1", "").toString(), false);
    assignToQPButton(ui->qp2Button, 2, s.value("button2", "").toString(), false);
    assignToQPButton(ui->qp3Button, 3, s.value("button3", "").toString(), false);
    assignToQPButton(ui->qp4Button, 4, s.value("button4", "").toString(), false);
    assignToQPButton(ui->qp5Button, 5, s.value("button5", "").toString(), false);
    assignToQPButton(ui->qp6Button, 6, s.value("button6", "").toString(), false);
    assignToQPButton(ui->qp7Button, 7, s.value("button7", "").toString(), false);
    assignToQPButton(ui->qp8Button, 8, s.value("button8", "").toString(), false);
    assignToQPButton(ui->qp9Button, 9, s.value("button9", "").toString(), false);
    assignToQPButton(ui->qp10Button, 10, s.value("button10", "").toString(), false);
    assignToQPButton(ui->qp11Button, 11, s.value("button11", "").toString(), false);
    assignToQPButton(ui->qp12Button, 12, s.value("button12", "").toString(), false);
    assignToQPButton(ui->qp13Button, 13, s.value("button13", "").toString(), false);
    assignToQPButton(ui->qp14Button, 14, s.value("button14", "").toString(), false);
    assignToQPButton(ui->qp15Button, 15, s.value("button15", "").toString(), false);
    assignToQPButton(ui->qp16Button, 16, s.value("button16", "").toString(), false);
  s.endGroup();

  s.beginGroup("Paths");
    ui->eepromFileEdit->setText(s.value("eepromfile", "").toString());
    ui->hbiosFileEdit->setText(s.value("hbiosfile", "").toString());
    ui->sdcardFileEdit->setText(s.value("sdmountdir", "").toString());
    ui->armpathFileEdit->setText(s.value("armdir", "").toString());
  s.endGroup();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HarmonyCartWindow::closeEvent(QCloseEvent* event)
{
  // Make sure we don't prematurely kill any running threads
  if(myFindHarmonyThread->isRunning())
  {
    event->ignore();
    return;
  }

  // Save settings
  QSettings s;

  s.beginGroup("MainWindow");
    s.setValue("harmonyport", QString(myManager.portName().c_str()));
    int retrycount = 0;
    if(ui->actRetry0->isChecked())       retrycount = 0;
    else if(ui->actRetry1->isChecked())  retrycount = 1;
    else if(ui->actRetry2->isChecked())  retrycount = 2;
    else if(ui->actRetry3->isChecked())  retrycount = 3;
    s.setValue("retrycount", retrycount);
    s.setValue("autodownload", ui->actAutoDownFileSelect->isChecked());
    s.setValue("autoverify", ui->actAutoVerifyDownload->isChecked());
    s.setValue("showlog", ui->actShowLogAfterDownload->isChecked());
  s.endGroup();

  s.beginGroup("Paths");
    s.setValue("eepromfile", ui->eepromFileEdit->text());
    s.setValue("hbiosfile", ui->hbiosFileEdit->text());
    s.setValue("sdmountdir", ui->sdcardFileEdit->text());
    s.setValue("armdir", ui->armpathFileEdit->text());
  s.endGroup();

  event->accept();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool HarmonyCartWindow::eventFilter(QObject* object, QEvent* event)
{
  // This is necessary because braindead QPushButtons don't return right-click events
  if(event->type() == QEvent::ContextMenu)
  {
    int id = 0;
    if(object == ui->qp1Button)       id = 1;
    else if(object == ui->qp2Button)  id = 2;
    else if(object == ui->qp3Button)  id = 3;
    else if(object == ui->qp4Button)  id = 4;
    else if(object == ui->qp5Button)  id = 5;
    else if(object == ui->qp6Button)  id = 6;
    else if(object == ui->qp7Button)  id = 7;
    else if(object == ui->qp8Button)  id = 8;
    else if(object == ui->qp9Button)  id = 9;
    else if(object == ui->qp10Button) id = 10;
    else if(object == ui->qp11Button) id = 11;
    else if(object == ui->qp12Button) id = 12;
    else if(object == ui->qp13Button) id = 13;
    else if(object == ui->qp14Button) id = 14;
    else if(object == ui->qp15Button) id = 15;
    else if(object == ui->qp16Button) id = 16;
    else return false;

    assignToQPButton(static_cast<QPushButton*>(object), id);
    return  true;
  }
  else
  {
    // pass the event on to the parent class
    return QMainWindow::eventFilter(object, event);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HarmonyCartWindow::slotConnectHarmonyCart()
{
  myStatus->setText("Searching for Harmony Cart.");
  myLED->setPixmap(QPixmap(":icons/pics/ledoff.png"));

  // Start a thread to do this potentially time-consuming operation
  myFindHarmonyThread->start();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HarmonyCartWindow::slotUpdateFindHarmonyStatus()
{
  if(myManager.harmonyCartAvailable())
  {
    myHarmonyCartMessage = "Harmony \'";
    myHarmonyCartMessage.append(myManager.versionID().c_str());
    myHarmonyCartMessage.append("\' @ \'");
    myHarmonyCartMessage.append(myManager.portName().c_str());
    myHarmonyCartMessage.append("\'.");
    myLED->setPixmap(QPixmap(":icons/pics/ledon.png"));
  }
  else
  {
    myHarmonyCartMessage = "Harmony Cart not found.";
    myLED->setPixmap(QPixmap(":icons/pics/ledoff.png"));
  }
  myStatus->setText(myHarmonyCartMessage);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HarmonyCartWindow::slotDownloadBIOS()
{
  if(myDownloadInProgress)
    return;

  if(!myManager.harmonyCartAvailable())
  {
    myDownloadInProgress = false;
    statusMessage("Harmony Cart not found.");
    return;
  }

  // Switch to BIOS tab
  ui->tabWidget->setCurrentIndex(0);
  myDownloadInProgress = true;

  QString biosfile = ui->eepromFileEdit->text();
  if(biosfile == "" || !QFile::exists(biosfile))
  {
    myDownloadInProgress = false;
    QMessageBox::critical(this, "Missing file",
      "Couldn't find eeloader.bin file.\nMake sure you've selected it.");
    return;
  }

  if(myManager.openCartPort())
  {
    myLog.str("");
    string result = myCart.downloadBIOS(myManager.port(), biosfile.toStdString(),
                      ui->actAutoVerifyDownload->isChecked());
    statusMessage(QString(result.c_str()));

    myManager.closeCartPort();

    if(ui->actShowLogAfterDownload->isChecked())
      QMessageBox::information(this, "Download BIOS", QString(myLog.str().c_str()));
  }
  else
    statusMessage("Couldn't open serial port.");

  myDownloadInProgress = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HarmonyCartWindow::slotOpenROM()
{
  // Switch to Development tab
  ui->tabWidget->setCurrentIndex(1);

  QString location = ui->romFileEdit->text() != "" ?
    QFileInfo(ui->romFileEdit->text()).absolutePath() :
    myLastDir.absolutePath();
  QString file = QFileDialog::getOpenFileName(this,
    tr("Select ROM Image"), location, tr("Atari 2600 ROM Image (*.a26 *.bin *.rom)"));

  if(!file.isNull())
    loadROM(file);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HarmonyCartWindow::slotDownloadROM()
{
  if(myDownloadInProgress)
    return;

  if(!myManager.harmonyCartAvailable())
  {
    myDownloadInProgress = false;
    statusMessage("Harmony Cart not found.");
    return;
  }

  // Switch to Development tab
  ui->tabWidget->setCurrentIndex(1);
  myDownloadInProgress = true;

  QString armpath = ui->armpathFileEdit->text();
  QString romfile = ui->romFileEdit->text();
  if(armpath == "" || !QFile::exists(armpath))
  {
    myDownloadInProgress = false;
    QMessageBox::critical(this, "Missing folder",
      "Couldn't find the 'arm' folder.\nMake sure you've selected it.");
    return;
  }
  if(romfile == "" || !QFile::exists(romfile))
  {
    myDownloadInProgress = false;
    QMessageBox::critical(this, "Missing file",
      "Couldn't find selected ROM image.");
    return;
  }

  if(myManager.openCartPort())
  {
    QRegExp regex("([a-zA-Z0-9]*)");
    regex.indexIn(ui->romBSType->currentText());
    QString t = regex.cap();
    BSType type = Bankswitch::nameToType(regex.cap().toStdString());

    myLog.str("");
    string result = myCart.downloadROM(myManager.port(), armpath.toStdString(),
      romfile.toStdString(), type, ui->actAutoVerifyDownload->isChecked());
    statusMessage(QString(result.c_str()));

    myManager.closeCartPort();

    if(ui->actShowLogAfterDownload->isChecked())
      QMessageBox::information(this, "Download ROM", QString(myLog.str().c_str()));
  }
  else
    statusMessage("Couldn't open serial port.");

  myDownloadInProgress = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HarmonyCartWindow::slotCopyHBIOS()
{
  // Switch to BIOS tab
  ui->tabWidget->setCurrentIndex(0);

  QString hbiosfile = ui->hbiosFileEdit->text();
  QString sdmountdir = ui->sdcardFileEdit->text();
  if(hbiosfile == "" || !QFile::exists(hbiosfile))
  {
    QMessageBox::critical(this, "Missing file",
      "Couldn't find hbios.bin file.\nMake sure you've selected it.");
    return;
  }
  else if(sdmountdir == "" || !QFile::exists(sdmountdir))
  {
    QMessageBox::critical(this, "Missing directory",
      "Couldn't find the SD mount directory.\nMake sure your SD card is properly mounted.");
    return;
  }

  // Since Qt won't copy over a file that already exists, we have to delete first
  QFileInfo source(hbiosfile);
  QFileInfo dest(QDir(sdmountdir), "hbios.bin");

  if(QFile::exists(dest.absoluteFilePath()) && dest.isFile())
    QFile::remove(dest.absoluteFilePath());

  if(QFile::copy(source.absoluteFilePath(), dest.absoluteFilePath()))
    statusMessage("HBIOS file copied.");
  else
    statusMessage("HBIOS file NOT copied; check file permissions.");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HarmonyCartWindow::slotRetry(QAction* action)
{
  if(action == ui->actRetry0)       myCart.setRetry(0);
  else if(action == ui->actRetry1)  myCart.setRetry(1);
  else if(action == ui->actRetry2)  myCart.setRetry(2);
  else if(action == ui->actRetry3)  myCart.setRetry(3);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HarmonyCartWindow::slotAbout()
{
  ostringstream about;
  about << "<center>"
        << "<p><b>Harmony Programming Tool v" << HARMONY_VERSION << "</b></p>"
        << "<p>Copyright &copy; 2009 <a href=\"mailto:stephena@users.sf.net\">Stephen Anthony</a><br>"
        << "Check for updates at <a href=\"http://atariage.com\">http://atariage.com</a><p>"
        << "</center>"
        << "<p>This&nbsp;software&nbsp;is&nbsp;released&nbsp;under&nbsp;the&nbsp;GNU&nbsp;GPLv3,<br>"
        << "and&nbsp;includes&nbsp;code&nbsp;from&nbsp;the&nbsp;following&nbsp;projects:</p>"
        << "<p></p>"
        << "<p>"
        << "&nbsp;&nbsp;&nbsp;lpc21isp&nbsp;:&nbsp;Philips&nbsp;LPCxxxx&nbsp;programming&nbsp;code<br>"
        << "&nbsp;&nbsp;&nbsp;KrokCom&nbsp;:&nbsp;UI&nbsp;code,&nbsp;icons&nbsp;and&nbsp;other&nbsp;images<br>"
        << "&nbsp;&nbsp;&nbsp;Stella&nbsp;:&nbsp;bankswitch&nbsp;autodetection&nbsp;code<br>"
        << "</p>";

  QMessageBox mb;
  mb.setWindowTitle("Info about Harmony Programming Tool");
  mb.setIconPixmap(QPixmap(":icons/pics/cart.png"));
  mb.setWindowIcon(QPixmap(":icons/pics/appicon.png"));
  mb.setTextFormat(Qt::RichText);
  mb.setText(about.str().c_str());
  mb.exec();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HarmonyCartWindow::slotQPButtonClicked(QAbstractButton* b)
{
  // Get the full path from the settings
  QPushButton* button = static_cast<QPushButton*>(b);
  int id = myQPGroup->id(b);
  QString key = "button" + QString::number(id);
  QSettings s;
  s.beginGroup("QPButtons");
    QString file = s.value(key, "").toString();
  s.endGroup();

  QFileInfo info(file);
  if(info.exists())
    loadROM(file);
  else if(file != "")
  {
    if(QMessageBox::Yes == QMessageBox::warning(this, "Warning",
      "This ROM no longer exists.  Do you wish to remove it\nfrom the QuickPick list?",
      QMessageBox::Yes, QMessageBox::No))
    {
      button->setText("");
      s.beginGroup("QPButtons");
        s.remove(key);
      s.endGroup();
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HarmonyCartWindow::slotBSTypeChanged(int /*id*/)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HarmonyCartWindow::slotShowLog(bool checked)
{
  if(checked)
    myCart.setLogger(&myLog);
  else
    myCart.setLogger(&cout);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HarmonyCartWindow::slotSelectEEPROM()
{
  QString location = ui->eepromFileEdit->text() != "" ?
    QFileInfo(ui->eepromFileEdit->text()).absolutePath() :
    myLastDir.absolutePath();
  QString file = QFileDialog::getOpenFileName(this,
    tr("Select EEPROM Loader Image"), location, tr("BIOS Image (*.bin)"));

  if(!file.isNull())
    ui->eepromFileEdit->setText(file);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HarmonyCartWindow::slotSelectHBIOS()
{
  QString location = ui->hbiosFileEdit->text() != "" ?
    QFileInfo(ui->hbiosFileEdit->text()).absolutePath() :
    myLastDir.absolutePath();
  QString file = QFileDialog::getOpenFileName(this,
    tr("Select HBIOS Image"), location, tr("BIOS Image (*.bin)"));

  if(!file.isNull())
    ui->hbiosFileEdit->setText(file);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HarmonyCartWindow::slotSelectSDMount()
{
  QString location = ui->sdcardFileEdit->text() != "" ?
    QFileInfo(ui->sdcardFileEdit->text()).absolutePath() :
    myLastDir.absolutePath();
  QString dir = QFileDialog::getExistingDirectory(this,
    tr("Select SD Card Location"), location, QFileDialog::ShowDirsOnly);

  if(!dir.isNull())
    ui->sdcardFileEdit->setText(dir);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HarmonyCartWindow::slotSelectARMPath()
{
  QString location = ui->armpathFileEdit->text() != "" ?
    QFileInfo(ui->armpathFileEdit->text()).absolutePath() :
    myLastDir.absolutePath();
  QString dir = QFileDialog::getExistingDirectory(this,
    tr("Select 'ARM' Directory"), location, QFileDialog::ShowDirsOnly);

  if(!dir.isNull())
    ui->armpathFileEdit->setText(dir);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HarmonyCartWindow::loadROM(const QString& filename)
{
  if(filename == "")
    return;

  ui->romBSType->setDisabled(true);
  ui->downloadButton->setDisabled(true);  ui->actDownloadROM->setDisabled(true);

  // Set name and file size
  QFile file(filename);
  ui->romFileEdit->setText(filename);
  ui->romSizeLabel->setText(QString::number(file.size()) + " bytes");

  // Set autodetected bankswitch type
  BSType type = CartDetector::autodetectType(filename.toStdString());
  QString bstype = Bankswitch::typeToName(type).c_str();
  int match = ui->romBSType->findText(bstype, Qt::MatchStartsWith);
  ui->romBSType->setCurrentIndex(match < ui->romBSType->count() && match >= 0 ? match : 0);
  ui->romBSType->setDisabled(false);
  ui->downloadButton->setDisabled(false);  ui->actDownloadROM->setDisabled(false);

  // See if we should automatically download
  if(ui->actAutoDownFileSelect->isChecked())
    slotDownloadROM();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HarmonyCartWindow::assignToQPButton(QPushButton* button, int id)
{
  QString file = QFileDialog::getOpenFileName(this,
    tr("Select ROM Image"), myLastDir.absolutePath(), tr("Atari 2600 ROM Image (*.a26 *.bin *.rom)"));

  if(!file.isNull())
  {
    assignToQPButton(button, id, file, true);
    // Remember this location for the next time a file is selected
    myLastDir.setPath(file);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HarmonyCartWindow::assignToQPButton(QPushButton* button, int id,
                                         const QString& file, bool save)
{
  QFileInfo info(file);

  // Only add files that exist
  QString filename = info.fileName();
  if(filename == "")
    return;

  // Otherwise, add the file itself to the button, and the full path to settings
  button->setText(filename);

  if(save)
  {
    QString key = "button" + QString::number(id);
    QSettings s;
    s.beginGroup("QPButtons");
      s.setValue(key, info.canonicalFilePath());
    s.endGroup();
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HarmonyCartWindow::statusMessage(const QString& msg)
{
  // Show the message for a short time, then reset to the default message
  myStatus->setText(msg);
  QTimer::singleShot(4000, this, SLOT(slotShowDefaultMsg()));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HarmonyCartWindow::slotShowDefaultMsg()
{
  myStatus->setText(myHarmonyCartMessage);
}
