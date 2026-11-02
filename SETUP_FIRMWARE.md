# SETUP_FIRMWARE.md – Cài môi trường, chạy từng ngoại vi (M0–M2) và Webserver (T07)

> Phần cứng: **2 kit Yolo UNO S3 (OhStem)**, DHT20, cảm biến độ ẩm đất, LCD1602 I2C.
> Đọc cùng `PROJECT.md` (kiến trúc, giao thức), `CLAUDE.md` (luật code, debug), `COREIOT_SETUP.md` (cloud).
> Nguồn: các bài 1, 3, 7, 10, 12, 14 của OhStem Wiki → *Lập Trình Arduino với Yolo UNO* (đã đọc), cộng kinh nghiệm chung về ESP32.
> Quy ước: `CẦN XÁC MINH` = chưa kiểm chứng trên kit thật. Ở mỗi bước có **kết quả mong đợi** để bạn tự biết đúng hay sai.

---

## Phần A – M0: Cài môi trường

### A1. Cài Arduino IDE

1. Tải Arduino IDE tại https://www.arduino.cc/en/software/ (Windows chọn bản *Win 10 and newer*).
2. Cài và mở lên.

### A2. Thêm board OhStem (theo bài 1 của OhStem)

1. **File → Preferences → Settings**.
2. Ô **Additional Board Manager URLs**, bấm nút mở cửa sổ nhỏ bên cạnh và dán:
   ```
   https://raw.githubusercontent.com/AITT-VN/ohstem_arduino_board/main/package_xcon_index.json
   ```
   (Nhiều URL thì mỗi URL một dòng.) **OK**.
3. **Tools → Board → Boards Manager…** (hoặc biểu tượng bo mạch ở thanh trái), gõ `ohstem`, chọn **OhStem Boards by OhStem Education** → **Install**.
4. Cắm Yolo UNO vào PC bằng cáp USB-C **truyền dữ liệu**. Trong **Tools** chọn board **Yolo UNO** (tên có thể có thêm hậu tố) và chọn **Port** (COM3, COM4… không phải COM1/COM2). Có COM port nghĩa là PC đã nhận board.
5. Ghi lại **phiên bản** "OhStem Boards" đã cài vào README.

### A3. Thêm thư viện (Library Manager: **Sketch → Include Library → Manage Libraries…**)

Cài lần lượt (gõ tên → Install). Ghi **phiên bản** vào bảng:

| Thư viện (gõ tên tìm) | Dùng cho | Cài ngay? | Phiên bản (điền) |
|---|---|---|---|
| `DHT20` (thư viện cho cảm biến DHT20, như bài 10 OhStem) | DHT20 | Có | |
| `LiquidCrystal_I2C` | LCD1602 I2C (bài 9, 10) | Có | |
| `Adafruit NeoPixel` | NeoPixel (bài 7) | Có | |
| `PubSubClient` | MQTT (bài 14) | M3 | |
| `ArduinoJson` | JSON | M3 | |
| `ESPAsyncWebServer` + `AsyncTCP` | Webserver T07 | M6 | |
| Thư viện TensorFlow Lite Micro cho ESP32 | TinyML | M7 | (chọn ở TINYML_GUIDE) |

Lưu ý:
- Có nhiều thư viện trùng tên (`DHT20`, `LiquidCrystal_I2C` của nhiều tác giả). Bài 10 OhStem dùng `#include "DHT20.h"` và `#include "LiquidCrystal_I2C.h"` với `LiquidCrystal_I2C lcd(0x21, 16, 2);`, gọi `lcd.init()`, `DHT.read()`, `DHT.getTemperature()`, `DHT.getHumidity()`. Nếu thư viện bạn cài không có hàm này thì sai thư viện: gỡ và chọn thư viện khác. **CẦN XÁC MINH** tác giả đúng bằng cách chạy bài mẫu ở mục B3.
- `ESPAsyncWebServer` thường **không có** trong Library Manager của Arduino. Cài bằng **Sketch → Include Library → Add .ZIP Library…** từ GitHub (tải ZIP từ repo `ESP32Async/ESPAsyncWebServer` và `ESP32Async/AsyncTCP`, hoặc bản fork đang được bảo trì). **CẦN XÁC MINH** bản tương thích với core của OhStem. Nếu không biên dịch được: dùng phương án dự phòng ở mục E1.

### A4. Công cụ chẩn đoán đầu tiên: in thông tin môi trường

Tạo sketch `tools/env_info` rồi nạp (để biết core arduino-esp32 là 2.x hay 3.x, ảnh hưởng chữ ký callback ESP-NOW):

```cpp
void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println("=== ENV INFO ===");
  Serial.printf("Chip: %s, cores: %d, rev: %d\n", ESP.getChipModel(), ESP.getChipCores(), ESP.getChipRevision());
  Serial.printf("CPU MHz: %u, Flash: %u bytes, PSRAM: %u bytes\n", ESP.getCpuFreqMHz(), ESP.getFlashChipSize(), ESP.getPsramSize());
  Serial.printf("SDK: %s\n", ESP.getSdkVersion());
#ifdef ESP_ARDUINO_VERSION_MAJOR
  Serial.printf("arduino-esp32 core: %d.%d.%d\n", ESP_ARDUINO_VERSION_MAJOR, ESP_ARDUINO_VERSION_MINOR, ESP_ARDUINO_VERSION_PATCH);
#endif
}
void loop() {}
```

Mở **Serial Monitor** tốc độ 115200. **Kết quả mong đợi:** thấy chip ESP32-S3, 2 core, flash/PSRAM, phiên bản core. Chép toàn bộ vào `docs/evidence/M0/env_info_node1.txt` (và `node2`).

Nếu Serial Monitor trống: thử bật **Tools → USB CDC On Boot → Enabled** (nếu có mục này) rồi nạp lại, đổi cổng, bấm RESET. **CẦN XÁC MINH** với board OhStem.

### A5. Nạp thử (blink LED D13)

```cpp
void setup() { pinMode(D13, OUTPUT); }
void loop() { digitalWrite(D13, HIGH); delay(500); digitalWrite(D13, LOW); delay(500); }
```

`D13` là macro chân của core OhStem (LED onboard). Nạp bằng nút **Upload**. **Kết quả:** LED nháy 1 s/chu kỳ. Làm cho **cả hai board**. Nếu không nạp được: giữ nút **BOOT**, nhấn **RESET**, thả BOOT rồi Upload lại; đổi cáp.

### A6. Kiểm tra quyền tài khoản CoreIOT (làm ngay, không đợi M3)

Đăng nhập https://app.coreiot.io và kiểm tra bạn **thấy và mở được** các mục: *Devices*, *Device profiles*, *Rule chains*, *Dashboards*, *Notification center*, *OTA updates*. Chụp ảnh menu → `docs/evidence/M0/coreiot_menu.png`. Nếu thiếu mục nào: ghi lại và hỏi thầy sớm (T12, T13, T14 phụ thuộc vào đó).

### A7. Tạo repo

1. Tạo repo GitHub (public hoặc cấp quyền cho giảng viên) tên `smart-greenhouse`, có `README.md`, `.gitignore`.
2. `.gitignore` tối thiểu:
   ```
   secrets.h
   *.bin
   build/
   __pycache__/
   venv/
   .DS_Store
   ```
3. Cấu trúc thư mục theo `PROJECT.md` mục 6. Mỗi thành viên **commit bằng tài khoản của mình**.

### A8. Checklist M0

- [ ] Arduino IDE + OhStem Boards cài xong, ghi phiên bản.
- [ ] Hai board đều nạp được và LED D13 nháy.
- [ ] `env_info` của cả hai board đã lưu.
- [ ] Danh sách thư viện đã cài kèm phiên bản.
- [ ] Đã kiểm tra menu CoreIOT.
- [ ] Repo đã tạo.

---

## Phần B – M1: Chạy từng ngoại vi riêng lẻ

Nguyên tắc: **một ngoại vi một sketch**, xong mới ghép. Mỗi sketch lưu vào `tools/` hoặc `docs/evidence/M1/` để dùng làm minh chứng và công cụ debug.

### B1. Sơ đồ cắm (Grove)

Yolo UNO có 12 cổng Grove (4 analog, 4 digital, 4 I2C). Theo ảnh board trong báo cáo mẫu và chữ in trên board: hàng **đỏ** là cổng I2C (`I2C1`…`I2C4`), hàng **vàng** là cổng analog (`A0-A1`, `A2-A3`…), hàng **trắng** là cổng digital (`D3-D4`…). Đối chiếu với chữ in trên board của bạn.

| Node | Thiết bị | Cắm vào |
|---|---|---|
| Node 1 | DHT20 | một cổng I2C (đỏ) |
| Node 1 | LCD1602 I2C | một cổng I2C khác (đỏ), cùng bus với DHT20 |
| Node 1 | Cảm biến độ ẩm đất | cổng analog chứa **A1** (vàng, ví dụ `A1-A0`). Bài 12 OhStem dùng `A1` |
| Node 2 | LCD1602 I2C | một cổng I2C (đỏ) |
| Node 2 | DHT20 thứ hai (tùy chọn) | cổng I2C còn lại |
| Cả hai | NeoPixel, LED D13 | có sẵn trên board, không cần dây |

Bốn cổng I2C cùng chung một bus nên cắm bất kỳ. Hai thiết bị I2C khác địa chỉ nên không xung đột. Cắm/rút dây khi **đã rút USB** cho an toàn.

### B2. I2C scan (tìm địa chỉ thật của LCD và DHT20)

Sketch `tools/i2c_scan`:

```cpp
#include <Wire.h>
void setup() {
  Serial.begin(115200);
  delay(1500);
  Wire.begin();                       // nếu không thấy thiết bị nào, thử Wire.begin(11, 12)  (CẦN XÁC MINH chân)
  Serial.println("Scanning I2C...");
  int found = 0;
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) { Serial.printf("Found device at 0x%02X\n", a); found++; }
  }
  Serial.printf("Done, %d device(s)\n", found);
}
void loop() {}
```

**Kết quả mong đợi:** thấy **DHT20 ở `0x38`** (địa chỉ cố định của DHT20) và **LCD ở `0x21`** theo bài 10 OhStem (hoặc `0x27`/`0x3F` nếu module khác). Ghi địa chỉ thật vào `config.h`. Không thấy gì: kiểm tra dây Grove (lắp đúng chiều, chắc khớp), thử `Wire.begin(11, 12)`, đổi cổng I2C khác.
Lưu `docs/evidence/M1/i2c_scan_node1.txt`, `node2`.

### B3. LCD + DHT20 theo bài mẫu OhStem (xác minh thư viện)

Đây là bài 10 của OhStem. Chạy nguyên bản để chắc thư viện đúng:

```cpp
#include "LiquidCrystal_I2C.h"
#include "DHT20.h"
#include <Arduino.h>

LiquidCrystal_I2C lcd(0x21, 16, 2);   // đổi nếu scan ra địa chỉ khác
DHT20 DHT;

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
}

void loop() {
  DHT.read();
  Serial.printf("T=%.2f H=%.2f\n", DHT.getTemperature(), DHT.getHumidity());
  lcd.setCursor(0, 0); lcd.printf("T:%.1f H:%.1f  ", DHT.getTemperature(), DHT.getHumidity());
  delay(2000);
}
```

(Khác bài mẫu: không dùng `lcd.clear()` mỗi vòng, theo luật của dự án.)

**Kết quả mong đợi:** LCD hiện nhiệt độ ~ nhiệt độ phòng và độ ẩm hợp lý (30–90 %); Serial in đều mỗi 2 s. Thử đặt tay gần cảm biến: nhiệt độ tăng dần, độ ẩm đổi.
Sự cố: LCD sáng nhưng không có chữ → chỉnh biến trở tương phản phía sau; chữ khối đen → sai địa chỉ; `T=0`/`NaN` → DHT20 chưa sẵn sàng, đợi ≥ 1 s, kiểm tra dây; không biên dịch → sai thư viện.
Lưu ảnh LCD + log Serial → `docs/evidence/M1/lcd_dht20_node1.jpg`, `log_dht20.txt`.

### B4. Cảm biến độ ẩm đất (A1) và hiệu chỉnh

Bước 1: in giá trị thô, sketch `tools/adc_calibrate`:

```cpp
void setup() { Serial.begin(115200); }
void loop() {
  long sum = 0;
  for (int i = 0; i < 16; i++) { sum += analogRead(A1); delay(10); }
  Serial.printf("raw avg = %ld\n", sum / 16);
  delay(500);
}
```

Bước 2: đo **hai điểm** và ghi vào bảng (mỗi điểm đo ≥ 10 lần rồi lấy trung bình):

| Điều kiện | Giá trị thô trung bình |
|---|---|
| **Khô** (cảm biến để trong không khí, lau khô) | `RAW_DRY = ______` |
| **Ướt** (nhúng phần đầu dò vào cốc nước, đúng vạch tối đa của nhà sản xuất, không ngập cả mạch) | `RAW_WET = ______` |
| Đất ẩm vừa (chậu thật) | tham khảo |

Bước 3: công thức quy đổi (đảo chiều tùy cảm biến: thường khô → giá trị lớn):

```
soil% = (RAW_DRY - raw) * 100 / (RAW_DRY - RAW_WET)   // kẹp vào 0..100
```

Ghi `RAW_DRY`, `RAW_WET` vào `config.h`. **Lưu ý:** bài 12 OhStem quy đổi bằng `analogRead(A1)/10.24` (giả định dải 0–1023). ESP32-S3 gốc đọc 12-bit (0–4095). Nếu số thô của bạn vượt 1023 thì công thức đó sai, dùng hiệu chỉnh hai điểm như trên. **CẦN XÁC MINH** bằng số đo thật. Ghi vào báo cáo đã hiệu chỉnh thế nào.
Lưu `docs/evidence/M1/soil_calibration.csv` (các lần đo) và ảnh thiết lập.

### B5. NeoPixel onboard

Sketch thử (chân thử **45**, theo báo cáo mẫu; **CẦN XÁC MINH**):

```cpp
#include <Adafruit_NeoPixel.h>
#define NEO_PIN 45
Adafruit_NeoPixel neo(1, NEO_PIN, NEO_GRB + NEO_KHZ800);
void setup() { neo.begin(); neo.setBrightness(40); }
void loop() {
  uint32_t c[] = { neo.Color(255,0,0), neo.Color(0,255,0), neo.Color(0,0,255), neo.Color(255,180,0) };
  for (int i = 0; i < 4; i++) { neo.setPixelColor(0, c[i]); neo.show(); delay(700); }
}
```

**Kết quả mong đợi:** LED RGB onboard lần lượt đỏ, xanh lá, xanh dương, vàng.
Không sáng: sai chân (tra sơ đồ chân của Yolo UNO trên OhStem hoặc macro trong core: tìm trong thư mục board của OhStem file `pins_arduino.h`), hoặc thứ tự màu sai (đổi `NEO_GRB` ↔ `NEO_RGB`).
Phân biệt: bài 7 của OhStem dùng **module LED RGB 4 bóng gắn ngoài** ở cổng `D3`, đó **không phải** NeoPixel onboard. Dự án dùng NeoPixel onboard. Ghi chân đã xác nhận vào `config.h` và README. Lưu video/ảnh.

### B6. Kiểm tra Node 2

Lặp B2, B3 (LCD, và DHT20 nếu có), B5 cho Node 2. LED D13 đã làm ở A5.

### B7. Checklist M1

- [ ] `i2c_scan` cả hai board, ghi địa chỉ LCD/DHT20 thật.
- [ ] LCD + DHT20 chạy, không dùng `lcd.clear()` mỗi vòng.
- [ ] `RAW_DRY`, `RAW_WET` đã đo và ghi lại.
- [ ] NeoPixel đổi màu đúng, chân đã xác nhận.
- [ ] Ảnh/log đã lưu `docs/evidence/M1/`.

---

## Phần C – M2: Firmware lõi (FreeRTOS + dữ liệu dùng chung)

Mục tiêu: Node 1 chạy ổn định ≥ 30 phút với các task đọc cảm biến, hiển thị, NeoPixel; **chưa** cần mạng. Node 2 chạy khung tương tự (hiển thị, NeoPixel, D13).

### C1. Cấu trúc file (Node 1)

```
node1_sensor/
├─ node1_sensor.ino      # setup(): khởi tạo, tạo task; loop(): vTaskDelay(portMAX_DELAY)
├─ config.h              # CHÂN, địa chỉ I2C, RAW_DRY/RAW_WET, ngưỡng, thời gian
├─ secrets.h.example     # WIFI/TOKEN mẫu (secrets.h bị .gitignore)
├─ shared_data.h/.cpp    # biến chung + mutex + getter/setter
└─ task_sensor.cpp  task_display.cpp  task_led.cpp   # M2 chỉ cần 3 task này
```

### C2. Quy tắc phải có (theo CLAUDE.md mục 4)

- Mỗi chức năng một Task; `loop()` chỉ `vTaskDelay(portMAX_DELAY)`.
- **Không biến toàn cục tự do.** Mọi truy cập dữ liệu chung qua getter/setter có mutex.
- Giữ mutex ngắn; không I2C/Serial/mạng khi đang giữ.
- Không `delay()` trong task, dùng `vTaskDelay`.
- LCD không `clear()` mỗi vòng.
- Hằng số tập trung trong `config.h`.

### C3. Việc của từng task (M2)

| Task | Chu kỳ | Việc |
|---|---|---|
| `sensor` | 2 s | đọc DHT20 (kiểm tra trả về), đọc A1 trung bình 8–16 mẫu, quy đổi `soil%`, ghi `shared_data`, kẹp giá trị |
| `display` | 500 ms | đọc `shared_data` (copy ra biến cục bộ), in 2 dòng LCD ghi đè |
| `led` | khi dữ liệu đổi | NeoPixel Node 1 = sưởi mô phỏng theo luật Auto (PROJECT.md 8.3) |

Log Serial có tiền tố `[SENSOR]`, `[LCD]`, `[LED]`, và mỗi 10 s in `heap` + `uxTaskGetStackHighWaterMark` của từng task.

### C4. Thử nghiệm chấp nhận M2

1. Chạy ≥ 30 phút không reset, `heap` không giảm đều (ghi mỗi phút vào `docs/measurements/heap_node1.csv`).
2. Đổi điều kiện (làm ấm DHT20, nhúng cảm biến đất vào nước): LCD và Serial cập nhật đúng; NeoPixel đổi theo luật.
3. Rút dây DHT20 khi chạy: firmware **không treo**, in `[SENSOR] DHT20 read failed`, giữ giá trị cũ hoặc đánh dấu lỗi, hồi phục khi cắm lại.
4. Stack high-water mark của mọi task còn dư ≥ 20 %.
5. Lưu: log Serial 30 phút, CSV heap, ảnh LCD.

### C5. Node 2 (M2 bản khung)

Task `display` (LCD), `led` (NeoPixel + D13) với dữ liệu giả cứng lập trình sẵn để test hiển thị. Nhận dữ liệu thật ở M5 (ESP-NOW).

### C6. Prompt mẫu giao cho Claude (M2)

> Đọc `PROJECT.md` và `CLAUDE.md`. Làm **M2** cho Node 1. Trạng thái: đã xong M0, M1. Địa chỉ LCD = `0x__`, DHT20 = `0x38`, `RAW_DRY` = `__`, `RAW_WET` = `__`, chân NeoPixel = `__` (đã xác nhận). Chỉ làm đúng M2: viết `config.h`, `shared_data.h/.cpp`, `task_sensor`, `task_display`, `task_led`, `node1_sensor.ino`. Không thêm mạng. Nêu kế hoạch ngắn trước, đưa đầy đủ từng file, cách nạp, cách kiểm tra theo mục C4 SETUP_FIRMWARE.md và minh chứng cần lưu.

### C7. Checklist M2

- [ ] Node 1 chạy 30 phút ổn định, CSV heap đã lưu.
- [ ] Rút/cắm DHT20 không làm treo.
- [ ] Stack dư ≥ 20 %.
- [ ] Không còn biến toàn cục tự do ngoài `shared_data`.
- [ ] Node 2 khung hiển thị chạy với dữ liệu giả.

---

## Phần D – Chuẩn bị mạng cho các milestone sau (làm sớm, không tốn nhiều thời gian)

1. **Wi-Fi 2.4 GHz** ổn định (router nhà hoặc điện thoại phát). Tên mạng không dấu, mật khẩu không ký tự lạ càng tốt khi test.
2. **MAC** hai board: nạp sketch in `WiFi.macAddress()` (cần `WiFi.mode(WIFI_STA)` trước) rồi dán nhãn lên board (Node 1 / Node 2). Lưu vào `docs/evidence/M0/mac.txt`. (Không lộ MAC công khai nếu không cần.)
3. **MQTT test từ PC** (xem `COREIOT_SETUP.md` mục 4) để chắc token đúng trước khi nạp firmware mạng.

---

## Phần E – T07: Webserver trên thiết bị (làm ở M6)

### E1. Chọn thư viện

| Phương án | Ưu | Nhược |
|---|---|---|
| **A (khuyến nghị): `ESPAsyncWebServer` + `AsyncTCP`** | Không chặn task, hỗ trợ WebSocket | Phải cài từ ZIP; cần đúng bản tương thích core (**CẦN XÁC MINH**) |
| **B (dự phòng): `WebServer` có sẵn trong core ESP32 + REST + polling** | Không cần thư viện ngoài, ổn định | Đồng bộ (chạy `server.handleClient()` trong một task riêng); không WebSocket |

Nếu A không biên dịch được sau 30 phút thử, **chuyển sang B**. T07 chấm "tự hiện thực, hiển thị và điều khiển local", cả hai đều đạt.

### E2. Nạp giao diện web: **nhúng vào mã nguồn** (khuyến nghị)

Cách chắc chắn nhất: giữ file HTML/CSS/JS trong thư mục `web/` rồi **nhúng thành chuỗi `PROGMEM`** (`index_html.h`) bằng script nhỏ trên PC, hoặc viết tay. Ưu điểm: không cần plugin "LittleFS data upload" (hay lỗi với Arduino IDE 2 và core ESP32 mới).
**LittleFS vẫn dùng để lưu cấu hình** (file JSON `config.json` ghi lúc chạy bằng `LittleFS.begin(true)` + `File`), không cần upload dữ liệu trước.

### E3. Trang và API cần có (mỗi node)

| Trang | Nội dung |
|---|---|
| `/` (Trang chủ) | Nhiệt độ, độ ẩm, độ ẩm đất, nhãn AI (Node 1), trạng thái chấp hành; cập nhật mỗi 2 s |
| `/control` | Auto/Manual; Node 1: thanh trượt sưởi 0–255; Node 2: công tắc quạt, bơm (mô phỏng); khóa điều khiển khi Auto |
| `/info` | SSID, IP, MAC, RSSI, heap, uptime, phiên bản firmware, trạng thái MQTT/ESP-NOW |
| `/settings` | WiFi SSID/mật khẩu, host/cổng/token CoreIOT; nút Lưu; che token/mật khẩu khi hiển thị |

Endpoint gợi ý (JSON, trả nhanh, không chặn):

| Method | Đường dẫn | Việc |
|---|---|---|
| GET | `/api/state` | trả trạng thái hiện tại (đọc `shared_data` bằng getter) |
| POST | `/api/mode` | `{"auto":true/false}` |
| POST | `/api/heater` | `{"value":0..255}` (Node 1, chỉ Manual) |
| POST | `/api/fan`, `/api/pump` | `{"on":true/false}` (Node 2, chỉ Manual) |
| POST | `/api/config` | lưu cấu hình vào LittleFS, trả OK rồi **trì hoãn ~1.5 s trước khi `ESP.restart()`** |

Luật: handler **không** chờ I/O chậm, chỉ đọc/ghi `shared_data` qua getter/setter có mutex rồi trả lời ngay. Lệnh bị từ chối (đang Auto) trả `{"ok":false,"reason":"auto_mode"}` giống RPC.

### E4. AP cứu hộ (khi mất Wi-Fi hoặc chưa cấu hình)

1. Chế độ `WIFI_AP_STA`. Nếu không có cấu hình hoặc không kết nối được STA sau thời gian chờ (ví dụ 20 s), bật AP tên `GREENHOUSE-N1-SETUP` (Node 2: `-N2-`), IP mặc định `192.168.4.1`.
2. Khi **có máy kết nối vào AP** (`WiFi.softAPgetStationNum() > 0`), **tạm dừng** quét/thử kết nối lại để không làm rớt AP khi người dùng đang nhập.
3. Khi không còn máy nào, cứ 30 s thử lại STA; kết nối được thì tắt AP.
4. Mật khẩu AP đặt riêng (không để AP mở nếu có thể).
5. Việc kết nối lại/quét mạng chạy trong task riêng, **không** chặn task khác.

### E5. Bảo mật tối thiểu

- Không in token/mật khẩu đầy đủ lên web hoặc Serial; hiển thị dạng `abcd****`.
- Endpoint đổi cấu hình yêu cầu thao tác xác nhận (nút Lưu rõ ràng); thêm mật khẩu quản trị đơn giản nếu kịp.
- Không để lộ `/api/config` trả về mật khẩu.

### E6. Thử nghiệm chấp nhận T07

1. Mở `http://<IP node>/` từ điện thoại và laptop cùng mạng: thấy dữ liệu cập nhật.
2. Gạt Manual, kéo thanh trượt/bật công tắc: chấp hành đổi ngay (NeoPixel/D13), LCD hiện `MAN`.
3. Ở Auto: điều khiển bị khóa/từ chối.
4. Mở nhiều tab/thiết bị cùng lúc: firmware **không** treo, các task khác vẫn chạy (xem log và LCD).
5. Đổi cấu hình WiFi qua `/settings` → thiết bị khởi động lại và vào mạng mới.
6. Tắt router: sau thời gian chờ, thiết bị phát AP; kết nối điện thoại vào AP, mở `192.168.4.1`, nhập lại WiFi.
7. Đo: độ trễ phản hồi điều khiển (DevTools → Network, lấy thời gian của request `/api/heater`; ≥ 20 lần) và thời gian từ mất Wi-Fi đến lúc AP lên. Ghi `docs/measurements/web_latency.csv`.
8. Minh chứng: ảnh từng trang của **cả hai node**, video thao tác điều khiển + AP cứu hộ → `docs/evidence/T07/`.

### E7. Prompt mẫu giao cho Claude (M6)

> Đọc `PROJECT.md` và `CLAUDE.md`. Làm **M6 (T07 Webserver)** cho Node `<1|2>`. Đã xong M0–M5. Dùng thư viện `<ESPAsyncWebServer+AsyncTCP | WebServer có sẵn>` (đã cài/đã thử). Giao diện nhúng PROGMEM, cấu hình lưu LittleFS. Làm đúng các trang và API ở mục E3–E5 của SETUP_FIRMWARE.md, tuân thủ mutex và không chặn task. Nêu kế hoạch ngắn, đưa đầy đủ từng file, cách nạp, cách kiểm tra theo mục E6, minh chứng cần lưu.

### E8. Checklist T07

- [ ] 4 trang chạy trên **cả hai node**.
- [ ] Điều khiển Auto/Manual đúng luật.
- [ ] AP cứu hộ bật/tắt đúng, không làm treo task khác.
- [ ] Cấu hình lưu LittleFS và áp dụng sau khi khởi động lại.
- [ ] Token/mật khẩu không lộ.
- [ ] Số đo độ trễ và thời gian AP đã ghi.
- [ ] Ảnh + video đã lưu.

---

## Phần F – Lỗi thường gặp (xem thêm CLAUDE.md mục 6)

| Triệu chứng | Hướng xử lý |
|---|---|
| Không thấy COM port | Đổi cáp (cáp sạc không có dữ liệu), cổng USB khác, cài driver USB-serial nếu Windows yêu cầu |
| Upload báo lỗi kết nối | Giữ BOOT → nhấn RESET → thả BOOT → Upload; đóng Serial Monitor |
| `fatal error: ... .h: No such file` | Chưa cài thư viện hoặc sai tên; cài lại qua Library Manager |
| Biên dịch OK nhưng LCD không hiện | Sai địa chỉ I2C; chỉnh tương phản |
| DHT20 đọc ra 0 | Đợi ≥ 1 s sau cấp nguồn; kiểm tra dây; kiểm tra thư viện đúng |
| Độ ẩm đất nhảy | Lấy trung bình nhiều mẫu; kiểm tra dây; cảm biến không chạm nhau |
| NeoPixel không sáng | Chân sai/thứ tự màu; tăng `setBrightness` |
| Board reset liên tục | Xem log backtrace; tăng stack; thêm `vTaskDelay`; tránh giữ mutex lâu |

---

## Phần G – Tổng hợp minh chứng giai đoạn này

| Thư mục | File |
|---|---|
| `docs/evidence/M0/` | `env_info_node1.txt`, `env_info_node2.txt`, `coreiot_menu.png`, `mac.txt`, ảnh cài Arduino/board |
| `docs/evidence/M1/` | `i2c_scan_node1.txt`, `i2c_scan_node2.txt`, `lcd_dht20_node1.jpg`, `log_dht20.txt`, `soil_calibration.csv`, ảnh NeoPixel |
| `docs/measurements/` | `heap_node1.csv`, `web_latency.csv` |
| `docs/evidence/T07/` | ảnh các trang web hai node, video điều khiển + AP cứu hộ |

---

## Tài liệu tham khảo cho phần này

- OhStem Education Wiki – Lập Trình Arduino với Yolo UNO: bài 1 (cài đặt), 3 (thêm thư viện), 7 (LED RGB), 9–10 (LCD1602, DHT20), 12 (Vườn thông minh), 14 (IoT): https://docs.ohstem.vn/en/latest/yolo_uno/arduino_yolo_uno.html
- Arduino IDE: https://www.arduino.cc/en/software/
- Tài liệu Espressif ESP32-S3, ESP-IDF/Arduino core (WiFi, FreeRTOS, LittleFS).
