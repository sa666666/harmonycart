//=========================================================================
//
//  H   H    A    RRRR   M   M   OOO   N   N  Y   Y
//  H   H   A A   R   R  MM MM  O   O  NN  N   Y Y
//  HHHHH  AAAAA  RRRR   M M M  O   O  N N N    Y   "Harmony Cart software"
//  H   H  A   A  R R    M   M  O   O  N  NN    Y
//  H   H  A   A  R  R   M   M   OOO   N   N    Y
//
// Copyright (c) 2009-2024 by Stephen Anthony <sa666666@gmail.com>
//
// See the file "License.txt" for information on usage and redistribution
// of this file, and for a DISCLAIMER OF ALL WARRANTIES.
//=========================================================================

#ifndef Q_LR_PUSHBUTTON_HXX
#define Q_LR_PUSHBUTTON_HXX

#include <QPushButton>
#include <QMouseEvent>

/**
  Normal QPushButton supports only left-click.
  We need a second option, so we use double-click too.

  Note: Using right-click causes all kinds of (different) problems
        on different platforms.  Best to avoid it, and leave the
        right button alone.

  @author  Stephen Anthony
*/
class QDoubleClickButton : public QPushButton
{
Q_OBJECT
  public:
    explicit QDoubleClickButton(QWidget* parent = nullptr) : QPushButton(parent) { }

    /**
      Determines which button to detect double-click events for.
      By default we detect the left button.
    */
    void setButtonToDetect(Qt::MouseButton b) { myButtonToDetect = b; }

  signals:
    void doubleClicked(QDoubleClickButton*);

  protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override {
      if(event->button() & myButtonToDetect) emit doubleClicked(this);
    }

  private:
    Qt::MouseButton myButtonToDetect{Qt::LeftButton};
};

#endif // Q_DOUBLECLICKBUTTON_HXX
