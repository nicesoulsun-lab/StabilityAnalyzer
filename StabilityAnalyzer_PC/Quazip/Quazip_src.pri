INCLUDEPATH += $$PWD/inc/ \

HEADERS += \
    $$PWD/inc/JlCompress.h \
    $$PWD/inc/QuaZipWidget.h \
    $$PWD/inc/ioapi.h \
    $$PWD/inc/minizip_crypt.h \
    $$PWD/inc/quaadler32.h \
    $$PWD/inc/quachecksum32.h \
    $$PWD/inc/quacrc32.h \
    $$PWD/inc/quagzipfile.h \
    $$PWD/inc/quaziodevice.h \
    $$PWD/inc/quazip.h \
    $$PWD/inc/quazip_global.h \
    $$PWD/inc/quazip_qt_compat.h \
    $$PWD/inc/quazipdir.h \
    $$PWD/inc/quazipfile.h \
    $$PWD/inc/quazipfileinfo.h \
    $$PWD/inc/quazipnewinfo.h \
    $$PWD/inc/unzip.h \
    $$PWD/inc/zip.h

SOURCES += \
    $$PWD/src/JlCompress.cpp \
    $$PWD/src/QuaZipWidget.cpp \
    $$PWD/src/main.cpp \
    $$PWD/src/qioapi.cpp \
    $$PWD/src/quaadler32.cpp \
    $$PWD/src/quachecksum32.cpp \
    $$PWD/src/quacrc32.cpp \
    $$PWD/src/quagzipfile.cpp \
    $$PWD/src/quaziodevice.cpp \
    $$PWD/src/quazip.cpp \
    $$PWD/src/quazipdir.cpp \
    $$PWD/src/quazipfile.cpp \
    $$PWD/src/quazipfileinfo.cpp \
    $$PWD/src/quazipnewinfo.cpp \
    $$PWD/src/unzip.c \
    $$PWD/src/zip.c

FORMS += \
    $$PWD/ui/QuaZipWidget.ui
