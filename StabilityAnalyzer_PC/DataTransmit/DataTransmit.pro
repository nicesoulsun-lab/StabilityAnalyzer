# ------------------------------------------------- #
# @file          DataTransmit.pro
# @brief         数据转发功能模块
# 为了减少模块之间的耦合性，这个模块专门负责数据转发，包括原始数据、计算之后的数据、数据库读取的数据等等
# ------------------------------------------------- #
QT += quick

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        datatransmitcontroller.cpp \
        main.cpp

HEADERS += \
        datatransmitcontroller.h

RESOURCES += qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
