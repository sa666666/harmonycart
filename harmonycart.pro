# -------------------------------------------------
# Project created by QtCreator 2009-05-21T12:40:28
# -------------------------------------------------
TARGET = HarmonyCart
TEMPLATE = app
SOURCES += main.cxx \
    HarmonyCartWindow.cxx \
    Cart.cxx \
    CartDetector.cxx \
    SerialPortManager.cxx \
    AboutDialog.cxx
HEADERS += HarmonyCartWindow.hxx \
    bspf.hxx \
    BSType.hxx \
    Cart.hxx \
    CartDetector.hxx \
    OSystem.hxx \
    SerialPortManager.hxx \
    SerialPort.hxx \
    Version.hxx \
    FindHarmonyThread.hxx \
    AboutDialog.hxx
FORMS += harmonycartwindow.ui \
    aboutdialog.ui
RESOURCES += resources.qrc
QT += widgets
QMAKE_CXXFLAGS += -std=c++14

windows { 
#  Uncomment the following to create a commandline-compatible Windows build
#    TARGET = HarmonyCart.com
#    CONFIG += qt console
    DEFINES += _CRT_SECURE_NO_WARNINGS BSPF_WINDOWS
    INCLUDEPATH += windows
    SOURCES += windows/SerialPortWindows.cxx windows/OSystemWindows.cxx
    HEADERS += windows/SerialPortWindows.hxx windows/OSystemWindows.hxx
    RC_FILE = windows/HarmonyCart.rc
}
unix:!macx { 
    DEFINES += BSPF_UNIX
    INCLUDEPATH += unix
    SOURCES += unix/SerialPortUNIX.cxx unix/OSystemUNIX.cxx
    HEADERS += unix/SerialPortUNIX.hxx unix/OSystemUNIX.hxx
    TARGET = harmonycart
    target.path = /usr/bin
    docs.path = /usr/share/doc/harmonycart
    docs.files = docs/* \
        Announce.txt \
        Changes.txt \
        Copyright.txt \
        License.txt \
        Readme.txt
    arm.path = /usr/share/harmonycart
    arm.files = arm/*
    desktop.path = /usr/share/applications
    desktop.files = unix/harmonycart.desktop
    icon.path = /usr/share/icons
    icon.files = unix/harmonycart.png
    INSTALLS += target \
        icon \
        docs \
        arm \
        desktop
}
macx { 
    DEFINES += BSPF_MAC_OSX
    INCLUDEPATH += macosx
    SOURCES += macosx/SerialPortMACOSX.cxx macosx/OSystemMACOSX.cxx
    HEADERS += macosx/SerialPortMACOSX.hxx macosx/OSystemMACOSX.hxx
    LIBS += -framework CoreFoundation -framework IOKit
    ICON = macosx/Harmony_icon.icns
}

DISTFILES += \
    windows/HarmonyCart.ico \
    windows/harmonycart.iss

