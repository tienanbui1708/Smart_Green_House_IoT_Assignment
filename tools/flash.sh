#!/usr/bin/env bash
# Build -> nap -> bat log Serial. Dung: tools/flash.sh <sketch> [port] [so giay log] [file log]
# Vi du: tools/flash.sh tools/env_info /dev/ttyACM0 15 docs/evidence/M0/env_info_node1.txt
# Khong co port: chi build.
set -euo pipefail
FQBN="OhStem:esp32:yolo_uno"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SKETCH="${1:?Thieu duong dan sketch}"
PORT="${2:-}"
SECONDS_LOG="${3:-15}"
LOG_FILE="${4:-}"
NAME="$(basename "$SKETCH")"

# common/ (protocol.h, app_log, coreiot_link) duoc nap nhu mot thu vien dung chung hai node.
arduino-cli compile -b "$FQBN" --library "$ROOT/common" --build-path "$ROOT/build/$NAME" "$SKETCH"
[ -n "$PORT" ] || exit 0

if [ ! -e "$PORT" ]; then echo "Khong thay $PORT. Kiem tra cap du lieu va cong." >&2; exit 1; fi
if [ ! -w "$PORT" ]; then
  echo "Khong co quyen ghi $PORT. Chay: sudo usermod -aG $(stat -c %G "$PORT") $USER  roi dang nhap lai." >&2
  exit 1
fi

arduino-cli upload -b "$FQBN" -p "$PORT" --input-dir "$ROOT/build/$NAME"

# Cong USB CDC bien mat roi hien lai sau khi board reset.
for _ in $(seq 1 20); do [ -w "$PORT" ] && break; sleep 0.5; done

echo "--- Log Serial ${SECONDS_LOG}s ---"
if [ -n "$LOG_FILE" ]; then
  mkdir -p "$(dirname "$LOG_FILE")"
  timeout "$SECONDS_LOG" arduino-cli monitor -p "$PORT" --config 115200 --quiet --raw </dev/null | tee "$LOG_FILE" || true
else
  timeout "$SECONDS_LOG" arduino-cli monitor -p "$PORT" --config 115200 --quiet --raw </dev/null || true
fi
