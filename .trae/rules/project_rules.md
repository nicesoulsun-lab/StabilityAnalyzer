# StabilityAnalyzer 项目规则

## 项目结构
- `StabilityAnalyzer_Device/` — 设备端（Qt/C++，嵌入式 Linux）
- `StabilityAnalyzer_PC/` — PC 端（Qt/QML/C++，Windows）

## 通信架构
- **Control 9000**：命令/响应（PC → Device 请求，Device → PC ACK）
- **Status 9001**：状态推送（Device → PC 单向，心跳、设备信息、通道状态）
- **Stream 9002**：数据流推送（Device → PC 单向，实验数据、校准数据）

## 关键协议

### 校准相关
- `start_calibration_scan`：PC → Device，触发单次校准扫描（不入库），参数：channel, scan_range_start, scan_range_end, scan_step
- `calibration_scan_data`：Device → PC（Stream 9002），校准扫描数据推送，数据为原始值（不经校准转换）
- `set_calibration`：PC → Device，下发校准参数，参数：channel, transmission_reference, backscatter_reference

### 实验相关
- `start_experiment` / `stop_experiment`：实验控制
- `get_experiment_export` / `get_experiment_scan_export`：数据导入（分页）
- `mark_experiment_imported`：标记已导入

## 校准功能设计决策
- 校准不在实验运行中进行，校准参数在下次实验启动时注入 SessionService
- 校准扫描独立于实验流程，不入库、不写内存缓存
- 设备端用 `runStatus` 判断扫描完成（必须先见过 runStatus==2，才能认定后续非2为完成）
- A/B 存储区哪个可取就取哪个，取完回写3
- PC 端平均光强用预期点数作除数（避免丢点导致均值偏高）
- PC 端维护校准缓存（m_transmissionCalibrations / m_backscatterCalibrations），避免只校准一种光时覆盖另一种
- 校准参数持久化到 INI 文件 `[Calibration/ChannelX]` 组

## 数据转换公式
- 校准参考值 ref > 0：`raw / ref * 100.0`
- 校准参考值 ref = 0：直接存原始值

## 代码风格
- 中文注释
- 不加英文注释
- 遵循现有代码风格和命名规范

## 构建与检查
- 修改后需运行 lint 和 typecheck（如有配置）
