#!/usr/bin/env bash
# Build -> nap -> bat log Serial. Dung: tools/flash.sh <sketch> [port] [so giay log] [file log]
# Vi du: tools/flash.sh tools/env_info /dev/ttyACM0 15 docs/evidence/M0/env_info_node1.txt
# Khong co port: chi build.
set -euo pipefail
FQBN="OhStem:esp32:yolo_uno"
# Partition co 2 slot app (OTA, T14): min_spiffs = 2 x 1.9 MB app + 128 KB LittleFS (/config.json).
# Doi scheme thi PHAI nap lai bang cap truoc khi OTA. Ghi de bang: PARTITION=default tools/flash.sh ...
PARTITION="${PARTITION:-min_spiffs}"
CLI="${ARDUINO_CLI:-arduino-cli}"  # Windows: ARDUINO_CLI="/c/Program Files/Arduino IDE/resources/app/lib/backend/resources/arduino-cli.exe"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SKETCH="${1:?Thieu duong dan sketch}"
PORT="${2:-}"
SECONDS_LOG="${3:-15}"
LOG_FILE="${4:-}"
NAME="$(basename "$SKETCH")"

# common/ (protocol.h, app_log, coreiot_link) duoc nap nhu mot thu vien dung chung hai node.
"$CLI" compile -b "$FQBN" --board-options "PartitionScheme=$PARTITION" --library "$ROOT/common"   --build-path "$ROOT/build/$NAME" ${EXTRA_BUILD_ARGS:-} "$SKETCH"
[ -n "$PORT" ] || exit 0

if [ ! -e "$PORT" ]; then echo "Khong thay $PORT. Kiem tra cap du lieu va cong." >&2; exit 1; fi
if [ ! -w "$PORT" ]; then
  echo "Khong co quyen ghi $PORT. Chay: sudo usermod -aG $(stat -c %G "$PORT") $USER  roi dang nhap lai." >&2
  exit 1
fi

"$CLI" upload -b "$FQBN" -p "$PORT" --input-dir "$ROOT/build/$NAME"

# Cong USB CDC bien mat roi hien lai sau khi board reset.
for _ in $(seq 1 20); do [ -w "$PORT" ] && break; sleep 0.5; done

echo "--- Log Serial ${SECONDS_LOG}s ---"
if [ -n "$LOG_FILE" ]; then
  mkdir -p "$(dirname "$LOG_FILE")"
  timeout "$SECONDS_LOG" "$CLI" monitor -p "$PORT" --config 115200 --quiet --raw </dev/null | tee "$LOG_FILE" || true
else
  timeout "$SECONDS_LOG" "$CLI" monitor -p "$PORT" --config 115200 --quiet --raw </dev/null || true
fi
