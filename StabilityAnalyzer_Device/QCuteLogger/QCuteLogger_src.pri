INCLUDEPATH += $$PWD/inc/

HEADERS += \
    $$PWD/inc/RollingFileAppender.h \
    $$PWD/inc/Logger.h \
    $$PWD/inc/FileAppender.h \
    $$PWD/inc/CuteLogger_global.h \
    $$PWD/inc/ConsoleAppender.h \
    $$PWD/inc/AbstractStringAppender.h \
    $$PWD/inc/AbstractAppender.h \
    $$PWD/inc/logmanager.h \
    $$PWD/inc/widget.h

SOURCES += \
    $$PWD/src/RollingFileAppender.cpp \
    $$PWD/src/Logger.cpp \
    $$PWD/src/FileAppender.cpp \
    $$PWD/src/ConsoleAppender.cpp \
    $$PWD/src/AbstractStringAppender.cpp \
    $$PWD/src/AbstractAppender.cpp \
    $$PWD/src/logmanager.cpp \
    $$PWD/src/main.cpp \
    $$PWD/src/widget.cpp

win32 {
    SOURCES += $$PWD/src/OutputDebugAppender.cpp
    HEADERS += $$PWD/inc/OutputDebugAppender.h
}

FORMS += \
    $$PWD/ui/widget.ui

DISTFILES += \
    auto-make.bat
