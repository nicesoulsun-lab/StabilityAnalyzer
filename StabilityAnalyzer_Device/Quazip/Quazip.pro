#-------------------------------------------------
#
# 解压缩
#
#-------------------------------------------------
#QuaZIP类说明类	说明
#JlCompress	典型操作工具类
#QuaAdler32	Adler32算法校验和
#QuaChecksum32	校验和接口
#QuaCrc32	CRC32校验和
#QuaGzipFile	GZIP 文件操作
#QuaZIODevice	压缩/解压 QIODevice
#QuaZip	ZIP 文件
#QuaZipDir	ZIP文件内目录导航
#QuaZipFile	ZIP文件内的文件
#QuaZipFileInfo	ZIP压缩包内的文件信息
#QuaZipFilePrivate	QuaZip的接口
#QuaZipNewInfo	被创建的文件信息
#QuaZipPrivate	QuaZIP内部类

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Quazip
TEMPLATE = lib

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += QUAZIP_LIBRARY QUAZIP_BUILD

CUSTOMDIRLEVEL = ../
include(../CommonBase.pri)
include(./Quazip_src.pri)

INCLUDEPATH += $$[QT_INSTALL_HEADERS]/QtZlib

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

mingw{
    LIBS += -lz
}
