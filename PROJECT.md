# PROJECT.md – Mô tả & cấu trúc dự án

> **Bộ tài liệu của dự án** (đọc theo thứ tự):
> 1. `PROJECT.md` – mô tả, kiến trúc, giao thức, kế hoạch đo (file này, nguồn sự thật về giao thức).
> 2. `CLAUDE.md` – luật làm việc, luật code, debug, milestone.
> 3. `COREIOT_SETUP.md` – cấu hình CoreIOT từng bước (device, dashboard, alarm, notification, RuleChain, OTA, N07).
> 4. `SETUP_FIRMWARE.md` (cài môi trường, M0–M2, Webserver T07), `TINYML_GUIDE.md` (T04, T05); (sẽ có) `REPORT_VIDEO_PLAN.md`.
> Khi hai file mâu thuẫn: giao thức/tên khóa theo **PROJECT.md mục 7**; thao tác trên CoreIOT theo **COREIOT_SETUP.md**.

> Bài tập lớn môn **Internet of Things** (HCMUT, HK261) – GVHD: TS. Lê Trọng Nhân.
> Nền tảng bắt buộc: **CoreIOT** + **ESP32-S3 (Yolo UNO của OhStem)**.
> Quy ước: mục nào ghi `CẦN XÁC MINH` là thông tin chưa kiểm chứng, phải kiểm tra trước khi dùng. Không được coi là sự thật.

---

## 1. Tên đề tài

**HỆ THỐNG GIÁM SÁT VÀ ĐIỀU KHIỂN VI KHÍ HẬU NHÀ KÍNH THÔNG MINH HAI NÚT TÍCH HỢP TINYML, ESP-NOW VÀ COREIOT**

(EN: *Smart Greenhouse Microclimate Monitoring and Control – Two-node System with TinyML, ESP-NOW and CoreIOT*)

Đề tài do nhóm tự đề xuất. Phần kế thừa từ dự án mẫu **Green House của OhStem** (docs.ohstem.vn → Yolo UNO → Dự án nhà kính thông minh) phải được ghi rõ trong báo cáo. Phần nhóm tự phát triển: TinyML, ESP-NOW, RuleChain/Notification, OTA, Webserver, dashboard.

## 2. Bài toán & mục tiêu

- Nhà kính cần giữ nhiệt độ, độ ẩm không khí, độ ẩm đất trong ngưỡng. Hệ thống đo, phân loại trạng thái bằng AI chạy ngay trên thiết bị, điều khiển chấp hành, cảnh báo và cho phép điều khiển từ xa.
- Hệ thống vẫn hoạt động cục bộ khi mất Internet (hai node nói chuyện trực tiếp qua ESP-NOW).
- Chấp hành bơm/quạt/sưởi được **mô phỏng** bằng NeoPixel/LED/LCD vì kit không có bơm, quạt (xem mục 4). Báo cáo phải nói rõ điều này.

## 3. Luật chấm điểm (nguồn: ThongBao_BaiTapLon_IoT.docx)

Điểm BTL = `min(8, Σ Công nghệ)` + `Báo cáo (≤2)` + `min(1, Σ Nâng cao)`. Thông báo ghi tối đa 10.

- Một yếu tố chỉ được tính khi có **minh chứng** (ảnh, log, video, mã nguồn) trong báo cáo.
- Hiện thực một phần thì được điểm tương ứng mức hoàn thành.
- Điểm cá nhân có thể điều chỉnh theo bảng phân công, đóng góp, vấn đáp. Mỗi thành viên phải giải thích được phần mình làm.
- Được dùng AI hỗ trợ nhưng phải ghi rõ phạm vi sử dụng trong báo cáo.
- Cấm sao chép giữa các nhóm. Mọi tài liệu/mã/dự án mẫu tham khảo phải trích dẫn.

### 3.1 Tổ hợp yếu tố công nghệ đã chọn

| Mã | Yếu tố | Điểm | Trạng thái kế hoạch |
|---|---|---|---|
| T04 | Thông tin nội suy từ AI (TinyML chạy trên thiết bị) | 1.0 | Chọn |
| T05 | Công cụ thu thập dữ liệu, huấn luyện, export model TinyML | 1.0 | Chọn |
| T06 | Giao tiếp không dây ESP-NOW | 1.5 | Chọn |
| T07 | Webserver trên thiết bị (tự hiện thực, hiển thị + điều khiển local) | 1.0 | Chọn |
| T11 | Dùng template CoreIOT, thiết kế dashboard, giám sát + điều khiển từ xa | 1.0 | Chọn |
| T12 | RuleChain cho tính năng thông minh | 1.0 | Chọn |
| T13 | Notification khi dữ liệu vượt ngưỡng | 0.5 | Chọn |
| T14 | OTA firmware cho thiết bị | 2.0 | Chọn (rủi ro cao nhất) |
| | **Tổng** | **9.0** | Vượt trần 8 (đệm 1.0) |

**Lưu ý đệm điểm:** nếu T14 không hoàn thành, tổng chỉ còn **7.0**. Phương án dự phòng 1.0 điểm không cần phần cứng: **T08** (IoT Gateway Python trên PC) hoặc **T10** (GUI trên PC). Chỉ làm dự phòng khi T14 có nguy cơ không xong.

### 3.2 Điểm nâng cao (tối đa 1)

- **N07**: tự deploy ThingsBoard trên máy local (Docker). Tùy chọn, làm sau cùng.

### 3.3 Không làm

T01 (Tasmota), T02 (Modbus RS485), T03 (BLE), T09, T15, N01–N06. Lý do: thiếu phần cứng hoặc chi phí/rủi ro cao so với điểm.

## 4. Phần cứng (chỉ dùng kit Yolo UNO của OhStem, không dùng Yolo:Bit)

### 4.1 Đang có

| Linh kiện | SL | Gắn vào |
|---|---|---|
| Yolo UNO S3 (ESP32-S3, Wi-Fi, BLE) | 2 | Node 1 và Node 2 |
| DHT20 (I2C) | 2 | Node 1 (chính), Node 2 (tùy chọn, đối chiếu) |
| Cảm biến độ ẩm đất (analog) | 1 | Node 1, cổng **A1** |
| LCD1602 I2C | 2 | Mỗi node 1 cái |
| NeoPixel onboard | 2 | Node 1: mô phỏng sưởi; Node 2: báo độ ẩm |
| LED onboard **D13** | 2 | Node 2: mô phỏng quạt |
| Dây Grove | đủ | Node 1 cần ~3 sợi (DHT20, LCD, độ ẩm đất) |
| Cáp USB-C truyền dữ liệu, router Wi-Fi 2.4 GHz, PC | | |

### 4.2 Không có (không được viết code phụ thuộc vào các món này)

Máy bơm mini + module 2 kênh USB, quạt mini (module PWM), cảm biến ánh sáng Grove. Thay bằng mô phỏng:
- "Bơm đang tưới" → dòng chữ `PUMP:ON` trên LCD Node 2 (không dùng NeoPixel vì NeoPixel Node 2 dành cho độ ẩm).
- "Quạt chạy" → LED D13 Node 2 sáng.
- "Sưởi" → độ sáng kênh đỏ của NeoPixel Node 1.

### 4.3 Thông tin phần cứng đã xác nhận từ docs OhStem

- Board package Arduino IDE: thêm URL `https://raw.githubusercontent.com/AITT-VN/ohstem_arduino_board/main/package_xcon_index.json`, cài "OhStem Boards".
- Dùng macro chân của core: `D13` (LED onboard), `A1` (độ ẩm đất), `D9`/`D10` (cổng bơm, không dùng).
- LCD1602 I2C trong docs OhStem dùng địa chỉ **`0x21`** (khác `0x27` thường gặp). Phải chạy I2C scan để xác nhận trên kit thật.
- Độ ẩm đất trong docs OhStem tính `analogRead(A1) / 10.24` (giả định dải 0–1023). **CẦN XÁC MINH** dải ADC thật trên kit (ESP32-S3 gốc là 12-bit, 0–4095); phải hiệu chỉnh khô/ướt bằng số đo thực.

### 4.4 Chưa xác nhận

- Chân của NeoPixel onboard: báo cáo mẫu khóa trước ghi GPIO 45. **CẦN XÁC MINH** với core OhStem (macro/variant). Không hard-code trước khi kiểm tra.
- Giao diện CoreIOT: https://app.coreiot.io (đã xác nhận qua tài liệu OhStem/CoreIOT). **Host và cổng MQTT thật**: lấy từ hộp *Check connectivity* trên trang device (xem COREIOT_SETUP.md mục 3.4); mặc định ThingsBoard là cổng 1883 (không mã hóa), 8883 (TLS). **CẦN XÁC MINH**.
- **Quyền tài khoản CoreIOT của lớp** (RỦI RO): chưa biết tài khoản có đủ quyền dùng *Device profiles*, *Rule chains*, *Notification center*, *OTA updates* hay không. Nếu thiếu quyền thì T12/T13/T14 bị ảnh hưởng. Phải kiểm tra ngay ở M0; nếu thiếu thì hỏi thầy (trongnhanle@hcmut.edu.vn) hoặc cân nhắc N07 (ThingsBoard local) để có đủ quyền.
- Phiên bản arduino-esp32 mà OhStem core đang dùng (2.x hay 3.x). Ảnh hưởng chữ ký callback ESP-NOW.

## 5. Kiến trúc hệ thống

```
        ┌───────────────── COREIOT ──────────────────┐
        │ Dashboard · RuleChain · Alarm/Notification │
        │ OTA · Device Node1 · Device Node2          │
        └──▲──────────▲──────────────▲──────────▲────┘
 telemetry │ attr/OTA │     telemetry│ RPC/OTA  │
           │          │              │          │
┌──────────┴────────────┐  ESP-NOW  ┌┴──────────┴───────────┐
│ NODE 1 – CẢM BIẾN     │ ────────► │ NODE 2 – CHẤP HÀNH    │
│ DHT20 · Đất (A1)      │ dữ liệu   │ LCD · NeoPixel · D13  │
│ TinyML (T04)          │ ◄──────── │ Auto/Manual + failover│
│ LCD · NeoPixel        │ lệnh/ack  │ WebServer (T07)       │
│ WebServer (T07)       │           └───────────────────────┘
└───────────────────────┘
PC: collect → train → export model.h (T05) · Docker ThingsBoard (N07, tùy chọn)
```

Mô hình kết nối: **trực tiếp** (mỗi node kết nối độc lập tới CoreIOT; tính năng thông minh bằng RuleChain) kết hợp liên lạc cục bộ ESP-NOW giữa hai node.

### 5.1 Ba luồng chính

1. **Bình thường:** Node 1 đo → TinyML phân loại → gửi gói ESP-NOW cho Node 2 và telemetry lên CoreIOT. Node 2 điều khiển chấp hành mô phỏng, đồng thời nhận RPC từ CoreIOT.
2. **Mất Internet:** Node 2 vẫn chạy luật cục bộ nhờ ESP-NOW (điểm cần demo và đo thời gian failover).
3. **OTA:** upload firmware `.bin` trên CoreIOT → thiết bị phát hiện bản mới → tải về → ghi flash → reboot → báo phiên bản mới.

### 5.2 Vai trò hai node

| | Node 1 (Sensor) | Node 2 (Actuator) |
|---|---|---|
| Đo | DHT20, độ ẩm đất | (tùy chọn) DHT20 thứ hai |
| AI | TinyML phân loại | – |
| Chấp hành | NeoPixel = sưởi mô phỏng (kênh đỏ) | NeoPixel = độ ẩm; D13 = quạt; LCD = trạng thái bơm |
| Cục bộ | LCD, Webserver | LCD, Webserver |
| Truyền | ESP-NOW gửi; MQTT gửi telemetry, nhận RPC | ESP-NOW nhận; MQTT gửi telemetry, nhận RPC |

## 6. Cấu trúc repo

```
smart-greenhouse/
├─ README.md                    # hướng dẫn cài đặt, cấu hình, chạy (báo cáo mục 6)
├─ CLAUDE.md                    # luật cho Claude Code
├─ PROJECT.md                   # file này
├─ COREIOT_SETUP.md             # cấu hình CoreIOT từng bước
├─ SETUP_FIRMWARE.md            # cài môi trường, firmware M0–M2, Webserver T07
├─ TINYML_GUIDE.md              # thu dữ liệu, train, export, chạy trên Node 1 (T04, T05)
├─ REPORT_VIDEO_PLAN.md         # (sẽ có) khung báo cáo, kịch bản video, lịch tuần
├─ node1_sensor/                # sketch Arduino IDE (thư mục trùng tên .ino)
│  ├─ node1_sensor.ino          # setup(), tạo task
│  ├─ config.h                  # CHÂN, địa chỉ I2C, ngưỡng, tên topic (KHÔNG chứa token)
│  ├─ secrets.h.example         # mẫu WIFI/TOKEN; secrets.h bị .gitignore
│  ├─ shared_data.h/.cpp        # dữ liệu chung + mutex + getter/setter
│  ├─ task_sensor.cpp  task_tinyml.cpp  task_espnow.cpp
│  ├─ task_coreiot.cpp task_web.cpp     task_display.cpp  task_ota.cpp
│  ├─ model.h                   # xuất từ ml/export_tflite.py
│  └─ data/                     # file web cho LittleFS
├─ node2_actuator/
│  ├─ node2_actuator.ino
│  ├─ config.h  secrets.h.example  shared_data.h/.cpp
│  ├─ task_espnow_rx.cpp task_control.cpp task_actuator.cpp
│  ├─ task_coreiot.cpp task_web.cpp task_display.cpp task_ota.cpp
│  └─ data/
├─ common/                      # định nghĩa dùng chung 2 node (gói ESP-NOW, tên method RPC)
│  └─ protocol.h
├─ ml/
│  ├─ collect_serial.py         # đọc Serial → CSV có nhãn
│  ├─ train.py                  # huấn luyện Keras
│  ├─ export_tflite.py          # TFLite INT8 → model.h
│  ├─ requirements.txt
│  └─ data/                     # CSV thô và ghi chú cách thu
├─ cloud/
│  ├─ rulechain_node1.json  rulechain_node2.json  dashboard.json
│  └─ notes.md                  # cách import, cấu hình notification
├─ local_tb/                    # N07 (tùy chọn)
│  └─ docker-compose.yml
├─ tools/                       # i2c_scan, đo độ trễ, script log
├─ docs/
│  ├─ evidence/                 # ảnh, log, đồ thị, ghi chú minh chứng theo mã T
│  ├─ measurements/             # CSV số đo thô cho chương 4
│  └─ report/                   # nguồn báo cáo
└─ .gitignore                   # secrets.h, *.bin lớn, __pycache__, venv
```

## 7. Giao thức dữ liệu (nguồn sự thật duy nhất, đặt tại `common/protocol.h`)

### 7.1 Gói ESP-NOW Node 1 → Node 2 (`Pkt`)

| Trường | Kiểu | Ý nghĩa |
|---|---|---|
| `ver` | `uint8_t` | phiên bản giao thức (=1) |
| `seq` | `uint32_t` | số thứ tự, đo mất gói |
| `t` | `float` | nhiệt độ °C |
| `h` | `float` | độ ẩm không khí % |
| `soil` | `uint8_t` | độ ẩm đất % (đã hiệu chỉnh) |
| `cls` | `uint8_t` | nhãn TinyML (xem 8.1) |
| `heater` | `uint8_t` | 0–255 sưởi mô phỏng |
| `tx_ms` | `uint32_t` | `millis()` lúc gửi, dùng đo độ trễ |

Gói lệnh Node 2 → Node 1 (`Cmd`) và `Ack` dùng cùng khuôn `ver/seq`. Hai node phải biên dịch với cùng `protocol.h`.

### 7.2 Telemetry MQTT (topic `v1/devices/me/telemetry`)

Node 1: `temperature, humidity, soil, ai_class, heater, auto, rssi, heap, uptime, fw_version` (và `sensor_fault: true` khi `ai_class = 3`)
Node 2: `fan, pump, auto, rssi, heap, uptime, fw_version, espnow_age_s`

Dùng tên khóa cố định, không đổi giữa chừng (dashboard và RuleChain phụ thuộc).

Khóa bổ sung **khi OTA** (cả hai node, gửi dạng telemetry): `current_fw_title`, `current_fw_version`, `fw_state` (ví dụ `DOWNLOADING`, `DOWNLOADED`, `VERIFIED`, `UPDATING`, `UPDATED`, `FAILED`), `fw_error` (chỉ khi `FAILED`). Tên và giá trị `fw_state` **CẦN XÁC MINH** với tài liệu ThingsBoard (xem COREIOT_SETUP.md mục 10).

Shared attributes do server ghi khi gán OTA (thiết bị chỉ đọc): `fw_title`, `fw_version`, `fw_size`, `fw_checksum`, `fw_checksum_algorithm`.

### 7.3 RPC (topic `v1/devices/me/rpc/request/+`)

| Method | Params | Node | Điều kiện |
|---|---|---|---|
| `setMode` | `true`=Auto, `false`=Manual | 1, 2 | luôn |
| `setHeater` | `0..255` | 1 | chỉ Manual |
| `setFan` | `true/false` | 2 | chỉ Manual |
| `setPump` | `true/false` | 2 | chỉ Manual (mô phỏng) |
| `cloudAlert` | `{"level":0\|1\|2,"msg":"<≤16 ký tự>"}` | 2 | luôn (mọi chế độ) | `0` xóa cảnh báo; `1` cảnh báo (NeoPixel chớp vàng); `2` nghiêm trọng (chớp đỏ); `msg` hiện dòng 2 LCD 10 s. Do RuleChain gửi khi Alarm tạo/xóa. Không đổi trạng thái chấp hành. |

Phản hồi two-way RPC tại `v1/devices/me/rpc/response/<id>`. Lệnh bị từ chối (đang Auto) phải trả `{"ok":false,"reason":"auto_mode"}`. Các lý do khác: `unknown_method` (method lạ), `bad_params` (JSON hỏng hoặc `params` sai kiểu/ngoài dải). Hằng số nằm ở `common/protocol.h`.

## 8. Chức năng chi tiết

### 8.1 TinyML (T04, T05)

- **Đầu vào:** cửa sổ trượt 5 mẫu × 3 đặc trưng (nhiệt độ, độ ẩm không khí, độ ẩm đất) = 15 giá trị, chuẩn hóa. Chu kỳ lấy mẫu 2 s.
- **Nhãn (4 lớp):** `0` bình thường, `1` đất khô, `2` quá nóng/quá ẩm, `3` cảm biến lỗi.
- **Mô hình:** MLP nhỏ (ví dụ 15 → 32 → 16 → 4, ReLU, Softmax). Số lớp/nơ-ron có thể đổi nếu RAM/độ chính xác yêu cầu.
- **Dữ liệu:** thu bằng chính kit (`ml/collect_serial.py`). Kịch bản gợi ý: bình thường (để yên), nóng (đưa nguồn ấm lại gần DHT20), ẩm (hà hơi/khăn ẩm), đất khô/ướt (cắm cảm biến trong không khí và trong cốc nước), lỗi (rút dây/ngắt cảm biến). Lớp "lỗi" có thể bổ sung giá trị phi lý tổng hợp, **nhưng báo cáo phải tách bạch phần dữ liệu thật và phần tổng hợp**.
- **Export:** TFLite **float32 trước** (an toàn, mô hình chỉ vài KB), **INT8 nếu chạy tốt** để có số liệu so sánh kích thước/độ chính xác/thời gian; ghi vào `model.h`. Báo cáo ghi rõ bản nào chạy thật trên thiết bị. Thư viện chạy trên thiết bị: `Chirale_TensorFlowLite` và/hoặc `ArduTFLite` (**CẦN XÁC MINH** biên dịch được trên core OhStem). Phương án dự phòng: tự cài suy luận MLP bằng C, ghi rõ trong báo cáo. Chuẩn hóa dùng **hằng số cố định** (`TINYML_GUIDE.md` mục 2.2). Quy tắc sentinel khi cảm biến lỗi: T=−40, H=0.
- **Cần ghi số đo:** accuracy trên tập test thật (tách riêng khỏi dữ liệu lỗi tổng hợp), confusion matrix, so sánh với baseline luật ngưỡng, kích thước model, RAM (tensor arena), thời gian suy luận (µs/ms), kết quả parity test thiết bị vs Python.
- **Tác động hệ thống (T04):** khi `ai_class = 3` (cảm biến lỗi) Node 1 **không gửi** `temperature/humidity/soil` hỏng lên Cloud, chỉ gửi `ai_class` và `sensor_fault: true`; LCD hiện `FAULT`. Chi tiết: `TINYML_GUIDE.md` mục 8.3.

### 8.2 ESP-NOW (T06)

- Node 1 gửi `Pkt` mỗi 2 s; Node 2 gửi `Ack` và/hoặc `Cmd`.
- Callback chỉ đẩy vào queue (`xQueueSend`), không xử lý nặng trong callback.
- Node 2 coi dữ liệu là cũ nếu `espnow_age_s > 10`: LCD hiện `NO DATA`, dùng chế độ an toàn (tắt chấp hành mô phỏng).
- Hai node cùng kết nối Wi-Fi nên cùng kênh với router (nếu không cùng kênh ESP-NOW sẽ không nhận được).

### 8.3 Luật điều khiển (cấu hình trong `config.h`, đổi được)

| Điều kiện | Hành động |
|---|---|
| Node 1, Auto: `t < 28` | sưởi mô phỏng 255 |
| Node 1, Auto: `28 ≤ t < 31` | sưởi 80 |
| Node 1, Auto: `t ≥ 31` | sưởi 0 |
| Node 2, Auto: `t > 32` hoặc `h > 80` | quạt (D13) ON |
| Node 2, Auto: `soil < 20` | `PUMP:ON` trên LCD; `soil > 60` → OFF |
| Node 2: `h < 50` / `50–75` / `> 75` | NeoPixel xanh dương / vàng / đỏ |

Ngưỡng ở trên là giá trị khởi đầu, phải hiệu chỉnh theo số đo thật và ghi lý do trong báo cáo.

### 8.4 Auto / Manual

- Auto: luật cục bộ chạy; RPC `setHeater/setFan/setPump` bị từ chối.
- Manual: RPC/Web được ghi đè chấp hành; vẫn có an toàn cứng (ví dụ nhiệt độ vượt ngưỡng nguy hiểm → ép Auto và tắt sưởi).

### 8.5 Webserver (T07)

Tự hiện thực bằng `ESPAsyncWebServer` + LittleFS (**CẦN XÁC MINH** thư viện tương thích với core). Trang tối thiểu: Trang chủ (số liệu thời gian thực qua WebSocket), Điều khiển (Auto/Manual, chấp hành), Thông tin (SSID, IP, MAC, heap, uptime, phiên bản firmware), Cài đặt (WiFi, token, host, port; lưu LittleFS; khởi động lại). Nếu mất Wi-Fi: bật AP cứu hộ (`WIFI_AP_STA`), không chặn các task khác.

### 8.6 CoreIOT (T11, T12, T13)

- Hai device `Node1`, `Node2` (mỗi device 1 Access Token).
- **Dashboard (T11):** gauge nhiệt độ/độ ẩm/độ ẩm đất, line chart, hiển thị `ai_class`, bảng alarm, công tắc Auto/Manual, nút/thanh trượt gọi RPC. Dùng template CoreIOT (ghi rõ phần kế thừa).
- **Alarm rules (nền cho T12, T13):** đặt trên **device profile** (`Profile_Node1`, `Profile_Node2`): nhiệt độ cao, độ ẩm đất thấp, độ ẩm cao, `ai_class == 3`, mất dữ liệu ESP-NOW. Alarm tự tạo một lần và tự xóa khi hết vi phạm.
- **RuleChain (T12) – theo sự kiện alarm:** `RC_Node1_Ingest`: `Message Type Switch → (Post telemetry) Save Timeseries`; `(Alarm Created / Alarm Cleared) → Script → Change Originator (Node2, qua quan hệ Manages) → RPC call request (cloudAlert)`. Không kiểm tra ngưỡng từng tin nhắn (tránh bắn RPC mỗi 5 s). `RC_Node2_Command` lưu telemetry của Node 2. Xuất JSON vào `cloud/`. Chi tiết: COREIOT_SETUP.md mục 7 và 9.
- **Notification (T13):** dùng *Notification center* (Recipients → Templates → Rules, trigger **Alarm** và **Device activity**), kênh Web (chuông) và tùy chọn Mobile app. Cần ảnh chụp thông báo nhận được. Chi tiết: COREIOT_SETUP.md mục 8.

### 8.7 OTA (T14)

- Cơ chế đề xuất: OTA của ThingsBoard/CoreIOT (upload `.bin` ở mục OTA updates, gán cho device/profile; thiết bị đọc shared attributes `fw_title`, `fw_version`, `fw_size`, `fw_checksum`, `fw_checksum_algorithm` rồi tải bản mới qua MQTT chunk hoặc HTTP).
- **CẦN XÁC MINH** chính xác tên attribute/topic theo tài liệu ThingsBoard trước khi code.
- Phải có: phiên bản trước/sau hiển thị trên dashboard hoặc LCD, log tiến trình, xử lý lỗi (checksum sai, mất mạng giữa chừng không brick thiết bị), giữ bản backup firmware để nạp lại bằng cáp.
- Phương án yếu hơn (chỉ khi không làm được): `ArduinoOTA` qua LAN. Chỉ dùng như bước thử nghiệm, không thay thế T14 trong báo cáo.

## 9. Kế hoạch đo đạc (Chương 4 báo cáo, bắt buộc có số liệu thật)

| Chỉ số | Cách đo | Đầu ra |
|---|---|---|
| Độ trễ ESP-NOW | Node 1 gửi `tx_ms`, Node 2 trả `Ack`, tính RTT/2; ≥ 200 gói | CSV, trung bình, p95 |
| Tỉ lệ mất gói ESP-NOW | đếm khoảng trống `seq` theo khoảng cách (1 m, 3 m, 5 m, qua tường nếu có) | bảng |
| Độ trễ end-to-end qua CoreIOT | RPC từ dashboard → thiết bị đổi trạng thái (đo bằng video/timestamp) ≥ 20 lần | trung bình, max |
| Hiệu năng TinyML | accuracy/confusion matrix trên tập test thật; thời gian suy luận; kích thước model; tensor arena | bảng + ảnh |
| Failover mất Internet | tắt router, đo thời gian Node 2 chuyển sang luật cục bộ và duy trì chấp hành | giây |
| Thời gian OTA | từ lúc kích hoạt tới khi báo phiên bản mới | giây, kích thước `.bin` |
| Độ ổn định | chạy liên tục ≥ 2 giờ, ghi `heap` và `uptime` (có rò rỉ bộ nhớ không) | đồ thị heap |
| Mức dùng bộ nhớ | RAM/flash lúc biên dịch | bảng |

Số đo phải lấy từ kit thật. Không điền số ước lượng. Nếu không đo được thì viết rõ là không đo.

## 10. Bảng minh chứng bắt buộc theo từng mã

| Mã | Minh chứng cần có |
|---|---|
| T04 | log Serial kết quả suy luận ở từng lớp, đoạn video chuyển lớp khi thay đổi điều kiện |
| T05 | script trong `ml/`, đồ thị loss/accuracy, confusion matrix, ảnh `model.h`, kích thước model |
| T06 | log gửi/nhận cả hai node, video, số đo độ trễ và mất gói |
| T07 | ảnh từng trang web của cả hai node, video thao tác điều khiển, ảnh AP cứu hộ |
| T11 | ảnh dashboard, `dashboard.json`, ghi rõ template kế thừa |
| T12 | `rulechain_*.json`, ảnh sơ đồ RuleChain, video RuleChain kích hoạt |
| T13 | ảnh/video nhận được notification |
| T14 | video trước/sau, log OTA, ảnh phiên bản |
| N07 | ảnh docker, dashboard chạy trên server local, node trỏ về server local |

Mỗi ảnh/log lưu trong `docs/evidence/<Txx>/` kèm ghi chú ngày giờ và kịch bản.

## 11. Báo cáo (theo file mẫu BaoCaoMau_BaiTapLon_IoT.docx; nộp PDF)

Tên file: `[Mã lớp]_[Mã nhóm]_BaoCaoIoT.pdf`. Bắt buộc có **Bảng tự đánh giá yếu tố công nghệ** và **Bảng phân công công việc**.

1. Giới thiệu: vấn đề, tính năng hệ thống, tham khảo (**≥ 5 tài liệu**), phần kế thừa (OhStem Green House) và phần tự phát triển.
2. Phân tích hệ thống: phân rã chức năng, phân lớp (Cảm biến, Gateway, AI, Giao tiếp), ánh xạ tới mã T/N.
3. Hiện thực: từng phần + kết quả.
4. Đo đạc, thử nghiệm: kịch bản, số liệu, đánh giá (mục 9).
5. Kết luận, hạn chế, hướng phát triển (nêu rõ chấp hành chỉ mô phỏng).
6. Hướng dẫn sử dụng mã nguồn: cài Arduino IDE/board package, thư viện, cấu hình `secrets.h`, nạp hai node, cấu hình CoreIOT, chạy `ml/`.

Phải ghi rõ **phạm vi sử dụng công cụ AI** trong báo cáo.

Tài liệu tham khảo nên có: docs OhStem (Yolo UNO, Green House), YoloFarm, giáo trình IoT, tài liệu ThingsBoard (MQTT API, OTA, RuleChain), tài liệu ESP-NOW của Espressif, tài liệu TensorFlow Lite Micro, MQTT 3.1.1, báo cáo mẫu khóa trước (chỉ tham khảo cấu trúc, trích dẫn nếu dùng ý tưởng).

## 12. Nộp bài & mốc thời gian

- Tuần 10–11: đăng ký nhóm và tên đề tài.
- Tuần 12–13: trình bày đề cương (không bắt buộc).
- Tuần 10–15: demo trực tiếp (không bắt buộc).
- **Tuần 16: nộp trên LMS** báo cáo PDF + GitHub (public hoặc cấp quyền cho giảng viên, có `README.md`, lịch sử commit thể hiện đóng góp từng người) + **video demo 5–10 phút** (có link trong báo cáo, cho thấy từng tính năng đăng ký tính điểm hoạt động thực tế).
- Nộp trễ có thể bị trừ điểm.
- Thông báo không nói rõ có buổi vấn đáp bắt buộc không. Nên hỏi thầy (trongnhanle@hcmut.edu.vn hoặc diễn đàn LMS).

## 13. Thứ tự thực hiện (xem chi tiết milestone trong CLAUDE.md)

M0 chuẩn bị môi trường → M1 phần cứng từng ngoại vi → M2 firmware lõi (task + mutex) → M3 CoreIOT cơ bản (telemetry/RPC) → M4 dashboard + RuleChain + notification → M5 ESP-NOW → M6 Webserver → M7 TinyML (thu dữ liệu, train, chạy trên thiết bị) → M8 OTA → M9 đo đạc → M10 báo cáo, video, đóng gói. Nếu thiếu thời gian: hy sinh M8 (T14) và thay bằng T08 hoặc T10.

## 14. Phân công (điền khi chốt nhóm)

| Thành viên | MSSV | Phụ trách | Mã T/N |
|---|---|---|---|
| (điền) | | | |
| (điền) | | | |
| (điền) | | | |

Gợi ý: một người Node 1 + `ml/` (T04, T05, T06); một người Node 2 + Webserver + CoreIOT (T07, T11, T12, T13); OTA (T14) làm chung. Số người tối đa 3.
