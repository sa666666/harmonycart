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

#ifndef Q_LR_PUSHBUTTON_HXX
#define Q_LR_PUSHBUTTON_HXX

#include <QPushButton>
#include <QMouseEvent>

/**
  Normal QPushButton doesn't support signals on right-click,
  only on left-click.  So we need a new class to do it.

  @author  Stephen Anthony
*/
class QLRPushButton : public QPushButton
{
Q_OBJECT
  public:
    explicit QLRPushButton(QWidget* parent = 0);

  private slots:
    void mousePressEvent(QMouseEvent* e);

  signals:
    void leftClicked(QLRPushButton*);
    void rightClicked(QLRPushButton*);
};

#endif // Q_LR_PUSHBUTTON_HXX
