# ------------------------------------------------- #
# @file          DataTransmit.pro
# @brief         数据转发功能模块
# 为了减少模块之间的耦合性，这个模块专门负责数据转发，包括原始数据、计算之后的数据、数据库读取的数据等等
# ------------------------------------------------- #
QT += core network

TARGET = DataTransmit
TEMPLATE = lib
DEFINES += DATATRANSMIT_LIBRARY

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CUSTOMDIRLEVEL = ../
include(../CommonBase.pri)
include(./DataTransmit_src.pri)

win32:LIBS += -ladvapi32
