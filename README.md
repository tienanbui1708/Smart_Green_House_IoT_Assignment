# Smart Greenhouse – Hai nút TinyML, ESP-NOW và CoreIOT

Bài tập lớn IoT, HCMUT HK261. Mô tả và giao thức ở `PROJECT.md`, luật làm việc ở `CLAUDE.md`, đặc tả từng task ở `TASKS_SPEC.md`.

> **Trạng thái:** toàn bộ firmware M2–M8 và công cụ M9 **đã biên dịch được** (OhStem 3.1.3, partition `min_spiffs`), nhưng **chưa kiểm chứng trên kit**. Mọi kết quả trên board chỉ coi là đạt khi có log Serial thật.

## Phiên bản môi trường

| Thành phần | Phiên bản | Nguồn |
|---|---|---|
| arduino-cli | 1.5.1 (Linux), 1.4.1 (đi kèm Arduino IDE trên Windows) | GitHub Releases / Arduino IDE |
| OhStem Boards (`OhStem:esp32`) | 3.1.3 | Board Manager URL bên dưới |
| arduino-esp32 | 3.1.3 (IDF 5.3) | đi kèm OhStem Boards |
| DHT20 (Rob Tillaart) | 0.3.1 | đi kèm OhStem Boards |
| Adafruit NeoPixel | 1.12.0 | đi kèm OhStem Boards |
| WebServer, LittleFS, Update, Preferences | theo core 3.1.3 | đi kèm core |
| LiquidCrystal_I2C | 2.0.0 | Library Manager |
| PubSubClient | 2.8 | Library Manager |
| ArduinoJson | 7.4.3 | Library Manager |
| Chirale_TensorFLowLite | 2.0.0 | Library Manager (TinyML, chỉ Node 1) |
| Python (ml/) | 3.12.10, TensorFlow 2.21.0 | `ml/requirements.txt` |

Board: `Yolo UNO (ESP32-S3)`, FQBN `OhStem:esp32:yolo_uno`. **Partition scheme: `min_spiffs`** (*Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)*), bắt buộc để có hai slot OTA. Node 1 có TFLM nên đã dùng 1.15 MB, không vừa thoải mái trong slot 1.25 MB của scheme mặc định.

Macro chân đọc từ `variants/yolo_uno/pins_arduino.h` của core (chưa kiểm chứng trên kit):

| Macro | GPIO | Dùng cho |
|---|---|---|
| `D13` / `LED_BUILTIN` | 48 | LED onboard |
| `A1` | 2 | Độ ẩm đất |
| `RGB_LED` | 45 | NeoPixel onboard |
| `SDA` / `SCL` | 11 / 12 | I2C, `Wire.begin()` dùng mặc định |

## Cấu trúc thư mục

```
node1_sensor/      Node 1: taskSensor, taskLed, taskDisplay, taskCoreiot, taskEspnow, taskTinyml, taskWeb
node2_actuator/    Node 2: taskControl, taskActuator, taskDisplay, taskCoreiot, taskEspnowRx, taskWeb
common/            thư viện dùng chung: protocol.h, app_log, coreiot_link (Wi-Fi/MQTT/AP cứu hộ),
                   config_store (LittleFS /config.json), web_common, ota_client
web/index.html     giao diện web dùng chung -> nodeX/web_assets.h (tools/embed_web.py)
ml/                collect_serial.py, train.py, export_tflite.py, features.py, make_demo_data.py
tools/             flash.sh, make_ota_bin.sh, embed_web.py, parse_now_log.py, parse_stats_log.py,
                   web_latency.py, summarize_measurements.py, tinyml_parity/, collect_firmware/, sketch M1
cloud/  docs/      cấu hình CoreIOT xuất ra; minh chứng (docs/evidence/<Txx>/), số đo (docs/measurements/)
```

## Cài đặt từ máy trống

### Windows hoặc macOS (Arduino IDE)

1. Làm theo `SETUP_FIRMWARE.md` phần A (board OhStem 3.1.3).
2. Library Manager: `LiquidCrystal_I2C` 2.0.0, `PubSubClient` 2.8, `ArduinoJson` 7.4.3, `Chirale_TensorFLowLite` 2.0.0.
3. Thư mục `common/` là thư viện dùng chung hai node. Arduino IDE không tự thấy nó: chép (hoặc tạo symlink) `common/` vào `Documents/Arduino/libraries/common/`, và **chép lại mỗi khi `common/` thay đổi**.
4. **Tools → Partition Scheme → Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)** cho cả hai node.

arduino-cli đi kèm Arduino IDE dùng được từ Git Bash, không cần cài thêm:

```bash
export ARDUINO_CLI="/c/Program Files/Arduino IDE/resources/app/lib/backend/resources/arduino-cli.exe"
"$ARDUINO_CLI" core install OhStem:esp32@3.1.3 \
  --additional-urls https://raw.githubusercontent.com/AITT-VN/ohstem_arduino_board/main/package_xcon_index.json
"$ARDUINO_CLI" lib install LiquidCrystal_I2C@2.0.0 PubSubClient@2.8 ArduinoJson@7.4.3 Chirale_TensorFLowLite@2.0.0
```

### Linux

```bash
# 1. Cài arduino-cli (Arch: sudo pacman -S arduino-cli)
arduino-cli config init --additional-urls https://raw.githubusercontent.com/AITT-VN/ohstem_arduino_board/main/package_xcon_index.json

# 2. Cài core. Zip của OhStem có thư mục chỉ đọc nên `core install` lỗi "permission denied";
#    script xử lý lỗi này và kiểm SHA-256.
tools/install_core_linux.sh

# 3. Cài thư viện
arduino-cli lib install LiquidCrystal_I2C@2.0.0 PubSubClient@2.8 ArduinoJson@7.4.3 Chirale_TensorFLowLite@2.0.0

# 4. Cấp quyền cổng serial (Arch: uucp, Debian/Ubuntu: dialout), rồi đăng nhập lại
sudo usermod -aG uucp $USER
```

## Cấu hình bí mật

```bash
cp node1_sensor/secrets.h.example node1_sensor/secrets.h
cp node2_actuator/secrets.h.example node2_actuator/secrets.h
```

Điền `WIFI_SSID`, `WIFI_PASS`, `MQTT_HOST`, `MQTT_PORT`, `DEVICE_TOKEN`, `AP_PASS` (mật khẩu AP cứu hộ, ≥ 8 ký tự; nếu thiếu thì build sẽ báo lỗi). `secrets.h` nằm trong `.gitignore`, **không commit**. Nếu đã lưu cấu hình qua trang web (`/config.json` trên LittleFS) thì cấu hình đó được ưu tiên hơn `secrets.h`.

## Build và nạp

`tools/flash.sh` tự thêm `--library common` và `PartitionScheme=min_spiffs`. Trên Windows, đặt `ARDUINO_CLI` như ở trên.

```bash
tools/flash.sh node1_sensor                                    # chỉ build
tools/flash.sh node1_sensor COM5 60 docs/evidence/T06/node1_boot.txt   # build, nạp, bắt log 60 s
tools/flash.sh node2_actuator COM6 60 docs/evidence/T06/node2_boot.txt
```

Nếu không nạp được: giữ BOOT, nhấn RESET, thả BOOT rồi chạy lại. Lần đầu chuyển sang `min_spiffs` thì phải nạp bằng cáp, và LittleFS sẽ được format (mất `/config.json` cũ).

## Giá trị cần đo/điền trước khi chạy thật (`TODO(verify)` trong `config.h`)

| Hằng số | Giá trị tạm | Lấy giá trị thật bằng |
|---|---|---|
| `LCD_I2C_ADDR` | `0x21` | `tools/i2c_scan` |
| `SOIL_RAW_DRY` / `SOIL_RAW_WET` | `0` / `4095` | `tools/adc_calibrate` |
| `NEOPIXEL_COUNT`, `NEOPIXEL_TYPE` | `1`, `NEO_GRB` | `tools/neopixel_test` |
| `NODE2_MAC` (Node 1) | broadcast `FF:FF:…` | `tools/print_mac` trên Node 2 |
| `NODE1_MAC` (Node 2, tùy chọn) | tắt (nhận mọi nguồn) | `tools/print_mac` trên Node 1 |
| `ESPNOW_CHANNEL` (**cả hai node giống nhau**) | `0` = theo router | kênh router (log `[WIFI] connected ... ch=`); đặt khác 0 để ESP-NOW chạy tiếp khi router tắt |
| `MQTT_HOST` / `MQTT_PORT` (`secrets.h`) | `app.coreiot.io` / `1883` | *Check connectivity* trên CoreIOT |
| `AI_ARENA_SIZE` | 8 KB | `tools/tinyml_parity` (dùng thực + 20 %) |

## Chức năng theo milestone (đã build, chưa kiểm chứng trên kit)

| Milestone | Nội dung | Kiểm tra nhanh |
|---|---|---|
| M3 (T11) | Telemetry + RPC `setMode/setHeater/setFan/setPump` | `[MQTT] connected`, `[RPC] ...` |
| M4 (T12) | RPC `cloudAlert {level,msg}` trên Node 2: LCD dòng 2, NeoPixel chớp vàng 500 ms / đỏ 250 ms, mọi chế độ, không đổi quạt/bơm; `SELF_TEST_ALERT 1` để thử không cần Cloud | `[RPC] cloudAlert level=...` |
| M5 (T06) | ESP-NOW Node 1 → Node 2 mỗi 2 s, Ack, RTT, xử lý reboot Node 1; Node 2 safe mode khi > 10 s không có dữ liệu (`NO DATA`, NeoPixel tím) hoặc Node 1 báo lỗi; telemetry `espnow_age_s` | `[NOW] RTT seq=...`, `[NOW] stats ...` |
| M6 (T07) | Webserver `http://<IP>/` (4 tab), REST `/api/state`, `/api/mode`, `/api/heater` hoặc `/api/fan` + `/api/pump`, `/api/config`; AP cứu hộ `GREENHOUSE-N1-SETUP`/`-N2-` tại `192.168.4.1` | `[WEB] server on port 80`, `[WIFI] rescue AP ...` |
| M7 (T04, T05) | `task_tinyml` (TFLM float32), làm mượt đa số 3, `ai_class` trên LCD/telemetry/ESP-NOW; lỗi cảm biến → không gửi T/H/soil | `[AI] class=... p=... t_us=...` |
| M8 (T14) | OTA qua CoreIOT (chunk MQTT 4 KB, SHA256/MD5, tối đa 3 lần thử/phiên bản) | `[OTA] ...`, telemetry `fw_state` |

**`node1_sensor/model.h` hiện là model DEMO** (train từ dữ liệu tổng hợp, `MODEL_IS_DEMO 1`, khi khởi động log `[AI] WARNING: DEMO model`). Phải thu dữ liệu thật rồi train lại trước khi báo cáo (mục TinyML bên dưới).

Log Serial có tiền tố: `[SENSOR] [LCD] [LED] [CTRL] [ACT] [WIFI] [MQTT] [RPC] [NOW] [WEB] [AI] [OTA] [CFG] [STATS]`. Dòng `[STATS]` mỗi 10 s in heap và stack còn trống của từng task.

## Webserver (T07)

Sửa giao diện tại `web/index.html`, rồi nhúng lại vào firmware (bắt buộc trước khi build):

```bash
python tools/embed_web.py        # ghi node1_sensor/web_assets.h và node2_actuator/web_assets.h
```

Đo độ trễ (≥ 20 lần; dùng `POST /api/mode` với đúng giá trị hiện tại nên không làm đổi trạng thái):

```bash
python tools/web_latency.py --host 192.168.1.50 --node 1 --n 30 --condition "laptop Wi-Fi, 3 m"
```

## TinyML (T04, T05): collect → train → export

```bash
cd ml
python -m venv venv && venv/Scripts/activate          # Linux/macOS: source venv/bin/activate
python -m pip install -r requirements.txt

# 1. Thu dữ liệu: nạp Node 1 với COLLECT_MODE=1 (tools/collect_firmware/README.md), đóng Serial Monitor
python collect_serial.py --port COM5 --label 0 --session s01 --minutes 8 --note "để yên, 28 °C"
#    ... mỗi lớp >= 3 phiên (TINYML_GUIDE.md mục 5)

# 2. Huấn luyện (chia theo phiên; lỗi tổng hợp đánh giá riêng; có baseline luật ngưỡng)
python train.py                         # -> out/report.txt, confusion_*.png, curves.png, meta.json

# 3. Export float32 + INT8, chép model.h vào node1_sensor/ và tools/tinyml_parity/
python export_tflite.py --use float --install

# Chỉ chạy thử đường ống khi chưa có dữ liệu thật (KHÔNG dùng để báo cáo):
python make_demo_data.py && python train.py --data data_demo && python export_tflite.py --use float --install
```

Parity test trên thiết bị: nạp `tools/tinyml_parity` và lưu log vào `docs/evidence/T04/parity_log.txt` (mục tiêu `PARITY: 20/20`).

## OTA (T14)

1. Nạp bản `1.0.0` bằng cáp (partition `min_spiffs`). **Giữ cáp USB và file `.bin` ổn định** để nạp lại khi cần.
2. Đổi `FW_VERSION` trong `config.h` (ví dụ `1.0.1`), rồi chạy `tools/make_ota_bin.sh node1_sensor`: script in đường dẫn `.ino.bin`, kích thước, SHA256 và MD5.
3. Upload lên CoreIOT và gán cho device theo `COREIOT_SETUP.md` mục 10.3–10.6. Thiết bị sẽ đi qua `DOWNLOADING → DOWNLOADED → VERIFIED → UPDATING → UPDATED`.

## Đo đạc (Chương 4)

```bash
python tools/parse_now_log.py --node1 <log_n1> --node2 <log_n2> --out docs/measurements/espnow_rtt_1m.csv --condition "1 m"
python tools/parse_stats_log.py --log <log_2h> --out docs/measurements/heap_node1.csv
python tools/summarize_measurements.py      # -> docs/measurements/summary.md
```

Các script chỉ xử lý số đo thật; không có dữ liệu thì in "khong co du lieu".
