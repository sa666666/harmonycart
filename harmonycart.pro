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
    SerialPortManager.hxx \
    SerialPort.hxx \
    Version.hxx \
    FindHarmonyThread.hxx \
    bspf_harmony.hxx \
    AboutDialog.hxx
FORMS += harmonycartwindow.ui \
    aboutdialog.ui
RESOURCES += resources.qrc
windows { 
    INCLUDEPATH += win32
    SOURCES += win32/SerialPortWin32.cxx
    HEADERS += win32/SerialPortWin32.hxx
    RC_FILE = win32/HarmonyCartWin32.rc
}
unix:!macx { 
    DEFINES += HAVE_INTTYPES
    INCLUDEPATH += unix
    SOURCES += unix/SerialPortUNIX.cxx
    HEADERS += unix/SerialPortUNIX.hxx
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
    DEFINES += HAVE_INTTYPES
    INCLUDEPATH += macosx
    SOURCES += macosx/SerialPortMACOSX.cxx
    HEADERS += macosx/SerialPortMACOSX.hxx
    LIBS += -framework \
        CoreFoundation \
        -framework \
        IOKit
    ICON = macosx/Harmony_icon.icns
    QMAKE_MAC_SDK = /Developer/SDKs/MacOSX10.5.sdk
    CONFIG += x86 ppc
}
