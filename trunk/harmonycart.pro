# -------------------------------------------------
# Project created by QtCreator 2009-05-21T12:40:28
# -------------------------------------------------
TARGET = HarmonyCart
TEMPLATE = app
SOURCES += main.cxx \
    HarmonyCartWindow.cxx \
    Cart.cxx \
    CartDetector.cxx \
    SerialPortManager.cxx

HEADERS += HarmonyCartWindow.hxx \
    bspf.hxx \
    BSType.hxx \
    Cart.hxx \
    CartDetector.hxx \
    SerialPortManager.hxx \
    SerialPort.hxx \
    Version.hxx

FORMS += harmonycartwindow.ui
RESOURCES += resources.qrc

windows {
  SOURCES += SerialPortWin32.cxx
  HEADERS += SerialPortWin32.hxx
  RC_FILE = win32/HarmonyCartWin32.rc
}
unix:!macx {
  SOURCES += SerialPortUNIX.cxx
  HEADERS += SerialPortUNIX.hxx
  TARGET = harmonycart

  target.path = /usr/bin
  docs.path  = /usr/share/doc/harmonycart
  docs.files = docs/* Announce.txt  Changes.txt  Copyright.txt  License.txt  Readme.txt
  desktop.path  = /usr/share/applications
  desktop.files = unix/harmonycart.desktop
  icon.path  = /usr/share/icons
  icon.files = unix/harmonycart.png

  INSTALLS += target icon docs desktop
}
macx {
  SOURCES += SerialPortMACOSX.cxx
  HEADERS += SerialPortMACOSX.hxx
  LIBS += -framework CoreFoundation -framework IOKit
  CONFIG += x86 ppc
}
