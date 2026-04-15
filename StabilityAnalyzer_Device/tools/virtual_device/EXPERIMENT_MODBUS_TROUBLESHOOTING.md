# Experiment / Modbus 联调排障清单

这份文档用于 Linux 上联调 `experiment` 模块时快速定位问题。

## 先看当前前提

你的项目是 JSON 驱动的：

- `ExperimentCtrl` 只按任务名调度
- `TaskScheduler` 从 `config/experiment_devices/*.json` 解析 `slaveId`、`serialConfig`、`taskList`
- `TaskScheduler` 内部把 `slaveId` 当作 `deviceId`

程序实际读取的是运行目录下：

`QCoreApplication::applicationDirPath() + "/config/experiment_devices"`

## 串口配置现在支持两类

1. Qt 枚举到的短名，例如 `tnt1`
2. Linux 显式路径，例如 `/tmp/ttySA_A_HOST`

所以你当前机器没有 `tty0tty` 时，推荐用：

- ChannelA: `/tmp/ttySA_A_HOST`
- ChannelB: `/tmp/ttySA_B_HOST`
- ChannelC: `/tmp/ttySA_C_HOST`
- ChannelD: `/tmp/ttySA_D_HOST`

## 必看启动日志

- `[ExperimentCtrl][Init] initializeScheduler configPath=...`
- `[ExperimentCtrl][Init] config file count=...`
- `[ExperimentCtrl][Init][ConfigRaw] file=...`
- `[ExperimentCtrl][Init] configuration loaded`
- `[ExperimentCtrl][Init] scheduler started, connectedDevices=...`

## 常见故障

### 1. `connectedDevices=0`

优先看：

- `Port does not exist: xxx`
- `Failed to add port: xxx for device: yyy`

通常说明：

1. JSON 里的 `portName` 写错了
2. `socat` 还没启动
3. 你生成 JSON 的路径和程序实际运行目录不一致

### 2. 端口存在，但连接失败

优先看：

- `Failed to connect port: xxx for device: yyy`
- `正在连接串口:`
- `串口连接成功:`

如果只有“正在连接串口”没有“串口连接成功”，检查：

1. 模拟器是不是监听在对应的 `*_DEV` 一侧
2. 波特率、校验位、停止位是否一致
3. `socat` 进程是不是还活着

### 3. `Device not found with slaveId`

通常说明：

1. JSON 没加载成功
2. `slaveId` 配错了
3. 通道之间 `slaveId` 重复

### 4. `Task not found`

通常说明 `ExperimentCtrl` 调用的任务名和 JSON 里的 `taskList.name` 不一致。

### 5. 实验启动成功，但读不到数据

优先看：

- `[ExperimentCtrl][CMD] ... task=...`
- `[ExperimentCtrl] task completed: ... exception: ...`
- `[ExperimentCtrl][READ] ... size=... expect>=...`
- `[ExperimentCtrl][READ] insufficient result`

如果只有某一个 channel 失败，优先排查它自己的端口映射：

- ChannelA: `/tmp/ttySA_A_HOST` <-> `/tmp/ttySA_A_DEV`
- ChannelB: `/tmp/ttySA_B_HOST` <-> `/tmp/ttySA_B_DEV`
- ChannelC: `/tmp/ttySA_C_HOST` <-> `/tmp/ttySA_C_DEV`
- ChannelD: `/tmp/ttySA_D_HOST` <-> `/tmp/ttySA_D_DEV`

## Linux 上建议直接执行的检查

检查 JSON：

```bash
ls ./config/experiment_devices
grep -n '"portName"' ./config/experiment_devices/ChannelA.json
```

检查 `socat` 端口：

```bash
ls -l /tmp/ttySA_*
ps -ef | grep socat
```

检查虚拟下位机：

```bash
ps -ef | grep linux_virtual_lower_device.py
```

## 最推荐的排查顺序

1. 先确认程序实际加载的是哪份 JSON
2. 再确认 `portName` 是否和当前 `socat` 路径一致
3. 再确认 4 个 `socat` 端口对是否都已创建
4. 再确认 4 个虚拟下位机实例是否都已启动
5. 最后看具体任务读写日志
