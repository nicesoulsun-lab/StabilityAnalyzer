#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEVICE_PORT_PATH="${DEVICE_PORT_PATH:-/dev/tnt0}"
SLAVE_IDS="${SLAVE_IDS:-1}"
BAUDRATE="${BAUDRATE:-9600}"
BYTESIZE="${BYTESIZE:-8}"
PARITY="${PARITY:-N}"
STOPBITS="${STOPBITS:-1}"

if ! command -v python3 >/dev/null 2>&1; then
  echo "[sim] python3 not found"
  exit 1
fi

if [[ ! -e "${DEVICE_PORT_PATH}" ]]; then
  echo "[sim] device port not found: ${DEVICE_PORT_PATH}"
  exit 1
fi

echo "[sim] device_port=${DEVICE_PORT_PATH}"
echo "[sim] slave_ids=${SLAVE_IDS}"

exec python3 "${SCRIPT_DIR}/linux_virtual_lower_device.py" \
  --serial-port "${DEVICE_PORT_PATH}" \
  --slave-ids "${SLAVE_IDS}" \
  --baudrate "${BAUDRATE}" \
  --bytesize "${BYTESIZE}" \
  --parity "${PARITY}" \
  --stopbits "${STOPBITS}"
