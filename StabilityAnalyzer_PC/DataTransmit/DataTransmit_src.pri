INCLUDEPATH += $$PWD/inc/

HEADERS += \
        $$PWD/inc/Channel/controlchannelclient.h \
        $$PWD/inc/Channel/statuschannelclient.h \
        $$PWD/inc/Channel/streamchannelclient.h \
        $$PWD/inc/Channel/tcpchannelclient.h \
        $$PWD/inc/Controller/datatransmitcontroller.h \
        $$PWD/inc/datatransmit_global.h

SOURCES += \
        $$PWD/src/Channel/controlchannelclient.cpp \
        $$PWD/src/Channel/statuschannelclient.cpp \
        $$PWD/src/Channel/streamchannelclient.cpp \
        $$PWD/src/Channel/tcpchannelclient.cpp \
        $$PWD/src/Controller/datatransmitcontroller.cpp
