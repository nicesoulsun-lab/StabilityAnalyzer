# StabilityAnalyzer - Code Wiki

## 1. 项目概述

StabilityAnalyzer 是一套基于 Qt/QML 的**稳定性分析系统**，采用 C++ 开发，用于铁路检测领域的数据采集、通信、存储、分析与可视化。系统采用**设备端（Device）+ PC端**双端架构：

- **StabilityAnalyzer_Device**：运行在嵌入式设备端，负责通过 Modbus RTU 协议采集传感器数据，进行信号处理算法分析，并通过 RNDIS 网络将数据传输至 PC 端
- **StabilityAnalyzer_PC**：运行在 PC 端，负责接收设备端数据，进行数据管理、实验管理、高级分析、报告生成及可视化展示

### 技术栈

| 类别 | 技术 |
|------|------|
| 语言 | C++11, QML |
| 框架 | Qt 5 (Widgets + Quick/QML) |
| 构建 | qmake (.pro/.pri) |
| 数据库 | SQLite (sqlite_orm) |
| 通信 | Modbus RTU (串口), RNDIS/TCP (网络) |
| 编译器 | MSVC / MinGW |
| 平台 | Windows (主要), macOS (部分支持) |

---

## 2. 项目整体架构

### 2.1 顶层目录结构

```
StabilityAnalyzer/
├── StabilityAnalyzer_Device/    # 设备端应用
├── StabilityAnalyzer_PC/        # PC端应用
└── .gitignore
```

### 2.2 系统架构图

```
┌─────────────────────────────────────────────────────────────────┐
│                     StabilityAnalyzer_PC                         │
│  ┌──────────┐  ┌──────────────┐  ┌──────────────────────────┐  │
│  │Application│  │SubApplication│  │   Controller Layer       │  │
│  │  (QML)   │──│   (ANALYZER) │──│ CtrllerManager           │  │
│  └──────────┘  └──────────────┘  │  ├─ ExperimentCtrl       │  │
│                                  │  ├─ DataCtrl              │  │
│                                  │  ├─ UserCtrl              │  │
│                                  │  ├─ systemSettingCtrl     │  │
│                                  │  ├─ reportCtrl            │  │
│                                  │  ├─ realtimeCtrl          │  │
│                                  │  ├─ detailCtrl            │  │
│                                  │  └─ compareCtrl           │  │
│                                  └──────────┬───────────────┘  │
│                                             │                   │
│  ┌─────────────────┐  ┌──────────────┐      │                   │
│  │   SqlOrm        │  │DataTransmit  │◄─────┘                   │
│  │ (SQLite ORM)    │  │Controller    │                          │
│  └─────────────────┘  │ (RNDIS/TCP)  │                          │
│                       └──────┬───────┘                          │
└──────────────────────────────┼──────────────────────────────────┘
                               │ TCP/RNDIS
┌──────────────────────────────┼──────────────────────────────────┐
│                     StabilityAnalyzer_Device                     │
│                       ┌──────┴───────┐                          │
│                       │DataTransmit  │                          │
│                       │(RNDIS Server)│                          │
│                       └──────┬───────┘                          │
│                              │                                   │
│  ┌───────────────┐  ┌───────┴────────┐  ┌──────────────────┐  │
│  │  Algorithm    │  │ TaskScheduler  │  │  QModbusRTUUnit  │  │
│  │ (信号处理)    │◄─│ (任务调度)     │◄─│ (Modbus通信)     │  │
│  └───────────────┘  └────────────────┘  └──────────────────┘  │
│                                                                  │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────────┐   │
│  │CommonData│  │DataSaver │  │SqlOrm    │  │LoggerMonitor │   │
│  └──────────┘  └──────────┘  └──────────┘  └──────────────┘   │
└──────────────────────────────────────────────────────────────────┘
```

### 2.3 数据流

```
传感器 → [Modbus RTU串口] → QModbusRTUUnit → TaskScheduler → Algorithm → DataSaver → SqlOrm
                                                                    ↓
                                                              DataTransmit(RNDIS)
                                                                    ↓
                                                              PC DataTransmitController
                                                                    ↓
                                                              CtrllerManager → QML UI
```

---

## 3. 模块详解

### 3.1 StabilityAnalyzer_Device 模块

#### 3.1.1 Application（主程序入口）

| 项目 | 说明 |
|------|------|
| 路径 | `StabilityAnalyzer_Device/Application/` |
| 职责 | 应用程序入口，初始化日志、单例检测、QML引擎、控制器绑定 |
| 关键文件 | [main.cpp](StabilityAnalyzer_Device/Application/src/main.cpp), [Application.h](StabilityAnalyzer_Device/Application/inc/Application.h) |

**关键类：**

- **`Application`**：应用程序管理类，负责初始化、配置加载、模块生命周期管理
  - `initializeApplication()` / `shutdownApplication()`：应用启停
  - `saveSettings()` / `loadSettings()`：设置持久化

**启动流程（main.cpp）：**

1. 初始化日志系统 `LogManager`
2. 创建 `QtSingleApplication` 防止重复启动
3. 注册 QML 类型 `ExperimentCtrl`
4. 创建 `QQmlApplicationEngine` 和 `CtrllerManager`
5. 将控制器绑定到 QML 上下文
6. 加载 `Application.qml`
7. 延迟启动数据传输连接

---

#### 3.1.2 CommonData（通用数据模块）

| 项目 | 说明 |
|------|------|
| 路径 | `StabilityAnalyzer_Device/CommonData/` |
| 职责 | 定义全局共享的数据结构、专业枚举、字节序转换工具 |
| 依赖 | 无外部模块依赖（基础模块） |

**关键类与结构体：**

- **`CommonData`**（单例，通过 `COMMONDATA` 宏访问）：主窗口管理、事件过滤、大小端转换
  - `BLEndianUint16/32/64()`：大小端字节序转换
  - `reversememcpy()`：逆序内存拷贝
  - `moveWindow()` / `showMin()` / `showMax()` / `closeApp()`：窗口控制

- **`SubjectData`**：专业检测数据封装，每条解析数据一个实例
  - `m_kilometer`：里程标（时空同步后）
  - `m_speed`：速度
  - `m_data`：`QMap<int, double>` 解析后数值数据（字段序号→值）
  - `m_moreData`：`QMap<int, QVariant>` 非double类型数据

- **`TaskInfo`**：任务信息结构体，包含任务ID、名称、线路、站点、速度级、状态等
- **`SubTaskInfo`**：子任务信息，包含行别、方向、车站、里程等
- **`HorizCurveModel`**：圆曲线信息（曲线编号、半径、转向、超高）
- **`StationModel`**：车站信息
- **`SwitchModel`**：道岔信息
- **`LongShortChain`**：长短链信息

- **`SubjectType::EM_Subject`** 枚举：专业类型定义
  - `WheelForce(1)`：轮轨力/车辆动力学
  - `WheelGeometry(2)`：轨道几何
  - `Pantograp(3)`：弓网
  - `Contour(4)`：廓形
  - `TunnelSpect(5)`：隧道综合巡检
  - `LineSpect(6)`：线路巡检
  - `FourC(7)`：4C
  - `WearTear(8)`：磨耗
  - `TrackSpect(10)`：轨道巡检
  - `Communicate(11)`：通信
  - `Signal(12)`：信号
  - `Wayside(13)`：轨旁
  - `RunningGear(14)`：走行部

- **`SubjectEnumOperate`**：专业枚举工具类，提供枚举遍历和名称查询
- **`Transform`**：字节序转换和位操作工具类
- **`DeviceProfile`**：设备配置文件结构体，描述设备型号、通道数量、通道名称

- **`SerialConfig`**：串口通信配置结构体（portName, baudRate, dataBits, parity, stopBits, flowControl）

---

#### 3.1.3 QModbusRTUUnit（Modbus RTU 通信模块）

| 项目 | 说明 |
|------|------|
| 路径 | `StabilityAnalyzer_Device/QModbusRTUUnit/` |
| 职责 | 提供 Modbus RTU 串口通信能力，支持同步/异步操作 |
| 架构 | 三层架构：接口层 → 业务逻辑层 → 通信层 |

**三层架构设计：**

```
┌─────────────────────────────────┐
│  接口层：ModbusClient           │  外部调用的API接口
│  - 同步/异步操作接口            │
│  - 请求ID管理                   │
├─────────────────────────────────┤
│  业务逻辑层：ModbusWorker       │  子线程中运行
│  - 请求队列处理                 │
│  - Modbus协议编解码             │
├─────────────────────────────────┤
│  通信层：ConnectionManager      │  串口通信管理
│  - 串口连接/断开/重连           │
│  - 数据收发                     │
│  - CRC校验                      │
└─────────────────────────────────┘
```

**关键类：**

- **`ModbusClient`**：Modbus客户端主接口类
  - `initialize(ClientConfig)`：初始化客户端
  - `connect()` / `disconnect()`：连接管理
  - 同步操作：`readHoldingRegisters()`, `writeMultipleRegisters()` 等
  - 异步操作：`readHoldingRegistersAsync()`, `writeMultipleRegistersAsync()` 等
  - `cancelRequest()` / `clearQueue()`：请求管理
  - 信号：`requestCompleted(tag, result)`, `connectionStatusChanged(connected)`, `communicationError(error)`

- **`ModbusRtuClient`**：早期版Modbus RTU客户端（直接串口操作）
  - 支持全部标准功能码（0x01-0x10）
  - 支持同步和异步（QFuture）两种模式
  - 内置CRC校验和超时重试

- **`ConnectionManager`**：串口连接管理
  - 重连策略：`NoReconnect`, `ImmediateReconnect`, `IncrementalReconnect`, `ExponentialBackoff`
  - 心跳包机制：定时发送心跳查询，检测连接有效性
  - 通信统计：字节数、错误次数、信号质量评分
  - 自动串口检测和最优配置识别

- **`RequestQueue`**：请求队列管理
  - 多优先级队列：Low(0) → Normal(1) → High(2) → Critical(3) → System(4)
  - 请求状态跟踪：Pending → Queued → Sending → WaitingResponse → Completed/Failed
  - 批量操作、请求过滤、统计信息

- **`ConfigManager`**（Modbus配置管理）：设备配置和系统配置的JSON序列化

**关键类型定义（modbus_types.h）：**

| 类型 | 说明 |
|------|------|
| `ModbusException` | Modbus异常代码（标准+扩展） |
| `ModbusFunctionCode` | 功能码枚举 |
| `RequestPriority` | 请求优先级（0-4） |
| `RequestStatus` | 请求状态枚举 |
| `ConnectionState` | 连接状态枚举 |
| `ModbusResult` | 请求结果结构体 |
| `SerialPortConfig` | 串口配置结构体 |
| `CommunicationStats` | 通信统计信息 |

---

#### 3.1.4 TaskScheduler（任务调度模块）

| 项目 | 说明 |
|------|------|
| 路径 | `StabilityAnalyzer_Device/TaskScheduler/` |
| 职责 | 管理Modbus通信任务的调度执行，包括初始化任务、轮询任务和用户任务 |
| 依赖 | QModbusRTUUnit, CommonData |

**关键类：**

- **`ModbusTaskScheduler`**（单例，通过 `MODBUSTASKSCHEDULER` 宏访问）：任务调度器主类
  - `loadConfigurationFromDirectory()` / `loadConfigurationFromFile()`：从JSON配置加载
  - `startScheduler()` / `stopScheduler()`：调度器控制
  - `executeInitTasks()`：执行初始化任务
  - `executeUserTask()`：执行用户触发任务（支持同步等待）
  - `schedulePollingTask()`：调度轮询任务
  - `startPollingTasks()` / `stopPollingTasks()`：轮询任务生命周期
  - 串口配置缓存和动态重连

- **`TaskQueueManager`**：双队列任务调度系统
  - **高优先级队列（FIFO）**：应用层触发的流程性任务，强顺序执行
  - **轮询队列（FIFO）**：初始化轮询任务和定时任务，配合去重机制
  - 高优先级队列优先执行，空闲时执行轮询队列
  - `TaskExecutionWorker`：在子线程中执行任务，避免阻塞UI

- **`Task`**：Modbus通信任务
  - 从JSON配置创建：`createFromJson()`
  - 任务类型：`INIT_TASK`（启动自动执行）, `USER_TASK`（用户触发）
  - Modbus参数：功能码、起始地址、数量、写入数据
  - 执行控制：`execute()`, `cancel()`

- **`PortManager`**：串口管理器
  - 串口连接/断开管理
  - 设备注册和查找
  - 为每个串口创建独立的 `ModbusClient` 实例

- **`Device`**：Modbus设备抽象
  - 设备属性：ID、名称、端口
  - 任务列表管理
  - 连接状态跟踪

---

#### 3.1.5 Algorithm（算法模块）

| 项目 | 说明 |
|------|------|
| 路径 | `StabilityAnalyzer_Device/Algorithm/` |
| 职责 | 信号处理算法，包括FFT、滤波、矩阵运算、数学变换 |

**子模块结构：**

```
Algorithm/
├── inc/
│   ├── common/          # 基础数学工具
│   │   ├── common.h     # 通用数学函数（浮点比较、极值、数组打印）
│   │   ├── fft.h        # 快速傅里叶变换
│   │   ├── matrix.h     # 矩阵模板类
│   │   └── transform.h  # 数学变换
│   ├── frequencyAnalysis/
│   │   └── filter.h     # FIR滤波器
│   └── timeAnalysis/
│       └── timeanalysis.h  # 时域分析
└── src/
    ├── common/          # 基础数学工具实现
    └── ...
```

**关键类：**

- **`matrix<_Ty>`**：矩阵模板类
  - 基于 `std::valarray` 实现
  - 支持运算符重载：`+`, `-`, `*`, `/`
  - 类型别名：`matrixf`, `matrixd`, `matrixld`
  - 矩阵运算：乘法、转置、行列式、秩、逆矩阵、LU分解、QR分解、SVD分解、Cholesky分解

- **`Filter`**：FIR数字滤波器
  - 滤波器类型：低通、高通、带通、带阻
  - 窗口类型：矩形、图基、三角、汉宁、汉明、布拉克曼、凯塞
  - `Convolution()`：卷积运算
  - `fft()`：时域转频域
  - `fliter()`：滤波处理

- **`TimeAnalysis`**：时域分析
  - `InputSource()`：输入源数据
  - `DealProcess()`：处理流程
  - `OutputSource()`：输出结果

- **`FFT`**：快速傅里叶变换类

- **数学变换函数**（transform.h）：
  - `FourierSeriesApproach()`：傅里叶级数逼近
  - `FourierTransform()`：快速傅里叶变换
  - `WalshTransform()`：沃尔什变换
  - `Smooth5_3()`：五点三次平滑
  - `SievingKalman()`：卡尔曼滤波
  - `SievingABR()`：α-β-γ滤波

---

#### 3.1.6 DataTransmit（数据传输模块）

| 项目 | 说明 |
|------|------|
| 路径 | `StabilityAnalyzer_Device/DataTransmit/` |
| 职责 | 设备端RNDIS网络传输管理 |

**关键类：**

- **`RndisManager`**：RNDIS网络管理器，处理设备与PC之间的网络数据传输

---

#### 3.1.7 DataSaver（数据存储模块）

| 项目 | 说明 |
|------|------|
| 路径 | `StabilityAnalyzer_Device/DataSaver/` |
| 职责 | 专业数据分类存储、历史数据清理 |
| 依赖 | CommonData |

**关键类：**

- **`DataSaver_Base`**：专业独立存储基类
  - 存储策略：原始数据 → 拼接SQL → 缓存多条SQL → 事务写入临时表 → 定时从临时表取出执行
  - `start()`：启动存储
  - `createDataSqlList()`：纯虚函数，派生类实现不同专业的SQL生成
  - 定时存储（默认100ms间隔）

- **`SubjectClassification`**：数据分类分发器
  - 接收通信原始数据或算法数据
  - 将各专业数据发送至对应的存储模块

- **`WheelGeometry_Save`**：轨道几何数据存储（派生自DataSaver_Base）

- **`DataClearHandler`**：历史数据清理
  - 应用启动时执行
  - 清除10次任务之前的已上传数据
  - 按日期、上传状态等条件清理

---

#### 3.1.8 SqlOrm（数据库ORM模块）

| 项目 | 说明 |
|------|------|
| 路径 | `StabilityAnalyzer_Device/SqlOrm/` |
| 职责 | SQLite数据库ORM操作 |
| 依赖 | sqlite_orm |

**关键类：**

- **`SqlOrmManager`**（单例，通过 `SQLORM` 宏访问）：数据库管理器
  - 事务管理：`beginTransaction()`, `commitTransaction()`, `rollbackTransaction()`
  - 用户管理：增删改查、登录验证
  - 项目管理：增删改查、按创建者查询
  - 实验管理：增删改查、状态更新、按状态/操作者/项目查询
  - 实验数据管理：增删改查、批量操作、按范围查询、按扫描ID查询
  - 操作日志管理：增删查、按用户/类型/时间范围查询、旧日志清理
  - 线程安全（双重检查锁定）

---

#### 3.1.9 FileExporter（文件导出模块）

| 项目 | 说明 |
|------|------|
| 路径 | `StabilityAnalyzer_Device/FileExporter/` |
| 职责 | 多格式数据导出（Excel、PDF、文本） |
| 依赖 | QtXlsxWriter（内嵌） |

**关键类：**

- **`ExportManager`**（单例，通过 `EXPORTMANAGER` 宏访问）：导出管理器
  - `registerExporter()` / `unregisterExporter()`：注册/注销导出器
  - `exportToFile()`：通用导出方法
  - 策略模式，通过 `IExportInterface` 接口支持多种格式

- **`IExportInterface`**：导出接口（纯虚类）
  - `exportToFile()`：导出数据到文件
  - `getSupportedFormats()`：获取支持的格式

- **`ExcelExporter`** / **`PdfExporter`** / **`TextExporter`**：具体导出器实现

- **QtXlsxWriter**：内嵌的Excel文件操作库（xlsx子目录）

---

#### 3.1.10 ConfigManager（配置管理模块）

| 项目 | 说明 |
|------|------|
| 路径 | `StabilityAnalyzer_Device/ConfigManager/` |
| 职责 | 多格式配置文件读写（INI、JSON、XML） |

**关键类：**

- **`ConfigManager`**（单例，通过 `CONFIGMANAGER` 宏访问）：配置管理器
  - `load()` / `save()`：加载/保存配置文件
  - `getValue()` / `setValue()`：读写配置值
  - `detectFormat()`：根据文件扩展名自动检测格式
  - 策略模式，通过 `IConfigInterface` 接口支持多种格式

- **`IConfigInterface`**：配置接口（纯虚类）
- **`IniConfig`** / **`JsonConfig`** / **`XmlConfig`**：具体配置处理器

---

#### 3.1.11 QCuteLogger（日志模块）

| 项目 | 说明 |
|------|------|
| 路径 | `StabilityAnalyzer_Device/QCuteLogger/` |
| 职责 | 日志记录与管理 |

**关键类：**

- **`Logger`**：日志记录核心类
- **`LogManager`**：日志管理器，初始化日志系统
- **`AbstractAppender`**：日志输出目标基类
- **`FileAppender`** / **`RollingFileAppender`**：文件输出
- **`ConsoleAppender`**：控制台输出
- **`OutputDebugAppender`**：调试输出

---

#### 3.1.12 LoggerMonitor（日志监控模块）

| 项目 | 说明 |
|------|------|
| 路径 | `StabilityAnalyzer_Device/LoggerMonitor/` |
| 职责 | 日志监控与可视化展示 |

**关键类：**

- **`LoggerMonitor`**：日志监控核心类
- **`LoggerMonitorWidget`**：日志监控UI组件

---

#### 3.1.13 QSingleapplication（单例应用模块）

| 项目 | 说明 |
|------|------|
| 路径 | `StabilityAnalyzer_Device/QSingleapplication/` |
| 职责 | 保证程序不会重复运行 |

**关键类：**

- **`QtSingleApplication`**：单实例应用类，基于 `QtLocalPeer` 实现进程间通信

---

#### 3.1.14 Quazip（压缩解压模块）

| 项目 | 说明 |
|------|------|
| 路径 | `StabilityAnalyzer_Device/Quazip/` |
| 职责 | ZIP压缩/解压操作 |

**关键类：**

- **`QuaZip`** / **`QuaZipFile`**：ZIP文件操作
- **`JlCompress`**：便捷压缩/解压工具

---

#### 3.1.15 Plot（图表绘制模块）

| 项目 | 说明 |
|------|------|
| 路径 | `StabilityAnalyzer_Device/plot/` |
| 职责 | 实时曲线绘制、频谱分析、示波器显示 |
| 架构 | MVC模式：Model → Drawer → View |

**子模块结构：**

```
plot/
├── model/           # 数据模型层
│   ├── axismodel.h         # 坐标轴模型
│   ├── axislistmodel.h     # 坐标轴列表模型
│   ├── curvemodel.h        # 曲线模型
│   ├── curvelistmodel.h    # 曲线列表模型
│   ├── cursormodel.h       # 游标模型
│   ├── gridmodel.h         # 网格模型
│   └── ...
├── drawer/          # 绘制层
│   ├── axisdrawer.h        # 坐标轴绘制器
│   ├── curvedrawer.h       # 曲线绘制器
│   ├── cursordrawer.h      # 游标绘制器
│   └── tickers.h           # 刻度计算
├── view/            # 视图层
│   ├── PlotWidget.h        # 图表控件
│   ├── spectrumview.h      # 频谱视图
│   ├── cursorview.h        # 游标视图
│   └── cursoritem.h        # 游标项
└── 核心类
    ├── baseplot.h          # 图表基类
    ├── oscilloscope.h      # 示波器（QQuickPaintedItem）
    ├── realtimeplot.h      # 实时图表
    ├── analysisplot.h      # 分析图表
    ├── multifunctionplot.h # 多功能图表
    └── DataPipeControl.h   # 数据管道控制
```

**关键类：**

- **`BasePlot`**：图表基类
  - MVC架构：`AxisListModel` + `CurveListModel` + `GridModel`
  - 绘制器：`AxisDrawer` + `CurveDrawer` + `CursorDrawer`
  - 画布分层：坐标轴画布 → 曲线画布 → 游标画布
  - 交互：框选缩放、回退、移动、悬停提示

- **`Oscilloscope`**：示波器控件（QML集成）
  - 继承 `QQuickPaintedItem`
  - 独立绘制线程
  - 支持鼠标交互（双击、悬停）

- **`DataPipeControl`**（单例）：数据管道控制
  - 管理轮轨力（10通道）、平稳性（7通道）、轨道几何（11通道）数据
  - 时空同步数据处理
  - 数据推送和同步机制

---

#### 3.1.16 SubApplication（子应用模块）

| 项目 | 说明 |
|------|------|
| 路径 | `StabilityAnalyzer_Device/SubApplication/` |
| 职责 | 子应用容器，包含ANALYZER等子应用 |

---

### 3.2 StabilityAnalyzer_PC 模块

PC端在Device端模块基础上，额外包含以下核心模块：

#### 3.2.1 DataTransmit（PC端数据传输）

| 项目 | 说明 |
|------|------|
| 路径 | `StabilityAnalyzer_PC/DataTransmit/` |
| 职责 | PC侧RNDIS+TCP通信总控 |

**关键类：**

- **`DataTransmitController`**：数据传输控制器
  - 连接状态机：`INIT → WAIT_DEVICE → WAIT_ADAPTER → CONFIGURE_IP → WAIT_DEVICE_READY → CONNECT_CONTROL → CONNECT_STATUS → CONNECT_STREAM → ONLINE`
  - 三路TCP通道：
    - `ControlChannelClient`：控制命令通道
    - `StatusChannelClient`：状态查询通道
    - `StreamChannelClient`：数据流通道
  - RNDIS网卡自动发现和配置
  - 自动重连机制
  - 设备心跳检测
  - UI连接状态与后台重连状态解耦

- **`TcpChannelClient`**：TCP通道客户端基类

**连接状态枚举：**

| 状态 | 说明 |
|------|------|
| INIT | 初始化 |
| WAIT_DEVICE | 等待设备 |
| WAIT_ADAPTER | 等待网卡 |
| CONFIGURE_IP | 配置IP |
| WAIT_DEVICE_READY | 等待设备就绪 |
| CONNECT_CONTROL | 连接控制通道 |
| CONNECT_STATUS | 连接状态通道 |
| CONNECT_STREAM | 连接数据流通道 |
| ONLINE | 在线 |
| RECONNECTING | 重连中 |

---

#### 3.2.2 Controller Layer（控制器层）

| 项目 | 说明 |
|------|------|
| 路径 | `StabilityAnalyzer_PC/SubApplication/ANALYZER/MainWindow/inc/Controller/` |
| 职责 | MVC控制器层，连接QML前端与后端业务逻辑 |

**关键类：**

- **`CtrllerManager`**：控制器管理器（中央控制类）
  - 聚合所有子控制器
  - `bindToQmlContext()`：将所有控制器绑定到QML上下文
  - 子控制器：
    - `DataTransmitController`：数据传输
    - `systemSettingCtrl`：系统设置
    - `userCtrl`：用户管理
    - `dataCtrl`：数据管理
    - `ExperimentCtrl`：实验管理
    - `reportCtrl`：报告管理
    - `realtimeCtrl`：实时监控
    - `detailCtrl`：详情查看
    - `compareCtrl`：数据对比

- **`ExperimentCtrl`**：实验控制器
  - 多通道实验管理（ChannelA/B/C/D）
  - `startExperiment()` / `stopExperiment()`：实验启停
  - Modbus调度器集成：`initializeScheduler()`, `connectModbusDevice()`
  - 扫描定时器和实验超时管理
  - 通过 `DataTransmitController` 转发控制命令到设备端

- **`dataCtrl`**：数据控制器
  - 工程管理：创建、查询
  - 实验管理：状态更新、导入（从设备端）、软删除/恢复/彻底删除
  - 实验数据管理：增删查、批量操作、图表数据获取
  - 高级计算：均匀度、光强平均、分层厚度、峰厚度、颗粒迁移速度、流体力学、光学计算
  - 后台导入线程支持

- **`userCtrl`**：用户控制器
  - 用户CRUD、登录/登出
  - 当前用户状态管理（用户名、角色、登录状态）
  - 操作日志记录

- **`systemSettingCtrl`**：系统设置控制器
  - 语言切换（国际化）
  - 亮度调节
  - WiFi管理（扫描、连接、断开、信号强度监测）
  - 日期时间管理
  - 系统升级

---

#### 3.2.3 Analysis Engines（分析引擎）

| 项目 | 说明 |
|------|------|
| 路径 | `StabilityAnalyzer_PC/SubApplication/ANALYZER/MainWindow/inc/Analysis/` |
| 职责 | 高级数据分析计算 |

**关键类：**

- **`CurveChartAnalysisEngine`**：曲线图表分析引擎
- **`AdvancedCalculationEngine`**：高级计算引擎
- **`LightCurveAnalysisEngine`**：光曲线分析引擎

---

#### 3.2.4 PC端额外算法模块

PC端Algorithm模块比Device端多以下子模块：

- **`orderAnalysis/`**：阶次分析
- **`interfaceParam/`**：接口参数
- **`envelopeAnalysis/`**：包络分析（希尔伯特变换）
- **`encryptionAlgorithm/`**：加密算法
- **`frequencyAnalysis/`**：频域分析（频率分析、傅里叶变换）

---

## 4. 模块依赖关系

### 4.1 Device端模块依赖图

```
Application
├── QCuteLogger
├── QSingleapplication
├── CommonData ◄─────── (基础模块，被多个模块依赖)
├── LoggerMonitor
│   └── QCuteLogger
├── QModbusRTUUnit
├── Algorithm
├── DataTransmit
├── DataSaver
│   └── CommonData
├── SqlOrm
├── TaskScheduler
│   ├── QModbusRTUUnit
│   └── CommonData
├── SubApplication
│   └── ANALYZER
└── Application
```

### 4.2 PC端模块依赖图

```
Application (PC)
├── QCuteLogger
├── QSingleapplication
├── CommonData
├── LoggerMonitor
├── QModbusRTUUnit
├── Algorithm (扩展版)
├── DataTransmit (PC版，含DataTransmitController)
│   └── TCP通道客户端
├── DataSaver
├── SqlOrm
├── TaskScheduler
├── ConfigManager
├── FileExporter
├── Quazip
├── plot
└── SubApplication/ANALYZER
    └── MainWindow
        ├── Controller层 (9个控制器)
        ├── Analysis引擎
        ├── DataModel
        └── QML页面
```

### 4.3 关键依赖说明

| 模块 | 被依赖者 | 依赖类型 |
|------|----------|----------|
| CommonData | TaskScheduler, DataSaver, plot | 数据结构共享 |
| QModbusRTUUnit | TaskScheduler | Modbus通信能力 |
| SqlOrm | dataCtrl, userCtrl | 数据库操作 |
| DataTransmitController | ExperimentCtrl, dataCtrl, realtimeCtrl | 设备通信 |
| QCuteLogger | LoggerMonitor, Application | 日志基础设施 |

---

## 5. 构建与运行

### 5.1 构建系统

项目使用 **qmake** 构建系统，通过 `.pro` 文件管理项目配置。

**主项目文件：** `StabilityAnalyzer_Device/StabilityAnalyzer.pro`

```qmake
TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS += \
    QCuteLogger \
    QSingleapplication \
    CommonData \
    LoggerMonitor \
    QModbusRTUUnit \
    Algorithm \
    DataTransmit \
    SqlOrm \
    TaskScheduler \
    SubApplication \
    Application
```

**通用编译配置：** `CommonBase.pri`
- C++11 标准
- 同时生成 Debug 和 Release 版本
- MSVC：UTF-8编码，Release带调试信息
- MinGW：Release带调试信息，禁止优化
- 输出目录：`bin-msvc/` 或 `bin-mingw/`

### 5.2 构建步骤

1. 安装 Qt 5（需包含 Qt Serial Port、Qt SQL 模块）
2. 使用 Qt Creator 打开 `StabilityAnalyzer.pro`
3. 配置编译套件（MSVC 或 MinGW）
4. 执行 qmake → 构建

### 5.3 运行配置

**配置文件结构：**

```
bin-mingw/
├── config/
│   ├── sys_config/
│   │   └── config.ini          # 系统配置（语言、亮度、版本）
│   └── device_profile.json     # 设备配置（型号、通道数、通道名）
└── data/
    └── app_database.db         # SQLite数据库
```

**串口配置（config.ini）：**

```ini
[Serial]
PortName=COM1
BaudRate=9600
DataBits=8
Parity=none
StopBits=1

[Communication]
ResponseTimeout=3000
RetryCount=3
AutoReconnect=true
```

**设备配置（device_profile.json）：**

```json
{
    "deviceModel": "four_tower",
    "channelCount": 4,
    "channelNames": ["A", "B", "C", "D"]
}
```

### 5.4 实验设备通道配置

设备通道配置模板位于 `docs/config_templates_v1/`，每个通道一个JSON文件（ChannelA/B/C/D），定义了：
- 设备标识（slaveId）
- 串口配置
- 任务列表（初始化任务 + 轮询任务 + 用户任务）

---

## 6. 通信协议

### 6.1 Modbus RTU 通信

设备端通过 Modbus RTU 协议与传感器通信，支持的功能码：

| 功能码 | 名称 | 说明 |
|--------|------|------|
| 0x01 | Read Coils | 读线圈状态 |
| 0x02 | Read Discrete Inputs | 读离散输入 |
| 0x03 | Read Holding Registers | 读保持寄存器 |
| 0x04 | Read Input Registers | 读输入寄存器 |
| 0x05 | Write Single Coil | 写单个线圈 |
| 0x06 | Write Single Register | 写单个寄存器 |
| 0x0F | Write Multiple Coils | 写多个线圈 |
| 0x10 | Write Multiple Registers | 写多个寄存器 |

### 6.2 PC-Device TCP 通信

PC端与设备端通过 RNDIS 网络建立三路TCP通道：

| 通道 | 用途 |
|------|------|
| ControlChannel | 控制命令（启停实验、参数配置） |
| StatusChannel | 状态查询（设备状态、通道信息） |
| StreamChannel | 数据流（实验数据实时传输） |

通信格式为 JSON，基线规范参见 `docs/comm_baseline_v1.md`。

---

## 7. 设计模式与架构特点

### 7.1 使用的设计模式

| 模式 | 应用场景 |
|------|----------|
| 单例模式 | CommonData, SqlOrmManager, ModbusTaskScheduler, DataPipeControl, ConfigManager, ExportManager |
| 策略模式 | ConfigManager（INI/JSON/XML）, ExportManager（Excel/PDF/Text） |
| 三层架构 | QModbusRTUUnit（接口层→业务逻辑层→通信层） |
| MVC模式 | Plot模块（Model→Drawer→View） |
| 观察者模式 | Qt信号槽机制，贯穿全项目 |
| 生产者-消费者 | TaskQueueManager（双队列调度） |
| 工作线程模式 | TaskExecutionWorker, DataSaver_Base |
| 状态机模式 | DataTransmitController（连接状态机） |

### 7.2 线程模型

| 线程 | 职责 |
|------|------|
| 主线程 | UI渲染、QML事件循环 |
| ModbusWorker线程 | Modbus协议处理、请求队列管理 |
| TaskExecution线程 | 任务执行（避免阻塞UI） |
| DataSaver线程 | 数据存储（每个专业独立线程） |
| Plot绘制线程 | 曲线渲染 |
| 设备导入线程 | 后台导入设备实验数据 |

### 7.3 关键架构特点

1. **双端分离**：Device端负责数据采集和传输，PC端负责数据管理和分析，通过RNDIS/TCP通信
2. **模块化设计**：每个模块独立为 `.pro` 工程，通过 `.pri` 文件共享头文件和源文件
3. **全局宏访问**：`COMMONDATA`, `SQLORM`, `MODBUSTASKSCHEDULER`, `CONFIGMANAGER`, `EXPORTMANAGER` 等宏简化单例访问
4. **QML上下文绑定**：控制器通过 `setContextProperty()` 暴露给QML，实现前后端解耦
5. **双队列调度**：高优先级队列保证流程性任务顺序执行，轮询队列处理周期性采集
6. **画布分层渲染**：Plot模块采用坐标轴→曲线→游标三层画布叠加渲染

---

## 8. 开发工具

### 8.1 虚拟设备联调

位于 `tools/virtual_device/`，提供虚拟串口环境用于联调测试：

- `linux_virtual_lower_device.py`：Linux虚拟下位机
- `generate_experiment_config.py`：生成实验配置
- `list_serial_ports.py`：列出可用串口

详见 `tools/virtual_device/README.md`。

### 8.2 调试配置

- Windows平台内置崩溃转储（MiniDump）
- Release版本保留调试信息
- 日志输出到 `./log/` 目录
