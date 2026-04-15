#!/usr/bin/env bash
set -euo pipefail

echo "[check] kernel=$(uname -r)"

if ls /dev/tnt* >/dev/null 2>&1; then
  echo "[check] tty0tty device nodes already exist:"
  ls -1 /dev/tnt*
  exit 0
fi

echo "[check] /dev/tnt* not found"

if ! command -v modprobe >/dev/null 2>&1; then
  echo "[check] modprobe not found, cannot try loading tty0tty automatically"
  exit 1
fi

echo "[check] trying: modprobe tty0tty"
if modprobe tty0tty 2>/tmp/check_tty0tty.err; then
  sleep 1
  if ls /dev/tnt* >/dev/null 2>&1; then
    echo "[check] tty0tty loaded successfully:"
    ls -1 /dev/tnt*
    exit 0
  fi
  echo "[check] module loaded but /dev/tnt* still missing"
  exit 1
fi

echo "[check] modprobe tty0tty failed:"
cat /tmp/check_tty0tty.err
rm -f /tmp/check_tty0tty.err

echo "[check] tty0tty is probably not installed for this kernel"
exit 1
