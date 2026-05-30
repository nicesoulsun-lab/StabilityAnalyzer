# 实验数据存储优化方案

## 1. 背景与问题

### 1.1 当前存储方式

当前 `experiment_data` 表采用**一个数据点一条记录**的存储方式：

```sql
CREATE TABLE experiment_data (
    id                        INTEGER PRIMARY KEY AUTOINCREMENT,
    experiment_id             INTEGER NOT NULL,
    timestamp                 INTEGER NOT NULL,
    scan_id                   INTEGER NOT NULL DEFAULT 0,
    scan_elapsed_ms           INTEGER NOT NULL DEFAULT 0,
    height                    REAL    NOT NULL,
    backscatter_intensity     REAL    NOT NULL,
    transmission_intensity    REAL    NOT NULL
);
```

每次扫描最多产生 **2750 个数据点**（扫描范围 0~55mm，步长 20μm），即 2750 条数据库记录。

### 1.2 性能问题

| 环节 | 问题描述 |
|---|---|
| **设备端入库** | 每次扫描需执行 2750 次 INSERT，即使使用事务批量插入，bindValue + exec 的循环开销仍然巨大 |
| **设备端导出** | `get_experiment_scan_export` 按 offset/limit 分页查询，单扫描 2750 行需 2 轮网络往返；OFFSET 查询在大表上 B-tree 遍历代价高 |
| **PC 端导入** | 逐扫描分页取数据，每条都是独立 QVariantMap，序列化/反序列化开销大；百万行 INSERT + COMMIT 耗时极长 |
| **分析查询** | 光强平均值、均匀度等查询在百万行表上做 `GROUP BY scan_id` + `AVG()`，全表扫描代价高 |

### 1.3 实测数据

| 场景 | 实测耗时 |
|---|---|
| 600 次扫描实验，PC 完整导入 | **40 分钟** |
| 每扫描平均耗时 | **4 秒** |
| 每轮网络请求平均耗时 | **2 秒** |

按此推算，1000 次扫描实验的导入时间约为 **75~85 分钟**，严重影响用户体验。

---

## 2. 下位机数据格式与顺序（关键约束）

### 2.1 Modbus 寄存器布局

下位机通过 Modbus INPUT_REGISTERS 提供扫描数据，每个通道有 A/B 两个存储区：

| 存储区 | 寄存器地址范围 | 读取任务 | 最大字数 |
|---|---|---|---|
| A 区 | 0~499 | read_scan_data_a_0 ~ read_scan_data_a_400 | 500 words |
| B 区 | 500~999 | read_scan_data_b_500 ~ read_scan_data_b_900 | 500 words |

每个读取任务读 100 个寄存器（words），A 区 5 个任务、B 区 5 个任务，共 1000 words。

### 2.2 交替排列：透射光在前，背射光在后

**每个采样点占 2 个连续寄存器**，排列顺序为：

```
寄存器地址:  [0]     [1]     [2]     [3]     [4]     [5]     ...
             透射0   背射0   透射1   背射1   透射2   背射2   ...
             |--- 点0 ---|    |--- 点1 ---|    |--- 点2 ---|
```

对应代码（[parseStoragePairs](file:///d:/workspace/project/StabilityAnalyzer/StabilityAnalyzer_Device/SubApplication/ANALYZER/MainWindow/src/Common/experiment_session_service.cpp#L298)）：

```cpp
const int rawTransmission = raw[(i * 2)];      // 偶数位 = 透射光
const int rawBackscatter  = raw[(i * 2) + 1];  // 奇数位 = 背射光
```

> **注意**：透射光在前（偶数索引），背射光在后（奇数索引）。这个顺序在新方案中必须严格保持。

### 2.3 高度与点索引的对应关系

高度由点索引（`pointIndex`）决定，`pointIndex` 在扫描上下文中从 0 开始递增：

```cpp
const int pointIndex = startPointIndex + i;
const double heightUm = startHeightUm + (static_cast<double>(pointIndex) * stepUm);
```

关键约束：
- **`startPointIndex`** = `context.savedPointCount`，即本扫描已保存的点数
- 数据可能跨 A/B 存储区、跨多轮 fetch 到达，`savedPointCount` 逐批累积
- **点索引 0 对应最低高度**，点索引递增对应高度递增
- `intensity_values` 字符串中第 i 个值对应的高度 = `start_height_mm + i * step_um / 1000.0`

### 2.4 数据跨存储区到达的时序

一次扫描的数据可能分多批到达：

```
第1批: A区 0~99 寄存器 → 50 个点 (pointIndex 0~49)
第2批: A区 100~199 寄存器 → 50 个点 (pointIndex 50~99)
...
第N批: B区 500~599 寄存器 → 50 个点 (pointIndex 250~299)
```

每批到达时：
1. `buildRowsFn` 被调用，解析当前批次的原始寄存器
2. `savedPointCount` 累积，用于下一批的 `startPointIndex`
3. **入库**：新方案中，仅在 `savedPointCount >= expectedPointCount`（扫描完成）时才拼接字符串并写入 DB
4. **推送**：每批到达时立即逐点推送给 PC 端（Stream 9002）

---

## 3. 优化方案概述

### 3.1 核心思路

**一次扫描存储两条记录**：背射光一条、透射光一条。数据以逗号分隔的字符串存储，高度由起始高度和步长隐式计算。

### 3.2 关键设计决策

| 决策 | 选择 | 理由 |
|---|---|---|
| 存储粒度 | 一扫描两行 | 将行数从 2750 降至 2，减少 99.93% |
| 数据序列化 | 逗号分隔字符串 | 简单、紧凑、可读，解析性能可接受 |
| 高度存储 | start_height_mm + step_um 隐式计算 | 扫描为等间距，无需逐行存储 |
| 旧数据兼容 | 完全舍弃旧格式 | 无需迁移，代码彻底清理 |

---

## 4. 新表结构

### 4.1 `experiment_scan_data` 表

```sql
CREATE TABLE experiment_scan_data (
    id                INTEGER PRIMARY KEY AUTOINCREMENT,
    experiment_id     INTEGER NOT NULL,
    scan_id           INTEGER NOT NULL,
    timestamp         INTEGER NOT NULL,
    scan_elapsed_ms   INTEGER NOT NULL,
    light_type        INTEGER NOT NULL,   -- 0=背射, 1=透射
    start_height_mm   REAL    NOT NULL,   -- 起始高度(mm)
    step_um           REAL    NOT NULL,   -- 步长(μm)
    point_count       INTEGER NOT NULL,   -- 实际点数(≤2750)
    intensity_values  TEXT    NOT NULL    -- 逗号分隔的光强值，按高度递增排列
);

CREATE INDEX idx_scan_data_exp
    ON experiment_scan_data(experiment_id);

CREATE INDEX idx_scan_data_exp_scan
    ON experiment_scan_data(experiment_id, scan_id);

CREATE INDEX idx_scan_data_exp_scan_type
    ON experiment_scan_data(experiment_id, scan_id, light_type);
```

### 4.2 C++ 结构体

```cpp
struct ExperimentScanData {
    int id;
    int experiment_id;
    int scan_id;
    int timestamp;
    int scan_elapsed_ms;
    int light_type;           // 0=背射, 1=透射
    double start_height_mm;
    double step_um;
    int point_count;
    QString intensity_values; // "1234.56,2345.67,..." 按高度递增排列
};
```

### 4.3 intensity_values 的顺序约定

**intensity_values 中第 i 个值对应的高度为：**

```
height_mm = start_height_mm + i * step_um / 1000.0
```

即：
- `i=0` → 最低高度（`start_height_mm`）
- `i=1` → `start_height_mm + step_um / 1000.0`
- ...
- `i=point_count-1` → 最高高度

**这个顺序与当前 `parseStoragePairs` 中 `pointIndex` 从 0 递增的顺序完全一致。**

### 4.4 数据量对比

| 指标 | 旧方案（一点一行） | 新方案（一扫描两行） | 倍率 |
|---|---|---|---|
| 单扫描行数 | 2750 | 2 | 1/1375 |
| 100 扫描实验行数 | 275,000 | 200 | 1/1375 |
| 1000 扫描实验行数 | 2,750,000 | 2,000 | 1/1375 |
| 单扫描存储量 | ~220 KB | ~66 KB | ~1/3 |
| 1000 扫描实验存储量 | ~880 MB | ~13 MB | ~1/68 |

### 4.5 intensity_values 字段容量分析

- 2750 个 double 值，每个约 6~12 字符（如 `1234.56`）
- 逗号分隔：2750 × 8 + 2749 ≈ **24 KB**
- SQLite TEXT 最大 1 GB，完全无压力
- `QString::split(',')` + `toDouble()` 解析 2750 个值耗时 < 1ms

---

## 5. 数据流设计

### 5.1 设备端入库流程

#### 当前流程

```
原始寄存器 → parseStoragePairs() → 逐点生成 QVariantMap(2750条/扫描)
                                         ↓
                               batchSaveExperimentData()
                                         ↓
                               INSERT 2750行/扫描（事务批量）
```

#### 新流程

```
原始寄存器 → buildScanRowsFromStorageData() → 累积点数据
                                                    ↓
                                          扫描完成时拼接字符串
                                                    ↓
                                          输出 2条QVariantMap/扫描
                                                    ↓
                                          batchAddExperimentScanData()
                                                    ↓
                                          INSERT 2行/扫描（事务批量）
```

#### 核心改动：ExperimentSessionService

新增方法 `buildScanRowsFromStorageData`，替代现有的 `buildRowsFromStorageData`。

**关键设计**：由于数据跨存储区分批到达，需要**跨批次累积**背射/透射光强值，扫描完成时一次性拼接。

```cpp
// 扫描上下文扩展：增加累积缓冲区
struct ScanCycleContext {
    int sequence = 0;
    int scanId = 0;
    int expectedPointCount = 0;
    int savedPointCount = 0;
    double startHeightUm = 0.0;
    double stepUm = 20.0;
    qint64 startedAtMs = 0;
    qint64 elapsedSinceExperimentStartMs = 0;

    // 新增：跨批次累积的光强值（按高度递增顺序）
    QStringList backscatterAccum;    // 累积的背射光校准值
    QStringList transmissionAccum;   // 累积的透射光校准值
};

QVector<QVariantMap> ExperimentSessionService::buildScanRowsFromStorageData(
    int channel,
    const QVector<quint16>& raw,
    bool areaA)
{
    QVector<QVariantMap> result;
    QVector<ScanCycleContext>& contexts = m_scanContexts[channel];
    const int totalPairs = raw.size() / 2;
    int consumedPairs = 0;

    while (consumedPairs < totalPairs) {
        // 扫描上下文匹配逻辑同现有 buildRowsFromStorageData
        while (!contexts.isEmpty() &&
               contexts.first().savedPointCount >= contexts.first().expectedPointCount) {
            contexts.remove(0);
        }

        if (contexts.isEmpty()) {
            qWarning() << "[ExperimentSessionService][Fetch] channel=" << channel
                       << "drop pairs because no pending scan context";
            break;
        }

        ScanCycleContext& context = contexts[0];
        const int remainingPoints = qMax(0, context.expectedPointCount - context.savedPointCount);
        if (remainingPoints <= 0) continue;

        const int takePairs = qMin(remainingPoints, totalPairs - consumedPairs);

        // 逐点解析，保持与 parseStoragePairs 完全一致的顺序
        for (int i = 0; i < takePairs; ++i) {
            // 关键：透射光在偶数位，背射光在奇数位
            const int rawTransmission = raw[(consumedPairs + i) * 2];
            const int rawBackscatter  = raw[(consumedPairs + i) * 2 + 1];

            // 高度计算：pointIndex = savedPointCount + i，与现有逻辑一致
            const int pointIndex = context.savedPointCount + i;
            const double heightUm = context.startHeightUm
                + static_cast<double>(pointIndex) * context.stepUm;

            // 校准转换逻辑同现有 parseStoragePairs
            const double calTransRef = findCalibrationAvgTransmission(channel, heightUm);
            const double calBackRef  = findCalibrationAvgBackscatter(channel, heightUm);

            const double transIntensity = calTransRef > 0.0
                ? static_cast<double>(rawTransmission) / calTransRef * 100.0
                : static_cast<double>(rawTransmission);
            const double backIntensity = calBackRef > 0.0
                ? static_cast<double>(rawBackscatter) / calBackRef * 100.0
                : static_cast<double>(rawBackscatter);

            // 按高度递增顺序追加到累积缓冲区
            context.backscatterAccum.append(QString::number(backIntensity, 'f', 2));
            context.transmissionAccum.append(QString::number(transIntensity, 'f', 2));
        }

        context.savedPointCount += takePairs;
        consumedPairs += takePairs;

        // 仅在扫描完成时输出记录
        if (context.savedPointCount >= context.expectedPointCount) {
            const int pointCount = context.savedPointCount;
            const int baseTs = static_cast<int>(context.startedAtMs / 1000);

            QVariantMap bsRow;
            bsRow["scan_id"] = context.scanId;
            bsRow["timestamp"] = baseTs;
            bsRow["scan_elapsed_ms"] = static_cast<int>(context.elapsedSinceExperimentStartMs);
            bsRow["light_type"] = 0;  // 背射
            bsRow["start_height_mm"] = context.startHeightUm / 1000.0;
            bsRow["step_um"] = context.stepUm;
            bsRow["point_count"] = pointCount;
            bsRow["intensity_values"] = context.backscatterAccum.join(',');

            QVariantMap tRow;
            tRow["scan_id"] = context.scanId;
            tRow["timestamp"] = baseTs;
            tRow["scan_elapsed_ms"] = static_cast<int>(context.elapsedSinceExperimentStartMs);
            tRow["light_type"] = 1;  // 透射
            tRow["start_height_mm"] = context.startHeightUm / 1000.0;
            tRow["step_um"] = context.stepUm;
            tRow["point_count"] = pointCount;
            tRow["intensity_values"] = context.transmissionAccum.join(',');

            result.append(bsRow);
            result.append(tRow);

            const ScanCycleContext completed = contexts.first();
            contexts.remove(0);
            qDebug() << "[ExperimentSessionService][ScanCycle] channel=" << channel
                     << "scanId=" << completed.scanId
                     << "completed"
                     << "totalSavedPoints=" << completed.savedPointCount;
        }
    }

    refreshCurrentScanCount(channel);
    return result;
}
```

#### ExperimentDataService 适配

```cpp
void ExperimentDataService::batchSaveExperimentScanData(
    int experimentId, const QVector<QVariantMap>& scanRows) const
{
    if (!m_dbManager || scanRows.isEmpty()) {
        return;
    }

    QVector<QVariantMap> payload;
    payload.reserve(scanRows.size());
    for (const QVariantMap& item : scanRows) {
        QVariantMap row = item;
        row["experiment_id"] = experimentId;
        payload.append(row);
    }
    m_dbManager->batchAddExperimentScanData(payload);
}
```

### 5.2 设备端导出协议

#### 当前协议

`get_experiment_scan_export` 按扫描分页，每页最多 2000 行，单扫描 2750 行需 2 轮网络往返。

#### 新协议

单扫描仅 2 行，一次返回，无需分页。

**请求**：

```json
{
    "command": "get_experiment_scan_export",
    "experiment_id": 1,
    "scan_id": 0,
    "offset": 0,
    "limit": 0
}
```

**响应**：

```json
{
    "type": "command_result",
    "success": true,
    "experiment_id": 1,
    "scan_id": 0,
    "total_count": 2,
    "has_more": false,
    "data": [
        {
            "experiment_id": 1,
            "scan_id": 0,
            "timestamp": 1717000000,
            "scan_elapsed_ms": 0,
            "light_type": 0,
            "start_height_mm": 0.0,
            "step_um": 20.0,
            "point_count": 2750,
            "intensity_values": "12.34,23.45,34.56,..."
        },
        {
            "experiment_id": 1,
            "scan_id": 0,
            "timestamp": 1717000000,
            "scan_elapsed_ms": 0,
            "light_type": 1,
            "start_height_mm": 0.0,
            "step_um": 20.0,
            "point_count": 2750,
            "intensity_values": "45.67,56.78,67.89,..."
        }
    ]
}
```

> **顺序约定**：`intensity_values` 中第 i 个值对应高度 `start_height_mm + i * step_um / 1000.0`。light_type=0 为背射光，light_type=1 为透射光。

#### controllerManager 适配

```cpp
QObject::connect(m_dataTransmitCtrl, &DataTransmitController::exportExperimentScanRequested,
                 this,
                 [this](int experimentId, int scanId, int offset, int limit, const QString &requestId) {
    const QVariantMap experiment = m_dataCtrl->getExperimentById(experimentId);
    if (experiment.isEmpty()) {
        m_dataTransmitCtrl->sendCommandResult("get_experiment_scan_export",
                                              requestId, false, "Experiment not found", ...);
        return;
    }

    const QVector<QVariantMap> rows = m_dataCtrl->getScanDataByExperimentAndScan(experimentId, scanId);
    QVariantList dataList;
    for (const QVariantMap &row : rows) {
        dataList.append(row);
    }

    m_dataTransmitCtrl->sendCommandResult("get_experiment_scan_export",
                                          requestId, true, QString(),
                                          QVariantMap{
                                              {"experiment_id", experimentId},
                                              {"scan_id", scanId},
                                              {"total_count", 2},
                                              {"has_more", false},
                                              {"data", dataList}
                                          });
});
```

### 5.3 PC 端导入流程

#### 当前流程

```
① get_experiment_export（1次请求）
② 对每个 scan_id 循环：
   while (hasMore):
     ③ get_experiment_scan_export（offset/limit 分页请求）
     ④ 反序列化 QVariantList（2000行/轮）
     ⑤ 累积到 importBatch，满 2000 条时 batchAddExperimentData
⑥ 最终 flush + commitTransaction
⑦ mark_experiment_imported（1次请求）
```

#### 新流程

```
① get_experiment_export（1次请求）
② 对每个 scan_id 循环：
   ③ get_experiment_scan_export（1次请求，返回2行，无分页）
   ④ 反序列化 2 条记录
   ⑤ 直接写入本地 DB
⑥ commitTransaction
⑦ mark_experiment_imported（1次请求）
```

#### data_ctrl 适配

```cpp
QVariantMap dataCtrl::importSingleExperimentFromDeviceInternal(int deviceExperimentId)
{
    // ... 前置校验同现有逻辑 ...

    QVariantMap exportResponse;
    sendRequestAndWait("get_experiment_export",
                       {{"experiment_id", deviceExperimentId}}, &exportResponse);

    const QVariantMap deviceExperiment = exportResponse.value("experiment").toMap();
    QVariantList scanIdVariants = exportResponse.value("scan_ids").toList();

    // ... 创建本地工程和实验记录 ...

    m_dbManager->beginTransaction();

    int importedRows = 0;
    for (int scanIndex = 0; scanIndex < scanIdVariants.size(); ++scanIndex) {
        const int scanId = scanIdVariants.at(scanIndex).toInt();

        QVariantMap scanResponse;
        if (!sendRequestAndWait("get_experiment_scan_export",
                                {{"experiment_id", deviceExperimentId},
                                 {"scan_id", scanId},
                                 {"offset", 0},
                                 {"limit", 0}},
                                &scanResponse, 30000)) {
            m_dbManager->rollbackTransaction();
            return result;
        }

        const QVariantList deviceDataRows = scanResponse.value("data").toList();
        if (deviceDataRows.isEmpty()) {
            m_dbManager->rollbackTransaction();
            return result;
        }

        QVector<QVariantMap> importBatch;
        for (const QVariant &rowVariant : deviceDataRows) {
            QVariantMap row = rowVariant.toMap();
            row.remove("id");
            row.insert("experiment_id", localExperimentId);
            importBatch.append(std::move(row));
        }

        if (!m_dbManager->batchAddExperimentScanData(importBatch)) {
            m_dbManager->rollbackTransaction();
            return result;
        }
        importedRows += importBatch.size();
    }

    if (!m_dbManager->commitTransaction()) {
        m_dbManager->rollbackTransaction();
        return result;
    }

    sendRequestAndWait("mark_experiment_imported",
                       {{"experiment_id", deviceExperimentId}, {"status", 1}});

    // ... 返回结果 ...
}
```

---

## 6. 分析查询重构

### 6.1 通用辅助函数

```cpp
// 解析逗号分隔的光强值
// 返回值顺序：第 i 个元素对应高度 start_height_mm + i * step_um / 1000.0
QVector<double> parseIntensityValues(const QString& values)
{
    QVector<double> result;
    const QStringList parts = values.split(',', Qt::SkipEmptyParts);
    result.reserve(parts.size());
    for (const QString& part : parts) {
        bool ok = false;
        const double val = part.toDouble(&ok);
        result.append(ok ? val : 0.0);
    }
    return result;
}

// 根据高度范围筛选点索引
// 返回满足 lowerMm <= height <= upperMm 的索引集合
QVector<int> indicesInRange(double startHeightMm, double stepUm,
                            int pointCount, double lowerMm, double upperMm)
{
    QVector<int> indices;
    for (int i = 0; i < pointCount; ++i) {
        const double h = startHeightMm + static_cast<double>(i) * stepUm / 1000.0;
        if (h >= lowerMm && h <= upperMm) {
            indices.append(i);
        }
    }
    return indices;
}

// 扫描数据对（同一 scan_id 的背射+透射合并）
struct ScanDataPair {
    int scanId = 0;
    int timestamp = 0;
    int elapsedMs = 0;
    double startHeightMm = 0.0;
    double stepUm = 20.0;
    int pointCount = 0;
    QVector<double> backscatterValues;   // 第 i 个元素对应高度 startHeightMm + i * stepUm / 1000.0
    QVector<double> transmissionValues;  // 同上
};

// 读取指定实验的所有扫描数据
QVector<ScanDataPair> loadScanDataByExperiment(int experimentId)
{
    // SELECT * FROM experiment_scan_data
    // WHERE experiment_id = ?
    // ORDER BY scan_id ASC, light_type ASC
    //
    // 每 2 条（light_type=0 背射 和 light_type=1 透射）合并为一个 ScanDataPair
    // backscatterValues = parseIntensityValues(light_type=0 的 intensity_values)
    // transmissionValues = parseIntensityValues(light_type=1 的 intensity_values)
}
```

### 6.2 高度重建

所有分析查询中，高度均由以下公式计算：

```cpp
for (int i = 0; i < scan.pointCount; ++i) {
    const double heightMm = scan.startHeightMm
        + static_cast<double>(i) * scan.stepUm / 1000.0;
    const double backscatter = scan.backscatterValues.value(i, 0.0);
    const double transmission = scan.transmissionValues.value(i, 0.0);
    // ...
}
```

**与旧方案的等价性**：
- 旧方案：`height` 直接从 DB 读取，值为 `startHeightUm + pointIndex * stepUm`
- 新方案：`heightMm = start_height_mm + i * step_um / 1000.0`
- 其中 `start_height_mm` = `startHeightUm / 1000.0`，`i` = `pointIndex`
- 数学上完全等价

### 6.3 光强平均值

**当前实现**：SQL `AVG() GROUP BY scan_id`，全表扫描 275 万行。

**新实现**：

```cpp
QVector<QVariantMap> SqlOrmManager::getLightIntensityAveragesByExperiment(int experimentId)
{
    const QVector<ScanDataPair> scans = loadScanDataByExperiment(experimentId);
    QVector<QVariantMap> result;

    for (const auto& scan : scans) {
        double bsSum = 0.0, tSum = 0.0;
        for (double v : scan.backscatterValues) bsSum += v;
        for (double v : scan.transmissionValues) tSum += v;
        // 用预期点数作除数，避免丢点导致均值偏高
        const int divisor = qMax(1, scan.pointCount);

        QVariantMap row;
        row["scan_id"] = scan.scanId;
        row["scan_elapsed_ms"] = scan.elapsedMs;
        row["avg_backscatter"] = bsSum / divisor;
        row["avg_transmission"] = tSum / divisor;
        result.append(row);
    }

    return result;
}
```

**计算等价性**：SQL `AVG(x)` = `SUM(x) / COUNT(x)`，C++ 实现 `sum / pointCount`，数学上完全等价。使用 `pointCount`（预期点数）而非实际解析出的 values.size()，与项目规则"PC 端平均光强用预期点数作除数"一致。

### 6.4 均匀度

**当前实现**：SQL `AVG(x)`, `AVG(x*x)` 计算 mean 和 std，全表扫描。

**新实现**：

```cpp
QVector<QVariantMap> SqlOrmManager::getUniformityIndicesByExperiment(int experimentId)
{
    const QVector<ScanDataPair> scans = loadScanDataByExperiment(experimentId);
    QVector<QVariantMap> result;

    for (const auto& scan : scans) {
        const int n = scan.pointCount;
        if (n <= 0) continue;

        double bsSum = 0.0, bsSqSum = 0.0;
        double tSum = 0.0, tSqSum = 0.0;
        for (double v : scan.backscatterValues) { bsSum += v; bsSqSum += v * v; }
        for (double v : scan.transmissionValues) { tSum += v; tSqSum += v * v; }

        const double avgBs = bsSum / n;
        const double avgT = tSum / n;
        const double stdBs = qSqrt(qMax(0.0, bsSqSum / n - avgBs * avgBs));
        const double stdT  = qSqrt(qMax(0.0, tSqSum / n - avgT * avgT));
        const double uiBs = avgBs > 0.0 ? qBound(0.0, 1.0 - stdBs / avgBs, 1.0) : 0.0;
        const double uiT  = avgT  > 0.0 ? qBound(0.0, 1.0 - stdT  / avgT,  1.0) : 0.0;

        QVariantMap row;
        row["scan_id"] = scan.scanId;
        row["scan_elapsed_ms"] = scan.elapsedMs;
        row["ui_backscatter"] = uiBs;
        row["ui_transmission"] = uiT;
        row["ui_combined"] = (uiBs + uiT) / 2.0;
        result.append(row);
    }

    return result;
}
```

**计算等价性**：SQL `AVG(x*x) - AVG(x)^2` = C++ `sqSum/n - avg*avg`，即方差公式 `Var = E[X²] - (E[X])²`，数学上完全等价。

### 6.5 峰厚度

**当前实现**：SQL `WHERE height >= ? AND height <= ?` 按高度范围筛选点，逐扫描识别阈值区段。

**新实现**：

```cpp
QVariantMap SqlOrmManager::getPeakThicknessChartDataByExperiment(
    int experimentId, int intensityMode,
    double lowerBoundMm, double upperBoundMm, double thresholdValue)
{
    const QVector<ScanDataPair> scans = loadScanDataByExperiment(experimentId);

    QVector<ScanPoint> referencePoints;
    QVector<ScanPoint> currentPoints;
    QVariantList rowList;
    QVector<QPointF> chartPoints;
    int currentScanId = std::numeric_limits<int>::min();
    qint64 currentElapsedMs = 0;
    double maxThickness = 0.0;
    bool invalidReference = false;

    auto flushScan = [&]() {
        // 与现有 flushScan 逻辑完全一致
        // ...
    };

    for (const auto& scan : scans) {
        currentScanId = scan.scanId;
        currentElapsedMs = scan.elapsedMs;
        currentPoints.clear();

        for (int i = 0; i < scan.pointCount; ++i) {
            const double heightMm = scan.startHeightMm
                + static_cast<double>(i) * scan.stepUm / 1000.0;

            // 高度范围筛选（原 SQL WHERE 子句的 C++ 等价）
            if (heightMm < lowerBoundMm || heightMm > upperBoundMm) continue;

            ScanPoint point;
            point.heightMm = heightMm;
            point.backscatter = scan.backscatterValues.value(i, 0.0);
            point.transmission = scan.transmissionValues.value(i, 0.0);
            currentPoints.append(point);
        }

        flushScan();
    }

    // ... 构建图表数据 ...
}
```

**计算等价性**：
- 旧方案 SQL `ORDER BY scan_id ASC, height ASC` 保证高度递增
- 新方案 `i` 从 0 递增，`heightMm` 递增，筛选后顺序与 SQL ORDER BY 一致
- 背射/透射值通过 `scan.backscatterValues.value(i)` 和 `scan.transmissionValues.value(i)` 取得，索引 i 与高度严格对应

### 6.6 分层厚度

**当前实现**：从 `experiment_data` 读全部点 → C++ 端逐扫描识别边界 → 缓存到 `separation_layer_data` 结果表。

**新实现**：

```cpp
QVector<QVariantMap> SqlOrmManager::getSeparationLayerDataByExperiment(int experimentId)
{
    // 缓存策略不变：先查 separation_layer_data 结果表
    // 需要重建时，从 experiment_scan_data 读取

    const QVector<ScanDataPair> scans = loadScanDataByExperiment(experimentId);

    QVector<QVariantMap> computedRows;
    for (const auto& scan : scans) {
        if (scan.pointCount < 4) {
            // 粗略处理，同现有逻辑
            continue;
        }

        // 重建 (height, backscatter, transmission) 三元组
        // 顺序：i 从 0 递增，高度递增，与旧方案 ORDER BY height ASC 一致
        QVector<SeparationRowEx> currentRows;
        for (int i = 0; i < scan.pointCount; ++i) {
            SeparationRowEx row;
            row.heightMm = scan.startHeightMm
                + static_cast<double>(i) * scan.stepUm / 1000.0;
            row.backscatter = scan.backscatterValues.value(i, 0.0);
            row.transmission = scan.transmissionValues.value(i, 0.0);
            currentRows.append(row);
        }

        // 后续归一化、平滑、边界识别逻辑与现有代码完全一致
        // ...
    }

    // 写入 separation_layer_data 结果表（缓存逻辑不变）
    // ...
}
```

**计算等价性**：分层厚度计算完全在 C++ 端完成（现有代码就是从 DB 读出后在 C++ 端计算），数据来源从逐行 SELECT 改为 split 解析，计算逻辑不变。

### 6.7 光强曲线

**当前实现**：从 `experiment_data` 读全部点，按扫描分组，降采样后返回。

**新实现**：

```cpp
QVector<QVariantMap> SqlOrmManager::getLightIntensityCurvesByExperiment(
    int experimentId, int pointsPerCurve)
{
    const QVector<ScanDataPair> scans = loadScanDataByExperiment(experimentId);
    QVector<QVariantMap> result;

    for (const auto& scan : scans) {
        // 重建 (height, backscatter, transmission) 三元组
        QVector<LightCurveRowEx> currentRows;
        for (int i = 0; i < scan.pointCount; ++i) {
            LightCurveRowEx row;
            row.heightMm = scan.startHeightMm
                + static_cast<double>(i) * scan.stepUm / 1000.0;
            row.backscatter = scan.backscatterValues.value(i, 0.0);
            row.transmission = scan.transmissionValues.value(i, 0.0);
            currentRows.append(row);
        }

        // min/max 统计直接从 QVector<double> 计算，无需 SQL
        // 降采样逻辑不变

        QVariantMap curve;
        curve["scan_id"] = scan.scanId;
        curve["timestamp"] = scan.timestamp;
        curve["scan_elapsed_ms"] = scan.elapsedMs;
        curve["point_count"] = scan.pointCount;
        curve["min_height_mm"] = scan.startHeightMm;
        curve["max_height_mm"] = scan.startHeightMm
            + static_cast<double>(scan.pointCount - 1) * scan.stepUm / 1000.0;
        // min/max backscatter, transmission 从 QVector 直接计算
        curve["backscatter_points"] = downsampleCurvePoints(currentRows, false, effectivePoints);
        curve["transmission_points"] = downsampleCurvePoints(currentRows, true, effectivePoints);
        result.append(curve);
    }

    return result;
}
```

---

## 7. 实时数据流推送

Stream 9002 端口的推送格式与存储格式解耦，**不受影响**。

### 7.1 推送与入库分离

| 环节 | 格式 | 说明 |
|---|---|---|
| **入库** | 逗号分隔字符串，2 行/扫描 | 扫描完成时写入 `experiment_scan_data` |
| **推送** | 逐点 QVariantMap，格式不变 | 每批存储区数据到达时立即推送 |

### 7.2 双轨输出设计

入库需要扫描完成后拼接字符串，推送需要逐点实时发送。两者时序不同，需通过双轨输出解决。

**方案**：`buildScanRowsFromStorageData` 同时返回入库数据和推送数据。

```cpp
// 新的 BuildRowsFn 签名
using BuildRowsFn = std::function<QPair<QVector<QVariantMap>, QVector<QVariantMap>>(
    int, const QVector<quint16>&, bool)>;
// first  = 入库数据（2条/扫描，仅扫描完成时有值）
// second = 推送数据（逐点，与现有格式一致，每批都有值）
```

**推送数据**仍然按现有格式逐点构造（包含 `height`、`backscatter_intensity`、`transmission_intensity` 等字段），PC 端 Stream 9002 接收逻辑无需改动。

### 7.3 ExperimentDataService 适配

```cpp
void ExperimentDataService::tryFetchStoredData(...) const
{
    auto fetchArea = [&](bool areaA, int readableCount, int initialState) {
        // ... 读取原始寄存器数据不变 ...

        const auto [dbRows, streamRows] = buildRowsFn(channel, sliced, areaA);

        // 入库：扫描完成时才有数据（2行/扫描）
        if (!dbRows.isEmpty()) {
            batchSaveExperimentScanData(experimentId, dbRows);
        }

        // 推送：每批都有逐点数据
        if (streamRowsFn && !streamRows.isEmpty()) {
            streamRowsFn(channel, experimentId, streamRows);
        }

        // 内存缓存：使用推送数据（逐点格式）
        if (memoryCache && !streamRows.isEmpty()) {
            *memoryCache += streamRows;
        }
    };
}
```

---

## 8. 性能评估

### 8.1 导入性能（基于实测校准）

**校准基准**：600 次扫描实验，旧方案实测 40 分钟。

| 场景 | 旧方案 | 新方案 | 提升 |
|---|---|---|---|
| 600 扫描导入 | 40 分钟（实测） | ~25 秒 | ~96× |
| 1000 扫描导入 | ~75~85 分钟 | ~42 秒 | ~107~121× |

### 8.2 单轮耗时对比（基于实测校准）

| 环节 | 旧方案（2000行/轮） | 新方案（2行/轮） |
|---|---|---|
| 设备端 DB 查询 | ~800ms（含 OFFSET） | ~5ms（索引直查） |
| 设备端序列化 | ~600ms（2000 QVariantMap） | ~15ms（2 QVariantMap） |
| 网络传输 | ~100ms（~320KB） | ~10ms（~48KB） |
| PC 端反序列化 | ~200ms | ~5ms |
| PC 端 DB 写入（均摊） | ~300ms | ~1ms |
| **单轮合计** | **~2s** | **~36ms** |

### 8.3 分析查询性能

| 查询 | 旧方案 | 新方案 | 说明 |
|---|---|---|---|
| 光强平均值 | SQL 全表扫描 275万行 | 读 2000 行 + C++ split | 提升 ~50× |
| 均匀度 | SQL 全表扫描 + 聚合 | 读 2000 行 + C++ 计算 | 提升 ~50× |
| 峰厚度 | SQL 范围扫描 + C++ | 读 2000 行 + C++ 筛选 | 提升 ~20× |
| 分层厚度 | SQL 全表扫描 + C++ | 读 2000 行 + C++ 计算 | 提升 ~20× |
| 光强曲线 | SQL 全表扫描 + 降采样 | 读 2000 行 + C++ 降采样 | 提升 ~30× |

### 8.4 存储空间

| 指标 | 旧方案 | 新方案 | 压缩比 |
|---|---|---|---|
| 1000 扫描实验 DB 体积 | ~880 MB | ~13 MB | ~68× |
| 设备端 DB（4 通道同时运行） | ~3.5 GB | ~52 MB | ~67× |

---

## 9. 需要删除的旧代码

### 9.1 设备端 SqlOrmManager

| 删除项 | 说明 |
|---|---|
| `ExperimentData` 结构体 | 替换为 `ExperimentScanData` |
| `make_table("experiment_data", ...)` | 替换为 `make_table("experiment_scan_data", ...)` |
| `addExperimentData()` | 删除 |
| `batchAddExperimentData()` | 替换为 `batchAddExperimentScanData()` |
| `getExperimentDataById()` | 删除 |
| `getExperimentDataByExperiment()` | 替换为 `getScanDataByExperiment()` |
| `getExperimentDataByRange()` | 替换为 `getScanDataByExperimentAndTimeRange()` |
| `getExperimentDataByExperimentAndScan()` | 替换为 `getScanDataByExperimentAndScan()` |
| `getExperimentScanIds()` | 保留，改为查 `experiment_scan_data` |
| `getExperimentDataCountByExperiment()` | 替换为 `getScanCountByExperiment()` |
| `getExperimentDataCountByExperimentAndScan()` | 删除（恒为 2） |
| `updateExperimentData()` | 删除 |
| `deleteExperimentData()` | 删除 |
| `deleteExperimentDataByExperiment()` | 替换为 `deleteScanDataByExperiment()` |
| `kExperimentDataInsertChunkSize` 常量 | 删除（不再需要分块） |
| `migrateExperimentDataSchema()` | 删除 |
| `ensureExperimentDataColumn()` | 删除 |

### 9.2 PC 端 SqlOrmManager

同设备端，额外需要重构的查询方法：

| 方法 | 改动 |
|---|---|
| `getLightIntensityAveragesByExperiment()` | SQL 聚合 → C++ 聚合 |
| `getUniformityIndicesByExperiment()` | SQL 聚合 → C++ 聚合 |
| `getPeakThicknessChartDataByExperiment()` | SQL 筛选 → C++ 筛选 |
| `getSeparationLayerDataByExperiment()` | SQL 全表读 → C++ split |
| `getLightIntensityCurvesByExperiment()` | SQL 全表读 → C++ split |
| `getLightIntensityCurveByScan()` | SQL 单扫描读 → C++ split |

### 9.3 其他文件

| 文件 | 改动 |
|---|---|
| `experiment_data_service.cpp` | `batchSaveExperimentData` → `batchSaveExperimentScanData` |
| `experiment_session_service.cpp` | `parseStoragePairs` → `buildScanRowsFromStorageData` |
| `experiment_session_service.h` | 更新方法签名，`ScanCycleContext` 增加累积缓冲区 |
| `experiment_types.h` | `ScanCycleContext` 增加累积缓冲区字段 |
| `controllerManager.h` | 导出接口适配新格式 |
| `data_ctrl.cpp` | 导入逻辑简化（无需分页循环） |

---

## 10. 实施步骤

| 步骤 | 内容 | 涉及端 | 依赖 |
|---|---|---|---|
| **1** | 新增 `ExperimentScanData` 结构体 + `experiment_scan_data` 表定义 + 索引 | 设备端+PC端 | 无 |
| **2** | 新增 `batchAddExperimentScanData()` 入库方法 | 设备端+PC端 | 步骤1 |
| **3** | 新增 `getScanDataByExperiment()` 等查询方法 | 设备端+PC端 | 步骤1 |
| **4** | 扩展 `ScanCycleContext` 增加累积缓冲区 | 设备端 | 无 |
| **5** | 重构 `ExperimentSessionService::buildScanRowsFromStorageData()` | 设备端 | 步骤2+4 |
| **6** | 适配 `ExperimentDataService` 入库调用（双轨输出） | 设备端 | 步骤5 |
| **7** | 适配导出协议 `get_experiment_scan_export` | 设备端 | 步骤3 |
| **8** | 重构 PC 端导入逻辑 | PC端 | 步骤7 |
| **9** | 新增 `loadScanDataByExperiment()` + `parseIntensityValues()` | PC端 | 步骤3 |
| **10** | 重构光强平均值查询 | PC端 | 步骤9 |
| **11** | 重构均匀度查询 | PC端 | 步骤9 |
| **12** | 重构峰厚度查询 | PC端 | 步骤9 |
| **13** | 重构分层厚度查询 | PC端 | 步骤9 |
| **14** | 重构光强曲线查询 | PC端 | 步骤9 |
| **15** | 删除所有旧 `experiment_data` 相关代码 | 双端 | 步骤10~14全部验证通过 |
| **16** | 集成测试 | 双端 | 步骤15 |

> 步骤 1~7 可独立完成并验证设备端功能；步骤 8~14 可并行开发；步骤 15 在全部验证通过后统一清理。

---

## 11. 风险与注意事项

### 11.1 数据顺序的正确性（最高风险）

**intensity_values 中值的顺序必须与高度严格对应**。具体约束：

1. **拼接顺序**：在 `buildScanRowsFromStorageData` 中，`backscatterAccum` 和 `transmissionAccum` 必须按 `pointIndex` 从 0 递增的顺序追加。由于数据跨批次到达，`savedPointCount` 作为 `startPointIndex` 确保了跨批次的连续性
2. **解析顺序**：在 PC 端 `parseIntensityValues` 后，第 i 个值对应高度 `start_height_mm + i * step_um / 1000.0`
3. **校准顺序**：校准参考值 `findCalibrationAvgTransmission(channel, heightUm)` 中的 `heightUm` 由 `pointIndex` 决定，`pointIndex` 与 `intensity_values` 的索引 i 一一对应

**验证方法**：新方案上线后，取同一扫描的数据，对比旧方案 DB 中的 height/backscatter/transmission 值与新方案 split 后按索引重建的值，确保完全一致。

### 11.2 高度隐式计算的前提

新方案中高度由 `start_height_mm + index * step_um / 1000.0` 隐式计算，前提是**扫描为等间距**。当前系统中步长固定（默认 20μm），此前提成立。若未来支持变步长扫描，需重新评估存储格式。

### 11.3 逗号分隔的精度

`QString::number(value, 'f', 2)` 保留 2 位小数。对于校准后的百分比值（0~100+），2 位小数精度为 0.01%，满足分析需求。如需更高精度，可改为 `'f', 4` 或 `'g', 8`。

### 11.4 内存峰值

`loadScanDataByExperiment` 一次性加载所有扫描数据到内存。1000 扫描实验约 13MB，4 通道约 52MB，在 PC 端完全可接受。设备端不使用此函数，无内存风险。

设备端 `ScanCycleContext` 中的 `backscatterAccum` / `transmissionAccum` 累积缓冲区，单扫描最多 2750 个字符串，约 24KB × 2 = 48KB，4 通道同时运行约 192KB，嵌入式设备完全可接受。

### 11.5 推送与入库的时序

入库需要扫描完成后拼接字符串，推送需要逐点实时发送。两者时序不同，需通过双轨输出解决。入库在扫描完成时触发，推送在每批存储区数据到达时触发。

### 11.6 旧数据库文件

完全舍弃旧格式后，已有设备上的 `app_database.db` 中的 `experiment_data` 表将成为孤儿数据。建议：
- 新版本首次启动时，检测到旧表存在则自动 DROP
- 或在初始化时执行 `DROP TABLE IF EXISTS experiment_data`

---

## 附录 A：下位机寄存器布局参考

### Modbus INPUT_REGISTERS 地址映射

| 地址范围 | 存储区 | 读取任务 | 寄存器数 |
|---|---|---|---|
| 0~99 | A 区第 1 段 | read_scan_data_a_0 | 100 |
| 100~199 | A 区第 2 段 | read_scan_data_a_100 | 100 |
| 200~299 | A 区第 3 段 | read_scan_data_a_200 | 100 |
| 300~399 | A 区第 4 段 | read_scan_data_a_300 | 100 |
| 400~499 | A 区第 5 段 | read_scan_data_a_400 | 100 |
| 500~599 | B 区第 1 段 | read_scan_data_b_500 | 100 |
| 600~699 | B 区第 2 段 | read_scan_data_b_600 | 100 |
| 700~799 | B 区第 3 段 | read_scan_data_b_700 | 100 |
| 800~899 | B 区第 4 段 | read_scan_data_b_800 | 100 |
| 900~999 | B 区第 5 段 | read_scan_data_b_900 | 100 |

### 单个采样点的寄存器布局

```
地址 N  : 透射光强度 (transmission)
地址 N+1: 背射光强度 (backscatter)
```

- A 区共 500 words = 250 个点对
- B 区共 500 words = 250 个点对
- A+B 合计 500 个点对（单次扫描最多 2750 个点，需多次 A/B 区轮转）
