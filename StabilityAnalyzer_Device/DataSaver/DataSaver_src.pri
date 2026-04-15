INCLUDEPATH += $$PWD/inc/ \

FORMS += \
    $$PWD/ui/DataSaverWidget.ui

HEADERS += \
    $$PWD/inc/DataClearHandler.h \
    $$PWD/inc/DataSaverWidget.h \
    $$PWD/inc/DataSaver_Base.h \
    $$PWD/inc/SubjectClassification.h \
    $$PWD/inc/SubjectSaveWorker.h \
    $$PWD/inc/datasaver_global.h

SOURCES += \
    $$PWD/src/DataClearHandler.cpp \
    $$PWD/src/DataSaverWidget.cpp \
    $$PWD/src/SubjectClassification.cpp \
    $$PWD/src/DataSaver_Base.cpp \
    $$PWD/src/SubjectSaveWorker.cpp \
    $$PWD/src/main.cpp
