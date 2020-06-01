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

#include "AboutDialog.hxx"
#include "CartDetectorWrapper.hxx"
#include "HarmonyCartWindow.hxx"
#include "FindHarmonyThread.hxx"
#include "ui_harmonycartwindow.h"
#include "Version.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HarmonyCartWindow::HarmonyCartWindow(QWidget* parent)
  : QMainWindow(parent),
    ui(new Ui::HarmonyCartWindow)
{
  // Create GUI
  ui->setupUi(this);

  // Fix BIOS and HBIOS buttons; make sure they're the same size
  int w = std::max(ui->updateBIOSButton->width(), ui->copyHBIOSButton->width());
  int h = std::max(ui->updateBIOSButton->height(), ui->copyHBIOSButton->height());
  ui->updateBIOSButton->setFixedSize(w, h);
  ui->copyHBIOSButton->setFixedSize(w, h);

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
  QCoreApplication::setApplicationName("HarmonyCart");
#ifdef BSPF_MACOS
  QCoreApplication::setOrganizationName("atariage");
#else
  QCoreApplication::setOrganizationName("atariage.com");
#endif
  readSettings();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HarmonyCartWindow::~HarmonyCartWindow()
{
  if(myFindHarmonyThread)
  {
    myFindHarmonyThread->quit();
    delete myFindHarmonyThread;  myFindHarmonyThread = nullptr;
  }
  delete ui;  ui = nullptr;
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

  connect(ui->actShowLogAfterDownload, &QAction::toggled, this,
      [=](bool checked){ showLog(checked); });
  connect(ui->actF4CompressionNoBank0, &QAction::toggled, this,
      [=](bool checked){ myCart.skipF4CompressionOnBank0(checked); });

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

  connect(ui->openARMPathButton, &QLRPushButton::leftClicked,  [=](){ slotSelectARMPath(); });
  connect(ui->openARMPathButton, &QLRPushButton::rightClicked, [=]() {
      ui->armpathFileEdit->setText(myOSystem.defaultARMPath());
  });

  // Quick-select buttons
  std::array<QLRPushButton*, 16> qpButtons = {
    ui->qp1Button,  ui->qp2Button,  ui->qp3Button,  ui->qp4Button,
    ui->qp5Button,  ui->qp6Button,  ui->qp7Button,  ui->qp8Button,
    ui->qp9Button,  ui->qp10Button, ui->qp11Button, ui->qp12Button,
    ui->qp13Button, ui->qp14Button, ui->qp15Button, ui->qp16Button
  };

  myQPGroup = new QButtonGroup(this);
  myQPGroup->setExclusive(false);
  for(size_t i = 0; i < qpButtons.size(); ++i)
  {
    myQPGroup->addButton(qpButtons[i]);
    connect(qpButtons[i], &QLRPushButton::leftClicked,  [=](QLRPushButton* b){ qpButtonClicked(b, i+1);  });
    connect(qpButtons[i], &QLRPushButton::rightClicked, [=](QLRPushButton* b){ assignToQPButton(b, i+1); });
  }

  // Other
//  connect(ui->romBSType, SIGNAL(activated(int)), this, SLOT(slotBSTypeChanged(int)));

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
    switch(retrycount)
    {
      case 1:  ui->actRetry1->setChecked(true);  break;
      case 2:  ui->actRetry2->setChecked(true);  break;
      case 3:  ui->actRetry3->setChecked(true);  break;
      case 0:
      default: ui->actRetry0->setChecked(true);  break;
    }
    ui->actShowLogAfterDownload->setChecked(s.value("showlog", false).toBool());
    ui->actF4CompressionNoBank0->setChecked(s.value("f4compressbank0skip", false).toBool());
    ui->actAutoDownFileSelect->setChecked(s.value("autodownload", false).toBool());
    ui->actAutoVerifyDownload->setChecked(s.value("autoverify", false).toBool());
    int activetab = s.value("activetab", 0).toInt();
    if(activetab < 0 || activetab > 1)  activetab = 0;
    ui->tabWidget->setCurrentIndex(activetab);
  s.endGroup();

  showLog(ui->actShowLogAfterDownload->isChecked());
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
    ui->sdcardFileEdit->setText(s.value("sdmountpath", "").toString());
    QString path = s.value("armpath", "").toString();
    QDir dir(path);
    if(path.length() == 0 || !dir.exists())
      path = myOSystem.defaultARMPath();
    ui->armpathFileEdit->setText(path);

    // Last directory used
    // Do some sanity checking
    path = s.value("lastpath", "").toString();
    if(path.length() == 0)
      myLastDir.setPath(QDir::home().absolutePath());
    else
    {
      QDir dir(path);
      // Attempt to move up one level, to bypass a file saved in the path
      if(!dir.exists())
        dir.cdUp();
      if(dir.exists())
        myLastDir.setPath(dir.absolutePath());
      else
        myLastDir.setPath(QDir::home().absolutePath());
    }
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
    s.setValue("f4compressbank0skip", ui->actF4CompressionNoBank0->isChecked());
    s.setValue("activetab", ui->tabWidget->currentIndex());
  s.endGroup();

  s.beginGroup("Paths");
    s.setValue("eepromfile", ui->eepromFileEdit->text());
    s.setValue("hbiosfile", ui->hbiosFileEdit->text());
    s.setValue("sdmountpath", ui->sdcardFileEdit->text());
    s.setValue("armpath", ui->armpathFileEdit->text());
    s.setValue("lastpath", myLastDir.absolutePath());
  s.endGroup();

  event->accept();
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

  QString biosfile = ui->eepromFileEdit->text();
  if(biosfile == "" || !QFile::exists(biosfile))
  {
    myDownloadInProgress = false;
    QMessageBox::critical(this, "Missing file",
      "Couldn't find eeloader.bin file.\nMake sure you've selected it.");
    return;
  }

  myDownloadInProgress = true;
  ui->updateBIOSButton->setEnabled(!myDownloadInProgress);

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
  ui->updateBIOSButton->setEnabled(!myDownloadInProgress);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HarmonyCartWindow::slotOpenROM()
{
  // Switch to Development tab
  ui->tabWidget->setCurrentIndex(1);

  QString location = ui->romFileEdit->text() != "" ?
    QFileInfo(ui->romFileEdit->text()).absolutePath() :
    myLastDir.absolutePath();
  myLastDir.setPath(location);
  QString file = getOpenROMName(location);
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

  myDownloadInProgress = true;
  ui->downloadButton->setEnabled(!myDownloadInProgress);

  if(myManager.openCartPort())
  {
    QRegExp regex("([a-zA-Z0-9+]*)");
    regex.indexIn(ui->romBSType->currentText());
    QString t = regex.cap();
    Bankswitch::Type type = Bankswitch::nameToType(regex.cap().toStdString());

    myLog.str("");
    string result = myCart.downloadROM(myManager.port(), armpath.toStdString(),
      romfile.toStdString(), type, ui->actAutoVerifyDownload->isChecked());
    statusMessage(QString(result.c_str()));
    ui->downloadButton->setEnabled(true);

    myManager.closeCartPort();

    if(ui->actShowLogAfterDownload->isChecked())
      QMessageBox::information(this, "Download ROM", QString(myLog.str().c_str()));
  }
  else
    statusMessage("Couldn't open serial port.");

  myDownloadInProgress = false;
  ui->downloadButton->setEnabled(!myDownloadInProgress);
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
        << "<p>Copyright &copy; 2009-2020 <a href=\"mailto:sa666666@gmail.com\">Stephen Anthony</a><br>"
        << "Check for updates at <a href=\"http://harmony.atariage.com\">http://harmony.atariage.com</a><p>"
        << "</center>"
        << "<p>This software is released under the GNU GPLv3, and includes items from the following projects:</p>"
        << "<p><ul>"
        << "<li><a href=\"http://sourceforge.net/projects/lpc21isp\">lpc21isp</a>: Philips&nbsp;LPCxxxx programming code</li>"
        << "<li><a href=\"http://krokcom.sf.net\">KrokCom</a>: UI code, icons and other images</li>"
        << "<li><a href=\"http://stella-emu.github.io\">Stella</a>: bankswitch autodetection code</li>"
        << "</ul></p>"

        << "<p>Version 1.4 (Jun. xx, 2020):</p>"
        << "<ul>"
        << "<li>Included NTSC HBIOS 1.06.</li>"
        << "<li>Fixed annoying bug where dialog box would appear when first starting "
        << "the app, and would have to be manually closed.</li>"
        << "<li>Codebase ported to (and now requires) Qt 5.7.</li>"
        << "</ul>"

        << "<p>Version 1.3 (Aug. 13, 2013):</p>"
        << "<ul>"
        << "<li>Fixed bug when saving DPC+ and Custom ROMs in single-cart mode; the "
        << "ROM data wasn't actually being written to the device.</li>"
        << "<li>Added support for 29KB DPC+ ROMs in single-cart mode (ie, ones "
        << "without any ARM code); these ROMs now have ARM code automatically "
        << "added before being downloaded.</li>"
        << "<li>Updated bankswitch autodetection code to latest from Stella 3.9.1.</li>"
        << "</ul>"

        << "<p>Version 1.2 (Dec. 20, 2012):</p>"
        << "<ul>"
        << "<li>Updated lpc21isp code to version 1.85 (supports latest LPCxxxx chips).</li>"
        << "<li>Updated HBIOS and ARM files to latest version (1.05).</li>"
        << "<li>Added support for Custom ROMs (such as 32KB 'DPC+') with the ARM code already embedded.</li>"
        << "<li>The bankswitch autodetection now also uses the ROM filename extensions as defined in the "
        << "Harmony manual; when present, these completely override the type of data in the ROM image.</li>"
        << "<li>Fixed crash when an 'F4' ROM couldn't be compressed; an error message is now shown. "
        << "Also improved compression function; at least one ROM that couldn't be compressed previously "
        << "now works fine.</li>"
        << "<li>The location for 'ARM' files is now automatically determined based "
        << "on where you've installed the application; right-clicking on the "
        << "directory selection button will also set this location.</li>"
        << "<li>The Windows release now includes a commandline-based version "
        << "which shows output on the commandline, and is meant to be run from "
        << "the commandline only.</li>"
        << "<li>Fixed bugs in user interface (cut off text, progress bar not always appearing, etc).</li>"
        << "<li>The previously selected tab (BIOS or Development) is now used when the app starts.</li>"
        << "<li>Updated bankswitch autodetection code to latest from Stella 3.7.4.</li>"
        << "</ul>"

        << "<p>Version 1.1 (Dec. 11, 2009):</p>"
        << "<ul>"
        << "<li>Added logging of download progress.  When activated, a dialog will "
        << "appear after the download has completed, outlining what operations "
        << "were performed.  This is useful for detecting errors in operation.</li>"
        << "<li>The Harmony cart autodetection is now done in a separate thread, so "
        << "the UI won't lock up while autodetection is running.  This fixes "
        << "problems where the app seems to be frozen during startup.</li>"
        << "<li>The various file request dialogs now remember the last location "
        << "selected, so you don't need to 'drill down' to the same location "
        << "multiple times.</li>"
        << "<li>When clicking on a ROM in the QuickPick list, the app now checks to "
        << "see if the file exists.  If it doesn't, a dialog asks whether to "
        << "remove it from the list.</li>"
        << "<li>Tooltips were added to all remaining UI items; this should make the "
        << "app much easier to use.</li>"
        << "<li>Increased the time that messages are shown in the status bar from two "
        << "to four seconds, so you have a better chance of reading the results.</li>"
        << "<li>Implemented 'Retry Count' menu setting, to configure how often a "
        << "write should be attempted before considering it a failure.</li>"
        << "<li>The download buttons are now disabled during an active download, to "
        << "indicate another download isn't possible at this time.</li>"
        << "<li>Fixed several typos in the list of bankswitch types; some ROMs were "
        << "being mislabeled.</li>"
        << "<li>Added support for OSX Snow Leopard.</li>"
        << "<li>Updated HBIOS and ARM files to latest version (1.03c).</li>"
        << "<li>Updated PAL50 version of eeloader.bin for better compatibility with A7800 PAL systems.</li>"
        << "</ul>"

        << "<p>Version 1.0 (Oct. 7, 2009):</p>"
        << "<ul>"
        << "<li>Initial release for Linux, Mac OSX and Windows."
        << "</ul>"
        ;

  AboutDialog aboutdlg(this, "Info about Harmony Programming Tool", about.str().c_str());
  aboutdlg.exec();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HarmonyCartWindow::qpButtonClicked(QAbstractButton* button, size_t id)
{
  // Get the full path from the settings
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
void HarmonyCartWindow::showLog(bool checked)
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
    myOSystem.defaultARMPath();
  QString file = QFileDialog::getOpenFileName(this,
    tr("Select EEPROM Loader Image"), location, tr("EEPROM Image (*loader*.bin);;All Files (*.*)"));

  if(!file.isNull())
    ui->eepromFileEdit->setText(file);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HarmonyCartWindow::slotSelectHBIOS()
{
  QString location = ui->hbiosFileEdit->text() != "" ?
    QFileInfo(ui->hbiosFileEdit->text()).absolutePath() :
    myOSystem.defaultARMPath();
  QString file = QFileDialog::getOpenFileName(this,
    tr("Select HBIOS Image"), location, tr("BIOS Image (*bios*.bin);;All Files (*.*)"));

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
  Bankswitch::Type type = CartDetectorHC::autodetectType(filename.toStdString());
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
void HarmonyCartWindow::assignToQPButton(QAbstractButton* button, size_t id)
{
  // Get the full path from the settings
  QString key = "button" + QString::number(id);
  QSettings s;
  s.beginGroup("QPButtons");
    QString path = s.value(key, "").toString();
  s.endGroup();

  // If the path exists to a file, use it
  // Otherwise, use the last selected path
  if(path.length() == 0)
    path = myLastDir.absolutePath();
  else
  {
    QDir dir(path);
    path = dir.filePath(path);
  }

  QString file = getOpenROMName(path);
  if(!file.isNull())
  {
    assignToQPButton(button, id, file, true);
    // Remember this location for the next time a file is selected
    myLastDir.setPath(file);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HarmonyCartWindow::assignToQPButton(QAbstractButton* button, size_t id,
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
QString HarmonyCartWindow::getOpenROMName(const QString& path)
{
  // What a whopper!
  static QString filter = "Atari 2600 ROM Image (*.a26 *.bin *.rom *.2K *.4K *.F4 *.F4S *.F6 *.F6S *.F8 *.F8S *.FA *.FE *.3F *.3E *.E0 *.E7 *.CV *.UA *.AR *.DPC *.084 *.CU);;All Files (*.*)";

  QString file = QFileDialog::getOpenFileName(this,
    tr("Select ROM Image"), path, tr(filter.toLatin1()), 0,
    QFileDialog::HideNameFilterDetails);

  return file;
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
