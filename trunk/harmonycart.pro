# -------------------------------------------------
# Project created by QtCreator 2009-05-21T12:40:28
# -------------------------------------------------
TARGET = harmonycart
TEMPLATE = app
SOURCES += main.cxx \
    HarmonyCartWindow.cxx \
    Cart.cxx \
    CartDetector.cxx \
    SerialPortManager.cxx \
    SerialPortUNIX.cxx \
    SerialPortWin32.cxx \
    SerialPortMACOSX.cxx
HEADERS += HarmonyCartWindow.hxx \
    bspf.hxx \
    BSType.hxx \
    Cart.hxx \
    CartDetector.hxx \
    SerialPortManager.hxx \
    SerialPort.hxx \
    SerialPortUNIX.hxx \
    SerialPortWin32.hxx \
    SerialPortMACOSX.hxx
FORMS += harmonycartwindow.ui
RESOURCES += resources.qrc
macx:LIBS += -framework \
    CoreFoundation \
    -framework \
    IOKit
windows:CONFIG += console
