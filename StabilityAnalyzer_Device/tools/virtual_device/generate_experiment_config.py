#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Generate ChannelA~D experiment JSON files for the current TaskScheduler parser.

Supported host port values:
  - Qt-visible short names such as "tnt1"
  - Linux explicit PTY paths such as "/tmp/ttySA_A_HOST"
"""

import argparse
import json
import os


CHANNEL_NAMES = ["ChannelA", "ChannelB", "ChannelC", "ChannelD"]


def normalize_port_name(value):
    value = (value or "").strip()
    if not value:
        raise ValueError("host port name is empty")
    return value


def parse_slave_ids(text):
    items = [item.strip() for item in text.split(",") if item.strip()]
    values = [int(item) for item in items]
    if len(values) != 4:
        raise ValueError("slave ids count must be exactly 4, e.g. 1,2,3,4")
    return values


def parse_host_port_names(text):
    items = [normalize_port_name(item) for item in text.split(",") if item.strip()]
    if len(items) != 4:
        raise ValueError("host port names count must be exactly 4")
    return items


def build_task_list():
    return [
        {"operate": 1, "name": "read_start_flag", "type": "READ_HOLDING_REGISTERS", "address": 0, "count": 1},
        {"operate": 1, "name": "read_status_block_0_23", "type": "READ_HOLDING_REGISTERS", "address": 0, "count": 24},
        {"operate": 1, "name": "read_run_status", "type": "READ_HOLDING_REGISTERS", "address": 1, "count": 1},
        {"operate": 1, "name": "read_cover_status", "type": "READ_HOLDING_REGISTERS", "address": 10, "count": 1},
        {"operate": 1, "name": "read_sample_status", "type": "READ_HOLDING_REGISTERS", "address": 11, "count": 1},
        {"operate": 1, "name": "read_realtime_values", "type": "READ_HOLDING_REGISTERS", "address": 12, "count": 2},
        {"operate": 1, "name": "read_current_temperature", "type": "READ_HOLDING_REGISTERS", "address": 16, "count": 1},
        {"operate": 1, "name": "read_storage_status", "type": "READ_HOLDING_REGISTERS", "address": 20, "count": 4},
        {"operate": 1, "name": "read_scan_data_a_0", "type": "READ_INPUT_REGISTERS", "address": 0, "count": 100},
        {"operate": 1, "name": "read_scan_data_a_100", "type": "READ_INPUT_REGISTERS", "address": 100, "count": 100},
        {"operate": 1, "name": "read_scan_data_a_200", "type": "READ_INPUT_REGISTERS", "address": 200, "count": 100},
        {"operate": 1, "name": "read_scan_data_a_300", "type": "READ_INPUT_REGISTERS", "address": 300, "count": 100},
        {"operate": 1, "name": "read_scan_data_a_400", "type": "READ_INPUT_REGISTERS", "address": 400, "count": 100},
        {"operate": 1, "name": "read_scan_data_b_500", "type": "READ_INPUT_REGISTERS", "address": 500, "count": 100},
        {"operate": 1, "name": "read_scan_data_b_600", "type": "READ_INPUT_REGISTERS", "address": 600, "count": 100},
        {"operate": 1, "name": "read_scan_data_b_700", "type": "READ_INPUT_REGISTERS", "address": 700, "count": 100},
        {"operate": 1, "name": "read_scan_data_b_800", "type": "READ_INPUT_REGISTERS", "address": 800, "count": 100},
        {"operate": 1, "name": "read_scan_data_b_900", "type": "READ_INPUT_REGISTERS", "address": 900, "count": 100},
        {"operate": 1, "name": "write_start_flag", "type": "WRITE_MULTIPLE_REGISTERS", "address": 0, "count": 1},
        {"operate": 1, "name": "write_storage_a_state", "type": "WRITE_MULTIPLE_REGISTERS", "address": 22, "count": 1},
        {"operate": 1, "name": "write_storage_b_state", "type": "WRITE_MULTIPLE_REGISTERS", "address": 23, "count": 1},
        {"operate": 1, "name": "set_temperature_control", "type": "WRITE_MULTIPLE_REGISTERS", "address": 14, "count": 1},
        {"operate": 1, "name": "set_temperature", "type": "WRITE_MULTIPLE_REGISTERS", "address": 15, "count": 1},
        {"operate": 1, "name": "set_scan_range", "type": "WRITE_MULTIPLE_REGISTERS", "address": 2, "count": 4},
        {"operate": 1, "name": "set_step", "type": "WRITE_MULTIPLE_REGISTERS", "address": 8, "count": 1},
    ]


def main():
    parser = argparse.ArgumentParser(description="Generate 4-channel experiment JSON config")
    parser.add_argument("--output-dir", required=True, help="runtime config/experiment_devices directory")
    parser.add_argument(
        "--host-port-names",
        default="/tmp/ttySA_A_HOST,/tmp/ttySA_B_HOST,/tmp/ttySA_C_HOST,/tmp/ttySA_D_HOST",
        help="4 port names for ChannelA~D, e.g. /tmp/ttySA_A_HOST,/tmp/ttySA_B_HOST,/tmp/ttySA_C_HOST,/tmp/ttySA_D_HOST or tnt1,tnt3,tnt5,tnt7",
    )
    parser.add_argument("--baudrate", type=int, default=9600)
    parser.add_argument("--data-bits", type=int, default=8)
    parser.add_argument("--parity", default="NoParity", choices=["NoParity", "EvenParity", "OddParity"])
    parser.add_argument("--stop-bits", type=int, default=1)
    parser.add_argument("--flow-control", default="NoFlowControl")
    parser.add_argument("--slave-ids", default="1,2,3,4", help="exactly 4 slave ids, comma separated")
    args = parser.parse_args()

    host_port_names = parse_host_port_names(args.host_port_names)
    slave_ids = parse_slave_ids(args.slave_ids)
    os.makedirs(args.output_dir, exist_ok=True)

    for index, channel_name in enumerate(CHANNEL_NAMES):
        payload = {
            "device": {
                "slaveId": slave_ids[index],
                "name": channel_name,
                "description": "%s modbus device for virtual debug" % channel_name,
                "manufacturer": "VirtualDevice",
                "model": "SA-Simulator",
                "serialConfig": {
                    "portName": host_port_names[index],
                    "baudRate": args.baudrate,
                    "dataBits": args.data_bits,
                    "parity": args.parity,
                    "stopBits": args.stop_bits,
                    "flowControl": args.flow_control,
                    "interFrameDelay": 5,
                    "responseTimeout": 1000,
                    "maxRetries": 3,
                },
                "taskList": build_task_list(),
            }
        }

        file_path = os.path.join(args.output_dir, "%s.json" % channel_name)
        with open(file_path, "w", encoding="utf-8") as fp:
            json.dump(payload, fp, ensure_ascii=False, indent=4)
            fp.write("\n")
        print("[config] wrote %s slaveId=%s hostPort=%s" % (file_path, slave_ids[index], host_port_names[index]))


if __name__ == "__main__":
    main()
