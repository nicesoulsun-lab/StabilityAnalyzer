# PC端记录详情高级计算实现分析

## 1. 整体架构

PC端"高级计算"位于实验记录详情页的第7个Tab，由三层结构协同完成：

```
┌──────────────────────────────────────────────────────────────┐
│  ExperimentDetailPage.qml                                    │
│  (记录详情主页面，Tab索引6 → AdvancedCalculationPage.qml)      │
├──────────────────────────────────────────────────────────────┤
│  AdvancedCalculationPage.qml                                 │
│  (高级计算UI：收集参数、展示结果、调用后端)                      │
├──────────────────────────────────────────────────────────────┤
│  dataCtrl (QML 上下文对象)                                    │
│  (桥接层：Q_INVOKABLE 方法暴露给 QML 调用)                     │
├──────────────────────────────────────────────────────────────┤
│  AdvancedCalculationEngine (纯C++静态类)                      │
│  (核心算法：颗粒迁移速率 / 流体力学 / 光学计算)                  │
├──────────────────────────────────────────────────────────────┤
│  SqlOrmManager                                               │
│  (数据库层：提供分层结果数据，计算 sediment_boundary_mm)         │
└──────────────────────────────────────────────────────────────┘
```

---

## 2. 页面入口

### 2.1 实验详情页 Tab 结构

**文件**: `qml/ExperimentDetailPage.qml`

详情页定义了7个Tab：

| 索引 | Tab名 | 子页面QML |
|------|-------|-----------|
| 0 | 光强 | `curve/LightIntensityCurvePage.qml` |
| 1 | 不稳定性 | `curve/InstabilityCurvePage.qml` |
| 2 | 均匀度 | `curve/UniformityIndexPage.qml` |
| 3 | 峰厚度 | `curve/PeakThicknessPage.qml` |
| 4 | 光强平均值 | `curve/LightIntensityAveragePage.qml` |
| 5 | 分层厚度 | `curve/SeparationLayerPage.qml` |
| **6** | **高级计算** | **`curve/AdvancedCalculationPage.qml`** |

---

## 3. 高级计算页面 UI

**文件**: `qml/curve/AdvancedCalculationPage.qml`

页面水平排列三个计算区块：

```
┌──────────────────┬──────────────────┬──────────────────┐
│  颗粒迁移速率     │  流体力学计算     │  光学计算         │
│                  │                  │                  │
│  迁移速率: [   ]  │  计算参数选择:    │  计算参数选择:    │
│  (mm/h) 手动输入  │  [平均颗粒粒径▼]  │  [粒径▼]         │
│                  │                  │                  │
│                  │  体积浓度: [   ]  │  分散相折射率:[ ] │
│                  │  分散相密度: [  ] │  分散相吸收率:[ ] │
│                  │  连续相粘度: [  ] │  连续相折射率:[ ] │
│                  │  连续相密度: [  ] │  连续相消光范围:[ ]│
│                  │                  │  体积浓度: [   ]  │
│                  │  [计算]          │                  │
│                  │                  │  [计算]          │
│                  │  计算目标: [结果]  │                  │
│                  │  平均颗粒粒径     │  计算目标: [结果]  │
│                  │                  │  粒径             │
└──────────────────┴──────────────────┴──────────────────┘
```

### 3.1 颗粒迁移速率区块

- 仅一个输入框：迁移速率（mm/h），标注"手动输入"
- 当前版本迁移速率需要用户手动输入，作为流体力学计算的输入

### 3.2 流体力学计算区块

**"五选一反算"模式**：下拉框选择计算目标，其余4项作为输入，点击"计算"按钮反算目标值。

| 计算目标 | key | 单位 |
|----------|-----|------|
| 平均颗粒粒径 | `diameter` | μm |
| 体积浓度 | `concentration` | % |
| 分散相密度 | `dispersedDensity` | g/cm³ |
| 连续相粘度 | `continuousViscosity` | cP |
| 连续相密度 | `continuousDensity` | g/cm³ |

切换计算目标时，UI动态调整：被选中的目标项移到底部"计算目标"行（只读），其余4项排到上方作为可编辑输入。

**输入值缓存机制**：
- `fluidValues` 对象缓存所有5项的当前值
- 切换目标时先 `captureFluidInputs()` 回写当前界面值到缓存
- 再 `refreshFluidInputs()` 从缓存恢复到新的输入布局

### 3.3 光学计算区块

**"二选一反算"模式**：在粒径和体积浓度之间互相反算。

| 计算目标 | key | 单位 |
|----------|-----|------|
| 粒径 | `diameter` | μm |
| 体积浓度 | `concentration` | % |

固定输入项（始终作为输入）：
- 分散相折射率 (`dispersedRefractive`)
- 分散相吸收率 (`dispersedAbsorption`)
- 连续相折射率 (`continuousRefractive`)
- 连续相消光范围 (`continuousExtinction`)

---

## 4. 调用链

### 4.1 流体力学计算

```
QML: advancedPanel.calculateHydrodynamicDiameter()
  │
  ├── captureFluidInputs()           // 回写界面值到缓存
  ├── 组装 params QVariantMap:
  │       targetKey, migrationRate, diameter,
  │       concentration, dispersedDensity,
  │       continuousViscosity, continuousDensity
  │
  └── data_ctrl.calculateHydrodynamic(params)    // QML → C++
          │
          └── AdvancedCalculationEngine::calculateHydrodynamic(params)  // 纯计算
                  │
                  └── 返回 QVariantMap { success, targetKey, value, displayText, unit, message }
```

### 4.2 光学计算

```
QML: advancedPanel.calculateOpticalDiameter()
  │
  ├── captureOpticalInputs()
  ├── 组装 params QVariantMap:
  │       targetKey, diameter, concentration,
  │       dispersedRefractive, dispersedAbsorption,
  │       continuousRefractive, continuousExtinction
  │
  └── data_ctrl.calculateOptical(params)
          │
          └── AdvancedCalculationEngine::calculateOptical(params)
```

### 4.3 颗粒迁移速率计算

```
QML: （迁移速率当前为手动输入，不触发自动计算）

C++: data_ctrl.calculateMigrationRate(params)
  │
  ├── params 含 experimentId → 从数据库加载分层结果
  │       AdvancedCalculationEngine::calculateMigrationRate(params, separationRows)
  │
  └── params 不含 experimentId → 返回错误
          AdvancedCalculationEngine::calculateMigrationRate(params)  // 无数据重载
```

---

## 5. 核心算法 — AdvancedCalculationEngine

**文件**: `inc/Analysis/AdvancedCalculationEngine.h` / `src/Analysis/AdvancedCalculationEngine.cpp`

### 5.1 物理常数

| 常量 | 值 | 说明 |
|------|-----|------|
| `kGravityAcceleration` | 9.80665 | 重力加速度 (m/s²) |
| `kRichardsonZakiN` | 4.65 | Richardson-Zaki 浓度修正指数 |
| `kMillimeterPerHourToMeterPerSecond` | 1/3600000 | mm/h → m/s |
| `kMicrometerToMeter` | 1e-6 | μm → m |
| `kGramPerCm3ToKgPerM3` | 1000 | g/cm³ → kg/m³ |
| `kCentipoiseToPascalSecond` | 0.001 | cP → Pa·s |

---

### 5.2 颗粒迁移速率计算

**方法**: `calculateMigrationRate(params, separationRows)`

**使用的数据**：
- **用户输入**：起止时间（startDay/startHour/startMinute/startSecond, endDay/endHour/endMinute/endSecond）
- **数据库数据**：`separation_layer_data` 表中的分层结果行，每行包含 `scan_elapsed_ms` 和 `sediment_boundary_mm`

**计算步骤**：

1. **解析起止时间**，转换为毫秒数：
```
startElapsedMs = ((startDay × 24 + startHour) × 60 + startMinute) × 60 × 1000 + startSecond × 1000
endElapsedMs   = ((endDay × 24 + endHour) × 60 + endMinute) × 60 × 1000 + endSecond × 1000
```

2. **在分层结果中查找最接近的行**（`nearestRowByElapsedMs()`）：
   - 遍历 `separationRows`，找到 `scan_elapsed_ms` 与目标时间差值最小的行

3. **计算迁移速率**：
```
Δboundary_mm = |endRow.sediment_boundary_mm - startRow.sediment_boundary_mm|
Δhours = |endRow.scan_elapsed_ms - startRow.scan_elapsed_ms| / 3600000

migrationRate = Δboundary_mm / max(1e-9, Δhours)    // 单位: mm/h
```

**校验条件**：
- 分层结果行数 ≥ 2
- 起止时间不能落在同一帧（ΔelapsedMs > 0）

---

### 5.3 流体力学计算

**方法**: `calculateHydrodynamic(params)`

**使用的数据**（全部为用户手动输入）：

| 参数 | key | 界面单位 | SI单位 | 转换系数 |
|------|-----|---------|--------|---------|
| 颗粒迁移速率 | `migrationRate` | mm/h | m/s | × 1/3600000 |
| 平均颗粒粒径 | `diameter` | μm | m | × 1e-6 |
| 体积浓度 | `concentration` | % | 无量纲 | ÷ 100 |
| 分散相密度 | `dispersedDensity` | g/cm³ | kg/m³ | × 1000 |
| 连续相粘度 | `continuousViscosity` | cP | Pa·s | × 0.001 |
| 连续相密度 | `continuousDensity` | g/cm³ | kg/m³ | × 1000 |

**核心物理模型 — Richardson-Zaki 浓度修正 Stokes 沉降公式**：

基础 Stokes 沉降速度（单颗粒、低雷诺数）：
```
v_Stokes = d² × (ρp - ρf) × g / (18μ)
```

Richardson-Zaki 浓度修正（考虑颗粒间相互影响）：
```
v = v_Stokes × (1 - φ)^n
```

合并后完整公式：
```
v = [d² × (ρp - ρf) × g / (18μ)] × (1 - φ)^n
```

其中：
- `v` — 颗粒受阻沉降速度 (m/s)
- `d` — 颗粒粒径 (m)
- `ρp` — 分散相密度 (kg/m³)
- `ρf` — 连续相密度 (kg/m³)
- `g` — 重力加速度 = 9.80665 m/s²
- `μ` — 连续相动力粘度 (Pa·s)
- `φ` — 体积浓度（0~1，界面输入百分数需 ÷100）
- `n` — Richardson-Zaki 指数 = 4.65（适用于低雷诺数沉降体系）

**"五选一反算"详细推导**：

令 `stokesNumerator = 18μv`，`hinderedFactor = (1-φ)^4.65`，`densityDiff = ρp - ρf`

#### 反算粒径 (targetKey = "diameter")

从 `v = d² × densityDiff × g × hinderedFactor / (18μ)` 反解 `d`：

```
d² = 18μv / (densityDiff × g × hinderedFactor)
d = √(stokesNumerator / (densityDiff × g × hinderedFactor))
```

结果从 m 转换回 μm：`d_μm = d_m / 1e-6`

**约束**：`densityDiff > 0`（分散相密度必须大于连续相密度，颗粒才能沉降）

#### 反算体积浓度 (targetKey = "concentration")

从 `v = d² × densityDiff × g × (1-φ)^n / (18μ)` 反解 `φ`：

```
(1-φ)^n = 18μv / (d² × densityDiff × g)
1-φ = (18μv / (d² × densityDiff × g))^(1/n)
φ = 1 - (stokesNumerator / (d² × densityDiff × g))^(1/4.65)
```

结果从无量纲转回百分数：`concentration_% = φ × 100`，并限制在 [0, 99.9999] 范围内。

**约束**：`densityDiff > 0`，且内层 `stokesNumerator / (d² × densityDiff × g) > 0`

#### 反算分散相密度 (targetKey = "dispersedDensity")

从 `v = d² × (ρp - ρf) × g × hinderedFactor / (18μ)` 反解 `ρp`：

```
ρp = ρf + 18μv / (d² × g × hinderedFactor)
ρp = ρf + stokesNumerator / (d² × g × hinderedFactor)
```

结果从 kg/m³ 转换回 g/cm³：`ρp_g/cm³ = ρp_kg/m³ / 1000`

**无特殊约束**（ρp 可以小于 ρf，此时结果为负数表示上浮体系）

#### 反算连续相粘度 (targetKey = "continuousViscosity")

从 `v = d² × densityDiff × g × hinderedFactor / (18μ)` 反解 `μ`：

```
μ = d² × densityDiff × g × hinderedFactor / (18v)
μ = d² × densityDiff × g × hinderedFactor / stokesNumerator × (18/18)
```

实际代码写法：
```
μ_SI = d² × densityDiff × g × hinderedFactor / max(1e-18, 18 × v_SI)
```

结果从 Pa·s 转换回 cP：`μ_cP = μ_Pa·s / 0.001`

**约束**：`densityDiff > 0`

#### 反算连续相密度 (targetKey = "continuousDensity")

从 `v = d² × (ρp - ρf) × g × hinderedFactor / (18μ)` 反解 `ρf`：

```
ρf = ρp - 18μv / (d² × g × hinderedFactor)
ρf = ρp - stokesNumerator / (d² × g × hinderedFactor)
```

结果从 kg/m³ 转换回 g/cm³：`ρf_g/cm³ = ρf_kg/m³ / 1000`

**无特殊约束**

**单位转换完整流程**：
```
界面输入 → SI单位 → 计算 → SI结果 → 界面单位
  mm/h  → ×1/3600000→ m/s                    → ×3600000→ mm/h
  μm    → ×1e-6    → m                        → ÷1e-6   → μm
  %     → ÷100     → 无量纲φ                  → ×100    → %
  g/cm³ → ×1000    → kg/m³                    → ÷1000   → g/cm³
  cP    → ×0.001   → Pa·s                     → ÷0.001  → cP
```

**输入校验**：
- 所有输入必须 > 0（`requirePositive()`）
- 体积浓度 φ 必须在 (0, 1) 之间（即界面输入 0~100）
- 反算粒径/浓度/粘度时，`densityDiff > 0`（分散相密度必须大于连续相密度）

---

### 5.4 光学计算

**方法**: `calculateOptical(params)`

**使用的数据**（全部为用户手动输入）：

| 参数 | key | 说明 |
|------|-----|------|
| 分散相折射率 | `dispersedRefractive` | 必须大于0 |
| 分散相吸收率 | `dispersedAbsorption` | 必须大于0 |
| 连续相折射率 | `continuousRefractive` | 必须大于0 |
| 连续相消光范围 | `continuousExtinction` | 必须大于0 |
| 粒径 | `diameter` | μm，反算浓度时作为输入 |
| 体积浓度 | `concentration` | %，反算粒径时作为输入 |

**当前为经验模型**（代码注释明确标注"光学计算当前是经验模型"），固定四个光学参数，在粒径和体积浓度之间做双向反算。

**中间量定义**：
```
refractiveDelta = |dispersedRefractive - continuousRefractive|    // 折射率差
baseSignal = dispersedAbsorption + continuousExtinction           // 基础信号
φ = concentration / 100                                           // 体积浓度（无量纲）
```

#### 反算粒径 (targetKey = "diameter")

```
d = (baseSignal + φ) / refractiveDelta
```

展开：
```
d = (dispersedAbsorption + continuousExtinction + concentration/100) / |dispersedRefractive - continuousRefractive|
```

结果单位：μm

**约束**：`refractiveDelta > 0`（分散相与连续相折射率不能相同）

#### 反算体积浓度 (targetKey = "concentration")

```
φ = max(0, d × refractiveDelta - baseSignal)
concentration = φ × 100
```

展开：
```
concentration = max(0, diameter × |dispersedRefractive - continuousRefractive| - dispersedAbsorption - continuousExtinction) × 100
```

结果单位：%

**约束**：`refractiveDelta > 0`

---

## 6. 分层结果数据来源（sediment_boundary_mm 的计算）

**文件**: `SqlOrm/src/SqlOrmManager.cpp` — `getSeparationLayerDataByExperiment()`

颗粒迁移速率计算依赖的 `sediment_boundary_mm` 并非直接存储在数据库中，而是由 SqlOrmManager 从原始扫描数据**实时计算**的。

### 6.1 原始数据来源

从 `experiment_scan_data` 表加载每轮扫描的原始数据（`loadScanDataByExperiment()`），每轮扫描包含：
- `scanId` — 扫描序号
- `elapsedMs` — 距实验开始的毫秒数
- `startHeightMm` — 起始高度 (mm)
- `stepUm` — 步长 (μm)
- `backscatterValues` — 背射光强度数组（已校准百分比）
- `transmissionValues` — 透射光强度数组（已校准百分比）

### 6.2 分层边界计算算法

对每轮扫描，计算三层分界：

#### 步骤1：选择光信号

```
if (平均透射光强 > 0.2) → 使用透射光 (T)
else → 使用背射光 (BS)
```

#### 步骤2：归一化

```
signal[i] = useTransmission ? transmission[i] : backscatter[i]
normalized[i] = (signal[i] - minSignal) / (maxSignal - minSignal)
proxy[i] = useTransmission ? (1.0 - normalized[i]) : normalized[i]
proxy[i] = clamp(proxy[i], 0, 1)
```

- 透射光取反（1-normalized），因为透射光在沉降区降低、在澄清区升高，取反后与背射光方向一致
- 归一化后 proxy 值：0 = 澄清区，1 = 浓缩/沉降区

#### 步骤3：平滑

```
smoothedProxy = smoothSeries(proxy, windowSize=5)
```

滑动平均平滑，窗口大小5个点，消除噪声。

#### 步骤4：阈值法找边界

```
sediment_boundary_mm = findBoundaryHeightForThreshold(rows, smoothedProxy, threshold=0.8, fromBottom=true)
clarification_boundary_mm = findBoundaryHeightForThreshold(rows, smoothedProxy, threshold=0.2, fromBottom=true)
```

`findBoundaryHeightForThreshold()` 从底部向上扫描，找到 proxy 值首次穿越阈值的点，**线性插值**精确定位：

```
当 proxy[prev] 与 proxy[i] 跨越 threshold 时：
ratio = (threshold - proxy[prev]) / (proxy[i] - proxy[prev])
boundaryHeight = height[prev] + ratio × (height[i] - height[prev])
```

- **沉降界面** (`sediment_boundary_mm`)：proxy 从 ≥0.8 降到 ≤0.8 的位置（从底部向上找，浓缩区与沉降区的分界）
- **澄清界面** (`clarification_boundary_mm`)：proxy 从 ≥0.2 降到 ≤0.2 的位置（从底部向上找，沉降区与澄清区的分界）

#### 步骤5：计算三层厚度

```
sediment_thickness_mm = clamp(sedimentBoundary - minHeight, 0, maxHeight-minHeight)
clarification_thickness_mm = clamp(maxHeight - clarificationBoundary, 0, maxHeight-minHeight)
concentrated_phase_thickness_mm = clamp(clarificationBoundary - sedimentBoundary, 0, maxHeight-minHeight)
```

```
┌──────────────────────────────┐ maxHeight
│  澄清区 (clarification)       │ ← clarification_thickness_mm
│                              │
├── clarification_boundary ────┤
│                              │
│  浓缩区 (concentrated phase) │ ← concentrated_phase_thickness_mm
│                              │
├── sediment_boundary ─────────┤
│                              │
│  沉降区 (sediment)           │ ← sediment_thickness_mm
│                              │
└──────────────────────────────┘ minHeight
```

#### 步骤6：置信度

```
confidence = clamp(range / 30.0, 0, 1)
```

其中 `range = maxSignal - minSignal`。信号动态范围越大，分层结果越可信。

### 6.3 缓存机制

计算结果写入 `separation_layer_data` 表持久化。下次查询时，若已有结果的行数和最大 `scan_elapsed_ms` 与当前扫描数据一致，则直接复用缓存，不再重新计算。

### 6.4 异常处理

- 扫描点数 < 4：无法做阈值分析，直接用首尾高度作为边界，confidence = 0
- 信号动态范围 < 1e-6（平坦信号）：confidence = 0，所有厚度设为0

---

## 7. dataCtrl 桥接层

**文件**: `inc/Controller/data_ctrl.h` / `src/Controller/data_ctrl.cpp`

dataCtrl 是 QML 与 C++ 后端之间的桥接层，暴露三个 `Q_INVOKABLE` 方法：

### 7.1 calculateMigrationRate

```cpp
QVariantMap dataCtrl::calculateMigrationRate(const QVariantMap& params)
{
    const int experimentId = params.value("experimentId").toInt();
    if (experimentId <= 0) {
        return AdvancedCalculationEngine::calculateMigrationRate(params);  // 无数据，返回错误
    }
    // 从数据库加载分层结果（含 sediment_boundary_mm），传入引擎计算
    return AdvancedCalculationEngine::calculateMigrationRate(
        params,
        m_dbManager->getSeparationLayerDataByExperiment(experimentId));
}
```

关键：迁移速率计算依赖实验的**分层结果数据**，dataCtrl 负责从数据库读取后传入引擎。分层结果本身是由原始扫描数据实时计算得到的（见第6节）。

### 7.2 calculateHydrodynamic

```cpp
QVariantMap dataCtrl::calculateHydrodynamic(const QVariantMap& params)
{
    return AdvancedCalculationEngine::calculateHydrodynamic(params);
}
```

纯透传，无额外数据加载。所有参数由用户手动输入。

### 7.3 calculateOptical

```cpp
QVariantMap dataCtrl::calculateOptical(const QVariantMap& params)
{
    return AdvancedCalculationEngine::calculateOptical(params);
}
```

纯透传，无额外数据加载。所有参数由用户手动输入。

---

## 8. 返回值结构

所有三个计算方法返回统一的 `QVariantMap` 结构：

### 成功时

| 字段 | 类型 | 说明 |
|------|------|------|
| `success` | bool | `true` |
| `targetKey` | QString | 计算目标 key |
| `value` | double | 计算结果数值 |
| `displayText` | QString | 格式化显示文本（保留2位小数） |
| `unit` | QString | 结果单位 |
| `message` | QString | 空 |

### 失败时

| 字段 | 类型 | 说明 |
|------|------|------|
| `success` | bool | `false` |
| `targetKey` | QString | 计算目标 key |
| `value` | double | `0.0` |
| `displayText` | QString | 空 |
| `unit` | QString | 结果单位 |
| `message` | QString | 错误原因描述 |

---

## 9. 数据流总览

```
┌─────────────────────────────────────────────────────────────────┐
│ 原始扫描数据 (experiment_scan_data)                              │
│   每轮扫描: scan_id, scan_elapsed_ms,                           │
│            backscatter_values[], transmission_values[]           │
│            (已校准百分比: raw/ref×100)                            │
└──────────────────────┬──────────────────────────────────────────┘
                       │
                       │ SqlOrmManager::getSeparationLayerDataByExperiment()
                       │ (归一化 → 平滑 → 阈值法 → 线性插值)
                       ▼
┌─────────────────────────────────────────────────────────────────┐
│ 分层结果 (separation_layer_data)                                 │
│   每轮扫描: scan_elapsed_ms, sediment_boundary_mm,               │
│            clarification_boundary_mm,                            │
│            sediment/clarification/concentrated_phase_thickness,  │
│            confidence                                            │
└──────────────────────┬──────────────────────────────────────────┘
                       │
                       │ dataCtrl::calculateMigrationRate()
                       │ (找最近两行 → Δboundary/Δtime)
                       ▼
┌─────────────────────────────────────────────────────────────────┐
│ 颗粒迁移速率 (mm/h)                                              │
│   migrationRate = |Δsediment_boundary_mm| / Δhours               │
└──────────────────────┬──────────────────────────────────────────┘
                       │
                       │ 用户手动输入到流体力学区块
                       ▼
┌─────────────────────────────────────────────────────────────────┐
│ 流体力学计算 (Richardson-Zaki 修正 Stokes)                       │
│   v = [d²×(ρp-ρf)×g/(18μ)] × (1-φ)^4.65                       │
│   五选一反算: diameter / concentration / ρp / μ / ρf             │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│ 光学计算 (经验模型)                                               │
│   d = (baseSignal + φ) / Δn                                      │
│   φ = max(0, d×Δn - baseSignal)                                  │
│   二选一反算: diameter / concentration                            │
└─────────────────────────────────────────────────────────────────┘
```

---

## 10. 关键文件索引

| 文件 | 路径 | 职责 |
|------|------|------|
| AdvancedCalculationEngine.h | `inc/Analysis/AdvancedCalculationEngine.h` | 高级计算引擎接口 |
| AdvancedCalculationEngine.cpp | `src/Analysis/AdvancedCalculationEngine.cpp` | 高级计算引擎实现（三大公式） |
| AdvancedCalculationPage.qml | `qml/curve/AdvancedCalculationPage.qml` | 高级计算页面UI |
| data_ctrl.h | `inc/Controller/data_ctrl.h` | 数据控制器接口 |
| data_ctrl.cpp | `src/Controller/data_ctrl.cpp` | 数据控制器实现（桥接QML与引擎） |
| ExperimentDetailPage.qml | `qml/ExperimentDetailPage.qml` | 记录详情主页面 |
| SqlOrmManager.cpp | `SqlOrm/src/SqlOrmManager.cpp` | 分层结果计算（sediment_boundary_mm来源） |

---

## 11. 外部验证方法

以下提供每个计算模块的**手工验算用例**，可用 Python / MATLAB / Excel 等外部工具独立复现，与软件输出对比验证。

### 11.1 颗粒迁移速率验证

**验证思路**：构造已知分层结果行，手工计算迁移速率，与软件输出对比。

**测试数据**：

| 行号 | scan_elapsed_ms | sediment_boundary_mm |
|------|-----------------|---------------------|
| 1 | 3600000 (1h) | 30.0 |
| 2 | 7200000 (2h) | 28.0 |
| 3 | 10800000 (3h) | 26.0 |

**验算**：起止时间 1h ~ 3h

```
startElapsedMs = 1×60×60×1000 = 3600000  → 匹配行1, boundary = 30.0
endElapsedMs   = 3×60×60×1000 = 10800000 → 匹配行3, boundary = 26.0

Δboundary = |26.0 - 30.0| = 4.0 mm
Δhours = |10800000 - 3600000| / 3600000 = 2.0 h

migrationRate = 4.0 / 2.0 = 2.0 mm/h
```

**Python 验证脚本**：

```python
# 颗粒迁移速率验证
start_boundary = 30.0   # mm
end_boundary = 26.0     # mm
start_elapsed_ms = 3600000
end_elapsed_ms = 10800000

delta_boundary = abs(end_boundary - start_boundary)
delta_hours = abs(end_elapsed_ms - start_elapsed_ms) / 3600000
migration_rate = delta_boundary / max(1e-9, delta_hours)

print(f"迁移速率: {migration_rate:.2f} mm/h")  # 期望: 2.00
```

---

### 11.2 流体力学计算验证

**验证思路**：先用正算验证公式正确性，再用反算验证闭环一致性。

#### 验证1：正算 — 已知全部参数求沉降速度

**测试数据**：

| 参数 | 界面值 | SI值 |
|------|--------|------|
| 粒径 d | 50 μm | 50e-6 m |
| 体积浓度 φ | 10 % | 0.1 |
| 分散相密度 ρp | 2.5 g/cm³ | 2500 kg/m³ |
| 连续相密度 ρf | 1.0 g/cm³ | 1000 kg/m³ |
| 连续相粘度 μ | 1.0 cP | 0.001 Pa·s |
| 重力加速度 g | — | 9.80665 m/s² |
| R-Z指数 n | — | 4.65 |

**手工验算**：

```
densityDiff = 2500 - 1000 = 1500 kg/m³
hinderedFactor = (1 - 0.1)^4.65 = 0.9^4.65

0.9^4.65 = exp(4.65 × ln(0.9)) = exp(4.65 × (-0.1053605)) = exp(-0.489926) = 0.61268

v = (50e-6)² × 1500 × 9.80665 × 0.61268 / (18 × 0.001)
  = 2.5e-9 × 1500 × 9.80665 × 0.61268 / 0.018
  = 2.5e-9 × 9014.5 / 0.018
  = 2.2536e-5 / 0.018
  = 1.2520e-3 m/s

migrationRate = 1.2520e-3 × 3600000 = 4507.2 mm/h
```

**Python 验证脚本**：

```python
import math

# 输入参数（SI单位）
d = 50e-6           # m
phi = 0.10          # 无量纲
rho_p = 2500.0      # kg/m³
rho_f = 1000.0      # kg/m³
mu = 0.001          # Pa·s
g = 9.80665         # m/s²
n = 4.65

# 正算
density_diff = rho_p - rho_f
hindered_factor = (1 - phi) ** n
v = (d**2 * density_diff * g * hindered_factor) / (18 * mu)
migration_rate = v * 3600000  # m/s → mm/h

print(f"受阻沉降速度: {v:.6e} m/s")        # 期望: 1.252e-3
print(f"迁移速率: {migration_rate:.2f} mm/h")  # 期望: 4507.20
```

#### 验证2：反算粒径 — 闭环验证

将正算得到的迁移速率 4507.2 mm/h 作为输入，反算粒径，应得到 50.00 μm。

```python
# 反算粒径
v_input = migration_rate / 3600000  # mm/h → m/s
d_calc = math.sqrt(18 * mu * v_input / (density_diff * g * hindered_factor))
d_um = d_calc / 1e-6

print(f"反算粒径: {d_um:.2f} μm")  # 期望: 50.00
```

#### 验证3：反算体积浓度 — 闭环验证

```python
# 反算体积浓度
inner = 18 * mu * v_input / (d**2 * density_diff * g)
phi_calc = 1 - inner ** (1 / n)
concentration_pct = phi_calc * 100

print(f"反算体积浓度: {concentration_pct:.2f} %")  # 期望: 10.00
```

#### 验证4：反算分散相密度 — 闭环验证

```python
# 反算分散相密度
rho_p_calc = rho_f + 18 * mu * v_input / (d**2 * g * hindered_factor)
rho_p_gcm3 = rho_p_calc / 1000

print(f"反算分散相密度: {rho_p_gcm3:.2f} g/cm³")  # 期望: 2.50
```

#### 验证5：反算连续相粘度 — 闭环验证

```python
# 反算连续相粘度
mu_calc = d**2 * density_diff * g * hindered_factor / (18 * v_input)
mu_cp = mu_calc / 0.001

print(f"反算连续相粘度: {mu_cp:.2f} cP")  # 期望: 1.00
```

#### 验证6：反算连续相密度 — 闭环验证

```python
# 反算连续相密度
rho_f_calc = rho_p - 18 * mu * v_input / (d**2 * g * hindered_factor)
rho_f_gcm3 = rho_f_calc / 1000

print(f"反算连续相密度: {rho_f_gcm3:.2f} g/cm³")  # 期望: 1.00
```

#### 验证7：文献对比 — 经典 Stokes 沉降

**参考**: Richardson & Zaki (1954), Chem. Eng. Sci., 3(2), 65-77

标准条件：25°C 水中 100μm 石英颗粒（ρp=2650 kg/m³, ρf=997 kg/m³, μ=0.891 cP）

```python
d = 100e-6
rho_p = 2650.0
rho_f = 997.0
mu = 0.891e-3
g = 9.80665
phi = 0.0  # 单颗粒，无浓度修正

v_stokes = d**2 * (rho_p - rho_f) * g / (18 * mu)
print(f"Stokes沉降速度: {v_stokes:.6e} m/s")  # 期望: ~1.01e-3 m/s (文献值约1.0e-3)

# 加浓度修正 φ=5%
phi = 0.05
v_hindered = v_stokes * (1 - phi)**4.65
print(f"受阻沉降速度(φ=5%): {v_hindered:.6e} m/s")  # 应小于 v_stokes
```

#### 验证8：边界条件 — 异常输入

| 场景 | 输入 | 期望结果 |
|------|------|---------|
| 分散相密度 ≤ 连续相密度 | ρp=0.8, ρf=1.0 | 反算粒径/浓度/粘度失败，message 含提示 |
| 体积浓度 = 0 | concentration=0 | hinderedFactor = 1.0，退化为纯 Stokes |
| 体积浓度 = 100 | concentration=100 | hinderedFactor = 0，v = 0，反算失败 |
| 迁移速率 = 0 | migrationRate=0 | 反算失败（除零保护） |

---

### 11.3 光学计算验证

**验证思路**：光学模型为经验公式，无物理文献可对照，主要验证**正反算闭环一致性**。

#### 验证1：反算粒径

**测试数据**：

| 参数 | 值 |
|------|-----|
| 分散相折射率 | 1.55 |
| 分散相吸收率 | 0.02 |
| 连续相折射率 | 1.33 |
| 连续相消光范围 | 0.01 |
| 体积浓度 | 15% |

**手工验算**：

```
refractiveDelta = |1.55 - 1.33| = 0.22
baseSignal = 0.02 + 0.01 = 0.03
φ = 15 / 100 = 0.15

d = (baseSignal + φ) / refractiveDelta
  = (0.03 + 0.15) / 0.22
  = 0.18 / 0.22
  = 0.8182 μm
```

**Python 验证脚本**：

```python
dispersed_refractive = 1.55
dispersed_absorption = 0.02
continuous_refractive = 1.33
continuous_extinction = 0.01
concentration = 15.0  # %

refractive_delta = abs(dispersed_refractive - continuous_refractive)
base_signal = dispersed_absorption + continuous_extinction
phi = concentration / 100

diameter = (base_signal + phi) / refractive_delta
print(f"反算粒径: {diameter:.2f} μm")  # 期望: 0.82
```

#### 验证2：反算体积浓度 — 闭环验证

将上面算出的粒径作为输入，反算浓度，应得到 15.00%。

```python
d_input = diameter  # 用验证1的结果
phi_calc = max(0, d_input * refractive_delta - base_signal)
concentration_calc = phi_calc * 100

print(f"反算体积浓度: {concentration_calc:.2f} %")  # 期望: 15.00
```

#### 验证3：边界条件

| 场景 | 输入 | 期望结果 |
|------|------|---------|
| 折射率相同 | dispersed=1.33, continuous=1.33 | refractiveDelta=0，计算失败 |
| 粒径极小 | d=0.01 | concentration = max(0, 0.01×0.22-0.03)×100 = 0% |
| 粒径极大 | d=1000 | concentration = (1000×0.22-0.03)×100 = 21997% |

---

### 11.4 分层边界计算验证

**验证思路**：构造已知光强分布的扫描数据，验证归一化、平滑、阈值法、线性插值的正确性。

#### 验证1：线性插值精度

构造一个简单的 proxy 序列，验证 `findBoundaryHeightForThreshold` 的线性插值：

```
高度:  [0, 5, 10, 15, 20, 25, 30] mm
proxy: [1.0, 0.9, 0.7, 0.5, 0.3, 0.1, 0.0]

找 threshold=0.8, fromBottom=true:
  从底部(索引0)向上扫描
  proxy[0]=1.0 ≥ 0.8, proxy[1]=0.9 ≥ 0.8, proxy[2]=0.7 < 0.8
  跨越点: i=2, prev=1
  ratio = (0.8 - 0.9) / (0.7 - 0.9) = (-0.1) / (-0.2) = 0.5
  boundaryHeight = 5 + 0.5 × (10 - 5) = 7.5 mm
```

**Python 验证脚本**：

```python
def find_boundary(heights, proxies, threshold, from_bottom=True):
    n = len(proxies)
    if n < 2:
        return heights[0] if heights else 0.0
    start = 1 if from_bottom else n - 1
    end = n if from_bottom else 0
    step = 1 if from_bottom else -1
    for i in range(start, end, step):
        prev = i - step
        left = proxies[prev]
        right = proxies[i]
        crossed = (left >= threshold and right <= threshold) or \
                  (left <= threshold and right >= threshold)
        if crossed and abs(left - right) > 1e-12:
            ratio = (threshold - left) / (right - left)
            return heights[prev] + ratio * (heights[i] - heights[prev])
    return heights[-1] if from_bottom else heights[0]

heights = [0, 5, 10, 15, 20, 25, 30]
proxies = [1.0, 0.9, 0.7, 0.5, 0.3, 0.1, 0.0]

sediment = find_boundary(heights, proxies, 0.8, from_bottom=True)
clarification = find_boundary(heights, proxies, 0.2, from_bottom=True)

print(f"沉降界面: {sediment:.2f} mm")          # 期望: 7.50
print(f"澄清界面: {clarification:.2f} mm")      # 期望: 22.50
print(f"浓缩区厚度: {clarification - sediment:.2f} mm")  # 期望: 15.00
```

#### 验证2：平滑算法

```python
def smooth_series(values, window_size=5):
    if not values or window_size <= 1:
        return values[:]
    half = window_size // 2
    result = []
    for i in range(len(values)):
        start = max(0, i - half)
        end = min(len(values) - 1, i + half)
        avg = sum(values[start:end+1]) / (end - start + 1)
        result.append(avg)
    return result

# 测试: 含噪声的阶跃信号
raw = [0.1, 0.1, 0.1, 0.9, 0.9, 0.1, 0.9, 0.9, 0.9, 0.9]
smoothed = smooth_series(raw, 5)
print(f"原始: {raw}")
print(f"平滑: {[round(v, 3) for v in smoothed]}")
# 期望: 噪声点(索引5的0.1)被平滑为中间值
```

---

### 11.5 完整端到端验证流程

**操作步骤**：

1. **准备已知数据**：在数据库中插入一组已知扫描数据（或通过软件运行一次标准样品实验）
2. **分层边界验证**：打开"分层厚度"Tab，对比软件显示的边界位置与上述 Python 脚本计算结果
3. **迁移速率验证**：在"高级计算"Tab 输入起止时间，对比软件输出与手工计算
4. **流体力学验证**：输入验证2中的参数（迁移速率=4507.2 mm/h, ρp=2.5, ρf=1.0, μ=1.0, φ=10%），反算粒径应为 50.00 μm
5. **光学验证**：输入验证1中的参数（折射率1.55/1.33, 吸收0.02, 消光0.01, 浓度15%），反算粒径应为 0.82 μm
6. **闭环一致性**：将反算结果重新作为输入，正向计算应回到原始值

**判定标准**：

| 指标 | 允许偏差 |
|------|---------|
| 迁移速率 | ±0.01 mm/h |
| 流体力学反算 | ±0.01（对应单位） |
| 光学反算 | ±0.01（对应单位） |
| 分层边界 | ±0.1 mm |
| 闭环误差 | < 0.01% |
