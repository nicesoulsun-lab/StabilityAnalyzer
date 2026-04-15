#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys

try:
    from serial.tools import list_ports
except Exception as exc:
    print("[ports] failed to import pyserial:", exc)
    sys.exit(1)


def main():
    ports = list(list_ports.comports())
    if not ports:
        print("[ports] no serial ports found")
        return

    for port in ports:
        print("[ports] device=%s name=%s description=%s" % (port.device, port.name, port.description))


if __name__ == "__main__":
    main()
