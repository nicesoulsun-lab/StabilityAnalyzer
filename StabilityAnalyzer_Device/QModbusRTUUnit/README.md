# QModbusRTUUnit - Modbus RTU 通信模块

## 📋 概述

QModbusRTUUnit 是一个基于 Qt 的高性能 Modbus RTU 通信模块，采用**三层架构设计**，提供完整的 Modbus RTU 协议实现。该模块可以独立运行，也可以作为库被其他项目调用和复用。

## ✨ 功能特性

### 🎯 核心功能
- ✅ **完整的 Modbus RTU 协议实现** - 支持所有标准功能码
- ✅ **线程安全的串口通信** - 基于 Qt 信号槽的异步通信
- ✅ **智能帧边界检测** - 自动识别完整 Modbus 帧
- ✅ **CRC16 校验** - 完整的数据完整性验证

### 🔧 架构特性
- ✅ **三层架构设计** - 接口层、业务逻辑层、通信层分离
- ✅ **同步/异步双模式** - 支持阻塞和非阻塞操作
- ✅ **请求队列管理** - 串行处理避免并发冲突
- ✅ **错误处理机制** - 超时、CRC失败、异常响应处理

### 🚀 高级特性
- ✅ **QML 集成接口** - 便于界面开发
- ✅ **纯粹的通信库** - 类似 cutelogger/quazip 的设计理念
- ✅ **无 commonbase 依赖** - 独立运行，易于集成
- ✅ **界面在调用项目中实现** - 灵活的架构设计

## 🏗️ 架构设计

### 三层架构设计

```
┌─────────────────────────────────────────────────────────────┐
│                   接口层 (Interface Layer)                   │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │                  ModbusClient 类                        │ │
│  │  • 提供用户友好的API接口                               │ │
│  │  • 支持同步/异步操作模式                               │ │
│  │  • 请求队列管理                                        │ │
│  └─────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
                               ↓
┌─────────────────────────────────────────────────────────────┐
│                 业务逻辑层 (Business Logic Layer)           │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │                 ModbusWorker 类                         │ │
│  │  • Modbus协议处理                                      │ │
│  │  • 帧边界检测算法                                      │ │
│  │  • CRC16校验计算                                      │ │
│  │  • 错误处理和重试机制                                 │ │
│  └─────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
                               ↓
┌─────────────────────────────────────────────────────────────┐
│                  通信层 (Communication Layer)                │
│  ┌─────────────────────────────────────────────────────────┐ │
│  │              ConnectionManager 类                      │ │
│  │  • 串口通信管理                                        │ │
│  │  • 数据收发处理                                        │ │
│  │  • 连接状态监控                                        │ │
│  └─────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

### 文件结构

```
QModbusRTUUnit/
├── inc/                           # 头文件目录
│   ├── modbus_client.h            # 接口层：Modbus客户端主接口
│   ├── modbus_worker.h            # 业务逻辑层：Modbus工作线程
│   ├── modbus_request.h           # 请求类定义（工厂模式）
│   ├── modbus_response.h          # 响应类定义
│   ├── modbus_types.h             # 类型定义和枚举
│   ├── connection_manager.h        # 通信层：串口连接管理
│   ├── config_manager.h           # 配置管理
│   └── request_queue.h            # 请求队列实现
├── src/                           # 源文件目录
│   ├── modbus_client.cpp          # 接口层实现
│   ├── modbus_worker.cpp          # 业务逻辑层实现（核心）
│   ├── modbus_request.cpp         # 请求类实现
│   ├── modbus_response.cpp        # 响应类实现
│   ├── connection_manager.cpp     # 通信层实现
│   ├── config_manager.cpp         # 配置管理实现
│   ├── request_queue.cpp          # 请求队列实现
│   └── main.cpp                   # 测试程序入口
├── ui/                            # 界面文件
│   └── ModbusRTUWidget.ui        # 主界面设计
├── QModbusRTUUnit.pro             # 项目配置文件
└── README.md                      # 项目说明文档
```

## 核心类说明

### ModbusRTUClient

核心通信类，提供 Modbus RTU 协议的所有功能：

```cpp
// 连接管理
bool connectToDevice(const QString &portName, int baudRate = 9600, ...);
void disconnectFromDevice();

// Modbus 功能码操作
bool readCoils(int slaveAddress, int startAddress, int count);
bool readHoldingRegisters(int slaveAddress, int startAddress, int count);
bool writeSingleCoil(int slaveAddress, int address, bool value);
bool writeSingleRegister(int slaveAddress, int address, quint16 value);
// ... 更多功能
```

### ModbusRTUWidget

UI 界面类，提供图形化操作界面。

### QmlModbusRTU

QML 集成接口类，提供 QML 可以直接使用的属性和方法。

## 使用方法

### 1. 作为独立程序运行

```bash
# 编译项目
qmake QModbusRTUUnit.pro
make

# 运行程序
./QModbusRTUUnit
```

### 2. 作为库在其他项目中使用

在项目的 .pro 文件中添加：

```pro
include($$PWD/../QModbusRTUUnit/QModbusRTUUnit.pri)
LIBS += -L$$PWD/../bin -lQModbusRTUUnit
```

在代码中使用：

```cpp
#include "ModbusRTUClient.h"

ModbusRTUClient *client = new ModbusRTUClient(this);
client->connectToDevice("COM1", 9600);
client->readHoldingRegisters(1, 0, 10);
```

### 3. 在 QML 中使用

在 C++ 中注册类型：

```cpp
#include "QmlModbusRTU.h"

qmlRegisterType<QmlModbusRTU>("ModbusRTU", 1, 0, "ModbusRTU");
```

在 QML 中使用：

```qml
import ModbusRTU 1.0

ModbusRTU {
    id: modbus
    slaveAddress: 1
    
    onConnectionStateChanged: {
        console.log("连接状态:", connectionState)
    }
    
    Component.onCompleted: {
        modbus.connectDevice("COM1", 9600)
    }
}
```

## 示例代码

### 控制台测试程序

位于 `example/test_modbus.cpp`，演示了基本的 Modbus 操作。

### QML 示例

位于 `example/ModbusRTUExample.qml`，展示了如何在 QML 中使用该模块。

## 依赖项

- Qt 5.12 或更高版本
- Qt SerialPort 模块
- QCuteLogger（项目内部日志模块）

## 构建说明

1. 确保 Qt 环境配置正确
2. 打开终端，进入项目目录
3. 执行 `qmake QModbusRTUUnit.pro`
4. 执行 `make`（或 `nmake` 在 Windows 上）
5. 运行生成的可执行文件

## 协议规范

本模块实现了 Modbus RTU 协议规范，支持以下功能码：

| 功能码 | 名称 | 描述 |
|--------|------|------|
| 0x01 | 读线圈 | 读取单个或多个线圈状态 |
| 0x02 | 读离散输入 | 读取单个或多个离散输入状态 |
| 0x03 | 读保持寄存器 | 读取单个或多个保持寄存器 |
| 0x04 | 读输入寄存器 | 读取单个或多个输入寄存器 |
| 0x05 | 写单个线圈 | 写入单个线圈状态 |
| 0x06 | 写单个寄存器 | 写入单个保持寄存器 |
| 0x0F | 写多个线圈 | 写入多个线圈状态 |
| 0x10 | 写多个寄存器 | 写入多个保持寄存器 |

## 错误处理

模块提供了完善的错误处理机制，包括：

- 串口通信错误
- Modbus 协议错误
- CRC 校验错误
- 超时错误

所有错误都会通过 `errorOccurred` 信号发出。

## 线程安全

模块采用多线程设计，所有通信操作都在独立的工作线程中执行，确保 UI 线程不会被阻塞。

## 许可证

本项目遵循项目现有的许可证协议。

## 作者

基于现有项目架构模式开发，遵循统一的代码规范和设计模式。