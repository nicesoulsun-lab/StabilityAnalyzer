·# ------------------------------------------------- #
# @file          DataSaver.pro
# @brief         数据存储功能模块
# 分为结构化数据的数据库存储
# 文件数据的本地存储
# ------------------------------------------------- #

QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = DataSaver
TEMPLATE = lib

DEFINES += $${upper($$member(TARGET))}_LIBRARY

include(./$$member(TARGET)_src.pri)
#include(./$$member(TARGET)_inc.pri)

CUSTOMDIRLEVEL = ../
include(../CommonBase.pri)

include(depends.pri)
