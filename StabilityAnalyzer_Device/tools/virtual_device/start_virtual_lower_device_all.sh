#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PID_DIR="${PID_DIR:-/tmp/stability_virtual_lower_device}"
mkdir -p "${PID_DIR}"

IFS=',' read -r -a PORTS <<< "${DEVICE_PORTS:-/dev/tnt0,/dev/tnt2,/dev/tnt4,/dev/tnt6}"
IFS=',' read -r -a SLAVES <<< "${SLAVE_IDS:-1,2,3,4}"
NAMES=("A" "B" "C" "D")

if [[ "${#PORTS[@]}" -ne 4 ]]; then
  echo "[sim] DEVICE_PORTS must contain 4 comma-separated values"
  exit 1
fi

if [[ "${#SLAVES[@]}" -ne 4 ]]; then
  echo "[sim] SLAVE_IDS must contain 4 comma-separated values"
  exit 1
fi

if ! command -v python3 >/dev/null 2>&1; then
  echo "[sim] python3 not found"
  exit 1
fi

for i in "${!PORTS[@]}"; do
  port="${PORTS[$i]}"
  slave="${SLAVES[$i]}"
  name="${NAMES[$i]}"
  pid_file="${PID_DIR}/channel_${name}.pid"
  log_file="${PID_DIR}/channel_${name}.log"

  if [[ ! -e "${port}" ]]; then
    echo "[sim] missing device port: ${port}"
    exit 1
  fi

  if [[ -f "${pid_file}" ]]; then
    old_pid="$(cat "${pid_file}")"
    if [[ -n "${old_pid}" ]] && kill -0 "${old_pid}" >/dev/null 2>&1; then
      echo "[sim] channel ${name} already running pid=${old_pid}"
      continue
    fi
    rm -f "${pid_file}"
  fi

  nohup python3 "${SCRIPT_DIR}/linux_virtual_lower_device.py" \
    --serial-port "${port}" \
    --slave-ids "${slave}" \
    >"${log_file}" 2>&1 &

  pid="$!"
  echo "${pid}" > "${pid_file}"
  echo "[sim] started channel ${name} pid=${pid} port=${port} slave=${slave} log=${log_file}"
done
