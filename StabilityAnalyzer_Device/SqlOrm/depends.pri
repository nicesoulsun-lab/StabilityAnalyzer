# 日志模块
include(../QCuteLogger/QCuteLogger_inc.pri)
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lQCuteLogger
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lQCuteLoggerd
else:unix:LIBS += -L$$PWD/$$DESTDIR/ -lQCuteLogger

# 基础数据结构
include(../CommonData/CommonData_inc.pri)
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lCommonData
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/$$DESTDIR/ -lCommonDatad
else:unix:LIBS += -L$$PWD/$$DESTDIR/ -lCommonData

# SQLite3 源码
include(../sql_orm/sqlite3/sqlite3.pri)

# sqlite_orm 头文件（仅包含，不编译模板）
INCLUDEPATH += $$PWD/../sql_orm
INCLUDEPATH += $$PWD/../sql_orm/sqlite_orm

# C++14 支持（sqlite_orm 需要）
CONFIG += c++14

# Windows 特定配置
win32 {
    DEFINES += SQLITE_ENABLE_FTS5
    DEFINES += SQLITE_ENABLE_JSON1
}

# Linux 特定配置
unix {
    DEFINES += SQLITE_ENABLE_FTS5
    DEFINES += SQLITE_ENABLE_JSON1
}
