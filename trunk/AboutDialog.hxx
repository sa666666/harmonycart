//=========================================================================
//
//  H   H    A    RRRR   M   M   OOO   N   N  Y   Y
//  H   H   A A   R   R  MM MM  O   O  NN  N   Y Y
//  HHHHH  AAAAA  RRRR   M M M  O   O  N N N    Y   "Harmony Cart software"
//  H   H  A   A  R R    M   M  O   O  N  NN    Y
//  H   H  A   A  R  R   M   M   OOO   N   N    Y
//
// Copyright (c) 2009-2015 by Stephen Anthony <stephena@users.sf.net>
//
// See the file "License.txt" for information on usage and redistribution
// of this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id$
//=========================================================================

#ifndef ABOUTDIALOG_HXX
#define ABOUTDIALOG_HXX

#include <QtWidgets/QDialog>

namespace Ui {
  class AboutDialog;
}

class AboutDialog : public QDialog
{
Q_OBJECT
  public:
    AboutDialog(QWidget* parent, const QString& title, const QString& info);
    ~AboutDialog();

  private:
    Ui::AboutDialog* m_ui;
};

#endif // ABOUTDIALOG_HXX
