# ------------------------------------------------- #
# @file          QModbusRTUUnit.pro
# @brief         Modbus RTU通信模块
# ###### 提供Modbus RTU通信接口
# ###### 支持串口通信，实现Modbus RTU协议
# ------------------------------------------------- #

QT       += core gui widgets serialport concurrent
TARGET = QModbusRTUUnit
TEMPLATE = lib

CONFIG += c++14

DEFINES += QMODBUSRTUUNIT_LIBRARY

CUSTOMDIRLEVEL = ../
include(../CommonBase.pri)
#include(./$$member(TARGET)_inc.pri)
include(./QModbusRTUUnit_src.pri)

DEFINES += QT_DEPRECATED_WARNINGS

# 日志
include(../QCuteLogger/QCuteLogger_inc.pri)
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lQCuteLogger
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lQCuteLoggerd
else:unix:LIBS += -L$$PWD/$$DESTDIR/ -lQCuteLogger

FORMS += \
    ui/ModbusRTUWidget.ui


