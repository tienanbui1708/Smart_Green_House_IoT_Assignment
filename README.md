# Smart Greenhouse – Hai nút TinyML, ESP-NOW và CoreIOT

Bài tập lớn IoT, HCMUT HK261. Mô tả và giao thức ở `PROJECT.md`, luật làm việc ở `CLAUDE.md`.

## Phiên bản môi trường

| Thành phần | Phiên bản | Nguồn |
|---|---|---|
| arduino-cli | 1.5.1 | GitHub Releases |
| OhStem Boards (`OhStem:esp32`) | 3.1.3 | Board Manager URL bên dưới |
| arduino-esp32 | 3.1.3 (IDF 5.3) | đi kèm OhStem Boards |
| DHT20 (Rob Tillaart) | 0.3.1 | đi kèm OhStem Boards |
| Adafruit NeoPixel | 1.12.0 | đi kèm OhStem Boards |
| LiquidCrystal_I2C | 2.0.0 | Library Manager |
| PubSubClient | 2.8 | Library Manager |
| ArduinoJson | 7.4.3 | Library Manager |

Board: `Yolo UNO (ESP32-S3)`, FQBN `OhStem:esp32:yolo_uno`.

Macro chân đọc từ `variants/yolo_uno/pins_arduino.h` của core (chưa kiểm chứng trên kit):

| Macro | GPIO | Dùng cho |
|---|---|---|
| `D13` / `LED_BUILTIN` | 48 | LED onboard |
| `A1` | 2 | Độ ẩm đất |
| `RGB_LED` | 45 | NeoPixel onboard |
| `SDA` / `SCL` | 11 / 12 | I2C, `Wire.begin()` dùng mặc định |

## Cài đặt từ máy trống

### Windows hoặc macOS

Làm theo `SETUP_FIRMWARE.md` phần A, cài thêm `PubSubClient` 2.8 và `ArduinoJson` 7.4.3 bằng Library Manager.

Thư mục `common/` là thư viện dùng chung hai node (`protocol.h`, `app_log`, `coreiot_link`). Arduino IDE không tự thấy nó: chép (hoặc tạo symlink) `common/` vào `Documents/Arduino/libraries/common/`, rồi chép lại mỗi khi `common/` thay đổi.

### Linux

```bash
# 1. Cài arduino-cli (Arch: sudo pacman -S arduino-cli)
arduino-cli config init --additional-urls https://raw.githubusercontent.com/AITT-VN/ohstem_arduino_board/main/package_xcon_index.json

# 2. Cài core. Zip của OhStem có thư mục chỉ đọc nên `core install` lỗi "permission denied";
#    script xử lý lỗi này và kiểm SHA-256.
tools/install_core_linux.sh

# 3. Cài thư viện
arduino-cli lib install LiquidCrystal_I2C@2.0.0 PubSubClient@2.8 ArduinoJson@7.4.3

# 4. Cấp quyền cổng serial (Arch: uucp, Debian/Ubuntu: dialout), rồi đăng nhập lại
sudo usermod -aG uucp $USER
```

## Build và nạp

```bash
# Chỉ build
tools/flash.sh tools/blink

# Build, nạp, bắt log. Tham số: sketch, cổng, số giây log, file log
tools/flash.sh tools/env_info /dev/ttyACM0 15 docs/evidence/M0/env_info_node1.txt
```

Firmware hai node (cần `secrets.h`, xem mục dưới). `flash.sh` tự thêm `--library common`:

```bash
tools/flash.sh node1_sensor /dev/ttyACM0 60 docs/evidence/M2/node1_boot.txt
tools/flash.sh node2_actuator /dev/ttyACM0 60 docs/evidence/M2/node2_boot.txt
```

Nếu không nạp được: giữ BOOT, nhấn RESET, thả BOOT rồi chạy lại.

## Trạng thái firmware

M1–M3 đã viết và biên dịch được, **chưa kiểm chứng trên kit**. Các giá trị còn tạm, đánh dấu `TODO(verify)` trong `config.h`:

| Hằng số | Giá trị tạm | Lấy giá trị thật bằng |
|---|---|---|
| `LCD_I2C_ADDR` | `0x21` | `tools/i2c_scan` |
| `SOIL_RAW_DRY` / `SOIL_RAW_WET` | `0` / `4095` | `tools/adc_calibrate` |
| `NEOPIXEL_COUNT`, `NEOPIXEL_TYPE` | `1`, `NEO_GRB` | `tools/neopixel_test` |
| `MQTT_HOST` / `MQTT_PORT` (`secrets.h`) | `app.coreiot.io` / `1883` | *Check connectivity* trên CoreIOT |

Chưa làm: `ai_class` (M7), `espnow_age_s` và dữ liệu thật cho Node 2 (M5; hiện dùng dữ liệu giả `USE_FAKE_SENSOR_DATA`), RPC `cloudAlert` (M4).

Log Serial có tiền tố task: `[SENSOR]`, `[LCD]`, `[LED]`, `[CTRL]`, `[ACT]`, `[WIFI]`, `[MQTT]`, `[RPC]`, `[STATS]`. Dòng `[STATS]` mỗi 10 s in heap và stack còn trống của từng task.

## Cấu hình bí mật

```bash
cp node1_sensor/secrets.h.example node1_sensor/secrets.h
cp node2_actuator/secrets.h.example node2_actuator/secrets.h
```

`secrets.h` nằm trong `.gitignore`, không commit file này.
