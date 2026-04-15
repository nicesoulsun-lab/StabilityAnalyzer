# ------------------------------------------------- #
# @brief        文件导出模块
# 支持pdf、excel、text三种格式的配置文件读写
# ------------------------------------------------- #
QT += core gui widgets printsupport xml

TEMPLATE = app
TARGET = FileExporter

DEFINES += $${upper($$member(TARGET))}_LIBRARY

include(./$$member(TARGET)_src.pri)
#include(./$$member(TARGET)_inc.pri)

CUSTOMDIRLEVEL = ../
include(../CommonBase.pri)

#xlsx外部库模块
message(">>> 1st include")
include(xlsx/qtxlsx.pri)


# 日志模块
include(../QCuteLogger/QCuteLogger_inc.pri)
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lQCuteLogger
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lQCuteLoggerd
else:unix:LIBS += -L$$PWD/$$DESTDIR/ -lQCuteLogger

# 解压缩模块
include(../Quazip/Quazip_inc.pri)
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lQuazip
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lQuazipd
else:unix: LIBS += -L$$PWD/$$DESTDIR/ -lQuazip

include(../Quazip/Quazip_inc.pri)
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lQuazip
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lQuazipd
else:unix: LIBS += -L$$PWD/$$DESTDIR/ -lQuazip
