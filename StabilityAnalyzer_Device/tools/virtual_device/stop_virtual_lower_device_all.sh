#!/usr/bin/env bash
set -euo pipefail

PID_DIR="${PID_DIR:-/tmp/stability_virtual_lower_device}"

for name in A B C D; do
  pid_file="${PID_DIR}/channel_${name}.pid"
  if [[ ! -f "${pid_file}" ]]; then
    continue
  fi

  pid="$(cat "${pid_file}")"
  if [[ -n "${pid}" ]] && kill -0 "${pid}" >/dev/null 2>&1; then
    kill "${pid}"
    echo "[sim] stopped channel ${name} pid=${pid}"
  fi
  rm -f "${pid_file}"
done
