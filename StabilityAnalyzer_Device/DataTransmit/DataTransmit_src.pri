INCLUDEPATH += \
    $$PWD/inc/ \
    $$PWD/inc/Controller/ \
    $$PWD/inc/Channel/ \
    $$PWD/inc/Rndis/ \

HEADERS += \
    $$PWD/inc/datatransmit_global.h \
    $$PWD/inc/Controller/datatransmitcontroller.h \
    $$PWD/inc/Channel/tcpchannelserver.h \
    $$PWD/inc/Channel/controlchannelserver.h \
    $$PWD/inc/Channel/statuschannelserver.h \
    $$PWD/inc/Channel/streamchannelserver.h \
    $$PWD/inc/Rndis/rndismanager.h

SOURCES += \
    $$PWD/src/Controller/datatransmitcontroller.cpp \
    $$PWD/src/Channel/tcpchannelserver.cpp \
    $$PWD/src/Channel/controlchannelserver.cpp \
    $$PWD/src/Channel/statuschannelserver.cpp \
    $$PWD/src/Channel/streamchannelserver.cpp \
    $$PWD/src/Rndis/rndismanager.cpp
