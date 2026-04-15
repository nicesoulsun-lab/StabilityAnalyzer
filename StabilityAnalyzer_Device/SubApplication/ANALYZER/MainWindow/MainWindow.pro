QT += quick serialport widgets concurrent sql

CONFIG += c++14
# 内存优化选项，减少编译时的内存占用（针对 A133 设备优化）
#QMAKE_CXXFLAGS += -O1 -fno-lto -fno-plt
#QMAKE_LFLAGS += -fno-lto -O1

TARGET = MainWindow
TEMPLATE = lib
DEFINES += MAINWINDOW_LIBRARY
# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# 使用通用构建配置
include(./$$member(TARGET)_src.pri)
include(../../../CommonBase.pri)

DEFINES += QT_DEPRECATED_WARNINGS
# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    mainwindow.qrc \
    #fonts.qrc \
    icons.qrc
# 添加子模块
include(./depends.pri)
