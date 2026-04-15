INCLUDEPATH += $$PWD/inc/ \

HEADERS += \
    $$PWD/inc/BasePackData.h \
    $$PWD/inc/CommonData.h \
    $$PWD/inc/SubjectData.h \
    $$PWD/inc/TaskInfo.h \
    $$PWD/inc/commondata_global.h \
    $$PWD/inc/struct.h

SOURCES += \
    $$PWD/src/CommonData.cpp \
    $$PWD/src/SubjectData.cpp

SUBDIRS += \
    $$PWD/CommonData.pro
