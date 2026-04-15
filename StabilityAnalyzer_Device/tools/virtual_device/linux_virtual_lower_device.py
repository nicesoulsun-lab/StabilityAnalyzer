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


class DeviceState(object):
    def __init__(self, slave_id):
        self.slave_id = slave_id
        self.lock = threading.Lock()
        self.hr = [0] * HR_SIZE
        self.ir = [0] * IR_SIZE
        self.seq = 0
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
        self.hr[REG_TRANSMISSION] = 1200
        self.hr[REG_BACKSCATTER] = 800
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

    def _update_realtime_values(self):
        x = self.seq / 12.0 + time.time() / 5.0
        transmission = int(1200 + 120 * math.sin(x))
        backscatter = int(800 + 90 * math.cos(x * 0.9))
        self.hr[REG_TRANSMISSION] = max(0, min(65535, transmission))
        self.hr[REG_BACKSCATTER] = max(0, min(65535, backscatter))

    def _fill_area_pairs(self, base, seq_seed, pair_count):
        for i in range(PAIR_COUNT):
            x = (self.seq + seq_seed + i) / 20.0
            transmission = int(1200 + 200 * math.sin(x))
            backscatter = int(800 + 150 * math.cos(x * 0.8))
            p = base + i * 2
            if i < pair_count:
                self.ir[p] = max(0, min(65535, transmission))
                self.ir[p + 1] = max(0, min(65535, backscatter))
            else:
                self.ir[p] = 0
                self.ir[p + 1] = 0

    def _clear_area_pairs(self, area_a):
        base = A_BASE if area_a else B_BASE
        for i in range(PAIR_COUNT * 2):
            self.ir[base + i] = 0

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
        if area_a:
            self._fill_area_pairs(A_BASE, 10, pair_count)
        else:
            self._fill_area_pairs(B_BASE, 100, pair_count)
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
