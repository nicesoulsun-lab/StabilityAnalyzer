#-------------------------------------------------
#
# 系统公共模块 提供公共参数 公共方法接口
#
#-------------------------------------------------

QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = CommonData
TEMPLATE = lib

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += COMMONDATA_LIBRARY

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CUSTOMDIRLEVEL = ../
include(../CommonBase.pri)
include(./CommonData_src.pri)

#win32{
#    CONFIG += debug_and_release build_all
#    CONFIG(release, debug|release) {
#            target_path = ./build_/dist
#        } else {
#            target_path = ./build_/debug
#            TARGET = $$member(TARGET,0)d
#        }
#        DESTDIR = ../bin
#        MOC_DIR = $$target_path/moc
#        RCC_DIR = $$target_path/rcc
#        UI_DIR = $$target_path/ui
#        OBJECTS_DIR = $$target_path/obj
#}
#CONFIG += c++11
#message(Qt version: $$[QT_VERSION])
#message(Qt is installed in $$[QT_INSTALL_PREFIX])
#message(the CommonData will create in folder: $$target_path)
# 屏幕分辨率尺寸 windows接口
win32{
LIBS += -lgdi32
}

