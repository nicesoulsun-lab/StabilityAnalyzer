INCLUDEPATH += $$PWD/inc/

HEADERS += \
    $$PWD/inc/QSingleApplication_global.h \
    $$PWD/inc/qtlocalpeer.h \
    $$PWD/inc/qtlockedfile.h \
    $$PWD/inc/qtsingleapplication.h \
    $$PWD/inc/qtsinglecoreapplication.h

SOURCES += \
    $$PWD/src/qtlocalpeer.cpp \
    $$PWD/src/qtlockedfile.cpp \
    $$PWD/src/qtlockedfile_unix.cpp \
    $$PWD/src/qtlockedfile_win.cpp \
    $$PWD/src/qtsingleapplication.cpp \
    $$PWD/src/qtsinglecoreapplication.cpp

DISTFILES += \
    auto-make.bat
