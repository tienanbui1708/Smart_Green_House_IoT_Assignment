# COREIOT_SETUP.md – Hướng dẫn cấu hình CoreIOT và chạy toàn hệ thống

> Phục vụ các mã: **T11** (dashboard + điều khiển từ xa), **T12** (RuleChain), **T13** (notification), **T14** (OTA), **N07** (ThingsBoard local, tùy chọn).
> Đọc cùng `PROJECT.md` (giao thức, tên khóa) và `CLAUDE.md` (luật làm việc).
>
> **Về độ tin cậy của tài liệu này:** CoreIOT (https://app.coreiot.io) là nền tảng dựa trên ThingsBoard. Các bước dưới đây dựa trên tài liệu ThingsBoard (MQTT API, OTA, Rule Engine, Notification) và slide workshop CoreIOT/OhStem. **Tên menu/nút trên giao diện có thể khác chút tùy phiên bản CoreIOT.** Nếu khác thì tìm mục tương đương và chụp lại ảnh cho báo cáo. Chỗ nào ghi `CẦN XÁC MINH` là phần mình chưa kiểm chứng được trên chính server CoreIOT của lớp.

---

## 0. Kết quả cuối cùng cần đạt

Sau khi làm hết tài liệu này, bạn có:

1. 2 device (`Node1`, `Node2`) online trên CoreIOT, gửi telemetry đều đặn.
2. 1 dashboard "Smart Greenhouse" có gauge, biểu đồ, bảng alarm, công tắc Auto/Manual, nút điều khiển (T11).
3. 1 RuleChain thông minh: lưu dữ liệu → phát hiện vượt ngưỡng → gửi lệnh RPC sang Node 2 qua `Change Originator` (T12).
4. Alarm tự tạo/xóa và **notification** nhận được khi vượt ngưỡng (T13).
5. OTA: upload `.bin` lên CoreIOT, thiết bị tự cập nhật `v1.0.0 → v1.0.1` (T14).
6. (Tùy chọn) ThingsBoard tự chạy trên máy bạn bằng Docker (N07).
7. File xuất `cloud/rulechain_*.json`, `cloud/dashboard.json`, ảnh minh chứng đúng thư mục `docs/evidence/`.

---

## 1. Kiến thức cần nắm (5 phút)

| Khái niệm | Ý nghĩa trong dự án |
|---|---|
| **Device** | Một thiết bị trên server, có **Access Token** riêng. Mỗi Yolo UNO = 1 device. |
| **Device profile** | Nhóm cấu hình cho một loại thiết bị: transport (MQTT), alarm rules, rule chain mặc định, firmware OTA mặc định. |
| **Telemetry** | Dữ liệu theo thời gian (nhiệt độ, độ ẩm…). Lưu thành chuỗi thời gian, vẽ biểu đồ được. |
| **Attributes** | Thuộc tính. *Client*: thiết bị tự báo. *Server*: chỉ server ghi. *Shared*: server ghi, thiết bị đọc (OTA dùng loại này). |
| **RPC** | Gọi hàm từ xa: server gửi `{method, params}` xuống thiết bị. *Two-way*: chờ phản hồi. *One-way*: không chờ. |
| **Rule chain** | Chuỗi các "nút" xử lý mỗi tin nhắn (lọc, lưu, biến đổi, gọi RPC…). |
| **Alarm** | Sự kiện cảnh báo có vòng đời (tạo → cập nhật → xóa), tạo bằng *alarm rules* của device profile. |
| **Notification center** | Gửi thông báo (web, mobile app, email…) khi có sự kiện như alarm. |
| **OTA** | Server giữ file firmware, báo cho thiết bị qua shared attributes `fw_*`; thiết bị tải về từng chunk. |

Thông số kết nối MQTT chuẩn của ThingsBoard (áp dụng cho CoreIOT):

| Thông số | Giá trị |
|---|---|
| Host | hostname server (dự kiến `app.coreiot.io`, **xác nhận ở bước 3.4**) |
| Cổng | `1883` (không mã hóa), `8883` (TLS) |
| Username | **Access Token** của device |
| Password | để trống |
| Client ID | tùy ý (mỗi device nên khác nhau) |

Topic dùng trong dự án (dạng chuẩn `v1/...`; ThingsBoard cũng có dạng ngắn `v2/...`, nhưng dùng một dạng cho thống nhất):

| Việc | Topic |
|---|---|
| Gửi telemetry | `v1/devices/me/telemetry` |
| Gửi client attributes | `v1/devices/me/attributes` |
| Nhận RPC | subscribe `v1/devices/me/rpc/request/+` |
| Trả lời RPC | publish `v1/devices/me/rpc/response/{requestId}` |
| Nhận cập nhật shared attributes | subscribe `v1/devices/me/attributes` |
| Yêu cầu giá trị attributes | publish `v1/devices/me/attributes/request/{requestId}`, nhận ở `v1/devices/me/attributes/response/+` |
| OTA: xin từng chunk | publish `v2/fw/request/{requestId}/chunk/{chunkIndex}` (payload = kích thước chunk theo byte) |
| OTA: nhận chunk | subscribe `v2/fw/response/+/chunk/+` |

---

## 2. Chuẩn bị

1. Tài khoản CoreIOT của lớp (đăng nhập tại https://app.coreiot.io; dùng thông tin thầy/lớp cung cấp). Nếu chưa có: hỏi thầy hoặc diễn đàn LMS.
2. Cài trên PC một MQTT client để **test không cần board**: một trong hai
   - **MQTTX** (giao diện đồ họa), hoặc
   - **Mosquitto** (`mosquitto_pub`, `mosquitto_sub`, dòng lệnh).
3. Trình duyệt mở sẵn 2 tab: CoreIOT và tài liệu ThingsBoard (https://thingsboard.io/docs).
4. Tạo thư mục ghi minh chứng: `docs/evidence/T11`, `T12`, `T13`, `T14` (mục 12 liệt kê cần chụp gì).
5. Quy ước đặt tên (giữ **đúng y** để khớp code và RuleChain):
   - Device: `Node1`, `Node2`
   - Profile: `Profile_Node1`, `Profile_Node2`
   - Rule chain: `RC_Node1_Ingest`, `RC_Node2_Command`
   - Dashboard: `Smart Greenhouse`

---

## 3. Bước 1 – Tạo device và lấy Access Token

### 3.1 Tạo device profile (làm trước để gắn khi tạo device)

1. Menu **Profiles → Device profiles** (hoặc **Device profiles**) → nút **+** (Add device profile).
2. Tạo **`Profile_Node1`**: Name = `Profile_Node1`; Transport type = **MQTT** (mặc định); các tab còn lại để mặc định → **Add**. (Alarm rules thêm ở bước 7.)
3. Làm tương tự tạo **`Profile_Node2`**.
4. Ảnh minh chứng: danh sách 2 profile → `docs/evidence/T12/profiles.png`.

### 3.2 Tạo device

1. Menu **Entities → Devices** (hoặc **Devices**) → **+** → *Add new device*.
2. **Node1**: Name = `Node1`; Device profile = `Profile_Node1`; Label (nếu có) = `Node 1 – Cảm biến` → **Add**.
3. **Node2**: Name = `Node2`; Device profile = `Profile_Node2`; Label = `Node 2 – Chấp hành` → **Add**.

### 3.3 Lấy Access Token

1. Bấm vào device (ví dụ `Node1`) → **Details** → nút **Copy access token** (hoặc **Manage credentials**, credentials type = *Access token*).
2. Dán token vào `secrets.h` của node tương ứng (không dán lên chat, GitHub, báo cáo, ảnh chụp):
   ```cpp
   // node1_sensor/secrets.h  (file này nằm trong .gitignore)
   #define WIFI_SSID   "..."
   #define WIFI_PASS   "..."
   #define MQTT_HOST   "app.coreiot.io"
   #define MQTT_PORT   1883
   #define DEVICE_TOKEN "TOKEN_NODE1"
   ```
3. Làm tương tự với `Node2`.
4. **Không bao giờ** dùng chung một token cho hai board (CoreIOT sẽ coi như một device và đá kết nối lẫn nhau).

### 3.4 Xác nhận host/cổng chính xác (CẦN XÁC MINH)

Trong trang chi tiết device có hộp thoại **Check connectivity** (tên có thể là *Connectivity check*): chọn giao thức **MQTT**, nó hiển thị sẵn lệnh `mosquitto_pub` với **host và cổng thật** của server kèm token. Chép host/cổng đó vào `secrets.h`. Đây là cách chắc nhất, không dựa vào việc đoán `app.coreiot.io`.

---

## 4. Bước 2 – Test kết nối bằng PC (chưa dùng board)

Mục đích: chắc chắn server, token, topic đều đúng **trước khi** nghi ngờ firmware.

### 4.1 Gửi telemetry giả

Thay `HOST` và `TOKEN`:

```bash
mosquitto_pub -d -q 1 -h HOST -p 1883 -t "v1/devices/me/telemetry" -u "TOKEN" \
  -m '{"temperature":27.5,"humidity":65,"soil":40,"ai_class":0,"heater":80,"auto":true}'
```

Kiểm tra: device `Node1` → tab **Latest telemetry** thấy các khóa vừa gửi, trạng thái device chuyển **Active**.

Nếu dùng MQTTX: New connection → Host = HOST, Port = 1883, Username = TOKEN, Password để trống, Client ID bất kỳ → Connect → Publish topic `v1/devices/me/telemetry` với JSON ở trên.

### 4.2 Nhận RPC giả

Terminal 1 (đóng vai thiết bị `Node2`):

```bash
mosquitto_sub -d -h HOST -p 1883 -u "TOKEN_NODE2" -t "v1/devices/me/rpc/request/+"
```

Trên CoreIOT: device `Node2` → tab **RPC** hoặc dùng widget thử (bước 5) gửi lệnh `setMode`. Terminal 1 phải in ra topic dạng `v1/devices/me/rpc/request/<số>` và nội dung `{"method":"setMode","params":true}`.

Để trả lời two-way RPC (thay `<số>` bằng số lấy từ topic):

```bash
mosquitto_pub -h HOST -p 1883 -u "TOKEN_NODE2" -t "v1/devices/me/rpc/response/<số>" -m '{"ok":true}'
```

### 4.3 Lỗi thường gặp ở bước test

| Hiện tượng | Nguyên nhân | Cách xử lý |
|---|---|---|
| `Connection refused`/`not authorised` | Token sai/thừa khoảng trắng; sai host | Copy lại token; dùng host từ *Check connectivity* |
| Gửi được nhưng **không thấy** dữ liệu | JSON sai (nháy đơn, thiếu ngoặc); gửi nhầm topic | Dùng nháy kép; topic đúng `v1/devices/me/telemetry` |
| Device luôn *Inactive* | Chưa có kết nối đang mở (MQTT client đã ngắt) | Giữ kết nối, gửi lại mỗi vài giây |
| Cổng 1883 không kết nối được | Mạng trường/nhà chặn | Thử mạng khác (điện thoại phát 4G), hoặc cổng 8883 (TLS) |

Chụp ảnh **Latest telemetry** có dữ liệu → `docs/evidence/T11/latest_telemetry_node1.png`.

---

## 5. Bước 3 – Nạp firmware và xác nhận dữ liệu thật

Thứ tự chạy (đúng thứ tự để khỏi nhầm MAC):

1. Nạp **Node 2** trước, mở Serial Monitor (115200) → ghi lại **MAC** `AA:BB:...` (in ra khi khởi động).
2. Dán MAC vào `NODE2_MAC` trong `node1_sensor/config.h`, nạp **Node 1**.
3. Cấp nguồn cả hai, cùng kết nối **một router Wi-Fi 2.4 GHz** (để ESP-NOW cùng kênh với router).
4. Trên CoreIOT: `Node1` và `Node2` đều **Active**, tab **Latest telemetry** có khóa theo `PROJECT.md` mục 7.2:
   - Node 1: `temperature, humidity, soil, ai_class, heater, auto, rssi, heap, uptime, fw_version`
   - Node 2: `fan, pump, auto, rssi, heap, uptime, fw_version, espnow_age_s, alert_level`
5. Nếu thấy khóa lạ/thiếu: **sửa firmware cho khớp** `PROJECT.md`, không sửa ngược dashboard. Tên khóa phải ổn định vì dashboard và rule chain dựa vào đó.

---

## 6. Bước 4 – Dashboard (T11)

### 6.1 Tạo dashboard và entity alias

1. Menu **Dashboards** → **+** → *Create new dashboard*: Title = `Smart Greenhouse`.
2. Mở dashboard → bấm **Edit mode** (biểu tượng bút) → **Entity aliases** → **Add alias**:
   - Alias `Node1`: Filter type = *Single entity*, Type = *Device*, chọn device `Node1`.
   - Alias `Node2`: tương tự với `Node2`.
3. Lưu alias, **Apply changes**.

### 6.2 Danh sách widget cần có

Tạo bằng **Add new widget** → chọn bundle phù hợp. Tên bundle/widget có thể khác phiên bản; chọn loại tương đương.

| # | Widget (gợi ý bundle) | Alias | Data key | Ghi chú |
|---|---|---|---|---|
| 1 | Gauge / Radial (Charts hoặc Analogue gauges) | Node1 | `temperature` | Đơn vị °C, min 0, max 50, vùng màu: xanh < 31, vàng 31–35, đỏ > 35 |
| 2 | Gauge | Node1 | `humidity` | % , 0–100 |
| 3 | Gauge | Node1 | `soil` | % , 0–100 |
| 4 | Timeseries line chart | Node1 | `temperature`, `humidity`, `soil` | Khoảng thời gian: Realtime last 10 min; chart này dùng làm minh chứng |
| 5 | Value card / Latest values | Node1 | `ai_class` | Dùng *Post-processing function* đổi số → chữ (xem 6.3) |
| 6 | Value card | Node1 | `heater` | 0–255 |
| 7 | Alarms table (Alarm widgets) | Node1 + Node2 | – | Hiển thị alarm đang hoạt động |
| 8 | Value card | Node2 | `fan`, `pump` | Hiển thị bằng chữ ON/OFF (post-processing) |
| 9 | Control: **Switch control** | Node1 | RPC `setMode` | Auto/Manual của Node 1 |
| 10 | Control: **Switch control** | Node2 | RPC `setMode` | Auto/Manual của Node 2 |
| 11 | Control: **Knob / Slider** | Node1 | RPC `setHeater` | 0–255; firmware chỉ nhận khi Manual |
| 12 | Control: **Switch control** | Node2 | RPC `setFan` | Chỉ nhận khi Manual |
| 13 | Control: **Switch control** | Node2 | RPC `setPump` | Chỉ nhận khi Manual |
| 14 | Latest values | Cả hai | `rssi`, `heap`, `uptime`, `fw_version` | Phục vụ đo đạc + OTA |

### 6.3 Biến số thành chữ (cho `ai_class`, `fan`, `pump`)

Trong cài đặt data key → **Post-processing function** (hàm JS), ví dụ cho `ai_class`:

```js
var names = ["Bình thường", "Đất khô", "Quá nóng/ẩm", "Cảm biến lỗi"];
return names[Math.round(value)] || "N/A";
```

Cho `fan`/`pump` (giá trị 0/1):

```js
return value ? "ON" : "OFF";
```

### 6.4 Cấu hình widget điều khiển RPC (Switch control)

Mục tiêu: khi gạt công tắc, dashboard gửi `{"method":"setMode","params":true|false}` xuống thiết bị.

Trong cài đặt widget (tên trường có thể khác chút):

- **Target device**: alias `Node1` (hoặc `Node2`).
- **RPC set value method**: `setMode` (thay cho mặc định `setValue`).
- **Retrieve value**: chọn lấy giá trị từ *Time series* khóa `auto` (hoặc *Attribute*), **không** dùng RPC get (firmware không cài `getValue`).
- **Parse value function** (nếu có): `return value ? true : false;`
- Timeout ~ 5000 ms. Nếu thiết bị offline, widget báo lỗi/timeout là bình thường.

Làm tương tự cho các widget còn lại, đổi `RPC set value method`: `setHeater` (số 0–255), `setFan`, `setPump` (true/false).

Firmware phải trả lời two-way RPC bằng `{"ok":true}` hoặc `{"ok":false,"reason":"auto_mode"}` (đã nêu trong `CLAUDE.md`/`PROJECT.md`). Nếu firmware không trả lời thì widget treo đến hết timeout.

### 6.5 Kiểm thử dashboard

1. Gạt `setMode` của Node 1 sang Manual → LCD Node 1 đổi `MD:MAN`, telemetry `auto` đổi thành `false`.
2. Kéo `setHeater` → NeoPixel Node 1 sáng/tối theo giá trị (chỉ khi Manual).
3. Gạt `setFan` ở Node 2 (Manual) → LED D13 sáng.
4. Ở Auto, thử `setFan` → bị từ chối (widget hiển thị lỗi hoặc không đổi).
5. Quay **video ngắn** các thao tác trên → `docs/evidence/T11/rpc_demo.mp4`; chụp ảnh toàn dashboard → `docs/evidence/T11/dashboard.png`.

### 6.6 Dùng template (T11 yêu cầu "sử dụng template")

Thông báo yêu cầu "sử dụng template để phát triển ứng dụng". Cách đảm bảo: bắt đầu từ **dashboard/template mẫu của CoreIOT hoặc dashboard mẫu ThingsBoard** (nếu có ở mục Solution templates/Dashboards mẫu của CoreIOT) rồi chỉnh. Nếu bạn tự dựng từ đầu thì **nói rõ** trong báo cáo là tự thiết kế dựa trên widget library. **Hỏi thầy** nếu cần chắc về cách chấm mục này.

### 6.7 Xuất dashboard

Danh sách Dashboards → biểu tượng **Export** → lưu thành `cloud/dashboard.json`.

---

## 7. Bước 5 – Alarm rules (nền cho T13 và T12)

Dùng **alarm rules của device profile** để cảnh báo (cách chính thống của ThingsBoard, tạo alarm một lần rồi tự xóa khi hết vi phạm, không spam).

### 7.1 Alarm cho Node 1

1. **Device profiles → Profile_Node1 →** chế độ chỉnh sửa → tab **Alarm rules** → **Add alarm rule**.
2. Tạo các alarm (mỗi cái một rule):

| Alarm type | Severity | Điều kiện tạo (Create) | Điều kiện xóa (Clear) |
|---|---|---|---|
| `High temperature` | Critical | `temperature` > 35 | `temperature` ≤ 33 |
| `Low soil moisture` | Major | `soil` < 20 | `soil` ≥ 25 |
| `High humidity` | Warning | `humidity` > 85 | `humidity` ≤ 80 |
| `Sensor fault (AI)` | Critical | `ai_class` = 3 | `ai_class` ≠ 3 |

Cách tạo điều kiện: *Add condition* → **Add key filter** → Key type = **Time series**, key = tên khóa, value type = **Numeric**, operation (`greater`, `less`…) và giá trị. Ngưỡng ghi ở đây là giá trị khởi đầu, **chỉnh theo số đo thật** và ghi lý do trong báo cáo.

3. Với Node 2 (`Profile_Node2`): thêm `Node offline`/mất dữ liệu ESP-NOW: `espnow_age_s` > 30 (Warning).
4. Bấm **Save**. Test bằng cách gửi telemetry giả (mục 4.1) với `temperature` = 40: tab **Alarms** của `Node1` phải hiện alarm `High temperature` trạng thái *Active*; gửi lại `temperature` = 30 → alarm tự chuyển *Cleared*.
5. Ảnh: `docs/evidence/T13/alarm_active.png`, `alarm_cleared.png`.

---

## 8. Bước 6 – Notification (T13)

### 8.1 Tạo người nhận (Recipients)

1. Menu **Notification center** → tab **Recipients** → **Add recipients group**.
2. Name = `Nhóm Greenhouse`; Type = *Users* (chọn tài khoản của bạn, và đồng đội nếu có quyền) hoặc *All users*.

### 8.2 Tạo template (Templates)

Tab **Templates** → Add: loại **Alarm**; Subject: `[Greenhouse] {alarmType} ({alarmSeverity})`; Message: `Thiết bị: ${alarmOriginatorName}. Trạng thái: ${alarmStatus}.` Biến trong `${...}` có thể khác tên theo phiên bản: dùng gợi ý biến hiển thị bên cạnh ô nhập (CẦN XÁC MINH).
Kênh gửi (delivery methods): bật **Web** (chuông thông báo trên giao diện) và, nếu có, **Mobile app**. **Email/SMS** cần cấu hình SMTP/SMS của server (thường không có quyền) nên đừng dựa vào.

### 8.3 Tạo rule gửi thông báo (Rules)

1. Tab **Rules** → **Add rule**:
   - Name = `Cảnh báo Greenhouse`
   - Trigger = **Alarm**
   - Alarm types = các loại ở mục 7 (hoặc để trống = tất cả)
   - Severities = Warning, Major, Critical
   - Notify on = **Created** (và *Cleared* nếu muốn)
   - Recipients = `Nhóm Greenhouse`; Template = template vừa tạo
2. Thêm rule thứ hai: Trigger = **Device activity**, thiết bị = `Node1`, `Node2`, notify khi **Inactive** (mất kết nối). Rất hợp để chứng minh "node mất kết nối".
3. **Test:** gửi telemetry `temperature` = 40 (mục 4.1) → biểu tượng chuông (Web) xuất hiện thông báo. Chụp ảnh chuông + nội dung → `docs/evidence/T13/notification.png`. Tắt Node → thông báo *Device inactive*.

### 8.4 Thông báo trên điện thoại (tùy chọn)

Cài app **Core IoT App** (Android/iOS), đăng nhập cùng tài khoản; thông báo kênh *Mobile app* sẽ đẩy về máy (**CẦN XÁC MINH** CoreIOT của lớp có bật push hay không). Chụp màn hình điện thoại nếu được.

---

## 9. Bước 7 – RuleChain (T12)

### 9.1 Ý tưởng

- **Chuỗi 1 – `RC_Node1_Ingest`** (gắn cho `Profile_Node1`): nhận mọi tin của Node 1 → lọc "Post telemetry" → lưu chuỗi thời gian → khi alarm **tạo/xóa** thì chuyển đích sang Node 2 và gửi RPC `cloudAlert`.
- **Chuỗi 2 – `RC_Node2_Command`** (gắn cho `Profile_Node2`): nhận mọi tin của Node 2 → lọc/lưu.

Tại sao dùng sự kiện **Alarm Created/Cleared** thay vì kiểm tra mỗi tin nhắn: Node 1 gửi mỗi 5 s; nếu kiểm tra ngưỡng từng tin sẽ bắn RPC liên tục (spam). Alarm chỉ phát sinh **một lần** khi vi phạm bắt đầu và một lần khi hết.

### 9.2 Vì sao dùng RPC `cloudAlert` mà không dùng `setFan`

Firmware Node 2 chỉ nhận `setFan/setPump` khi ở **Manual**; ở **Auto** thì Node 2 tự quyết bằng dữ liệu ESP-NOW. Lệnh từ RuleChain cần chạy được ở mọi chế độ, nên dùng một RPC "thông báo": **`cloudAlert`**. Node 2 hiển thị cảnh báo lên LCD và đổi NeoPixel, **không** thay đổi chấp hành.

**Bổ sung giao thức (cần cập nhật vào `PROJECT.md` mục 7.3 và nhắc Claude cài trong firmware Node 2):**

| Method | Params | Node | Điều kiện | Hành vi |
|---|---|---|---|---|
| `cloudAlert` | `{"level":0\|1\|2,"msg":"<tối đa 16 ký tự>"}` | 2 | luôn | `level=0` xóa cảnh báo; `1` cảnh báo (NeoPixel chớp vàng); `2` nghiêm trọng (NeoPixel chớp đỏ); `msg` hiện dòng 2 LCD trong 10 s |

Ghi nhớ: đây là điểm thể hiện **T12 – tính năng thông minh trên Cloud** (server phát hiện, server ra lệnh cho thiết bị khác).

### 9.3 Tạo quan hệ giữa Node 1 và Node 2 (cách A, khuyến nghị)

ThingsBoard có sẵn ví dụ dùng quan hệ **Manages** để chuyển đích sang thiết bị liên quan.

1. Devices → `Node1` → tab **Relations** → **+**:
   - Direction = **From** (Node1 là nguồn), Relation type = `Manages`, Entity type = **Device**, chọn `Node2` → **Add**.
2. Lưu và kiểm tra bảng quan hệ.
3. Ảnh: `docs/evidence/T12/relation.png`.

Cách B (không cần quan hệ): nút **Change originator** có tùy chọn chọn thực thể theo **tên** (entity name pattern) = `Node2`. Tùy phiên bản có hoặc không; nếu có và bạn thấy dễ hơn thì dùng.

### 9.4 Tạo `RC_Node1_Ingest`

1. **Rule chains** → **+** → Name = `RC_Node1_Ingest` → Add.
2. Mở chuỗi, kéo các nút sau (tìm trong thanh bên trái theo tên), nối bằng cách kéo từ chấm ra của nút này sang chấm vào của nút kia rồi chọn tên quan hệ (relation):

```
[Input]
   │
[Message type switch]
   ├── Post telemetry ───► [Save Timeseries] ──(Success)──► (kết thúc)
   ├── Alarm Created  ───► [Script: Tạo lệnh cảnh báo] ─(Success)─► [Change originator → Node2] ─(Success)─► [RPC call request]
   └── Alarm Cleared  ───► [Script: Tạo lệnh xóa]      ─(Success)─► [Change originator → Node2] ─(Success)─► [RPC call request]
```

(Dùng chung một nút `Change originator` và `RPC call request` cho cả hai nhánh bằng cách nối hai script vào cùng một nút Change originator.)

**Cấu hình từng nút:**

| Nút | Cấu hình |
|---|---|
| Message type switch | Mặc định (tự tạo các relation theo loại tin: Post telemetry, Alarm Created, Alarm Cleared…). |
| Save Timeseries | Mặc định (TTL mặc định). |
| Script `Tạo lệnh cảnh báo` (loại *Transformation → Script*) | Xem đoạn mã 9.5. |
| Script `Tạo lệnh xóa` | Xem đoạn mã 9.5. |
| Change originator | *Originator source* = **Related entity**; Direction = **To** (từ Node1 hướng tới entity được quản lý, nếu không thấy kết quả thì thử **From**); Relation type = `Manages`; Entity type = **Device**. Nếu chọn theo tên: Originator source = *Entity by name pattern* = `Node2`. (Nhãn có thể khác: nếu gặp lỗi `Failure`, bật debug và xem 9.8.) |
| RPC call request | Timeout = 10 s (giây). Mặc định hai chiều: Node 2 sẽ trả `{"ok":true}`. |

3. Lưu chuỗi (**Apply changes**).

### 9.5 Mã script

Nút **Script (Transformation)** cho phép JavaScript (hoặc TBEL, tùy phiên bản). Dùng JavaScript:

**Tạo lệnh cảnh báo** (nhánh Alarm Created):

```js
// msg là thông tin alarm; lấy loại và mức độ để hiển thị
var level = (msg.severity === "CRITICAL") ? 2 : 1;
var text = (msg.type || "ALARM");
if (text.length > 16) text = text.substring(0, 16);
var newMsg = { method: "cloudAlert", params: { level: level, msg: text } };
return { msg: newMsg, metadata: metadata, msgType: msgType };
```

**Tạo lệnh xóa** (nhánh Alarm Cleared):

```js
var newMsg = { method: "cloudAlert", params: { level: 0, msg: "" } };
return { msg: newMsg, metadata: metadata, msgType: msgType };
```

Ghi chú:
- Cấu trúc `{method, params}` là định dạng RPC chuẩn của ThingsBoard. Ví dụ chính thức trong tài liệu ThingsBoard cũng trả về `msgType` nguyên bản và RPC Call Request vẫn hoạt động. Báo cáo mẫu khóa trước đã ép `msgType` thành `RPC_CALL_FROM_SERVER_TO_DEVICE`; nếu bản CoreIOT của lớp yêu cầu thì thêm `msgType: "RPC_CALL_FROM_SERVER_TO_DEVICE"` (**CẦN XÁC MINH**, chỉ thêm khi RPC không đi xuống).
- Muốn đổi ngưỡng cảnh báo/nội dung hiển thị: sửa script này **trên Cloud, không cần nạp lại firmware**. Đây là điểm đáng nêu trong báo cáo.
- Tên trường `severity`, `type` trong `msg` của sự kiện alarm: kiểm tra bằng debug (9.8) vì có thể khác tên (**CẦN XÁC MINH**).

### 9.6 Gắn chuỗi vào device profile

1. **Device profiles → Profile_Node1 →** Edit → trường **Default rule chain** = `RC_Node1_Ingest` → Save.
2. Tạo `RC_Node2_Command`: Input → Message type switch → *Post telemetry* → Save Timeseries. Gắn vào `Profile_Node2` tương tự.
3. Nếu bạn không đổi *Default rule chain* của profile thì tin nhắn đi vào **Root Rule Chain** mặc định, chuỗi của bạn không chạy. Kiểm tra bước này đầu tiên khi RuleChain "không hoạt động".

### 9.7 Firmware Node 2 cần xử lý `cloudAlert`

Nhắc Claude (theo `CLAUDE.md`, làm đúng một milestone): thêm nhánh `if (method == "cloudAlert")` trong callback RPC của Node 2, đọc `params.level` và `params.msg`, lưu vào `shared_data` có mutex, task display/neopixel xử lý (không làm trong callback). Trả `{"ok":true}`.

### 9.8 Gỡ lỗi RuleChain

1. Mở nút → bật **Debug mode** (chọn *All messages* hoặc *Failures*), **Apply**.
2. Mở lại nút → tab **Events** (hoặc biểu tượng nhật ký của nút) → xem *Debug*: tin đầu vào, tin đầu ra, lỗi.
3. Hoặc dùng **Test script** (biểu tượng ✓ trong nút Script) nhập `msg`/`metadata` mẫu để thử.
4. **Tắt Debug mode sau khi xong** (tránh tốn tài nguyên).
5. Các lỗi hay gặp:

| Triệu chứng | Nguyên nhân | Sửa |
|---|---|---|
| Không thấy lưu telemetry | Profile chưa gắn rule chain; tin đi Root | 9.6 |
| Có Alarm nhưng không có RPC | Alarm không đi vào chuỗi của bạn (alarm gắn rule chain khác); relation `Manages` thiếu; hướng sai | Xem *Debug* ở Message type switch; kiểm tra relation; thử đảo From/To |
| `Change originator` báo Failure | Không tìm được thực thể | Kiểm tra relation hoặc tên Node2 |
| RPC `Failure`/timeout | Node 2 offline; không trả lời RPC; chưa subscribe topic RPC | Kiểm tra Node 2 Active; firmware phải subscribe `v1/devices/me/rpc/request/+` và publish response |
| Script lỗi | Cú pháp, trường không tồn tại | Dùng Test script |

### 9.9 Minh chứng T12

- Ảnh sơ đồ cả hai rule chain → `docs/evidence/T12/rulechain_node1.png`, `rulechain_node2.png`.
- Xuất JSON: Rule chains → biểu tượng **Export** → `cloud/rulechain_node1.json`, `cloud/rulechain_node2.json`.
- Video: tạo điều kiện vượt ngưỡng (ví dụ làm ấm DHT20 của Node 1) → alarm xuất hiện → Node 2 hiển thị `cloudAlert` trên LCD và NeoPixel chớp → hết vi phạm → Node 2 xóa cảnh báo. Đặt `docs/evidence/T12/rulechain_demo.mp4`.
- Ghi vào báo cáo: **phần kế thừa** (ý tưởng Change Originator từ tài liệu ThingsBoard/báo cáo mẫu nếu bạn tham khảo) và **phần tự làm** (cơ chế alarm-driven, RPC `cloudAlert`).

---

## 10. Bước 8 – OTA (T14)

### 10.1 Điều kiện để OTA hoạt động

1. **Phân vùng flash có hai slot ứng dụng.** Dự án chọn **`min_spiffs`** = Arduino IDE **Tools → Partition Scheme → "Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)"** (đã đối chiếu `boards.txt` của OhStem 3.1.3: `app0`/`app1` mỗi slot 0x1E0000, `spiffs` 128 KB cho LittleFS `/config.json`). `tools/flash.sh` và `tools/make_ota_bin.sh` mặc định dùng scheme này. Bản build để upload OTA phải dùng **cùng** scheme với bản đang chạy. Phải **nạp lại bằng cáp** với phân vùng đúng trước khi thử OTA lần đầu. Chọn sai phân vùng thì OTA thất bại hoặc hỏng.
2. Firmware **đã cài sẵn** logic OTA (đã làm ở milestone M8, theo `CLAUDE.md` mục 4.6) và có hằng `FW_VERSION`.
3. `mqtt.setBufferSize(...)` đủ lớn: kích thước buffer phải **lớn hơn** kích thước chunk yêu cầu + vài chục byte (ví dụ chunk 4096 B → buffer ≥ 4200 B). Chunk nhỏ (1024 B) chậm nhưng an toàn hơn khi mới thử.
4. Thiết bị phải có đủ RAM trống (log `heap`).

### 10.2 Xuất file `.bin`

Arduino IDE: **Sketch → Export Compiled Binary** (tên menu có thể là *Export compiled Binary*). File nằm trong thư mục sketch (`build/...`). Dùng file **`<tên>.ino.bin`** (ứng dụng). **Không** dùng `bootloader.bin`, `partitions.bin` hay `*.merged.bin` cho OTA.
Mẹo: đổi `FW_VERSION` rồi xuất. Ví dụ:

| Bản | `FW_VERSION` | Dùng để |
|---|---|---|
| v1 | `1.0.0` | Nạp bằng cáp (bản đang chạy) |
| v2 | `1.0.1` | Upload lên CoreIOT, thiết bị tự cập nhật |

Để thấy rõ "trước/sau" trong video, ở bản 1.0.1 hãy đổi một chi tiết dễ nhìn (ví dụ LCD hiện `v1.0.1`, hoặc màu NeoPixel lúc khởi động).

### 10.3 Upload gói OTA lên CoreIOT

1. Menu **OTA updates** (hoặc **Advanced features → OTA updates**) → **+** (Add).
2. Điền:
   - **Title**: `greenhouse-node1` (tiêu đề firmware)
   - **Version**: `1.0.1`
   - **Device profile**: `Profile_Node1` (mỗi gói chỉ tương thích với **một** device profile)
   - **Package type**: **Firmware**
   - **Upload binary file**: chọn `.bin`
   - **Checksum**: để **tự tính** (auto-generate), thuật toán mặc định
3. **Add**. Lặp lại với `Profile_Node2` (Title `greenhouse-node2`).
4. **Chú ý:** *Title + Version* phải là tổ hợp duy nhất trong hệ thống.

### 10.4 Gán gói cho thiết bị

Hai cách (gán ở mức device sẽ **ghi đè** gán ở mức profile):
- **Gán cho device** (khuyến nghị khi demo, an toàn hơn): Devices → `Node1` → Details → chỉnh sửa → trường **Assigned firmware** = `greenhouse-node1 1.0.1` → Save.
- **Gán cho profile**: Device profiles → `Profile_Node1` → chỉnh sửa → **Firmware** = gói tương ứng.

Việc gán sẽ kích hoạt cập nhật: ThingsBoard đặt các **shared attributes** của thiết bị:

| Attribute | Ý nghĩa |
|---|---|
| `fw_title` | tên gói |
| `fw_version` | phiên bản |
| `fw_size` | kích thước (byte) |
| `fw_checksum` | giá trị kiểm tra toàn vẹn |
| `fw_checksum_algorithm` | thuật toán (ví dụ SHA256/MD5/CRC32) |

### 10.5 Quy trình phía thiết bị (firmware phải làm đúng thứ tự)

1. Khi khởi động, báo phiên bản hiện tại bằng **telemetry**:
   ```json
   {"current_fw_title":"greenhouse-node1","current_fw_version":"1.0.0","fw_state":"UPDATED"}
   ```
2. Subscribe topic cập nhật attribute `v1/devices/me/attributes` (để nhận `fw_*` mới) và `v1/devices/me/attributes/response/+`; ngoài ra có thể **chủ động hỏi** lúc boot bằng publish vào `v1/devices/me/attributes/request/1` payload `{"sharedKeys":"fw_title,fw_version,fw_size,fw_checksum,fw_checksum_algorithm"}`.
3. Nếu `fw_title`/`fw_version` **khác** bản đang chạy → bắt đầu tải. Gửi telemetry `{"fw_state":"DOWNLOADING"}`.
4. Subscribe `v2/fw/response/+/chunk/+`. Với `requestId` mới (số tăng dần mỗi lần cập nhật) và `chunkIndex = 0,1,2…`, publish vào `v2/fw/request/{requestId}/chunk/{chunkIndex}` payload là **kích thước chunk** (ví dụ `4096`). Nhận chunk, ghi bằng `Update.write()`. Dừng khi nhận chunk **rỗng** (hoặc đủ `fw_size` byte).
5. Gửi `{"fw_state":"DOWNLOADED"}`; **tính checksum** (mbedtls) và so với `fw_checksum`. Sai → `{"fw_state":"FAILED","fw_error":"checksum mismatch"}`, **hủy** `Update.abort()`, giữ bản cũ.
6. Đúng → `{"fw_state":"VERIFIED"}` → `Update.end(true)` → `{"fw_state":"UPDATING"}` → `ESP.restart()`.
7. Sau khi khởi động bản mới, bước (1) tự động gửi `current_fw_version = 1.0.1`, `fw_state = UPDATED`.
8. Mọi lỗi (mất mạng, hết RAM, chunk sai kích thước) → `{"fw_state":"FAILED","fw_error":"<lý do>"}` và vẫn chạy bản cũ.

Các giá trị `fw_state` đã thấy trong tài liệu: `UPDATING`, `UPDATED`, `FAILED`; các trạng thái trung gian `DOWNLOADING`, `DOWNLOADED`, `VERIFIED` (**CẦN XÁC MINH** tên chính xác trong tài liệu ThingsBoard bản CoreIOT đang dùng). Theo dõi ở: Devices → `Node1` → tab **Attributes/Latest telemetry** khóa `fw_state`, `fw_version`; hoặc bảng *History of firmware updates* trong mục OTA updates.

Hạn chế mà bạn cần biết: `PubSubClient` phải xử lý tin nhắn chunk lớn trong `loop()`/callback; hãy ghi flash ngay trong callback có kiểm soát hoặc đẩy vào queue cho task OTA, tránh làm treo task MQTT quá lâu (watchdog). Đây là phần khó nhất của dự án, giữ **cáp USB và bản `1.0.0` .bin dự phòng** để nạp lại.

### 10.6 Kịch bản test OTA (để lấy minh chứng + số đo)

1. Nạp bản `1.0.0` bằng cáp, xác nhận Node 1 online, dashboard hiện `fw_version = 1.0.0`.
2. Upload gói `1.0.1` (10.3), gán cho `Node1` (10.4).
3. Bắt đầu quay màn hình + Serial: thấy `DOWNLOADING → DOWNLOADED → VERIFIED → UPDATING`, thiết bị reset, `UPDATED`.
4. Ghi thời gian từ lúc gán đến lúc `UPDATED` (đo ≥ 3 lần) + kích thước `.bin` → `docs/measurements/ota_time.csv`.
5. Thử lỗi: ngắt Wi-Fi giữa chừng → thiết bị vẫn chạy bản cũ, báo `FAILED`, và lần gán sau tải lại được.
6. Ảnh/video: `docs/evidence/T14/before.png`, `after.png`, `ota_log.txt`, `ota_demo.mp4`.

---

## 11. Bước 9 – Chạy toàn hệ thống (runbook demo)

Thực hiện theo thứ tự này khi quay video/demo:

| # | Việc | Kết quả nhìn thấy |
|---|---|---|
| 1 | Bật router, cấp nguồn Node 2 rồi Node 1 | LCD cả hai sáng; Serial báo Wi-Fi/MQTT connected |
| 2 | Mở CoreIOT → dashboard | Gauge nhiệt độ/độ ẩm/độ ẩm đất cập nhật mỗi ~5 s; `ai_class` đổi theo điều kiện |
| 3 | Làm ấm DHT20 (đặt tay gần/ hơi thở) | `temperature` tăng; `ai_class` chuyển; (T04) |
| 4 | Rút cảm biến đất ra khỏi cốc nước/cho vào cốc nước | `soil` đổi; Node 2 cập nhật `PUMP:ON/OFF` trên LCD |
| 5 | Dashboard gạt Manual, kéo `setHeater`, bật `setFan` | NeoPixel Node 1 đổi, LED D13 Node 2 sáng (T11) |
| 6 | Vượt ngưỡng nhiệt độ | Alarm xuất hiện; chuông notification; Node 2 hiện `cloudAlert` (T12, T13) |
| 7 | **Tắt router** (mất Internet) | CoreIOT báo Node 1/2 inactive; Node 2 vẫn nhận ESP-NOW và vẫn điều khiển quạt mô phỏng (T06, failover) |
| 8 | Bật lại router | Hai node tự kết nối lại, dữ liệu tiếp tục |
| 9 | Mở Webserver cục bộ của từng node | Trang xem/điều khiển hoạt động (T07) |
| 10 | Upload OTA 1.0.1 và gán | Thiết bị cập nhật, phiên bản đổi (T14) |

Video demo 5–10 phút dựng theo bảng này, có chú thích chữ hiển thị **mã T** (T04, T06, T07, T11, T12, T13, T14).

---

## 12. Danh sách minh chứng cần chụp (CoreIOT)

| Mã | File | Nội dung |
|---|---|---|
| T11 | `docs/evidence/T11/latest_telemetry_node1.png`, `…node2.png` | Tab Latest telemetry đủ khóa |
| T11 | `docs/evidence/T11/dashboard.png` | Toàn dashboard có dữ liệu thật |
| T11 | `docs/evidence/T11/rpc_demo.mp4` | Gạt công tắc, thiết bị phản ứng |
| T11 | `cloud/dashboard.json` | File export |
| T12 | `docs/evidence/T12/profiles.png`, `relation.png` | Profile và quan hệ |
| T12 | `docs/evidence/T12/rulechain_node1.png`, `rulechain_node2.png` | Sơ đồ rule chain |
| T12 | `cloud/rulechain_node1.json`, `rulechain_node2.json` | File export |
| T12 | `docs/evidence/T12/rulechain_demo.mp4` | Alarm → RPC → Node 2 |
| T13 | `docs/evidence/T13/alarm_active.png`, `alarm_cleared.png` | Alarm tạo/xóa |
| T13 | `docs/evidence/T13/notification.png` | Thông báo nhận được (chuông web, điện thoại) |
| T14 | `docs/evidence/T14/before.png`, `after.png`, `ota_log.txt`, `ota_demo.mp4` | OTA trước/sau |
| Đo đạc | `docs/measurements/*.csv` | Độ trễ RPC, thời gian OTA, failover |

Ảnh chụp **không được lộ token** (che hoặc cắt). Mỗi ảnh nên thấy rõ thời gian/giờ hệ thống và tên device.

---

## 13. Đo đạc phía CoreIOT (để điền Chương 4)

| Chỉ số | Cách đo |
|---|---|
| Độ trễ RPC end-to-end | Bấm công tắc trên dashboard (quay video có đồng hồ/millis), đo đến lúc LED/NeoPixel đổi; hoặc firmware ghi `millis()` khi nhận RPC và so với thời điểm bấm (ghi tay). Lặp ≥ 20 lần, báo trung bình + lớn nhất. |
| Độ trễ telemetry | Firmware gửi kèm `tx_ms`; đối chiếu thời gian lưu (timestamp server) với giờ thiết bị (cần đồng bộ); hoặc đo bằng quay video. Nêu rõ giới hạn đo. |
| Tỉ lệ nhận telemetry | Đếm số bản ghi trong 10 phút so với số lần gửi lý thuyết (mỗi 5 s → 120). |
| Thời gian alarm → Node 2 phản ứng | Quay video lúc dữ liệu vượt ngưỡng đến lúc LCD Node 2 đổi. |
| Thời gian OTA | Mục 10.6. |

Ghi rõ điều kiện đo (mạng, khoảng cách, số lần). Không bịa số.

---

## 14. Xuất/sao lưu cấu hình

1. Dashboards → **Export** dashboard → `cloud/dashboard.json`.
2. Rule chains → **Export** từng chuỗi → `cloud/rulechain_node1.json`, `rulechain_node2.json`.
3. Device profiles → **Export** (nếu có) → `cloud/profile_node1.json`, `profile_node2.json`.
4. Viết `cloud/notes.md`: cách import lại, tên device/profile, alarm rules, thiết lập notification, ngày xuất.
5. Commit vào repo (**kiểm tra không có token** trong file xuất; export dashboard/rule chain thường không chứa token, nhưng kiểm tra bằng tìm kiếm văn bản).

---

## 15. (Tùy chọn) N07 – ThingsBoard tự chạy local bằng Docker

Chỉ làm sau khi T04–T14 đã ổn. Điểm: 1.0 (nâng cao).

1. Cài Docker Desktop.
2. Làm theo tài liệu **cài đặt ThingsBoard bằng Docker** trên https://thingsboard.io/docs/ (mục *Installation → Docker (Linux or Mac OS) / Windows*). Cấu hình cơ bản (**CẦN XÁC MINH** bằng tài liệu mới nhất, vì tên image và cách khởi tạo lần đầu có thể thay đổi):
   ```yaml
   # local_tb/docker-compose.yml
   services:
     mytb:
       restart: always
       image: "thingsboard/tb-postgres"
       ports:
         - "8080:9090"
         - "1883:1883"
         - "7070:7070"
         - "5683-5688:5683-5688/udp"
       environment:
         TB_QUEUE_TYPE: in-memory
       volumes:
         - ./tb-data:/data
         - ./tb-logs:/var/log/thingsboard
   ```
3. Khởi tạo dữ liệu lần đầu theo hướng dẫn (có thể cần chạy bước *install* một lần trước khi `docker compose up`).
4. Mở `http://localhost:8080`, đăng nhập tài khoản mặc định theo tài liệu (thường là tài khoản tenant demo; đổi mật khẩu ngay).
5. Lặp lại: tạo device, token, import `cloud/dashboard.json` và rule chain đã xuất.
6. Trỏ Node về server local: trong Webserver (trang Cài đặt) đổi **host** thành IP LAN của máy chạy Docker (không dùng `localhost`), cổng 1883. Máy chạy Docker và hai board phải cùng mạng; tường lửa Windows phải mở cổng 1883/8080.
7. Minh chứng: ảnh `docker ps`, ảnh dashboard truy cập qua IP máy bạn, thiết bị online trên server local.

---

## 16. Bảng xử lý sự cố phía CoreIOT

| Triệu chứng | Kiểm tra theo thứ tự |
|---|---|
| Device không lên Active | Token đúng chưa; host/cổng từ *Check connectivity*; Wi-Fi 2.4 GHz; log `mqtt.state()` (mã 5 = token không hợp lệ) |
| Dữ liệu lên nhưng dashboard trống | Alias trỏ đúng device; tên khóa trong widget khớp telemetry; khoảng thời gian widget (Realtime) |
| Widget RPC báo timeout | Firmware subscribe `v1/devices/me/rpc/request/+`; trả lời `v1/devices/me/rpc/response/<id>`; thiết bị online |
| RPC từ rule chain không tới Node 2 | Rule chain đã gắn vào profile; relation `Manages`; Change originator đúng; debug ở từng nút |
| Alarm không tạo | Alarm rule gắn trên **profile** của device; key type = Time series; tên khóa đúng; gửi giá trị thật vượt ngưỡng |
| Không có notification | Rule trigger = Alarm đã bật; recipients có tài khoản bạn; kênh *Web* đã bật; xem chuông ở góc trên |
| OTA không bắt đầu | Gói đúng profile của device; đã gán; thiết bị đã subscribe attributes/hỏi `fw_*`; buffer đủ lớn; đúng partition scheme |
| OTA tải rồi không đổi phiên bản | `FW_VERSION` bản mới chưa đổi; chưa báo `current_fw_version`; checksum lỗi |
| Dữ liệu bị lệch múi giờ | CoreIOT hiển thị theo múi giờ tài khoản; đặt múi giờ ở profile người dùng nếu cần |
| Mất kết nối ngẫu nhiên | Hai thiết bị dùng chung token/Client ID; keepAlive; sóng Wi-Fi yếu (`rssi` thấp) |

---

## 17. Đồng bộ với tài liệu khác (việc cần cập nhật)

- `PROJECT.md` mục 7.3: thêm RPC `cloudAlert` (mục 9.2 của tài liệu này).
- `PROJECT.md` mục 7.2: thêm khóa OTA `current_fw_title`, `current_fw_version`, `fw_state`, `fw_error` trong telemetry khi OTA.
- `PROJECT.md` mục 8.6: ghi rõ RuleChain dùng **alarm-driven** (Alarm Created/Cleared) thay vì kiểm tra từng tin.
- `CLAUDE.md`: khi làm M4, M8 yêu cầu Claude đọc tài liệu này trước.
- Báo cáo: nêu rõ phần kế thừa (OhStem Green House, template/widget CoreIOT, ý tưởng RuleChain từ tài liệu ThingsBoard) và phần tự làm.

---

## 18. Tài liệu tham khảo dùng cho phần này

- ThingsBoard – MQTT Device API (Getting connected, topics, RPC, attributes, firmware): https://thingsboard.io/docs/reference/mqtt-api/getting-connected/
- ThingsBoard – OTA updates: https://thingsboard.io/docs/user-guide/ota-updates
- ThingsBoard – Rule engine action nodes (Change originator, RPC call request, Create alarm…): https://thingsboard.io/docs/reference/rule-engine/nodes/action/
- ThingsBoard – Ví dụ quản lý alarm và RPC (Change originator qua quan hệ `Manages`): https://thingsboard.io/docs/edge/recipes/manage-alarms-rpc-requests/
- CoreIOT platform: https://app.coreiot.io và slide workshop CoreIOT/OhStem (tạo device, lấy token, widget telemetry/attribute/RPC, dashboard)
- OhStem Education Wiki – Yolo UNO với IoT: https://docs.ohstem.vn/en/latest/yolo_uno/arduino_yolo_uno/iot_yolouno.html
