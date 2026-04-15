# ------------------------------------------------- #
# @file          SqlOrm.pro
# @brief         SQLite ORM 动态库模块
# 封装 sqlite_orm 模板库，提供统一的数据库访问接口
# 一次编译，多处使用，避免重复编译模板代码
# ------------------------------------------------- #

QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SqlOrm
TEMPLATE = lib

DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += $${upper($$member(TARGET))}_LIBRARY

include(./$$member(TARGET)_src.pri)
#include(./$$member(TARGET)_inc.pri)

CUSTOMDIRLEVEL = ../
include(../CommonBase.pri)

include(depends.pri)

# 降低内存消耗的编译选项（sqlite_orm模板库非常消耗内存）
unix {
    # 只编译Release版本，不编译Debug
    CONFIG -= debug_and_release build_all
    CONFIG += release

    # 降低优化级别，减少内存消耗
    QMAKE_CFLAGS_RELEASE -= -O2
    QMAKE_CFLAGS_RELEASE += -O1
    QMAKE_CXXFLAGS_RELEASE -= -O2
    QMAKE_CXXFLAGS_RELEASE += -O1

    # 关闭一些调试信息和警告
    QMAKE_CXXFLAGS += -w
    QMAKE_CFLAGS += -w
}
