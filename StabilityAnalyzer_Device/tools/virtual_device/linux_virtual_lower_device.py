#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Linux virtual lower-device simulator for StabilityAnalyzer.
"""

import argparse
from datetime import datetime
import math
import threading
import time

try:
    from pymodbus.server.sync import StartSerialServer
    from pymodbus.datastore import ModbusServerContext, ModbusSlaveContext, ModbusSequentialDataBlock
    from pymodbus.device import ModbusDeviceIdentification
    from pymodbus.transaction import ModbusRtuFramer
except Exception:
    from pymodbus.server import StartSerialServer
    from pymodbus.datastore import ModbusServerContext, ModbusSlaveContext, ModbusSequentialDataBlock
    from pymodbus.device import ModbusDeviceIdentification
    try:
        from pymodbus.transaction import ModbusRtuFramer
    except Exception:
        from pymodbus.framer.rtu_framer import ModbusRtuFramer


HR_SIZE = 256
IR_SIZE = 1200
A_BASE = 0
B_BASE = 500
PAIR_COUNT = 250
INTENSITY_SCALE = 10
INTENSITY_MIN_PERCENT = 0.0
INTENSITY_MAX_PERCENT = 100.0

REG_START_FLAG = 0
REG_RUN_STATUS = 1
REG_START_POS = 2
REG_END_POS = 4
REG_CURRENT_POS = 6
REG_STEP = 8
REG_INTERVAL_TIME = 9
REG_COVER_STATUS = 10
REG_SAMPLE_STATUS = 11
REG_TRANSMISSION = 12
REG_BACKSCATTER = 13
REG_TEMP_CONTROL = 14
REG_TARGET_TEMP = 15
REG_CURRENT_TEMP = 16
REG_STORAGE_LIMIT = 17
REG_TOTAL_REQUIRED = 18
REG_SCANNED_COUNT = 19
REG_STORAGE_A_READABLE = 20
REG_STORAGE_B_READABLE = 21
REG_STORAGE_A_STATE = 22
REG_STORAGE_B_STATE = 23

RUN_IDLE = 0
RUN_HOMING = 1
RUN_SCANNING = 2
RUN_ERROR = 3

IDLE_STATE = 0
OCCUPIED_STATE = 1
READY_STATE = 2
TAKEN_STATE = 3


def log(message):
    timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
    print("%s %s" % (timestamp, message), flush=True)


def percent_to_reg(percent_value):
    clamped = max(INTENSITY_MIN_PERCENT, min(INTENSITY_MAX_PERCENT, float(percent_value)))
    return int(round(clamped * INTENSITY_SCALE))


def smooth_transition(value, center, width):
    width = max(0.001, float(width))
    return 0.5 * (1.0 + math.tanh((float(value) - float(center)) / width))


class DeviceState(object):
    def __init__(self, slave_id):
        self.slave_id = slave_id
        self.lock = threading.Lock()
        self.hr = [0] * HR_SIZE
        self.ir = [0] * IR_SIZE
        self.seq = 0
        self.scan_cycle = 0
        self.current_scan_index = 0
        self.next_area_is_a = True
        self.start_pulse_until = 0.0
        self.fill_complete_at = 0.0
        self.filling_area_is_a = None
        self.filling_pair_count = 0
        self.scan_active = False

        self.hr[REG_START_FLAG] = 0
        self.hr[REG_RUN_STATUS] = RUN_IDLE
        self._write_u32(REG_START_POS, 0)
        self._write_u32(REG_END_POS, 5000)
        self._write_u32(REG_CURRENT_POS, 0)
        self.hr[REG_STEP] = 20
        self.hr[REG_INTERVAL_TIME] = 1
        self.hr[REG_COVER_STATUS] = 1
        self.hr[REG_SAMPLE_STATUS] = 1
        self.hr[REG_TRANSMISSION] = percent_to_reg(68.0)
        self.hr[REG_BACKSCATTER] = percent_to_reg(36.0)
        self.hr[REG_TEMP_CONTROL] = 0
        self.hr[REG_TARGET_TEMP] = 250
        self.hr[REG_CURRENT_TEMP] = 250
        self.hr[REG_STORAGE_LIMIT] = PAIR_COUNT
        self.hr[REG_TOTAL_REQUIRED] = 0
        self.hr[REG_SCANNED_COUNT] = 0
        self.hr[REG_STORAGE_A_READABLE] = 0
        self.hr[REG_STORAGE_B_READABLE] = 0
        self.hr[REG_STORAGE_A_STATE] = IDLE_STATE
        self.hr[REG_STORAGE_B_STATE] = IDLE_STATE
        self._refresh_total_required()
        self._update_current_position()

    def _read_u32(self, start):
        low = self.hr[start] & 0xFFFF
        high = self.hr[start + 1] & 0xFFFF
        return (high << 16) | low

    def _write_u32(self, start, value):
        value = int(max(0, value))
        self.hr[start] = value & 0xFFFF
        self.hr[start + 1] = (value >> 16) & 0xFFFF

    def _storage_state_index(self, area_a):
        return REG_STORAGE_A_STATE if area_a else REG_STORAGE_B_STATE

    def _storage_count_index(self, area_a):
        return REG_STORAGE_A_READABLE if area_a else REG_STORAGE_B_READABLE

    def _get_storage_limit(self):
        return max(1, min(PAIR_COUNT, int(self.hr[REG_STORAGE_LIMIT] or PAIR_COUNT)))

    def _refresh_total_required(self):
        start_pos = self._read_u32(REG_START_POS)
        end_pos = self._read_u32(REG_END_POS)
        step = max(1, int(self.hr[REG_STEP] or 1))
        if end_pos < start_pos:
            start_pos, end_pos = end_pos, start_pos
        total = ((end_pos - start_pos) // step) + 1
        self.hr[REG_TOTAL_REQUIRED] = max(1, min(65535, int(total)))

    def _update_current_position(self):
        total = max(1, int(self.hr[REG_TOTAL_REQUIRED]))
        scanned = max(0, min(total, int(self.hr[REG_SCANNED_COUNT])))
        start_pos = self._read_u32(REG_START_POS)
        end_pos = self._read_u32(REG_END_POS)
        if total <= 1:
            current = end_pos
        else:
            current = start_pos + ((end_pos - start_pos) * scanned) // (total - 1)
        self._write_u32(REG_CURRENT_POS, current)

    def _point_height_ratio(self, point_index):
        total = max(1, int(self.hr[REG_TOTAL_REQUIRED]))
        if total <= 1:
            return 0.5
        bounded = max(0, min(total - 1, int(point_index)))
        return float(bounded) / float(total - 1)

    def _build_profile_point(self, height_ratio, scan_index):
        progress = max(0.0, min(1.0, float(scan_index) / 18.0))
        channel_bias = (float(self.slave_id) - 2.5) * 0.9

        # Stable analyzer data should evolve scan by scan:
        # top becomes clearer, bottom forms a denser sediment layer,
        # and the interface moves smoothly rather than oscillating.
        clear_front = 0.93 - 0.60 * progress
        sediment_front = 0.05 + 0.18 * progress
        clarification = smooth_transition(height_ratio, clear_front, 0.045)
        sedimentation = 1.0 - smooth_transition(height_ratio, sediment_front, 0.035)
        interface_peak = math.exp(-((height_ratio - clear_front) / 0.040) ** 2)
        texture = math.sin((height_ratio * 6.5) + (scan_index * 0.33) + self.slave_id) * 0.9

        transmission = (
            43.0
            + channel_bias
            + (2.5 * progress)
            + (42.0 * clarification)
            - (14.0 * sedimentation)
            - (4.5 * interface_peak)
            + texture
        )
        backscatter = (
            57.0
            - (0.8 * channel_bias)
            - (1.5 * progress)
            - (34.0 * clarification)
            + (18.0 * sedimentation)
            + (6.0 * interface_peak)
            - (0.6 * texture)
        )
        return transmission, backscatter

    def _update_realtime_values(self):
        current_point = int(self.hr[REG_SCANNED_COUNT])
        if not self.scan_active:
            current_point = max(0, int(self.hr[REG_TOTAL_REQUIRED]) // 2)
        height_ratio = self._point_height_ratio(current_point)
        transmission_pct, backscatter_pct = self._build_profile_point(height_ratio, self.current_scan_index)
        wobble = math.sin(time.time() * 0.6 + self.slave_id) * 0.5
        transmission = percent_to_reg(transmission_pct + wobble)
        backscatter = percent_to_reg(backscatter_pct - wobble * 0.5)
        self.hr[REG_TRANSMISSION] = max(0, min(65535, transmission))
        self.hr[REG_BACKSCATTER] = max(0, min(65535, backscatter))

    def _fill_area_pairs(self, base, start_pair_index, pair_count):
        last_transmission = percent_to_reg(50.0)
        last_backscatter = percent_to_reg(50.0)
        for i in range(PAIR_COUNT):
            p = base + i * 2
            if i < pair_count:
                height_ratio = self._point_height_ratio(start_pair_index + i)
                transmission_pct, backscatter_pct = self._build_profile_point(
                    height_ratio, self.current_scan_index
                )
                last_transmission = max(0, min(65535, percent_to_reg(transmission_pct)))
                last_backscatter = max(0, min(65535, percent_to_reg(backscatter_pct)))
                self.ir[p] = last_transmission
                self.ir[p + 1] = last_backscatter
            else:
                # Keep the tail continuous instead of hard-zeroing unused registers.
                # This avoids accidental "sudden zero segments" if an upper layer
                # reads beyond the readable pair count.
                self.ir[p] = last_transmission
                self.ir[p + 1] = last_backscatter

    def _clear_area_pairs(self, area_a):
        base = A_BASE if area_a else B_BASE
        start_pair_index = 0 if area_a else PAIR_COUNT
        self._fill_area_pairs(base, start_pair_index, PAIR_COUNT)

    def _set_area_state(self, area_a, state):
        self.hr[self._storage_state_index(area_a)] = state

    def _get_area_state(self, area_a):
        return self.hr[self._storage_state_index(area_a)]

    def _set_area_readable_count(self, area_a, count):
        self.hr[self._storage_count_index(area_a)] = max(0, min(PAIR_COUNT * 2, int(count)))

    def _remaining_pairs(self):
        total = int(self.hr[REG_TOTAL_REQUIRED])
        scanned = int(self.hr[REG_SCANNED_COUNT])
        return max(0, total - scanned)

    def _complete_fill_area(self, area_a):
        self.seq += 1
        pair_count = max(0, min(PAIR_COUNT, int(self.filling_pair_count)))
        start_pair_index = int(self.hr[REG_SCANNED_COUNT])
        if area_a:
            self._fill_area_pairs(A_BASE, start_pair_index, pair_count)
        else:
            self._fill_area_pairs(B_BASE, start_pair_index, pair_count)
        self._set_area_readable_count(area_a, pair_count * 2)
        self._set_area_state(area_a, READY_STATE)
        self.hr[REG_SCANNED_COUNT] = min(65535, int(self.hr[REG_SCANNED_COUNT]) + pair_count)
        self._update_current_position()
        self.next_area_is_a = not area_a
        self.filling_pair_count = 0

    def _start_fill_area(self, area_a, now):
        if self.filling_area_is_a is not None:
            return False
        if self._get_area_state(area_a) != IDLE_STATE:
            return False
        remaining = self._remaining_pairs()
        if remaining <= 0:
            return False
        self._set_area_state(area_a, OCCUPIED_STATE)
        self._set_area_readable_count(area_a, 0)
        self.filling_area_is_a = area_a
        self.filling_pair_count = min(self._get_storage_limit(), remaining)
        self.fill_complete_at = now + 1.0
        return True

    def _try_start_next_fill(self, now):
        if self._start_fill_area(self.next_area_is_a, now):
            return True
        return self._start_fill_area(not self.next_area_is_a, now)

    def _reset_scan(self):
        self.scan_active = False
        self.filling_area_is_a = None
        self.filling_pair_count = 0
        self.fill_complete_at = 0.0
        self.hr[REG_RUN_STATUS] = RUN_IDLE
        self.hr[REG_SCANNED_COUNT] = 0
        self._write_u32(REG_CURRENT_POS, self._read_u32(REG_START_POS))
        for area_a in (True, False):
            self._clear_area_pairs(area_a)
            self._set_area_readable_count(area_a, 0)
            self._set_area_state(area_a, IDLE_STATE)

    def _start_scan(self, now):
        self._refresh_total_required()
        self._reset_scan()
        self.current_scan_index = self.scan_cycle
        self.scan_cycle += 1
        self.start_pulse_until = now + 0.3
        self.scan_active = True
        self.hr[REG_START_FLAG] = 1
        self.hr[REG_RUN_STATUS] = RUN_SCANNING
        self._try_start_next_fill(now)

    def on_write_hr(self, address, values):
        with self.lock:
            for i, value in enumerate(values):
                idx = address + i
                if 0 <= idx < HR_SIZE:
                    self.hr[idx] = int(value) & 0xFFFF

            if (address <= REG_START_POS + 1 < address + len(values) or
                    address <= REG_END_POS + 1 < address + len(values) or
                    address <= REG_STEP < address + len(values) or
                    address <= REG_STORAGE_LIMIT < address + len(values)):
                self._refresh_total_required()
                self._update_current_position()

            if address <= REG_START_FLAG < address + len(values):
                if self.hr[REG_START_FLAG] != 0:
                    self._start_scan(time.time())
                else:
                    self.hr[REG_START_FLAG] = 0
                    self._reset_scan()

            for area_a in (True, False):
                state_index = self._storage_state_index(area_a)
                if address <= state_index < address + len(values) and self.hr[state_index] == TAKEN_STATE:
                    self._clear_area_pairs(area_a)
                    self._set_area_readable_count(area_a, 0)
                    self._set_area_state(area_a, IDLE_STATE)

    def tick(self):
        with self.lock:
            now = time.time()
            if self.start_pulse_until > 0 and now >= self.start_pulse_until:
                self.hr[REG_START_FLAG] = 0
                self.start_pulse_until = 0.0

            self._update_realtime_values()

            target = self.hr[REG_TARGET_TEMP]
            current = self.hr[REG_CURRENT_TEMP]
            if self.hr[REG_TEMP_CONTROL] != 0:
                if current < target:
                    current += 1
                elif current > target:
                    current -= 1
            else:
                ambient = 250
                if current < ambient:
                    current += 1
                elif current > ambient:
                    current -= 1
            self.hr[REG_CURRENT_TEMP] = current

            if self.filling_area_is_a is not None and now >= self.fill_complete_at:
                self._complete_fill_area(self.filling_area_is_a)
                self.filling_area_is_a = None
                self.fill_complete_at = 0.0

            if self.scan_active and self.filling_area_is_a is None:
                if self._remaining_pairs() > 0:
                    self._try_start_next_fill(now)
                else:
                    self.scan_active = False

            self.hr[REG_RUN_STATUS] = RUN_SCANNING if self.scan_active else RUN_IDLE


class DeviceHoldingBlock(ModbusSequentialDataBlock):
    def __init__(self, state):
        ModbusSequentialDataBlock.__init__(self, 0, [0] * HR_SIZE)
        self.state = state

    def getValues(self, address, count=1):
        log("[sim][slave=%s][HR][read] addr=%s count=%s" % (
            self.state.slave_id, address, count
        ))
        with self.state.lock:
            return self.state.hr[address: address + count]

    def setValues(self, address, values):
        if not isinstance(values, list):
            values = [values]
        log("[sim][slave=%s][HR][write] addr=%s values=%s" % (
            self.state.slave_id, address, values
        ))
        self.state.on_write_hr(address, values)


class DeviceInputBlock(ModbusSequentialDataBlock):
    def __init__(self, state):
        ModbusSequentialDataBlock.__init__(self, 0, [0] * IR_SIZE)
        self.state = state

    def getValues(self, address, count=1):
        log("[sim][slave=%s][IR][read] addr=%s count=%s" % (
            self.state.slave_id, address, count
        ))
        with self.state.lock:
            return self.state.ir[address: address + count]


class Simulator(object):
    def __init__(self, slave_ids):
        self.devices = {}
        for sid in slave_ids:
            self.devices[sid] = DeviceState(sid)
        self._stop = threading.Event()

    def build_context(self):
        slaves = {}
        for sid, dev in self.devices.items():
            store = ModbusSlaveContext(
                di=ModbusSequentialDataBlock(0, [0] * 16),
                co=ModbusSequentialDataBlock(0, [0] * 16),
                hr=DeviceHoldingBlock(dev),
                ir=DeviceInputBlock(dev),
                zero_mode=True,
            )
            slaves[sid] = store
        return ModbusServerContext(slaves=slaves, single=False)

    def run_tick_loop(self, period_sec=1.0):
        while not self._stop.is_set():
            for dev in self.devices.values():
                dev.tick()
            self._stop.wait(period_sec)


def parse_slave_ids(text):
    values = []
    for item in text.split(","):
        item = item.strip()
        if item:
            values.append(int(item))
    return sorted(list(set(values)))


def main():
    parser = argparse.ArgumentParser(description="Virtual lower-device (Modbus RTU) simulator")
    parser.add_argument("--serial-port", default="/dev/tnt0", help="device-side serial port path")
    parser.add_argument("--slave-ids", default="1", help="comma-separated slave IDs")
    parser.add_argument("--baudrate", type=int, default=9600)
    parser.add_argument("--bytesize", type=int, default=8)
    parser.add_argument("--parity", default="N", choices=["N", "E", "O"])
    parser.add_argument("--stopbits", type=int, default=1)
    args = parser.parse_args()

    slave_ids = parse_slave_ids(args.slave_ids)
    sim = Simulator(slave_ids)
    context = sim.build_context()

    identity = ModbusDeviceIdentification()
    identity.VendorName = "StabilityAnalyzer"
    identity.ProductCode = "VDEV"
    identity.ProductName = "Virtual Lower Device"
    identity.ModelName = "SA-Simulator"
    identity.MajorMinorRevision = "1.0"

    tick_thread = threading.Thread(target=sim.run_tick_loop)
    tick_thread.daemon = True
    tick_thread.start()

    log("[sim] started")
    log("[sim] mode=rtu")
    log("[sim] serial_port=%s" % args.serial_port)
    log("[sim] slave_ids=%s" % slave_ids)

    StartSerialServer(
        context=context,
        identity=identity,
        port=args.serial_port,
        framer=ModbusRtuFramer,
        baudrate=args.baudrate,
        bytesize=args.bytesize,
        parity=args.parity,
        stopbits=args.stopbits,
        timeout=0.2,
    )


if __name__ == "__main__":
    main()
