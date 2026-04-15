# ------------------------------------------------- #
# @file          LoggerMonitor
# @brief         日志监控视图模块 提供对系统运行的实时日志查看功能
# ------------------------------------------------- #

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = LoggerMonitor
TEMPLATE = lib
# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

DEFINES += $${upper($$member(TARGET))}_LIBRARY
include(./$$member(TARGET)_src.pri)
#include(./$$member(TARGET)_inc.pri)

CUSTOMDIRLEVEL = ../
include(../CommonBase.pri)

#include(../DataStruct/DataStruct.pri)


# 日志模块
include(../QCuteLogger/QCuteLogger_inc.pri)
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lQCuteLogger
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lQCuteLoggerd
else:unix:LIBS += -L$$PWD/$$DESTDIR/ -lQCuteLogger


