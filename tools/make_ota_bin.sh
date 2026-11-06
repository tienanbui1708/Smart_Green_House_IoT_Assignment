#!/usr/bin/env bash
# Build file .bin de upload len CoreIOT OTA (T14, TASKS_SPEC 6.3, COREIOT_SETUP.md 10.2).
# Dung: tools/make_ota_bin.sh node1_sensor     (doi FW_VERSION trong config.h TRUOC khi chay)
# In: duong dan <sketch>.ino.bin (KHONG phai bootloader/partitions/merged), kich thuoc, SHA256, MD5.
set -euo pipefail
FQBN="OhStem:esp32:yolo_uno"
PARTITION="${PARTITION:-min_spiffs}"   # PHAI giong ban dang chay tren board
CLI="${ARDUINO_CLI:-arduino-cli}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SKETCH="${1:?Thieu sketch: node1_sensor | node2_actuator}"
NAME="$(basename "$SKETCH")"
OUT="$ROOT/build/ota_$NAME"

VERSION="$(sed -n 's/^#define FW_VERSION "\(.*\)"/\1/p' "$ROOT/$NAME/config.h")"
TITLE="$(sed -n 's/^#define FW_TITLE "\(.*\)"/\1/p' "$ROOT/$NAME/config.h")"
"$CLI" compile -b "$FQBN" --board-options "PartitionScheme=$PARTITION" --library "$ROOT/common" \
  --build-path "$OUT" --export-binaries "$ROOT/$NAME" >/dev/null
BIN="$OUT/$NAME.ino.bin"
[ -f "$BIN" ] || { echo "Khong thay $BIN" >&2; exit 1; }
SIZE="$(wc -c < "$BIN" | tr -d ' ')"
echo "Title    : $TITLE"
echo "Version  : $VERSION"
echo "File     : $BIN"
echo "Size     : $SIZE bytes"
echo "SHA256   : $(sha256sum "$BIN" | cut -d' ' -f1)"
echo "MD5      : $(md5sum "$BIN" | cut -d' ' -f1)"
echo "Upload len CoreIOT: OTA updates -> + -> Title=$TITLE Version=$VERSION (COREIOT_SETUP.md 10.3)"
