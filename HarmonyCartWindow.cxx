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
#include <QProgressDialog>
#include <QAction>
#include <QActionGroup>
#include <QButtonGroup>
#include <QRegExp>
#include <QSettings>
#include <QCloseEvent>
#include <QMessageBox>
#include <QTimer>

#include <iostream>
#include <sstream>
#include <sstream>
using namespace std;

#include "HarmonyCartWindow.hxx"
#include "ui_harmonycartwindow.h"

static uInt32 STEPS = 0;
static QProgressDialog* PROGRESS;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HarmonyCartWindow::HarmonyCartWindow(QWidget* parent)
  : QMainWindow(parent),
    ui(new Ui::HarmonyCartWindow),
    myDetectedBSType(BS_NONE)
{
  // Create GUI
  ui->setupUi(this);

  // Set up signal/slot connections
  setupConnections();

  // The text part of the status bar
  myStatus = new QLabel();
  myStatus->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
  statusBar()->addPermanentWidget(myStatus, 1000);

  // The LED part of the status bar
  myLED = new QLabel();
  statusBar()->addPermanentWidget(myLED);

  // Deactivate bankswitch, download and verify until it makes sense to use them
  ui->romBSType->setDisabled(true);
  ui->downloadButton->setDisabled(true);  ui->actDownloadROM->setDisabled(true);
  ui->verifyButton->setDisabled(true);  ui->actVerifyROM->setDisabled(true);

  // Initialize settings
  QCoreApplication::setOrganizationName("HarmonyCart");
  QCoreApplication::setApplicationName("Harmony Programming Tool");
  readSettings();

  // Find and connect to HarmonyCart (make sure ::readSettings() is called first)
  slotConnectHarmonyCart();

// disable this until we get the second tab activated
ui->actSelectROM->setDisabled(true);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
HarmonyCartWindow::~HarmonyCartWindow()
{
  delete ui;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HarmonyCartWindow::setupConnections()
{
  // File menu
  connect(ui->actSelectROM, SIGNAL(triggered()), this, SLOT(slotOpenROM()));
  connect(ui->actDownloadROM, SIGNAL(triggered()), this, SLOT(slotDownloadROM()));
  connect(ui->actVerifyROM, SIGNAL(triggered()), this, SLOT(slotVerifyROM()));
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

  // Help menu
  connect(ui->actAbout, SIGNAL(triggered()), this, SLOT(slotAbout()));
  connect(ui->actAboutQt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

  ///////////////////////////////////////////////////////////
  // 'BIOS Update' tab
  ///////////////////////////////////////////////////////////
  // Buttons
  connect(ui->updateBIOSButton, SIGNAL(clicked()), this, SLOT(slotUpdateBIOS()));

  ///////////////////////////////////////////////////////////
  // 'Development' tab
  ///////////////////////////////////////////////////////////
  // Buttons
  connect(ui->openRomButton, SIGNAL(clicked()), this, SLOT(slotOpenROM()));
  connect(ui->downloadButton, SIGNAL(clicked()), this, SLOT(slotDownloadROM()));
  connect(ui->verifyButton, SIGNAL(clicked()), this, SLOT(slotVerifyROM()));
  connect(ui->romBSType, SIGNAL(activated(const QString&)), this, SLOT(slotSetBSType(const QString&)));

  // Quick-select buttons
  QButtonGroup* qpGroup = new QButtonGroup(this);
  qpGroup->setExclusive(false);
  qpGroup->addButton(ui->qp1Button, 1);   ui->qp1Button->installEventFilter(this);
  qpGroup->addButton(ui->qp2Button, 2);   ui->qp2Button->installEventFilter(this);
  qpGroup->addButton(ui->qp3Button, 3);   ui->qp3Button->installEventFilter(this);
  qpGroup->addButton(ui->qp4Button, 4);   ui->qp4Button->installEventFilter(this);
  qpGroup->addButton(ui->qp5Button, 5);   ui->qp5Button->installEventFilter(this);
  qpGroup->addButton(ui->qp6Button, 6);   ui->qp6Button->installEventFilter(this);
  qpGroup->addButton(ui->qp7Button, 7);   ui->qp7Button->installEventFilter(this);
  qpGroup->addButton(ui->qp8Button, 8);   ui->qp8Button->installEventFilter(this);
  qpGroup->addButton(ui->qp9Button, 9);   ui->qp9Button->installEventFilter(this);
  qpGroup->addButton(ui->qp10Button, 10); ui->qp10Button->installEventFilter(this);
  qpGroup->addButton(ui->qp11Button, 11); ui->qp11Button->installEventFilter(this);
  qpGroup->addButton(ui->qp12Button, 12); ui->qp12Button->installEventFilter(this);
  qpGroup->addButton(ui->qp13Button, 13); ui->qp13Button->installEventFilter(this);
  qpGroup->addButton(ui->qp14Button, 14); ui->qp14Button->installEventFilter(this);
  qpGroup->addButton(ui->qp15Button, 15); ui->qp15Button->installEventFilter(this);
  qpGroup->addButton(ui->qp16Button, 16); ui->qp16Button->installEventFilter(this);
  connect(qpGroup, SIGNAL(buttonClicked(int)), this, SLOT(slotQPButtonClicked(int)));
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
    myCart.setRetry(retrycount);
    ui->actAutoDownFileSelect->setChecked(s.value("autodownload", false).toBool());
    ui->actAutoVerifyDownload->setChecked(s.value("autoverify", false).toBool());
  s.endGroup();

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


}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HarmonyCartWindow::closeEvent(QCloseEvent* event)
{
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
  myManager.connectHarmonyCart(myCart);
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
void HarmonyCartWindow::slotUpdateBIOS()
{
  if(!myManager.harmonyCartAvailable())
  {
    myStatus->setText("Harmony Cart not found.");
    return;
  }

  // Eventually, we'll be delegating this to the Cart class, which will
  // properly divide each piece of the file into managable 'chunks' that
  // can be written separately, and continuous progress shown
  // For now, we just deal with it all here
  QString biosfile = "arm/eeloader.bin";
  if(!QFile::exists(biosfile))
  {
    QMessageBox::critical(this, "Missing file",
      "Couldn't find eeloader.bin file.\nCheck the \'arm\' folder.");
    return;
  }
#if 0
  STEPS = biosfile.size() / 45;
  PROGRESS = new QProgressDialog("Updating BIOS...", QString(), 0, STEPS, this);
  PROGRESS->setWindowModality(Qt::WindowModal);
  PROGRESS->setMinimumDuration(0);

  // This will periodically trigger an interrupt to update the progress
  lpc_Download(myManager.portName().c_str(), biosfile.toStdString().c_str(), HarmonyCartWindow::downloadInterrupt);

  PROGRESS->setValue(STEPS);
  delete PROGRESS;
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HarmonyCartWindow::slotOpenROM()
{
  QString file = QFileDialog::getOpenFileName(this,
    tr("Select ROM Image"), "", tr("Atari 2600 ROM Image (*.bin *.a26)"));

  if(!file.isNull())
    loadROM(file);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HarmonyCartWindow::slotDownloadROM()
{
  ui->verifyButton->setDisabled(true);  ui->actVerifyROM->setDisabled(true);

  if(!myManager.harmonyCartAvailable())
  {
    myStatus->setText("Harmony Cart not found.");
    return;
  }
  else if(!myCart.isValid())
  {
    myStatus->setText("Invalid cartridge.");
    QTimer::singleShot(2000, this, SLOT(slotShowDefaultMsg()));
    return;
  }

  // Write to serial port
  uInt16 sector = 0, numSectors = myCart.initSectors();
  QProgressDialog progress("Downloading ROM...", QString(), 0, numSectors, this);
  progress.setWindowModality(Qt::WindowModal);
  progress.setMinimumDuration(0);
  try
  {
    for(sector = 0; sector < numSectors; ++sector)
    {
      myCart.writeNextSector(myManager.port());
      progress.setValue(sector);
    }
  }
  catch(const char* msg)
  {
    cout << msg << endl;
  }

  if(sector == numSectors)
  {
    progress.setValue(numSectors);
    myStatus->setText("Cartridge downloaded.");

    ui->verifyButton->setDisabled(false);  ui->actVerifyROM->setDisabled(false);

    // See if we should automatically verify the download
    if(ui->actAutoVerifyDownload->isChecked())
      slotVerifyROM();
  }
  else
    myStatus->setText("Download failure on sector " + QString::number(sector) + ".");

  QTimer::singleShot(2000, this, SLOT(slotShowDefaultMsg()));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HarmonyCartWindow::slotVerifyROM()
{
  if(!myManager.harmonyCartAvailable())
  {
    myStatus->setText("Harmony Cart not found.");
    return;
  }
  else if(!myCart.isValid())
  {
    myStatus->setText("Invalid cartridge.");
    QTimer::singleShot(2000, this, SLOT(slotShowDefaultMsg()));
    return;
  }

  // Verify data previously written to serial port
  uInt16 sector = 0, numSectors = myCart.initSectors();
  QProgressDialog progress("Verifying ROM...", QString(), 0, numSectors, this);
  progress.setWindowModality(Qt::WindowModal);
  progress.setMinimumDuration(0);
  try
  {
    for(sector = 0; sector < numSectors; ++sector)
    {
      myCart.verifyNextSector(myManager.port());
      progress.setValue(sector);
    }
  }
  catch(const char* msg)
  {
    cout << msg << endl;
  }

  if(sector == numSectors)
  {
    progress.setValue(numSectors);
    myStatus->setText("Verified download of " + QString::number(sector) + " sectors.");
  }
  else
    myStatus->setText("Verify failure on sector " + QString::number(sector) + ".");

  QTimer::singleShot(2000, this, SLOT(slotShowDefaultMsg()));
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
void HarmonyCartWindow::slotSetBSType(const QString& text)
{
  QRegExp regex("([a-zA-Z0-9]*)");
  regex.indexIn(text);
  QString t = regex.cap();
  BSType selectedType = Bankswitch::nameToType(regex.cap().toStdString());

  if(myCart.isValid())
  {
    if(myDetectedBSType != selectedType)
    {
cerr << "Sure you want to override type (yes/no/always)?\n";
    }
    else
      myCart.setBSType(myDetectedBSType);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HarmonyCartWindow::slotAbout()
{
  ostringstream about;
  about << "<center>"
        << "<p><b>Harmony Programming Tool v0.1</b></p>"
        << "<p>Copyright &copy; 2009 Stephen Anthony<br>"
        << "<a href=\"mailto:stephena@users.sf.net\">stephena@users.sf.net</a></p>"
//        << "<p><a href=\"http://krokcom.sf.net\">http://krokcom.sf.net</a><p>"
//        << "<p>Based on the original Windows version<br>"
//        << "Copyright &copy; 2002-2009 <a href=\"mailto:Armin.Vogl@gmx.net\">Armin Vogl</a>"
        << "<p>This software is released under the GNU GPLv3</p>"
        << "</center>";

  QMessageBox mb;
  mb.setWindowTitle("Info about Harmony Programming Tool");
  mb.setIconPixmap(QPixmap(":icons/pics/cart.png"));
  mb.setTextFormat(Qt::RichText);
  mb.setText(about.str().c_str());
  mb.exec();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HarmonyCartWindow::slotQPButtonClicked(int id)
{
  // Get the full path from the settings
  QString key = "button" + QString::number(id);
  QSettings s;
  s.beginGroup("QPButtons");
    QString file = s.value(key, "").toString();
  s.endGroup();

  loadROM(file);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HarmonyCartWindow::loadROM(const QString& file)
{
  if(file == "")
    return;

  ui->romBSType->setDisabled(true);
  ui->downloadButton->setDisabled(true);  ui->actDownloadROM->setDisabled(true);
  ui->verifyButton->setDisabled(true);  ui->actVerifyROM->setDisabled(true);

  // Create a cart from the given filename
  myCart.create(file.toStdString());

  if(myCart.isValid())
  {
    ui->romFileEdit->setText(file);
    ui->romSizeLabel->setText(QString::number(myCart.getSize()) + " bytes.");
    myDetectedBSType = myCart.getBSType();
    QString bstype = Bankswitch::typeToName(myDetectedBSType).c_str();
    int match = ui->romBSType->findText(bstype, Qt::MatchStartsWith);
    ui->romBSType->setCurrentIndex(match < ui->romBSType->count() ? match : 0);
    ui->romBSType->setDisabled(false);
    ui->downloadButton->setDisabled(false);  ui->actDownloadROM->setDisabled(false);

    // See if we should automatically download
    if(ui->actAutoDownFileSelect->isChecked())
      slotDownloadROM();
  }
  else
  {
    myStatus->setText("Invalid cartridge.");
    QTimer::singleShot(2000, this, SLOT(slotShowDefaultMsg()));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HarmonyCartWindow::assignToQPButton(QPushButton* button, int id)
{
  QString file = QFileDialog::getOpenFileName(this,
    tr("Select ROM Image"), "", tr("Atari 2600 ROM Image (*.bin *.a26)"));

  if(!file.isNull())
    assignToQPButton(button, id, file, true);
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
void HarmonyCartWindow::slotShowDefaultMsg()
{
  // TODO - check which tab is activated and customize message
  myStatus->setText(myHarmonyCartMessage);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void HarmonyCartWindow::downloadInterrupt()
{
//cerr << "interrupt received\n";
  PROGRESS->setValue(++STEPS);
}
