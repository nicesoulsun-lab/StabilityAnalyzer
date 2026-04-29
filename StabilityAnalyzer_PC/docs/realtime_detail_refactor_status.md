# 实时/详情重构状态说明

更新时间：2026-04-29

适用范围：`StabilityAnalyzer_PC`

当前约束：
- Qt 版本：`Qt 5.12`
- 前端画面布局保持不变
- 实时不落库
- 详情只看导入后的正式实验数据
- 优化项可后置，当前优先保证实时稳定、详情不拖垮内存

## 一、这轮已经完成的工作

### 1. 实时与详情控制器拆分
- 新增实时控制器：
  - `StabilityAnalyzer_PC/SubApplication/ANALYZER/MainWindow/inc/Controller/realtime_ctrl.h`
  - `StabilityAnalyzer_PC/SubApplication/ANALYZER/MainWindow/src/Controller/realtime_ctrl.cpp`
- 新增详情控制器：
  - `StabilityAnalyzer_PC/SubApplication/ANALYZER/MainWindow/inc/Controller/detail_ctrl.h`
  - `StabilityAnalyzer_PC/SubApplication/ANALYZER/MainWindow/src/Controller/detail_ctrl.cpp`
- `controllerManager.h` 已负责统一装配和注入 `realtime_ctrl` / `detail_ctrl`

### 2. 实时页主链已独立
- 首页进入实时页的入口在：
  - `StabilityAnalyzer_PC/SubApplication/ANALYZER/MainWindow/qml/HomePage.qml`
- 实时主页面在：
  - `StabilityAnalyzer_PC/SubApplication/ANALYZER/MainWindow/qml/RealtimeExperimentPage.qml`
- 实时页不再加载共享详情子页，已切到三张实时专用页面：
  - `StabilityAnalyzer_PC/SubApplication/ANALYZER/MainWindow/qml/curve/RealtimeLightIntensityPage.qml`
  - `StabilityAnalyzer_PC/SubApplication/ANALYZER/MainWindow/qml/curve/RealtimeInstabilityCurvePage.qml`
  - `StabilityAnalyzer_PC/SubApplication/ANALYZER/MainWindow/qml/curve/RealtimeUniformityIndexPage.qml`
- `mainwindow.qrc` 已注册上述实时专用页面

### 3. 实时缓存策略已从“页面隐藏即清空”改为“后台轻缓存”
- 实时流会继续进入 `realtime_ctrl`
- 页面不可见时不做前台刷新，但后台仍保留最近缓存
- 页面重新进入或重新激活后，会先回放缓存，再接收后续增量
- 当前缓存策略：
  - 每个实验最多保留最近 `20` 条曲线
  - 单条曲线最大点数 `512`
  - 全局最多保留 `8` 个实验缓存
  - 同一通道切到新实验时，会清理该通道旧实验缓存

### 4. 详情页异步化已完成第一批
- 光强详情已改为 `detail_ctrl` 异步加载
- 不稳定性详情已改为 `detail_ctrl` 异步加载
- 详情页原位 loading 已补上，不改变页面布局
- 相关页面：
  - `StabilityAnalyzer_PC/SubApplication/ANALYZER/MainWindow/qml/ExperimentDetailPage.qml`
  - `StabilityAnalyzer_PC/SubApplication/ANALYZER/MainWindow/qml/curve/LightIntensityCurvePage.qml`
  - `StabilityAnalyzer_PC/SubApplication/ANALYZER/MainWindow/qml/curve/InstabilityCurvePage.qml`

### 5. 旧实时路径已清掉一批
- `data_ctrl` 中大量旧的实时缓存、实时曲线、旧光强接口已删除
- 共享曲线页中的 realtime/detail 双轨逻辑已收缩一批
- 已删除旧 `LightCurveAnalysisCache`

## 二、当前实时部分的行为边界

### 1. 现在实时会做什么
- 接收 `type = experiment_scan_data` 的完整实时消息
- 只处理 `scan_completed = true` 的完整扫描结果
- 降采样后写入 `realtime_ctrl` 内存缓存
- 当前实时页可见时，向前台页面发送变更通知
- 当前实时页重新进入时，从缓存恢复最近一段曲线

### 2. 现在实时不会做什么
- 不落数据库
- 不补历史全量数据
- 不追溯页面未打开之前更久远的实验曲线
- 不做详情异步链那套大查询

### 3. 当前实时内存边界
- 后台保留的是最近窗口，不是整场实验
- 当前参数：
  - 每实验 `20` 条 curve
  - 每 curve 最多 `512` 点
  - 全局最多 `8` 个实验缓存
- 这套边界的目标是：
  - 切页面回来不空白
  - 又不让实时缓存无限涨

## 三、当前仍然必须继续做的事项

### 1. Qt 5.12 构建和回归验证
- 当前改动已做静态检查，但还没有在本机做完整编译验证
- 当前阻塞是：环境里 `qmake` 不在 `PATH`
- 必须补做：
  - `Application` 入口编译
  - `MainWindow` 相关工程编译
  - Qt 5.12 QML 运行态验证

### 2. 实时路径回归测试
- 必测场景：
  - 从首页进入实时页
  - 实时页切 `光强 / 不稳定性 / 均匀度`
  - 从实时页切走，再切回来
  - 切通道
  - 实验停止后回到实时页
  - 连续接收超过 `20` 条曲线时是否稳定

### 3. 详情剩余重页仍需继续迁移
- 还未完全迁移到 `detail_ctrl` 的页面包括：
  - `StabilityAnalyzer_PC/SubApplication/ANALYZER/MainWindow/qml/curve/UniformityIndexPage.qml`
  - `StabilityAnalyzer_PC/SubApplication/ANALYZER/MainWindow/qml/curve/PeakThicknessPage.qml`
  - `StabilityAnalyzer_PC/SubApplication/ANALYZER/MainWindow/qml/curve/LightIntensityAveragePage.qml`
  - `StabilityAnalyzer_PC/SubApplication/ANALYZER/MainWindow/qml/curve/SeparationLayerPage.qml`
  - `StabilityAnalyzer_PC/SubApplication/ANALYZER/MainWindow/qml/curve/AdvancedCalculationPage.qml`
- 这些页面后续仍要继续拆出同步大查询

### 4. 废弃代码还需再清一轮
- 目标不是简单“能跑”，而是避免形成新的双轨代码
- 当前还需要继续检查并清理：
  - 共享详情曲线页中的残留兼容分支
  - `data_ctrl` 中剩余的过时接口
  - 无实际调用的旧函数和旧注释

## 四、可以后置的优化项

以下内容可以放到后续阶段，不作为当前阻断项：

### 1. 实时绘图性能优化
- 将实时光强从多 `CurveItem` 叠加改成单个批量绘制组件
- 将“缓存 20 条”与“默认绘制条数”进一步拆开
- 做更细的渲染节流

### 2. 实时缓存结构继续瘦身
- 现在 `realtime_ctrl` 仍然大量使用 `QVariantMap / QVariantList`
- 后续可进一步改成强类型环形缓存，减少多轮复制

### 3. 日志体系统一
- 统一 realtime/detail 的日志字段
- 加 warning 节流
- 对高频日志做 debug/info 分级

### 4. 派生图的轻量结果缓存
- 不稳定性、均匀度目前仍是按实时缓存临时计算
- 后续可补轻量派生缓存，减少重复计算

## 五、建议的下一阶段顺序

### 第一阶段
- 先补 Qt 5.12 编译验证
- 先做实时页完整回归

### 第二阶段
- 继续迁移详情剩余重页到 `detail_ctrl`
- 继续清理 `data_ctrl` 和共享页中的废弃代码

### 第三阶段
- 再做实时绘图和缓存结构优化
- 再统一日志和性能治理

## 六、当前关键日志点

### 实时链路
- `[HomePage][open realtime]`
- `[RealtimeLight][subscribe active]`
- `[RealtimeLight][restore cached state]`
- `[RealtimeLight][load curves]`
- `[RealtimePage][tab loaded]`
- `[RealtimeLightPage][state]`
- `[RealtimeInstability][reload]`
- `[RealtimeUniformity][ready]`
- `[realtimeCtrl][stream applied]`
- `[realtimeCtrl][trim experiment cache]`
- `[realtimeCtrl][channel experiment switched]`

### 详情链路
- `[DetailLight][request]`
- `[DetailLight][finished signal]`
- `[DetailLight][ready]`
- `[DetailInstability][overview request]`
- `[DetailInstability][overview ready]`
- `[DetailInstability][custom request]`
- `[DetailInstability][custom ready]`

## 七、当前已知未验证风险

- 尚未完成 Qt 5.12 实机构建验证
- 实时专用页虽然已独立，但尚未做完整长时间稳定性回归
- 详情剩余页面尚未全部切到异步控制器

