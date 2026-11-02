# CLAUDE.md – Luật làm việc cho Claude trong dự án Smart Greenhouse (BTL IoT)

> Đọc `PROJECT.md` trước mọi việc. File này quy định **cách làm**: quy trình, luật code, luật debug, tiêu chí hoàn thành, những điều cấm.
> Ngôn ngữ giao tiếp với người dùng: **tiếng Việt**. Tên biến, hàm, file, commit: tiếng Anh. Comment trong code: tiếng Việt ngắn gọn, chỉ giải thích "vì sao".

---

## 0. Bối cảnh trong 10 dòng

- Bài tập lớn IoT (HCMUT, HK261), nhóm ≤ 3 người, nền tảng **CoreIOT + Yolo UNO S3 (ESP32-S3) của OhStem**.
- Có **2 board**, 2 DHT20, 1 cảm biến độ ẩm đất (A1), 2 LCD1602 I2C, NeoPixel + LED D13 onboard.
- **KHÔNG có**: bơm, quạt, cảm biến ánh sáng. Chấp hành là **mô phỏng**. Không được viết code giả định có các linh kiện này. Không dùng Yolo:Bit.
- Tổ hợp điểm: T04, T05, T06, T07, T11, T12, T13, T14 (+ N07 tùy chọn). Dự phòng: T08/T10 nếu OTA hỏng.
- Điểm chỉ tính khi **có minh chứng** (ảnh, log, video, mã). Mọi tính năng phải để lại dấu vết kiểm chứng được.
- Môi trường chính: **Arduino IDE** + "OhStem Boards" (có thể chuyển sang PlatformIO nếu người dùng yêu cầu).
- Người dùng muốn **câu trả lời ngắn gọn, thực tế, có thể copy chạy ngay**, xử lý trọn vẹn thay vì liệt kê lựa chọn.

## 1. Chế độ làm việc (quan trọng nhất)

1. **Chỉ làm đúng milestone/việc người dùng giao.** Người dùng đã nói rõ: *chưa có lệnh thì đừng tự ý code*. Không tự mở rộng sang tính năng, file, hay milestone khác. Có ý tưởng hay thì ghi một dòng đề xuất ở cuối, không làm.
2. Trước khi code: nêu **kế hoạch 3–6 dòng** (file nào đổi, kết quả mong đợi, cách kiểm tra). Việc nhỏ (sửa 1 lỗi) thì làm luôn, không cần kế hoạch dài.
3. Sau khi code: nêu **cách nạp, cách kiểm tra, kết quả mong đợi trên Serial/LCD/dashboard**, và **minh chứng cần lưu** (mục 8).
4. Một lần chỉ làm **một thay đổi có thể kiểm tra được**. Không dồn nhiều thay đổi chưa chạy thử vào một lượt.
5. Không bao giờ nói "đã chạy tốt" khi chưa chạy. Kết quả trên board chỉ được coi là **đã kiểm chứng** khi (a) người dùng dán log thật, hoặc (b) chính Claude đã chạy được vòng build → nạp → bắt log Serial trên máy người dùng (nếu đã cài `arduino-cli`, board đang cắm và có các script trong `tools/`) và đưa log làm bằng chứng. Ngoài hai trường hợp đó, mọi kết quả là *dự kiến*; ghi rõ "chưa kiểm chứng trên kit". Không bao giờ tự nạp OTA hoặc ghi đè firmware ổn định khi chưa được người dùng cho phép (nguy cơ làm hỏng board).
6. Gặp thông tin `CẦN XÁC MINH` (PROJECT.md mục 4.4): **hỏi/đối chiếu trước**, không đoán. Đặt `TODO(verify): ...` trong code và nêu rõ trong câu trả lời.
7. Nếu yêu cầu mâu thuẫn với luật chấm hoặc phần cứng (ví dụ yêu cầu dùng bơm thật), **nói ngay** và đề xuất phương án mô phỏng, không âm thầm làm khác.
8. Khi người dùng dán log/lỗi: đọc kỹ log, chỉ ra **dòng/nguyên nhân cụ thể** rồi mới sửa. Không sửa mò.
9. **Tài liệu bổ trợ có thể chưa được đưa.** Các file `COREIOT_SETUP.md`, `SETUP_FIRMWARE.md`, `TINYML_GUIDE.md` được nhắc trong tài liệu này. Nếu milestone đang làm cần file nào mà bạn **không có trong ngữ cảnh**, hãy nói rõ ngay ở đầu câu trả lời: "Milestone này cần `<tên file>` mà tôi chưa có", rồi **hỏi người dùng đưa file đó**. Không tự bịa nội dung file, không đoán tên menu CoreIOT, tên topic OTA, API thư viện hay quy trình thu dữ liệu. Nếu người dùng vẫn muốn làm tiếp khi thiếu file, chỉ làm phần chắc chắn từ `PROJECT.md` và `CLAUDE.md`, đánh dấu mọi chỗ còn lại là `CẦN XÁC MINH`.
   Bảng file cần đưa theo milestone: M0–M2 và M6 → `SETUP_FIRMWARE.md`; M3, M4, M8 → `COREIOT_SETUP.md`; M7 → `TINYML_GUIDE.md`; M5 chỉ cần `PROJECT.md` + `CLAUDE.md`; M9–M10 → thông báo và báo cáo mẫu của thầy.

## 2. Luật nguồn sự thật

- **Yêu cầu điểm:** `ThongBao_BaiTapLon_IoT.docx` (đã tóm tắt trong PROJECT.md mục 3). Mã T/N và điểm không được tự ý thay đổi.
- **Giao thức dữ liệu (tên khóa telemetry, tên RPC, gói ESP-NOW):** `PROJECT.md` mục 7 là nguồn sự thật. Sửa giao thức ở đâu thì phải cập nhật **cùng lúc** `common/protocol.h`, firmware hai node, dashboard, RuleChain và PROJECT.md.
- **Cấu hình CoreIOT, RuleChain, OTA phía server:** `COREIOT_SETUP.md`. Khi làm M3, M4, M8 phải đọc file này trước và làm đúng tên device/profile/rule chain trong đó.
- **Cài môi trường, chạy từng ngoại vi, firmware lõi, Webserver:** `SETUP_FIRMWARE.md` (M0–M2, M6).
- **TinyML (thu dữ liệu, train, export, chạy trên thiết bị):** `TINYML_GUIDE.md` (M7). Chuẩn hóa và quy tắc sentinel ở mục 2.2–2.3 của file đó là nguồn sự thật.
- **Hướng dẫn kit:** docs.ohstem.vn (Yolo UNO → Lập trình Arduino với Yolo UNO, Green House). Khi cần chi tiết chân/thư viện của kit, **đọc lại docs**, không dựa trí nhớ.
- **Báo cáo mẫu khóa trước (BTL_IoT_TranMinhDuong…pdf):** chỉ để tham khảo ý tưởng. **Cấm sao chép** câu chữ, sơ đồ, đoạn code. Thông báo xử lý nghiêm sao chép giữa các nhóm. Nếu dùng ý tưởng (ví dụ `Change Originator` trong RuleChain) thì ghi vào phần trích dẫn.
- **Các lỗi đã biết trong báo cáo mẫu, tránh lặp lại:** gọi nhầm cảm biến (DHT20 vs SHT30), mô tả ngưỡng không khớp code, tuyên bố QoS 1 trong khi `PubSubClient` chỉ publish QoS 0, F1=1.00 trên dữ liệu tổng hợp, số liệu "90% giảm lưu lượng", "<450 ms" không có kịch bản đo. Báo cáo của nhóm phải nhất quán và có số đo thật.
- Không bịa số liệu, URL, tên API, tên thư viện, tên attribute. Không chắc → ghi `CẦN XÁC MINH`.

## 3. Quy ước repo & Git

- Cấu trúc theo `PROJECT.md` mục 6. Không tạo thư mục/tệp ngoài cấu trúc nếu chưa nói rõ lý do.
- **Không commit bí mật:** `secrets.h` (WiFi, token) nằm trong `.gitignore`; chỉ commit `secrets.h.example`. Không dán token thật vào code, ví dụ, log, hay câu trả lời.
- Commit nhỏ, thường xuyên, theo mẫu `type(scope): nội dung`, ví dụ `feat(node1): read DHT20 and soil`, `fix(espnow): recv callback signature for core 3.x`, `docs(evidence): add T06 latency log`. Lịch sử commit phải thể hiện đóng góp từng người (người dùng tự commit bằng tài khoản của mình, Claude chỉ soạn nội dung).
- Nhánh: `main` luôn nạp được. Làm việc trên `feature/<tên>` rồi merge.
- Không đưa file `.bin` lớn, dataset thô > vài MB, hay thư mục build vào repo (dùng `.gitignore`, hoặc Releases).
- README.md hướng dẫn **từ máy trống**: cài IDE, board package, thư viện (kèm **phiên bản**), cấu hình secrets, nạp từng node, cấu hình CoreIOT, chạy `ml/`. Cập nhật README cùng lúc với mỗi thay đổi ảnh hưởng cách chạy.

## 4. Luật code firmware (ESP32-S3, Arduino core, FreeRTOS)

### 4.1 Kiến trúc

1. Mỗi chức năng là **một Task FreeRTOS** riêng (xem bảng task trong PROJECT.md). Hàm `loop()` chỉ `vTaskDelay(portMAX_DELAY)`.
2. **Không dùng biến toàn cục tự do.** Dữ liệu chung nằm trong `shared_data.cpp`, truy cập qua getter/setter bọc mutex. Biến chỉ đọc/ghi từ một task thì để `static` trong file task.
3. Mutex: lấy → làm việc **cực ngắn** → trả. Không gọi I/O chậm (I2C, mạng, `Serial.print` dài, `delay`) khi đang giữ mutex. Copy dữ liệu ra biến cục bộ rồi mới dùng.
4. Tránh deadlock: mỗi đoạn code chỉ giữ **một mutex một lúc**, hoặc thống nhất thứ tự lấy khóa nếu buộc phải giữ nhiều.
5. Giao tiếp giữa task: `Queue` (dữ liệu), `Binary Semaphore`/`Event Group` (cờ sự kiện). Không polling vòng lặp bận.
6. **Callback** (ESP-NOW, MQTT) chạy trong ngữ cảnh khác: chỉ copy dữ liệu và `xQueueSend(..., 0)`/set cờ, không `delay`, không `Serial` dài, không gọi mạng blocking.
7. Mọi vòng lặp vô hạn trong task phải có `vTaskDelay(...)` (hoặc chờ queue/semaphore có timeout) để không chiếm CPU và không kích hoạt watchdog.
8. Stack task: bắt đầu 4096 byte (MQTT/Web/TinyML có thể 8192+). Dùng `uxTaskGetStackHighWaterMark()` log định kỳ khi debug, chốt giá trị có dư 20–30%.
9. Ưu tiên task: `control/espnow_rx` (3) > `mqtt/sensor/web` (2) > `lcd/led` (1). Không để task hiển thị chặn task điều khiển.

### 4.2 Bộ nhớ

- Cấm `String` nối chuỗi trong vòng lặp thường xuyên (phân mảnh heap). Dùng `char buf[N]` + `snprintf`, hoặc `ArduinoJson` với `JsonDocument` cục bộ (v7) và kích thước hợp lý.
- Cấm `malloc/new` trong vòng lặp nóng. Cấp phát một lần lúc khởi tạo.
- Log `ESP.getFreeHeap()` và `ESP.getMinFreeHeap()` trong telemetry (`heap`) để chứng minh không rò rỉ.
- Tensor arena TinyML: mảng tĩnh `alignas(16)`, kích thước ghi hằng số và log ra khi khởi tạo.

### 4.3 Ngoại vi

- I2C chung bus cho DHT20 và LCD: khởi tạo `Wire.begin()` **một lần**, truy cập I2C chỉ từ task được phân công; nếu hai task cùng đụng I2C thì bọc bằng một mutex I2C riêng.
- LCD: **không dùng `lcd.clear()` trong vòng lặp**. Dùng `setCursor` và ghi đè chuỗi có độ dài cố định (đệm khoảng trắng). Địa chỉ I2C và kích thước nằm trong `config.h` (mặc định `0x21` theo docs OhStem, phải xác nhận bằng I2C scan).
- NeoPixel: chỉ cập nhật khi giá trị đổi (tránh `show()` liên tục). Chân lấy từ `config.h`. **Không hard-code GPIO 45** khi chưa xác minh.
- Độ ẩm đất: hiệu chỉnh bằng hai điểm (khô/ướt) lưu trong `config.h`, quy đổi % theo công thức tuyến tính có kẹp 0–100. Không dùng cứng `/10.24` nếu ADC đọc ra dải khác.
- Chân dùng macro của core OhStem (`D13`, `A1`…). Không dùng số GPIO thô trừ khi macro không có (khi đó ghi chú vì sao).

### 4.4 Mạng

- **Wi-Fi:** chỉ 2.4 GHz. Kết nối lại có backoff (3 s, rồi 5 s, 10 s…), không `while(WiFi.status()!=WL_CONNECTED)` chặn vô hạn.
- **MQTT (PubSubClient):**
  - `mqtt.setBufferSize(512)` (mặc định 256 byte dễ làm rớt gói lớn).
  - `mqtt.loop()` phải được gọi đều (chu kỳ ≤ 100 ms), `keepAlive` hợp lý.
  - Username = Access Token, password rỗng. Topic: `v1/devices/me/telemetry`, `v1/devices/me/rpc/request/+`, phản hồi `v1/devices/me/rpc/response/<id>`.
  - `PubSubClient` chỉ publish QoS 0. **Không tuyên bố QoS 1** trong báo cáo trừ khi thực sự dùng thư viện hỗ trợ.
  - Log mã lỗi `mqtt.state()` khi kết nối thất bại để debug (bảng ở mục 6.3).
  - Callback RPC phải xử lý đúng các method trong `PROJECT.md` mục 7.3, **kể cả `cloudAlert` ở Node 2**: chỉ lưu vào `shared_data` (mutex) rồi để task display/NeoPixel xử lý, trả lời two-way RPC `{"ok":true}` hoặc `{"ok":false,"reason":"..."}`. Phương thức lạ: trả `{"ok":false,"reason":"unknown_method"}`, không bỏ qua im lặng.
- **ESP-NOW:**
  - Cả hai node `WiFi.mode(WIFI_STA)` trước `esp_now_init()`.
  - Hai node phải **cùng kênh**: khi cùng join một router thì tự khớp; nếu không phải đặt kênh cố định.
  - Chữ ký callback nhận khác nhau theo core: arduino-esp32 **3.x** `void(const esp_now_recv_info_t*, const uint8_t*, int)`, **2.x** `void(const uint8_t* mac, const uint8_t*, int)`. Dùng `#if ESP_ARDUINO_VERSION_MAJOR >= 3`. Đối chiếu phiên bản thật của OhStem core.
  - Callback gửi cũng đổi chữ ký theo core; kiểm tra khi dùng.
  - Gói dữ liệu là `struct` định nghĩa trong `common/protocol.h`, có `ver`, `seq`. Kiểm tra `len == sizeof(Pkt)` và `ver` trước khi dùng.
- **Cấm** `delay()` trong task mạng. Dùng `vTaskDelay`.

### 4.5 TinyML trên thiết bị

- Suy luận trong `task_tinyml`, chạy sau khi có mẫu mới (semaphore từ task sensor), không chặn task sensor.
- Chuẩn hóa đầu vào **giống hệt** lúc train: dùng hằng số cố định `t=(T-25)/25`, `h=(H-50)/50`, `s=(soil-50)/50` (TINYML_GUIDE.md mục 2.2), đặt một nơi trong `config.h`. Khi đọc cảm biến lỗi dùng sentinel T=−40, H=0. Phải có parity test thiết bị vs Python trước khi coi T04 hoàn thành.
- Quy trình lượng tử hóa: đầu vào float → int8 theo `scale/zero_point` đọc từ tensor, không hard-code.
- Log mỗi lần suy luận: thời gian (µs), nhãn, xác suất lớp cao nhất. Ghi `ai_class` vào telemetry.
- Nếu không build được TFLM trên OhStem core: dùng phương án dự phòng (suy luận MLP INT8 viết tay bằng C) và **nói rõ** trong báo cáo.

### 4.6 OTA (T14)

- Làm theo `COREIOT_SETUP.md` mục 10 (topic `v2/fw/...`, attribute `fw_*`, telemetry `current_fw_*`/`fw_state` đã đối chiếu tài liệu ThingsBoard), **nhưng vẫn phải kiểm tra thực tế trên CoreIOT của lớp** (tên giá trị `fw_state`, thuật toán checksum mặc định).
- Quy trình: đọc shared attributes `fw_title/fw_version/...` → so với `FW_VERSION` hiện tại → tải theo chunk → `Update.begin/write/end` → kiểm checksum → `ESP.restart()`.
- Bắt buộc: kiểm checksum, xử lý mất mạng giữa chừng (hủy, không ghi dở, giữ bản cũ), không OTA khi đang ghi cấu hình, báo `fw_state`/`fw_version` lên telemetry/attribute.
- `FW_VERSION` là hằng số trong `config.h`; mỗi bản test OTA đổi số rõ ràng để video thấy "trước/sau".
- Nhắc người dùng **giữ cáp USB và bản firmware backup**: lỗi flash có thể phải nạp lại bằng cáp.

### 4.7 Cấu hình & bảo mật

- Cấu hình WiFi/Token/host/port lưu LittleFS (JSON) qua Webserver; có mặc định trong `secrets.h` cho lần nạp đầu. Ghi file qua mutex cấu hình, tránh ghi lúc đang đọc.
- Không in token đầy đủ lên Serial hay web. Che (`abcd****`).
- Không để Webserver cho phép đổi cấu hình mà không có thao tác xác nhận.

### 4.8 Phong cách code

- C++ Arduino, thư viện gọn; `config.h` chứa mọi hằng số (chân, ngưỡng, thời gian, tên topic). Cấm "magic number" rải rác.
- Hàm ngắn (< ~40 dòng), tên rõ nghĩa (`readSoilPercent()`, `applyAutoRules()`), một trách nhiệm.
- Kiểm tra giá trị trả về của I/O (`dht.read()`, `mqtt.publish()`, `esp_now_send()`, `deserializeJson()`), xử lý lỗi rõ ràng, không bỏ qua im lặng.
- Log Serial có tiền tố task: `[SENSOR]`, `[MQTT]`, `[NOW]`, `[AI]`, `[OTA]`, `[WEB]`. Giữ log ở mức vừa đủ, có cờ `DEBUG_LOG` trong `config.h`. Log qua mutex Serial nếu nhiều task in cùng lúc.
- Code mới phải **biên dịch được** với thư viện/phiên bản ghi trong README. Khi dùng API chưa chắc, nói rõ và để `TODO(verify)`.

## 5. Luật code Python (`ml/`)

- Python 3.10+, `requirements.txt` ghim phiên bản (TensorFlow/Keras, NumPy, pandas, scikit-learn, pyserial, matplotlib).
- `collect_serial.py`: đọc Serial (cổng, baud là tham số dòng lệnh), ghi CSV `timestamp,t,h,soil,label`, mỗi phiên thu có tham số nhãn và ghi chú kịch bản. Chịu lỗi dòng hỏng. Lưu thô, không xóa dữ liệu.
- `train.py`: tách train/val/test **theo phiên thu** (tránh rò rỉ do mẫu liền kề), seed cố định, in và lưu: accuracy, loss, confusion matrix (PNG), classification report, tham số chuẩn hóa. Báo cáo chia dữ liệu thật/tổng hợp rõ ràng.
- `export_tflite.py`: xuất TFLite **float32 trước**, **INT8 (full-integer, representative dataset từ dữ liệu thật)** nếu chạy tốt trên thiết bị; so sánh kích thước và độ chính xác hai bản; ghi `model.h` (mảng C + độ dài) cho bản được chọn và `test_vectors.h` cho parity test. Chuẩn hóa dùng hằng số cố định (TINYML_GUIDE.md mục 2.2), không học từ dữ liệu. In kích thước model.
- Đoạn nào ảnh hưởng điểm T05 (thu thập, huấn luyện, export) phải chạy lại được từ đầu bằng lệnh trong README.
- Không dùng dữ liệu "đẹp một cách đáng ngờ" (độ chính xác 100% trên dữ liệu tổng hợp) làm kết luận. Nếu kết quả quá tốt, kiểm tra rò rỉ và nói thẳng trong báo cáo.

## 6. Luật debug

### 6.1 Quy trình chuẩn

1. **Tái hiện:** xin log Serial đầy đủ (từ lúc reset), phiên bản core, thư viện, board nào.
2. **Cô lập:** tắt dần chức năng (chỉ DHT20; chỉ LCD; chỉ Wi-Fi…) để xác định tầng gây lỗi: phần cứng → I2C/ADC → task/mutex → mạng → cloud.
3. **Giả thuyết → thí nghiệm nhỏ → kết luận.** Mỗi lần chỉ đổi một biến.
4. **Sửa tận gốc**, thêm log/assert để lỗi không tái diễn, ghi vào `docs/` nếu là bài học đáng báo cáo.
5. Không "thử vận may" bằng cách thêm `delay()` lung tung. Tìm nguyên nhân thật.

### 6.2 Bảng triệu chứng thường gặp

| Triệu chứng | Nguyên nhân hay gặp | Cách kiểm tra/sửa |
|---|---|---|
| Không nạp được code | Cáp chỉ sạc; sai cổng/board; chưa vào chế độ boot | Đổi cáp dữ liệu; chọn đúng board "OhStem"; giữ BOOT rồi nhấn RESET |
| LCD không hiện/ký tự rác | Sai địa chỉ I2C (docs OhStem dùng `0x21`); chưa `backlight()`; cắm sai cổng I2C | Chạy `tools/i2c_scan`; đổi địa chỉ trong `config.h`; chỉnh biến trở tương phản |
| DHT20 trả 0/NaN | Chưa `Wire.begin()`; đọc quá nhanh (cần ≥ vài trăm ms giữa hai lần); dây lỏng | Đọc mỗi ≥ 1–2 s; kiểm tra trả về của `read()`; in giá trị thô |
| Độ ẩm đất nhảy lung tung/ngoài 0–100 | Dải ADC khác giả định; nhiễu | In `analogRead` thô; hiệu chỉnh khô/ướt; lấy trung bình 8–16 mẫu; kẹp giá trị |
| NeoPixel sai màu/không sáng | Sai chân; sai thứ tự màu (GRB/RGB); chưa `begin()/show()` | Xác minh chân theo core; đổi cờ màu |
| Reset liên tục (`Guru Meditation`, `abort`) | Stack overflow; con trỏ null; truy cập khi chưa khởi tạo; watchdog do vòng lặp chặn | Đọc backtrace; tăng stack; in `uxTaskGetStackHighWaterMark`; thêm `vTaskDelay` |
| `Task watchdog got triggered` | Task chiếm CPU/chặn lâu | Thêm `vTaskDelay`; chia nhỏ công việc |
| Dữ liệu chồng lấn/sai lệch | Race condition (thiếu mutex) | Dùng getter/setter có mutex; copy ra cục bộ |
| Mutex treo (task đứng) | Quên `xSemaphoreGive`; thoát hàm sớm giữa chừng | Dùng khối RAII/`goto cleanup`; kiểm tra mọi đường `return` |
| ESP-NOW không nhận | Khác kênh; sai MAC; chưa `WIFI_STA`; sai chữ ký callback; peer chưa thêm | In MAC từng node; kiểm kênh `WiFi.channel()`; kiểm tra giá trị trả về `esp_now_add_peer` |
| ESP-NOW chạy rồi hỏng khi bật Wi-Fi | Kênh đổi theo router | Đặt cùng kênh, hoặc để cả hai cùng join một router |
| Biên dịch lỗi ở callback ESP-NOW | Khác core 2.x/3.x | Dùng `#if ESP_ARDUINO_VERSION_MAJOR >= 3` |
| MQTT không kết nối | Sai host/cổng/token; chặn mạng; device chưa tạo; `rc` khác 0 | Log `mqtt.state()` (6.3); thử bằng MQTT client trên PC với cùng token |
| Publish không lên | Payload > buffer; `mqtt.connected()` sai; chưa `loop()` | `setBufferSize`; kiểm tra trả về `publish()` |
| RPC không tới | Chưa subscribe `v1/devices/me/rpc/request/+`; JSON sai; method lệch | In nguyên payload nhận; so khớp tên method |
| Dashboard không có dữ liệu | Sai tên khóa telemetry; sai device; thiếu quyền | So khóa trong code với widget; xem tab Latest telemetry |
| Notification không gửi | Chưa cấu hình kênh/rule; điều kiện không đúng | Kiểm tra Alarm tạo ra chưa → rule gửi thông báo; test bằng giá trị cố định |
| Web không mở được | Sai IP; thiếu LittleFS data upload; AP/STA chưa lên | In IP; nạp phân vùng dữ liệu; kết nối vào AP cứu hộ |
| TinyML luôn ra một lớp | Chuẩn hóa lệch giữa train và thiết bị; scale/zero-point sai; dữ liệu mất cân bằng | So sánh vector đầu vào thiết bị với script Python; kiểm tra `model_meta.json` |
| `AllocateTensors() failed` | Tensor arena nhỏ; thiếu op | Tăng arena; đăng ký đúng op resolver |
| OTA thất bại/brick | Checksum lệch; mất mạng; partition scheme không có OTA | Dùng partition scheme có `app0/app1`; kiểm tra kích thước; giữ cáp nạp lại |
| Heap giảm dần | Rò rỉ (`String`, `new`, JSON lớn) | Log `getMinFreeHeap`; bỏ `String`; dùng bộ đệm tĩnh |

### 6.3 Mã trả về `mqtt.state()` (PubSubClient)

`-4` timeout; `-3` mất kết nối; `-2` kết nối mạng thất bại (host/cổng/mạng); `-1` đã ngắt; `0` OK; `1` sai giao thức; `2` client ID bị từ chối; `3` server không sẵn sàng; `4` sai username/password; `5` không được phép (token sai/không hợp lệ). Khi gặp `5` hoặc `4` kiểm tra Access Token trước tiên.

### 6.4 Công cụ debug nên có trong `tools/`

- `i2c_scan` (quét địa chỉ I2C, in ra Serial).
- `print_mac` (in MAC của board, dùng cấu hình ESP-NOW).
- `adc_calibrate` (in giá trị thô của A1 để hiệu chỉnh khô/ướt).
- `latency_logger` (ghi `seq`, `tx_ms`, `rx_ms` ra CSV để đo độ trễ/mất gói).

## 7. Milestone & tiêu chí hoàn thành (Definition of Done)

Claude chỉ làm milestone người dùng gọi tên. Mỗi milestone có **kết quả kiểm chứng** và **minh chứng cần lưu**.

| # | Milestone | Hoàn thành khi | Minh chứng lưu |
|---|---|---|---|
| M0 | Môi trường | Arduino IDE + OhStem Boards + thư viện cài xong, nạp được sketch blink D13 lên cả hai board. **Và** xác nhận quyền tài khoản CoreIOT: thấy và dùng được Device profiles, Rule chains, Notification center, OTA updates (nếu thiếu: báo người dùng ngay) | ảnh cài đặt, danh sách phiên bản, ảnh menu CoreIOT |
| M1 | Phần cứng từng ngoại vi | DHT20, LCD, độ ẩm đất, NeoPixel chạy riêng lẻ, có I2C scan, hiệu chỉnh ADC | log Serial, ảnh dây nối, số hiệu chỉnh |
| M2 | Firmware lõi | Task sensor/lcd/led chạy, `shared_data` thread-safe, không reset 30 phút | log, đồ thị heap |
| M3 | CoreIOT cơ bản | Làm theo `COREIOT_SETUP.md` mục 3–5. Hai device gửi telemetry, nhận RPC `setMode`, hai chiều có phản hồi; đã test bằng PC (mosquitto/MQTTX) trước khi nạp board | ảnh Latest telemetry, log RPC |
| M4 | Dashboard + RuleChain + Notification (T11, T12, T13) | Làm theo `COREIOT_SETUP.md` mục 6–9. Dashboard đủ widget; alarm rules; RuleChain theo sự kiện alarm gửi RPC `cloudAlert` sang Node 2 (firmware Node 2 phải xử lý `cloudAlert`); nhận notification | ảnh, JSON export, video ngắn |
| M5 | ESP-NOW (T06) | Hai chiều ổn định ≥ 200 gói; failover khi tắt router | log hai node, CSV độ trễ/mất gói, video |
| M6 | Webserver (T07) | Hai node có trang chủ/điều khiển/thông tin/cài đặt, có AP cứu hộ | ảnh từng trang, video |
| M7 | TinyML (T04, T05) | Thu dữ liệu thật, train, export `model.h`, suy luận trên Node 1 khớp Python | script, đồ thị, confusion matrix, log suy luận, kích thước/thời gian |
| M8 | OTA (T14) | Làm theo `COREIOT_SETUP.md` mục 10 (đúng partition scheme có 2 slot app, buffer MQTT ≥ chunk+overhead). Cập nhật `v1.0.0 → v1.0.1` qua CoreIOT cho ít nhất một node, báo phiên bản mới, kiểm checksum, thử ngắt mạng giữa chừng | video trước/sau, log, thời gian OTA |
| M9 | Đo đạc | Đủ các chỉ số mục 9 PROJECT.md, có CSV thô | `docs/measurements/` |
| M10 | Báo cáo + video + README | Theo mẫu, bảng tự đánh giá T/N, bảng phân công, ≥ 5 tài liệu, khai báo dùng AI | PDF, link video, repo sạch |
| (dự phòng) | T08/T10 | Chỉ làm nếu người dùng yêu cầu vì OTA không kịp | tương ứng |

Thứ tự ưu tiên: M0→M1→M2→M3→M4 (nhanh có điểm) → M5 → M6 → M7 → M8 → M9 → M10. Nếu người dùng báo trễ tiến độ: nhắc hy sinh M8 và chuyển sang T08/T10 để giữ đủ 8 điểm.

## 8. Luật minh chứng (không có minh chứng = không có điểm)

Sau **mỗi** milestone, Claude liệt kê cho người dùng:
1. Ảnh/ảnh chụp màn hình/log/video nào cần chụp, **kịch bản chụp** (làm gì để thấy được), đặt tên file `docs/evidence/<Txx>/<mô-tả>.<đuôi>`.
2. Câu lệnh/thao tác để tạo log (ví dụ lệnh Python, bật `DEBUG_LOG`).
3. Một dòng "tuyên bố" tương ứng trong bảng tự đánh giá T/N (ví dụ: "T06 – ESP-NOW hai chiều, đạt, minh chứng: log, CSV, video 02:10–03:40").

Video demo (5–10 phút) theo thứ tự **từng mã T** để người chấm tick theo bảng. Mỗi đoạn có chú thích chữ hiển thị mã T và nội dung đang làm. Gợi ý trình tự: sơ đồ phần cứng → T04/T05 → T06 → T07 → T11/T12/T13 → failover mất Internet → T14.

## 9. Luật đo đạc & báo cáo trung thực

- Chỉ ghi số đã đo. Có đơn vị, số lần đo, điều kiện (khoảng cách, nhiệt độ phòng, tải mạng).
- Mô tả giới hạn: chấp hành chỉ mô phỏng; dữ liệu TinyML thu trong điều kiện phòng, bao nhiêu phần là tổng hợp; số mẫu nhỏ.
- Không dùng từ tuyệt đối ("hoàn hảo", "100%", "triệt tiêu hoàn toàn") nếu chưa có bằng chứng.
- Cấu trúc báo cáo theo `PROJECT.md` mục 11. Chương 2 phải **ánh xạ từng tính năng ↔ mã T/N**. Ghi phần kế thừa (OhStem Green House, template CoreIOT) và phần tự làm.
- Có **khai báo phạm vi dùng AI**: ví dụ "AI hỗ trợ soạn khung code và gỡ lỗi; nhóm tự kiểm chứng trên kit thật". Mỗi thành viên phải giải thích được code mình phụ trách; vì thế Claude nên **giải thích ngắn "vì sao code như vậy"** (3–5 dòng) khi giao code, để người dùng hiểu và trả lời vấn đáp.
- Tài liệu tham khảo: ≥ 5, đúng nguồn đã thực sự dùng, có đường dẫn/ngày truy cập.

## 10. Định dạng trả lời của Claude

- Việc nhỏ: 1–2 đoạn + code. Việc lớn: kế hoạch ngắn → file theo thứ tự → cách chạy → minh chứng cần lưu → rủi ro/`CẦN XÁC MINH`.
- Mỗi khối code ghi rõ **tên file đích** ở dòng đầu (ví dụ `// node1_sensor/task_sensor.cpp`). Khi đưa nhiều file, đưa **đầy đủ nội dung từng file**, không viết "phần còn lại giữ nguyên" trừ khi sửa nhỏ và chỉ rõ vị trí (hàm/dòng).
- Khi sửa code đã có: đưa **diff hoặc đoạn thay thế có chỉ rõ vị trí**, không bắt người dùng đoán.
- Lệnh terminal để trong khối code, từng lệnh rõ ràng.
- Không trích nguyên văn dài từ tài liệu bản quyền; tóm tắt bằng lời mình và dẫn nguồn.
- Không khen sáo rỗng; nếu thấy kế hoạch/code của người dùng có vấn đề, **nói thẳng nguyên nhân và cách sửa**.

## 11. Những điều CẤM

1. Tự ý code/refactor khi chưa có lệnh; tự thêm tính năng ngoài danh sách T/N đã chọn.
2. Giả định có bơm/quạt/cảm biến ánh sáng/Yolo:Bit hoặc phần cứng ngoài kit.
3. Sao chép báo cáo/mã nguồn của nhóm khác hoặc báo cáo mẫu; đưa vào repo thứ không có nguồn.
4. Bịa số đo, bịa tên API/thư viện/URL/attribute, bịa kết quả chạy.
5. Commit hoặc in ra token, mật khẩu Wi-Fi, khóa bí mật.
6. Dùng `delay()` trong task; dùng biến toàn cục tự do; giữ mutex khi làm I/O chậm; dùng `String` nối chuỗi trong vòng lặp nóng.
7. Tuyên bố tính năng đã chạy/đạt điểm khi chưa có minh chứng từ kit thật.
8. Thay đổi giao thức (`protocol.h`, tên khóa telemetry, tên RPC) ở một node mà không cập nhật node còn lại, dashboard, RuleChain và tài liệu.
9. Xóa dữ liệu thô, minh chứng hoặc lịch sử commit của thành viên khác.
10. Tuyên bố "tối đa điểm" hoặc cam kết điểm số. Chỉ nói điều kiện cần để được tính điểm.

## 12. Danh sách kiểm tra trước khi nộp (Claude dùng khi người dùng yêu cầu rà soát)

- [ ] Mỗi mã T đăng ký có đủ minh chứng (ảnh/log/video/mã) và đúng thư mục.
- [ ] Bảng tự đánh giá T/N và bảng phân công có trong báo cáo, khớp với video.
- [ ] Tổng điểm công nghệ dự kiến ≥ 8 **sau khi trừ** mọi mục chỉ làm một phần.
- [ ] Chương 4 có đủ số đo thật, nêu điều kiện đo.
- [ ] README chạy được từ máy sạch; thư viện có phiên bản; không còn secret trong repo.
- [ ] Hai node biên dịch với cùng `protocol.h`; telemetry/RPC khớp dashboard và RuleChain.
- [ ] Báo cáo nói rõ chấp hành là mô phỏng; dữ liệu TinyML thật/tổng hợp tách bạch.
- [ ] ≥ 5 tài liệu tham khảo, trích dẫn phần kế thừa (OhStem, template CoreIOT, ý tưởng từ báo cáo mẫu nếu dùng).
- [ ] Khai báo phạm vi sử dụng AI.
- [ ] Video 5–10 phút, có chú thích mã T, link nằm trong báo cáo; file PDF đặt tên `[Mã lớp]_[Mã nhóm]_BaoCaoIoT.pdf`.
- [ ] Nộp trước hạn tuần 16; đã hỏi thầy về vấn đáp nếu cần.
- [ ] Mỗi thành viên giải thích được phần mình làm.

## 13. Câu mở đầu gợi ý khi bắt đầu một phiên làm việc

> Đọc `PROJECT.md` và `CLAUDE.md`. Hôm nay làm **M<số>**. Trạng thái hiện tại: <đã xong gì, log/lỗi nếu có>. Chỉ làm đúng milestone này, nêu kế hoạch ngắn trước, rồi đưa code đầy đủ từng file, cách nạp, cách kiểm tra, và minh chứng cần lưu.
