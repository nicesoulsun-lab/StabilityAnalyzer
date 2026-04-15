# Communication Baseline V1

## Scope
This baseline is for the current 4-channel experiment workflow (ChannelA/ChannelB/ChannelC/ChannelD) and must match the current `TaskScheduler` parser.

## 1) Required JSON Structure (Scheduler-side)
The scheduler currently expects:

- Top-level key: `device`
- Required inside `device`:
  - `slaveId` (int)
  - `name` (string)
  - `serialConfig` (object)
  - `taskList` (array)

### serialConfig
- `portName` (string)
- `baudRate` (int)
- `dataBits` (int)
- `parity` (string, e.g. `NoParity`)
- `stopBits` (int)
- `flowControl` (string, e.g. `NoFlowControl`)
- Optional: `interFrameDelay`, `responseTimeout`, `maxRetries`

### taskList item
- `name` (string)
- `operate` (int)
  - `0` = init/polling task
  - `1` = user-triggered task
- `type` (string enum, uppercase)
  - `READ_COILS`
  - `READ_DISCRETE_INPUTS`
  - `READ_HOLDING_REGISTERS`
  - `READ_INPUT_REGISTERS`
  - `WRITE_SINGLE_COIL`
  - `WRITE_SINGLE_REGISTER`
  - `WRITE_MULTIPLE_COILS`
  - `WRITE_MULTIPLE_REGISTERS`
- `address` (int)
- `count` (int)
- Optional: `pollingTime` (ms; if present and `operate=0`, task can be periodic)

## 2) Migration Mapping (Current generated format -> V1)
- `tasks` -> `taskList`
- `taskName` -> `name`
- `startAddress` -> `address`
- `quantity` -> `count`
- `taskType` (`user_task`) -> `operate=1`
- `taskType` (`init_task`) -> `operate=0`
- `type` lowercase (`read_input_registers`) -> uppercase enum (`READ_INPUT_REGISTERS`)
- Root `{ deviceId, deviceName, serialConfig, ... }` -> `{ "device": { slaveId, name, serialConfig, ... } }`

## 3) Four-channel template files
Templates are generated under:
- `docs/config_templates_v1/ChannelA.json`
- `docs/config_templates_v1/ChannelB.json`
- `docs/config_templates_v1/ChannelC.json`
- `docs/config_templates_v1/ChannelD.json`

These templates are aligned with current calls in `ExperimentCtrl`:
- read_sensor_data
- set_temperature
- set_scan_range
- set_step
- start_scan
- stop_scan
- enable_temperature_control
- disable_temperature_control

## 4) Notes
- If one COM port has multiple slave devices, keep `portName` same and use different `slaveId`.
- `TaskScheduler` currently uses `slaveId` as internal device id string.
- Keep `type` uppercase exactly as listed above.
