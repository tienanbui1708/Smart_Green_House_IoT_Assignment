# CLAUDE.md – Luật làm việc & trạng thái dự án Smart Greenhouse (BTL IoT)

> **Đọc file này đầu tiên, trước mọi việc.**
> Ngôn ngữ giao tiếp: **tiếng Việt**. Tên biến/hàm/file/commit: tiếng Anh. Comment code: tiếng Việt ngắn gọn, chỉ giải thích "vì sao".

---

## 0. Bối cảnh nhanh

- BTL IoT HCMUT HK261, nền tảng **CoreIOT + Yolo UNO S3 (ESP32-S3, OhStem)**, ≤ 3 người.
- Có: 2 board, 2 DHT20 (I2C), 1 cảm biến đất (A1), 2 LCD1602 I2C, NeoPixel + LED D13 onboard.
- **KHÔNG có**: bơm, quạt, cảm biến ánh sáng → chấp hành là **mô phỏng**. Không code cho các linh kiện này.
- Môi trường: **Arduino IDE** + "OhStem Boards".

---

## 1. TRẠNG THÁI HIỆN TẠI (cập nhật 06/11/2026, sau Task 2–7)

> ⚠️ "Đã code" = **biên dịch được** (OhStem 3.1.3, partition `min_spiffs`), **chưa kiểm chứng trên kit**. Chỉ đánh dấu "đã chạy" khi có log Serial thật.

### ✅ Đã code (build OK cả hai node)

| Milestone | Nội dung | File chính |
|---|---|---|
| **M2 – Firmware lõi** | task + shared_data mutex, app_log, coreiot_link | mục 2 |
| **M4 – phần firmware** | RPC `cloudAlert` Node 2 (LCD, NeoPixel chớp, `SELF_TEST_ALERT`), telemetry `alert_level` | `node2_actuator/*` |
| **M5 – ESP-NOW (T06)** | `EspNowSensorPkt/EspNowAckPkt`, RTT, xử lý reboot Node 1, safe mode, `espnow_age_s`, `ESPNOW_CHANNEL` | `task_espnow.cpp`, `task_espnow_rx.cpp`, `tools/parse_now_log.py` |
| **M6 – Webserver (T07)** | WebServer.h + trang PROGMEM, REST API, `/config.json` LittleFS, AP cứu hộ | `task_web.cpp`, `common/web_common`, `common/config_store`, `web/index.html` |
| **M7 – TinyML (T04, T05)** | TFLM (Chirale) trong `taskTinyml`, pipeline `ml/`, parity sketch, COLLECT_MODE | `task_tinyml.cpp`, `ml/*`, `tools/tinyml_parity/` |
| **M8 – OTA (T14)** | OTA chunk MQTT, SHA256/MD5, NVS chống lặp, `make_ota_bin.sh` | `common/ota_client` |
| **M9 – công cụ đo** | RTT, heap/stack, web latency, tổng hợp | `tools/*.py` |

### 🔄 Chờ người dùng (cần kit/CoreIOT thật)

| Việc | Ghi chú |
|---|---|
| **Task 1 – giá trị thật** | `LCD_I2C_ADDR`, `SOIL_RAW_DRY/WET`, `NEOPIXEL_*`, `NODE2_MAC`, `ESPNOW_CHANNEL` (README bảng `TODO(verify)`) |
| **M3** | `secrets.h` (+ `AP_PASS`), host/port từ *Check connectivity*, log kết nối |
| **M4 – CoreIOT** | dashboard, alarm, RuleChain, Notification (COREIOT_SETUP.md 6–9) |
| **M7 – dữ liệu thật** | `node1_sensor/model.h` hiện là **model DEMO** (`MODEL_IS_DEMO 1`): thu dữ liệu → train → export `--install` |
| **Kiểm chứng trên kit** | ESP-NOW ≥ 200 gói + bài thử tắt router (`docs/evidence/T06/failover_test.txt`), web, parity 20/20, OTA ≥ 3 lần |

---

## 2. Cấu trúc code hiện tại

### Node 1 (`node1_sensor/`)

| File | Task | Trạng thái |
|---|---|---|
| `node1_sensor.ino` | setup() + tạo task + `[STATS]` | ✅ |
| `config.h` | Hằng số chân, ngưỡng, chu kỳ, MQTT, ESP-NOW, web, TinyML, OTA | ✅ |
| `secrets.h.example` | Mẫu WiFi/token/AP_PASS | ✅ |
| `shared_data.h/.cpp` | Mutex RAII, getter/setter, mutex I2C, semaphore mẫu mới, `ai_class`, thống kê ESP-NOW, `sharedHoldSafe` | ✅ |
| `tasks.h` | Khai báo 7 task + `coreiotLoadConfig()` | ✅ |
| `task_sensor.cpp` | `taskSensor` – DHT20 + đất, sentinel, dòng `D,` khi `COLLECT_MODE` | ✅ |
| `task_led.cpp` | `taskLed` – sưởi Auto + an toàn cứng (tắt khi lỗi/AI lớp 3) | ✅ |
| `task_display.cpp` | `taskDisplay` – LCD, nhãn AI, `FAULT` | ✅ |
| `task_coreiot.cpp` | `taskCoreiot` – link, telemetry (`ai_class`, `sensor_fault`), RPC, OTA, AP cứu hộ | ✅ |
| `task_tinyml.cpp` | `taskTinyml` – TFLM MLP, đa số 3, 255 khi khởi động | ✅ |
| `task_espnow.cpp` | `taskEspnow` – gửi gói 2 s, Ack, RTT, thống kê | ✅ |
| `task_web.cpp` | `taskWeb` – WebServer.h, `/api/state`, `/api/mode`, `/api/heater` | ✅ |
| `model.h` | Mảng TFLite `g_model` | ⚠️ DEMO (dữ liệu tổng hợp) |
| `web_assets.h` | Trang PROGMEM (sinh bởi `tools/embed_web.py`) | ✅ |

OTA không có `task_ota.cpp` riêng: `common/ota_client` chạy trong `taskCoreiot` vì PubSubClient không thread-safe (mọi publish/loop phải cùng một task).

### Node 2 (`node2_actuator/`)

| File | Task | Trạng thái |
|---|---|---|
| `node2_actuator.ino` | setup() + tạo task + `[STATS]` | ✅ |
| `config.h` | Hằng số + `USE_FAKE_SENSOR_DATA 0`, ESP-NOW, cloudAlert, web, OTA | ✅ |
| `secrets.h.example` | Mẫu WiFi/token/AP_PASS | ✅ |
| `shared_data.h/.cpp` | Mutex RAII, getter/setter, `CloudAlertState`, `sharedHoldSafe` | ✅ |
| `tasks.h` | Khai báo 6 task + `coreiotLoadConfig()` | ✅ |
| `task_control.cpp` | `taskControl` – luật Auto, safe mode khi stale/lỗi, `SELF_TEST_ALERT` | ✅ |
| `task_actuator.cpp` | `taskActuator` – D13 quạt, NeoPixel ẩm / tím NO DATA / chớp cloudAlert | ✅ |
| `task_display.cpp` | `taskDisplay` – LCD, NO DATA, N1 SENSOR FAULT, dòng cảnh báo | ✅ |
| `task_coreiot.cpp` | `taskCoreiot` – link, RPC (+`cloudAlert`), telemetry (`espnow_age_s`, `alert_level`), OTA | ✅ |
| `task_espnow_rx.cpp` | `taskEspnowRx` – nhận gói, kiểm seq/MAC/ver, gửi Ack | ✅ |
| `task_web.cpp` | `taskWeb` – `/api/state`, `/api/mode`, `/api/fan`, `/api/pump` | ✅ |
| `web_assets.h` | Trang PROGMEM | ✅ |

### Dùng chung (`common/`)

| File | Nội dung | Trạng thái |
|---|---|---|
| `protocol.h` | Topic MQTT/OTA, RPC, khóa telemetry, gói ESP-NOW, sentinel, `AI_CLASS_*` | ✅ |
| `coreiot_link.h/.cpp` | Wi-Fi backoff + kênh cố định + AP cứu hộ + MQTT + queue RPC + hook topic phụ | ✅ |
| `config_store.h/.cpp` | `/config.json` LittleFS (tmp + rename), ưu tiên hơn `secrets.h` | ✅ |
| `web_common.h/.cpp` | JSON, lỗi 400/409, `/api/config`, thông tin hệ thống | ✅ |
| `ota_client.h/.cpp` | OTA ThingsBoard MQTT | ✅ (chưa thử trên CoreIOT) |
| `app_log.h/.cpp` | Log Serial an toàn nhiều task (mutex), `appLogRaw` | ✅ |

### ML (`ml/`)

`features.py` (chuẩn hóa, sentinel – khớp `config.h`), `collect_serial.py`, `train.py`, `export_tflite.py`, `make_demo_data.py` (chỉ để thử đường ống), `requirements.txt`. Kết quả ở `ml/out/` (không commit).

### Tools (`tools/`)

| Sketch/script | Dùng để |
|---|---|
| `i2c_scan/` | Quét địa chỉ I2C → xác nhận LCD = 0x21? |
| `adc_calibrate/` | Đọc A1 thô để lấy SOIL_RAW_DRY / SOIL_RAW_WET |
| `print_mac/` | In MAC để cấu hình ESP-NOW peer |
| `blink/` | Test nạp board |
| `lcd_dht20/` | Test LCD + DHT20 độc lập |
| `neopixel_test/` | Test NeoPixel chân/màu |
| `env_info/` | In thông tin core, chip, flash |
| `tinyml_parity/` | Parity test 20 vector TFLM vs Python |
| `collect_firmware/` | Hướng dẫn build Node 1 với `COLLECT_MODE=1` |
| `flash.sh`, `make_ota_bin.sh` | Build/nạp/log (partition `min_spiffs`); xuất `.bin` OTA + SHA256/MD5 |
| `embed_web.py` | `web/index.html` → `node*/web_assets.h` |
| `parse_now_log.py`, `parse_stats_log.py`, `web_latency.py`, `summarize_measurements.py` | Số đo Chương 4 |

---

## 3. Quy trình làm mỗi phiên

**Trước khi code:** nêu kế hoạch 3–6 dòng (file nào đổi, kết quả mong đợi, cách kiểm tra). Việc nhỏ thì làm luôn.

**Thứ tự milestone:** M3 → M4 → M5 → M6 → M7 → M8 → M9 → M10. Nếu trễ: bỏ M8 (T14), thêm T08/T10 để đủ 8 điểm.

**Chỉ làm đúng milestone được gọi.** Không tự mở rộng sang milestone khác.

---

## 4. Luật code firmware (không được vi phạm)

### 4.1 Kiến trúc bắt buộc

1. Mỗi chức năng = **1 FreeRTOS Task**. `loop()` chỉ `vTaskDelay(portMAX_DELAY)`.
2. Dữ liệu chung → `shared_data.cpp` với getter/setter bọc mutex. Biến 1 task dùng → `static` trong file task.
3. Mutex: lấy → làm cực ngắn → trả. **Không I/O, không delay, không Serial dài** khi giữ mutex.
4. Callback (ESP-NOW, MQTT): chỉ `xQueueSend(..., 0)` hoặc set cờ. Không làm gì nặng.
5. Mọi vòng for(;;) phải có `vTaskDelay(...)` → không watchdog.
6. Giao tiếp giữa task: `Queue` (dữ liệu) hoặc `Semaphore`/`EventGroup` (cờ). Không polling bận.

### 4.2 Bộ nhớ

- **Cấm** `String` nối chuỗi trong vòng lặp. Dùng `char buf[N]` + `snprintf`.
- **Cấm** `malloc/new` trong vòng lặp nóng. Cấp phát 1 lần lúc khởi tạo.
- Log `ESP.getFreeHeap()` và `ESP.getMinFreeHeap()` định kỳ → phát hiện rò rỉ.

### 4.3 Ngoại vi

- `Wire.begin()` **1 lần** trong `setup()`. Hai task dùng I2C → mutex I2C riêng (đã có ở Node 1).
- LCD: **không `clear()`**. Ghi đè chuỗi dài cố định, chỉ ghi khi nội dung đổi.
- NeoPixel: chỉ `show()` khi giá trị đổi. Chân = `RGB_LED` (macro core), không hard-code GPIO.
- Soil: hiệu chỉnh 2 điểm (`SOIL_RAW_DRY`, `SOIL_RAW_WET` trong `config.h`), công thức tuyến tính kẹp 0–100.

### 4.4 Mạng

- Wi-Fi: backoff `{3s, 5s, 10s, 20s, 30s}`. Không `while(WiFi!=CONNECTED)` chặn.
- MQTT: `setBufferSize(512)`, `loop()` ≤ 100 ms, username = Access Token, password rỗng. Chỉ QoS 0.
- **Cấm `delay()`** trong task mạng. Dùng `vTaskDelay`.
- ESP-NOW callback: chữ ký khác theo core 2.x/3.x → dùng `#if ESP_ARDUINO_VERSION_MAJOR >= 3`.

### 4.5 Style

- Hàm < ~40 dòng, 1 trách nhiệm. Tên rõ nghĩa (`readSoilPercent`, `applyAutoRules`).
- Hằng số → `config.h`. Cấm magic number rải rác.
- Kiểm tra giá trị trả về của I/O (`dht.read()`, `mqtt.publish()`, `esp_now_send()`). Không bỏ qua im lặng.
- Log: tiền tố `[SENSOR]`, `[MQTT]`, `[NOW]`, `[AI]`, `[OTA]`, `[WEB]`. Cờ `DEBUG_LOG` trong `config.h`.

---

## 5. Giao thức dữ liệu (KHÔNG được thay đổi một nửa)

Nguồn sự thật: `common/protocol.h` + `PROJECT.md` mục 7. Sửa giao thức phải cập nhật **cùng lúc** protocol.h, firmware 2 node, dashboard, RuleChain và PROJECT.md.

### 5.1 Telemetry MQTT

**Node 1:** `temperature, humidity, soil, ai_class, sensor_fault, heater, auto, rssi, heap, uptime, fw_version`
> Khi DHT20 lỗi: **không gửi** `temperature/humidity/soil`, chỉ gửi `ai_class=3, sensor_fault=true`.

**Node 2:** `fan, pump, auto, rssi, heap, uptime, fw_version, espnow_age_s, alert_level`

**OTA (cả 2 node):** `current_fw_title, current_fw_version, fw_state, fw_error`

### 5.2 RPC nhận tại thiết bị

| Method | Params | Node | Khi nào nhận |
|---|---|---|---|
| `setMode` | `bool` (true=Auto) | 1, 2 | luôn |
| `setHeater` | `0..255` | 1 | chỉ Manual |
| `setFan` | `bool` | 2 | chỉ Manual |
| `setPump` | `bool` | 2 | chỉ Manual |
| `cloudAlert` | `{level:0\|1\|2, msg:"≤16 ký tự"}` | 2 | luôn |

Phản hồi two-way RPC: `{"ok":true}` hoặc `{"ok":false,"reason":"auto_mode"|"unknown_method"|"bad_params"}`.

### 5.3 Gói ESP-NOW (`common/protocol.h`)

**`EspNowSensorPkt` (Node 1 → Node 2):** `ver, seq, temperature, humidity, soil, ai_class, heater, tx_ms`
**`EspNowAckPkt` (Node 2 → Node 1):** `ver, seq, rx_ms`

---

## 6. Việc cần làm ngay (M3)

Firmware code đã sẵn sàng. Chỉ cần cấu hình thực tế:

```
1. Chạy tools/i2c_scan → ghi địa chỉ LCD thật vào config.h (LCD_I2C_ADDR)
2. Chạy tools/adc_calibrate với cảm biến đất khô → ghi SOIL_RAW_DRY
   Nhúng vào nước → ghi SOIL_RAW_WET
3. Trên CoreIOT: tạo device Node1, Node2 → lấy Access Token
4. Lấy MQTT host/port từ "Check connectivity" trên trang device
5. Tạo node1_sensor/secrets.h (copy từ .example, điền SSID/pass/token/host/port)
   Tạo node2_actuator/secrets.h tương tự
6. Nạp Node 1 → mở Serial Monitor 115200 → xác nhận:
   [WIFI] connected ip=...
   [MQTT] connected, subscribe RPC ok
   [SENSOR] seq=1 T=... H=... soil=...%
   [MQTT] telemetry sent: {...}
7. Vào CoreIOT → device Node1 → Latest telemetry → thấy dữ liệu
8. Từ CoreIOT gửi RPC setMode(false) → Serial Node1 in [RPC] → ok
9. Làm tương tự Node 2
```

**Minh chứng M3 cần lưu:**
- `docs/evidence/T11/latest_telemetry_node1.png`
- `docs/evidence/T11/latest_telemetry_node2.png`
- Log Serial cả 2 node lúc kết nối (copy vào `docs/evidence/T11/serial_node1.txt`)

---

## 7. Luật debug

### 7.1 Quy trình chuẩn

1. **Tái hiện**: xin log Serial đầy đủ từ lúc reset, phiên bản core/thư viện.
2. **Cô lập**: tắt dần chức năng (chỉ DHT20 → chỉ LCD → chỉ Wi-Fi…).
3. **Giả thuyết → thí nghiệm nhỏ → kết luận**. Mỗi lần chỉ đổi 1 biến.
4. Không "thử vận may" bằng `delay()` lung tung.

### 7.2 Triệu chứng thường gặp

| Triệu chứng | Nguyên nhân | Sửa |
|---|---|---|
| LCD không hiện | Sai địa chỉ I2C | Chạy `tools/i2c_scan` |
| DHT20 trả 0/NaN | Đọc quá nhanh; dây lỏng | ≥ 2s giữa 2 lần đọc; kiểm tra dây |
| Soil nhảy loạn | Dải ADC khác | In raw, hiệu chỉnh 2 điểm |
| NeoPixel sai màu | Sai thứ tự màu (GRB/RGB) | Đổi cờ `NEOPIXEL_TYPE` |
| Reset liên tục | Stack overflow; watchdog | Tăng stack; thêm `vTaskDelay` |
| MQTT state=-5 | Token không hợp lệ | Kiểm tra Access Token |
| MQTT state=-4 | Timeout (sai host/cổng) | Dùng host từ "Check connectivity" |
| ESP-NOW không nhận | Khác kênh; sai MAC; sai chữ ký callback | In MAC; `WiFi.channel()`; kiểm tra core version |
| RPC không tới | Chưa subscribe `v1/devices/me/rpc/request/+` | Kiểm tra log `[MQTT] subscribe RPC` |

### 7.3 Mã `mqtt.state()`

`-4` timeout · `-3` mất kết nối · `-2` host/cổng sai · `4` username/pass sai · `5` token không hợp lệ.

---

## 8. Luật repo & commit

- **Không commit** `secrets.h` (trong `.gitignore`). Chỉ commit `secrets.h.example`.
- **Không dán** token thật vào code, chat, log, ảnh chụp.
- Commit: `type(scope): nội dung` → `feat(node1): add task_espnow`, `fix(sensor): sentinel on DHT20 fault`.
- `main` luôn nạp được. Làm trên `feature/<tên>` rồi merge.
- File `.bin` lớn, dataset thô >vài MB → dùng `.gitignore` hoặc Git LFS.

---

## 9. Luật minh chứng (không có = không có điểm)

Sau mỗi milestone Claude phải liệt kê:
1. Ảnh/log/video nào cần chụp, kịch bản chụp, tên file `docs/evidence/<Txx>/<mô-tả>.<đuôi>`.
2. Lệnh tạo log.
3. Dòng "tuyên bố" tương ứng bảng tự đánh giá T/N.

---

## 10. Những điều CẤM

1. Tự code khi chưa có lệnh; tự thêm tính năng ngoài T/N đã chọn.
2. Giả định có bơm/quạt/cảm biến ánh sáng thật.
3. `delay()` trong task; biến toàn cục tự do; giữ mutex khi I/O chậm; `String` nối chuỗi vòng lặp.
4. Sửa `protocol.h` / tên khóa telemetry / tên RPC chỉ 1 phía.
5. Tuyên bố "đã chạy tốt" khi chưa có log từ kit thật.
6. Bịa số đo, tên API, tên thư viện, URL.
7. Commit hoặc in token/mật khẩu.

---

## 11. Bảng kiểm tra trước nộp

- [ ] Mỗi mã T có đủ minh chứng (ảnh/log/video/JSON) đúng thư mục `docs/evidence/<Txx>/`
- [ ] Bảng tự đánh giá T/N và bảng phân công trong báo cáo
- [ ] Điểm công nghệ dự kiến ≥ 8 sau khi trừ phần chưa xong
- [ ] Hai node biên dịch cùng `protocol.h`; telemetry/RPC khớp dashboard và RuleChain
- [ ] README chạy được từ máy sạch; thư viện có phiên bản; không secret trong repo
- [ ] Báo cáo nói rõ chấp hành là mô phỏng; dữ liệu TinyML thật/tổng hợp tách bạch
- [ ] ≥ 5 tài liệu tham khảo; trích dẫn phần kế thừa (OhStem, CoreIOT template)
- [ ] Khai báo phạm vi sử dụng AI
- [ ] Video 5–10 phút có chú thích mã T; link trong báo cáo
- [ ] Nộp trước tuần 16

---

## 12. Tài liệu bổ trợ (đọc khi cần)

| File | Đọc khi làm milestone |
|---|---|
| `PROJECT.md` | Tham chiếu giao thức, phần cứng, luật chấm |
| `COREIOT_SETUP.md` | M3, M4, M8 (device, dashboard, RuleChain, Notification, OTA) |
| `SETUP_FIRMWARE.md` | M0, M1, M2, M6 (cài IDE, ngoại vi, Webserver) |
| `TINYML_GUIDE.md` | M7 (thu dữ liệu, train, export, chạy trên thiết bị) |
