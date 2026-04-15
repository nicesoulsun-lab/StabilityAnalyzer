#-------------------------------------------------
#
# 日志模块
#
#-------------------------------------------------

QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET =  QCuteLogger
TEMPLATE = lib

DEFINES += CUTELOGGER_LIBRARY

CUSTOMDIRLEVEL = ../
include(../CommonBase.pri)
# 引入文件
include(./QCuteLogger_src.pri)

# 版本控制文件
RC_FILE += ./QCuteLogger_resource.rc

## 输出路径
#{
#    CONFIG += debug_and_release build_all
#    CONFIG(release, debug|release) {
#            target_path = ./build_/dist
#        }
#        else {
#            target_path = ./build_/debug
#            #同时生成debug和release版本
#            TARGET = $$member(TARGET,0)d
#        }
#        msvc {
#                DESTDIR = ../bin-msvc
#        } else {
#                DESTDIR = ../bin-mingw
#        }
#        MOC_DIR = $$target_path/moc
#        RCC_DIR = $$target_path/rcc
#        UI_DIR = $$target_path/ui
#        OBJECTS_DIR = $$target_path/obj
#}
## msvc 编译器
#msvc {
#    # 编码
#    QMAKE_CFLAGS += /utf-8
#    QMAKE_CXXFLAGS += /utf-8
#    # pdb
#    QMAKE_CFLAGS_RELEASE += /Zi
#    QMAKE_LFLAGS_RELEASE +=/debug /opt:ref
#}

# 输出编译套件信息
#message(Qt version: $$[QT_VERSION])
#message(Qt is installed in $$[QT_INSTALL_PREFIX])
#message(the Application will create in folder: $$target_path)
#message(the Application $$member(TARGET) tempalte is: $$member(TEMPLATE))
