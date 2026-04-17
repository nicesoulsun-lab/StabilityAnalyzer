# sql_orm.pri - SQLite ORM 模块配置
# 在 MainWindow.pro 中使用：include(../sql_orm/sql_orm.pri)

# C++14 支持（sqlite_orm 需要）
CONFIG += c++14

# 头文件路径
INCLUDEPATH += $$PWD
INCLUDEPATH += $$PWD/sqlite_orm
INCLUDEPATH += $$PWD/sqlite3

# 定义预处理器宏
DEFINES += USE_SQL_ORM

# 包含 SQLite3 源码配置
include($$PWD/sqlite3/sqlite3.pri)

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

SOURCES += $$SQL_ORM_SOURCES
HEADERS += $$SQL_ORM_HEADERS
INCLUDEPATH += $$SQL_ORM_INCLUDEPATH
