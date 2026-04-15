
INCLUDEPATH += $$PWD/inc/ \

HEADERS += \
        $$PWD/inc/taskscheduler_global.h \
        $$PWD/inc/device.h \
        $$PWD/inc/modbustaskscheduler.h \
        $$PWD/inc/portmanager.h \
        $$PWD/inc/task.h \
        $$PWD/inc/deviceconfig.h \
        $$PWD/inc/taskqueuemanager.h \
        $$PWD/inc/taskexecutionworker.h

SOURCES += \
        $$PWD/src/main.cpp \
        $$PWD/src/device.cpp \
        $$PWD/src/modbustaskscheduler.cpp \
        $$PWD/src/portmanager.cpp \
        $$PWD/src/task.cpp \
        $$PWD/src/deviceconfig.cpp \
        $$PWD/src/taskqueuemanager.cpp \
        $$PWD/src/taskexecutionworker.cpp

