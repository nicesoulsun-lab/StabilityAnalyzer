QT += quick widgets serialport qml sql

CONFIG += c++14
DEFINES += QT_MESSAGELOGCONTEXT

CONFIG -= qtquickcompiler

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000

win32 {
    LIBS += -ldbghelp
}

RESOURCES += qml.qrc

TARGET = ANALYZER
TEMPLATE = app

VERSION = 1.0.0.0
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

CONFIG += depend_includepath

include(./Application_src.pri)

{
    CONFIG += debug_and_release build_all
    CONFIG(release, debug|release) {
        target_path = ./build_/dist
    } else {
        target_path = ./build_/debug
        TARGET = $$member(TARGET,0)d
    }

    msvc {
        DESTDIR = ../bin-msvc
    } else {
        DESTDIR = ../bin-mingw
    }

    MOC_DIR = $$target_path/moc
    RCC_DIR = $$target_path/rcc
    UI_DIR = $$target_path/ui
    OBJECTS_DIR = $$target_path/obj
}

msvc {
    QMAKE_CFLAGS += /utf-8
    QMAKE_CXXFLAGS += /utf-8
    QMAKE_CFLAGS_RELEASE = $$QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO
    QMAKE_CXXFLAGS_RELEASE = $$QMAKE_CXXFLAGS_RELEASE_WITH_DEBUGINFO
    QMAKE_LFLAGS_RELEASE = $$QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO
}

mingw {
    QMAKE_CFLAGS_RELEASE += -g
    QMAKE_CXXFLAGS_RELEASE += -g
    QMAKE_CFLAGS_RELEASE -= -O2
    QMAKE_CXXFLAGS_RELEASE -= -O2
    QMAKE_LFLAGS_RELEASE += -mthreads
}

message(Qt version: $$[QT_VERSION])
message(Qt is installed in $$[QT_INSTALL_PREFIX])
message(the Application will create in folder: $$target_path)
message(the Application $$member(TARGET) tempalte is: $$member(TEMPLATE))

RC_FILE += App_resource.rc

QML_IMPORT_PATH =
QML_DESIGNER_IMPORT_PATH =

include(./depends.pri)

unix {
    message("Linking TaskScheduler and QModbusRTUUnit from ../../bin-mingw")
    LIBS += -L$$PWD/$$DESTDIR -lTaskScheduler -lQModbusRTUUnit
}

QMAKE_LFLAGS += -Wl,-rpath,/opt/$${TARGET}/lib
