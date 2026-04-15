# Linux 虚拟串口联调方案

这套方案按你当前项目代码整理，并兼容两种模式：

- `tty0tty` 模式：上位机用 `tnt1/tnt3/tnt5/tnt7`
- `socat` 模式：上位机直接用显式路径，例如 `/tmp/ttySA_A_HOST`

你现在这台 Linux 机器没有 `tty0tty` 模块，所以建议直接用 `socat` 模式。

## 关键前提

当前项目里真正限制虚拟串口方案的点在 `PortManager::addPort()`。
我已经把 Linux 下的端口校验补成两种都接受：

1. Qt 枚举到的短串口名，例如 `tnt1`
2. 显式存在的串口路径，例如 `/tmp/ttySA_A_HOST`

所以现在可以直接用 `socat` 生成 PTY 对，不需要内核 `tty0tty` 模块。

## 默认映射

- ChannelA: 上位机 `/tmp/ttySA_A_HOST`，模拟器 `/tmp/ttySA_A_DEV`，`slaveId=1`
- ChannelB: 上位机 `/tmp/ttySA_B_HOST`，模拟器 `/tmp/ttySA_B_DEV`，`slaveId=2`
- ChannelC: 上位机 `/tmp/ttySA_C_HOST`，模拟器 `/tmp/ttySA_C_DEV`，`slaveId=3`
- ChannelD: 上位机 `/tmp/ttySA_D_HOST`，模拟器 `/tmp/ttySA_D_DEV`，`slaveId=4`

## 本目录文件

- `generate_experiment_config.py`
  生成 `ChannelA~D.json`
- `linux_virtual_lower_device.py`
  单个虚拟下位机实例
- `start_virtual_lower_device_rtu.sh`
  单通道启动
- `start_virtual_lower_device_all.sh`
  4 通道启动
- `stop_virtual_lower_device_all.sh`
  停止 4 通道模拟器
- `start_socat_vtty_pairs.sh`
  创建 4 组 `socat` 虚拟串口对
- `stop_socat_vtty_pairs.sh`
  停止 `socat` 虚拟串口对
- `list_serial_ports.py`
  查看 Python 能看到的串口
- `check_tty0tty.sh`
  检查当前系统是否有 `tty0tty`
- `EXPERIMENT_MODBUS_TROUBLESHOOTING.md`
  `experiment` 联调排障清单

## 1. 依赖

```bash
apt-get update
apt-get install -y python3 python3-pip socat
python3 -m pip install "pymodbus==2.5.3" pyserial
```

## 2. 推荐方案：socat

先创建 4 组 PTY 对：

```bash
bash tools/virtual_device/start_socat_vtty_pairs.sh
```

正常时会看到：

- `/tmp/ttySA_A_HOST` <-> `/tmp/ttySA_A_DEV`
- `/tmp/ttySA_B_HOST` <-> `/tmp/ttySA_B_DEV`
- `/tmp/ttySA_C_HOST` <-> `/tmp/ttySA_C_DEV`
- `/tmp/ttySA_D_HOST` <-> `/tmp/ttySA_D_DEV`

然后生成 JSON：

```bash
python3 tools/virtual_device/generate_experiment_config.py \
  --output-dir ./config/experiment_devices \
  --host-port-names /tmp/ttySA_A_HOST,/tmp/ttySA_B_HOST,/tmp/ttySA_C_HOST,/tmp/ttySA_D_HOST \
  --slave-ids 1,2,3,4
```

再启动 4 个虚拟下位机：

```bash
DEVICE_PORTS=/tmp/ttySA_A_DEV,/tmp/ttySA_B_DEV,/tmp/ttySA_C_DEV,/tmp/ttySA_D_DEV \
bash tools/virtual_device/start_virtual_lower_device_all.sh
```

最后启动你的上位机程序。

## 3. 如果以后你的机器有 tty0tty

可以继续用旧式短名：

```bash
bash tools/virtual_device/check_tty0tty.sh

python3 tools/virtual_device/generate_experiment_config.py \
  --output-dir ./config/experiment_devices \
  --host-port-names tnt1,tnt3,tnt5,tnt7 \
  --slave-ids 1,2,3,4

bash tools/virtual_device/start_virtual_lower_device_all.sh
```

默认对应：

- ChannelA: `/dev/tnt0` <-> `tnt1`
- ChannelB: `/dev/tnt2` <-> `tnt3`
- ChannelC: `/dev/tnt4` <-> `tnt5`
- ChannelD: `/dev/tnt6` <-> `tnt7`

## 4. 单通道调试

例如只调 ChannelC：

```bash
DEVICE_PORT_PATH=/tmp/ttySA_C_DEV SLAVE_IDS=3 \
bash tools/virtual_device/start_virtual_lower_device_rtu.sh
```

## 5. 运行目录注意

`ExperimentCtrl` 实际读取的是：

`QCoreApplication::applicationDirPath() + "/config/experiment_devices"`

所以 JSON 要生成到程序真实运行目录下面。

## 6. 当前最推荐命令

```bash
bash tools/virtual_device/start_socat_vtty_pairs.sh

python3 tools/virtual_device/generate_experiment_config.py \
  --output-dir ./config/experiment_devices \
  --host-port-names /tmp/ttySA_A_HOST,/tmp/ttySA_B_HOST,/tmp/ttySA_C_HOST,/tmp/ttySA_D_HOST \
  --slave-ids 1,2,3,4

DEVICE_PORTS=/tmp/ttySA_A_DEV,/tmp/ttySA_B_DEV,/tmp/ttySA_C_DEV,/tmp/ttySA_D_DEV \
bash tools/virtual_device/start_virtual_lower_device_all.sh
```
