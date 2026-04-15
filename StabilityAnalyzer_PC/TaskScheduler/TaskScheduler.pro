# ------------------------------------------------- #
# @file          TaskScheduler.pro
# @brief         任务调度模块
# ------------------------------------------------- #
QT += quick serialport concurrent

CONFIG += c++11
TARGET = TaskScheduler
TEMPLATE = lib
DEFINES += TASKSCHEDULER_LIBRARY

# 使用通用构建配置
include(./$${TARGET}_src.pri)
CUSTOMDIRLEVEL = ../
include(../CommonBase.pri)

DEFINES += QT_DEPRECATED_WARNINGS

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += qml.qrc

# 添加子模块
include(./depends.pri)
include(./depends.pri)

