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
  RC_FILE = HarmonyCartWin32.rc
}
unix {
  SOURCES += SerialPortUNIX.cxx
  HEADERS += SerialPortUNIX.hxx
  TARGET = harmonycart
}
macx {
  SOURCES += SerialPortMACOSX.cxx
  HEADERS += SerialPortMACOSX.hxx
  LIBS += -framework CoreFoundation -framework IOKit
}
