# ------------------------------------------------- #
# @file          QModbusRTUUnit_src.pri
# @brief         Modbus RTU通信模块源文件配置
# ###### 包含所有源文件
# @author        qiuyuhan
# @date          2025-12-17
# ------------------------------------------------- #
INCLUDEPATH += $$PWD/inc/ \

SOURCES += \
    $$PWD/src/ModbusConfig.cpp \
    $$PWD/src/ModbusRTUWidget.cpp \
#    $$PWD/src/cache_manager.cpp \
    $$PWD/src/config_manager.cpp \
    $$PWD/src/connection_manager.cpp \
    $$PWD/src/main.cpp \
    $$PWD/src/modbus_client.cpp \
    $$PWD/src/modbus_request.cpp \
    $$PWD/src/modbus_response.cpp \
    $$PWD/src/modbus_types.cpp \
    $$PWD/src/modbus_worker.cpp \
#    $$PWD/src/request_queue.cpp

HEADERS += \
    $$PWD/inc/ModbusConfig.h \
    $$PWD/inc/ModbusRTUWidget.h \
#    $$PWD/inc/cache_manager.h \
    $$PWD/inc/config_manager.h \
    $$PWD/inc/connection_manager.h \
    $$PWD/inc/modbus_client.h \
    $$PWD/inc/modbus_request.h \
    $$PWD/inc/modbus_response.h \
    $$PWD/inc/modbus_types.h \
    $$PWD/inc/modbus_worker.h \
    $$PWD/inc/qmodbusrtuunit_global.h \
#    $$PWD/inc/request_queue.h

# 资源文件（如果有）
# RESOURCES += \
#     $$PWD/resources.qrc
