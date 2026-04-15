# 配置文件管理模块源文件配置
INCLUDEPATH += $$PWD/inc/ \

SOURCES += \
    $$PWD/src/main.cpp \
    src/ConfigManager.cpp \
    src/IniConfig.cpp \
    src/XmlConfig.cpp \
    src/JsonConfig.cpp

HEADERS += \
    inc/ConfigManager.h \
    inc/IniConfig.h \
    inc/XmlConfig.h \
    inc/JsonConfig.h \
    inc/IConfigInterface.h \
    inc/configmanager_global.h