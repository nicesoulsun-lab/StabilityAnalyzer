#!/usr/bin/env bash
set -euo pipefail

PID_DIR="${PID_DIR:-/tmp/stability_virtual_socat}"

for name in A B C D; do
  pid_file="${PID_DIR}/pair_${name}.pid"
  if [[ ! -f "${pid_file}" ]]; then
    continue
  fi

  pid="$(cat "${pid_file}")"
  if [[ -n "${pid}" ]] && kill -0 "${pid}" >/dev/null 2>&1; then
    kill "${pid}"
    echo "[socat] stopped pair ${name} pid=${pid}"
  fi
  rm -f "${pid_file}"
done

rm -f \
  /tmp/ttySA_A_HOST /tmp/ttySA_A_DEV \
  /tmp/ttySA_B_HOST /tmp/ttySA_B_DEV \
  /tmp/ttySA_C_HOST /tmp/ttySA_C_DEV \
  /tmp/ttySA_D_HOST /tmp/ttySA_D_DEV
