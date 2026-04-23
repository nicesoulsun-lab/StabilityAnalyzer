# ------------------------------------------------- #
# @file          DataTransmit.pro
# @brief         Device 侧 USB RNDIS + TCP 通信模块
#                该模块负责启动 RNDIS 脚本、监听三路 TCP 服务，
#                并向上层暴露统一的通信控制器。
# ------------------------------------------------- #

QT += core network

CONFIG += c++14
CONFIG += depend_includepath

TARGET = DataTransmit
TEMPLATE = lib

DEFINES += DATATRANSMIT_LIBRARY

include(./DataTransmit_src.pri)

{
    CONFIG += debug_and_release build_all
    CONFIG(release, debug|release) {
        target_path = ./build_/dist
    } else {
        target_path = ./build_/debug
        TARGET = $$member(TARGET, 0)d
    }

    msvc {
        DESTDIR = ../bin-msvc
    } else {
        DESTDIR = ../bin-mingw
    }

    MOC_DIR = $$target_path/moc
    OBJECTS_DIR = $$target_path/obj
}

msvc {
    QMAKE_CFLAGS += /utf-8
    QMAKE_CXXFLAGS += /utf-8
}

mingw {
    QMAKE_CFLAGS_RELEASE += -g
    QMAKE_CXXFLAGS_RELEASE += -g
    QMAKE_CFLAGS_RELEASE -= -O2
    QMAKE_CXXFLAGS_RELEASE -= -O2
    QMAKE_LFLAGS_RELEASE += -mthreads
}
