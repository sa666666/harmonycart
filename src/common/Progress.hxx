//=========================================================================
//
//  H   H    A    RRRR   M   M   OOO   N   N  Y   Y
//  H   H   A A   R   R  MM MM  O   O  NN  N   Y Y
//  HHHHH  AAAAA  RRRR   M M M  O   O  N N N    Y   "Harmony Cart software"
//  H   H  A   A  R R    M   M  O   O  N  NN    Y
//  H   H  A   A  R  R   M   M   OOO   N   N    Y
//
// Copyright (c) 2009-2025 by Stephen Anthony <sa666666@gmail.com>
//
// See the file "License.txt" for information on usage and redistribution
// of this file, and for a DISCLAIMER OF ALL WARRANTIES.
//=========================================================================

#ifndef PROGRESS_HXX
#define PROGRESS_HXX

#include <QApplication>
#include <QDir>
#include <QIcon>
#include <QPixmap>
#include <QProgressDialog>
#include <QString>

class Progress
{
  public:
    /**
      Create a new Progress object, which wraps a QProgressDialog, so that
      the underlying cart programming code doesn't have to know how the
      progress bar is implemented.
    */
    Progress()
    {
      myDlg.setWindowModality(Qt::WindowModal);
      myDlg.setWindowIcon(QPixmap(":icons/pics/appicon.png"));
      myDlg.reset();
    }
    ~Progress() = default;

    void setEnabled(bool enabled) { myEnabled = enabled; }

    void initialize(const QString& title, int minimum, int maximum) {
      myDlg.setWindowTitle(title);
      myDlg.setRange(minimum, maximum);
      myDlg.setMinimumDuration(0);
      myDlg.setValue(0);
    }

    void updateText(const QString& text) {
      if(myEnabled)
        myDlg.setLabelText(text);
    }

    bool updateValue(int step) {
      if(myEnabled)
      {
        myDlg.setValue(step);
        return !myDlg.wasCanceled();
      }
      else
        return true;
    }

    void finalize() {
      if(myEnabled)
        myDlg.setValue(myDlg.maximum());
    }

  private:
    QProgressDialog myDlg;
    bool myEnabled{false};

  private:
    // Following constructors and assignment operators not supported
    Progress(const Progress&) = delete;
    Progress(Progress&&) = delete;
    Progress& operator=(const Progress&) = delete;
    Progress& operator=(Progress&&) = delete;
};

#endif
