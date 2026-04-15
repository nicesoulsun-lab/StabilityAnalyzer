# ------------------------------------------------- #
# @file          ConfigManager.pro
# @brief         配置文件管理模块
# 支持INI、XML、JSON三种格式的配置文件读写
# ------------------------------------------------- #

QT       += core xml

TARGET = ConfigManager
TEMPLATE = app

DEFINES += CONFIGMANAGER_LIBRARY

include(./$$member(TARGET)_src.pri)

CUSTOMDIRLEVEL = ../
include(../CommonBase.pri)

# 日志模块
include(../QCuteLogger/QCuteLogger_inc.pri)
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lQCuteLogger
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lQCuteLoggerd
else:unix:LIBS += -L$$PWD/$$DESTDIR/ -lQCuteLogger
