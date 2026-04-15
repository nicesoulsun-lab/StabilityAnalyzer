QT += quick widgets serialport qml sql

CONFIG += c++14
DEFINES += QT_MESSAGELOGCONTEXT

CONFIG -= qtquickcompiler


# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# Windows-specific libraries for MiniDumpWriteDump
win32 {
    LIBS += -ldbghelp
}

RESOURCES += qml.qrc

TARGET = ANALYZER
TEMPLATE = app

# Version
VERSION = 1.0.0.0
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

# 仅对最终可执行模块强制检测变化,这个是为了解决有些电脑出现修改了代码不生效必须得重新构建才生效的问题
#这个的意思是控制qmake每次都检查文件时间戳，这个问题的原因是你改的是源码，但 Qt Creator 没检测到“需要重新编译”，直接运行了之前的旧二进制所以你修改了代码运行起来就是没生效，用的之前的文件。
CONFIG += depend_includepath

# 代码文件 - 使用主容器架构
include(./Application_src.pri)

#输出路径
{
    CONFIG += debug_and_release build_all
    CONFIG(release, debug|release) {
            target_path = ./build_/dist
        }
        else {
            target_path = ./build_/debug
            #控制同时生成debug和release 不同名
            TARGET = $$member(TARGET,0)d
            #也可以使用如下写法
            #TARGET = $$join(TARGET,,,d)
        }
        msvc {
                DESTDIR = ../bin-msvc
        } else {
                DESTDIR = ../bin-mingw
        }
        MOC_DIR = $$target_path/moc
        RCC_DIR = $$target_path/rcc
        UI_DIR = $$target_path/ui
        OBJECTS_DIR = $$target_path/obj
}
# msvc 编译器
msvc {
    # 编码
    QMAKE_CFLAGS += /utf-8
    QMAKE_CXXFLAGS += /utf-8
    # pdb
    #QMAKE_CFLAGS_RELEASE += /Z7
    #QMAKE_LFLAGS_RELEASE +=/debug /opt:ref
    #QMAKE_LFLAGS_RELEASE  += /INCREMENTAL:NO /DEBUG
    QMAKE_CFLAGS_RELEASE = $$QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO
    QMAKE_CXXFLAGS_RELEASE = $$QMAKE_CXXFLAGS_RELEASE_WITH_DEBUGINFO
    QMAKE_LFLAGS_RELEASE = $$QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO
}
mingw{
    # pdb
    #加入调试信息
    QMAKE_CFLAGS_RELEASE += -g
    QMAKE_CXXFLAGS_RELEASE += -g
    #禁止优化
    QMAKE_CFLAGS_RELEASE -= -O2
    QMAKE_CXXFLAGS_RELEASE -= -O2
    #release在最后link时默认有"-s”参数，表示"Omit all symbol information from the output file"，因此要去掉该参数
    QMAKE_LFLAGS_RELEASE += -mthreads
}

# 输出编译套件信息
message(Qt version: $$[QT_VERSION])
message(Qt is installed in $$[QT_INSTALL_PREFIX])
message(the Application will create in folder: $$target_path)
message(the Application $$member(TARGET) tempalte is: $$member(TEMPLATE))

# 版本信息 管理员运行权限
RC_FILE += App_resource.rc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

#添加依赖项，lib形式
include(./depends.pri)

unix {
    message("Linking TaskScheduler and QModbusRTUUnit from ../../bin-mingw")
    LIBS += -L$$PWD/$$DESTDIR -lTaskScheduler -lQModbusRTUUnit
}

QMAKE_LFLAGS += -Wl,-rpath,/opt/$${TARGET}/lib
