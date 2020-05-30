# -------------------------------------------------
# Project created by QtCreator 2009-05-21T12:40:28
# -------------------------------------------------
TARGET = HarmonyCart
TEMPLATE = app
SOURCES += src/common/main.cxx \
    src/common/Bankswitch.cxx \
    src/common/HarmonyCartWindow.cxx \
    src/common/Cart.cxx \
    src/common/CartDetector.cxx \
    src/common/CartDetectorWrapper.cxx \
    src/common/FSNode.cxx \
    src/common/SerialPortManager.cxx \
    src/common/AboutDialog.cxx
HEADERS += src/common/HarmonyCartWindow.hxx \
    src/common/bspf.hxx \
    src/common/Bankswitch.hxx \
    src/common/Cart.hxx \
    src/common/CartDetector.hxx \
    src/common/CartDetectorWrapper.hxx \
    src/common/FSNode.hxx \
    src/common/OSystem.hxx \
    src/common/SerialPortManager.hxx \
    src/common/SerialPort.hxx \
    src/common/Version.hxx \
    src/common/FindHarmonyThread.hxx \
    src/common/AboutDialog.hxx
FORMS += harmonycartwindow.ui \
    aboutdialog.ui
RESOURCES += resources.qrc
QT += widgets
QMAKE_CXXFLAGS += -std=c++14 -Wno-unused-parameter
DEFINES += CUSTOM_ARM
INCLUDEPATH += src/common

windows {
#  Uncomment the following to create a commandline-compatible Windows build
#    TARGET = HarmonyCart.com
#    CONFIG += qt console
    DEFINES += _CRT_SECURE_NO_WARNINGS BSPF_WINDOWS
    INCLUDEPATH += src/windows
    SOURCES += src/unix/FSNodeWindows.cxx src/windows/SerialPortWindows.cxx src/windows/OSystemWindows.cxx
    HEADERS += src/unix/FSNodeWindows.hxx src/windows/SerialPortWindows.hxx src/windows/OSystemWindows.hxx
    RC_FILE = src/windows/HarmonyCart.rc
}
unix:!macx {
    DEFINES += BSPF_UNIX
    INCLUDEPATH += src/unix
    SOURCES += src/unix/FSNodePOSIX.cxx src/unix/SerialPortUNIX.cxx src/unix/OSystemUNIX.cxx
    HEADERS += src/unix/FSNodePOSIX.hxx src/unix/SerialPortUNIX.hxx src/unix/OSystemUNIX.hxx
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
    DEFINES += BSPF_MACOS
    INCLUDEPATH += src/macos
    SOURCES += src/macos/SerialPortMACOS.cxx src/macos/OSystemMACOS.cxx
    HEADERS += src/macos/SerialPortMACOS.hxx src/macos/OSystemMACOS.hxx
    LIBS += -framework CoreFoundation -framework IOKit
    ICON = src/macos/Harmony_icon.icns
}

DISTFILES += \
    src/windows/HarmonyCart.ico \
    src/windows/harmonycart.iss
