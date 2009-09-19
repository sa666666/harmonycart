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
    SerialPortMACOSX.cxx \
    lpcprog.c \
    lpc21isp.c
HEADERS += HarmonyCartWindow.hxx \
    bspf.hxx \
    BSType.hxx \
    Cart.hxx \
    CartDetector.hxx \
    SerialPortManager.hxx \
    SerialPort.hxx \
    SerialPortUNIX.hxx \
    SerialPortWin32.hxx \
    SerialPortMACOSX.hxx \
    lpcprog.h \
    lpc21isp.h
FORMS += harmonycartwindow.ui
RESOURCES += resources.qrc
macx:LIBS += -framework \
    CoreFoundation \
    -framework \
    IOKit
windows:CONFIG += console
