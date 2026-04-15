#!/usr/bin/env bash
set -euo pipefail

PID_DIR="${PID_DIR:-/tmp/stability_virtual_socat}"
mkdir -p "${PID_DIR}"

if ! command -v socat >/dev/null 2>&1; then
  echo "[socat] socat not found"
  echo "[socat] install it first, for example: apt-get install -y socat"
  exit 1
fi

HOST_LINKS=(
  "/tmp/ttySA_A_HOST"
  "/tmp/ttySA_B_HOST"
  "/tmp/ttySA_C_HOST"
  "/tmp/ttySA_D_HOST"
)

DEV_LINKS=(
  "/tmp/ttySA_A_DEV"
  "/tmp/ttySA_B_DEV"
  "/tmp/ttySA_C_DEV"
  "/tmp/ttySA_D_DEV"
)

NAMES=("A" "B" "C" "D")

for i in "${!NAMES[@]}"; do
  name="${NAMES[$i]}"
  host_link="${HOST_LINKS[$i]}"
  dev_link="${DEV_LINKS[$i]}"
  pid_file="${PID_DIR}/pair_${name}.pid"
  log_file="${PID_DIR}/pair_${name}.log"

  rm -f "${host_link}" "${dev_link}"

  if [[ -f "${pid_file}" ]]; then
    old_pid="$(cat "${pid_file}")"
    if [[ -n "${old_pid}" ]] && kill -0 "${old_pid}" >/dev/null 2>&1; then
      echo "[socat] pair ${name} already running pid=${old_pid}"
      continue
    fi
    rm -f "${pid_file}"
  fi

  nohup socat -d -d \
    PTY,raw,echo=0,link="${host_link}",mode=666 \
    PTY,raw,echo=0,link="${dev_link}",mode=666 \
    >"${log_file}" 2>&1 &

  pid="$!"
  echo "${pid}" > "${pid_file}"
  sleep 1

  if [[ ! -e "${host_link}" || ! -e "${dev_link}" ]]; then
    echo "[socat] failed to create pair ${name}"
    echo "[socat] check log: ${log_file}"
    exit 1
  fi

  echo "[socat] started pair ${name} pid=${pid}"
  echo "[socat]   host=${host_link}"
  echo "[socat]   dev=${dev_link}"
done
