# tools/collect_firmware – Firmware thu dữ liệu TinyML (M7a, T05)

Firmware thu dữ liệu **chính là `node1_sensor` build với `COLLECT_MODE=1`**, không phải một sketch riêng.
Lý do: TASKS_SPEC 5.1 yêu cầu dùng lại *đúng* logic đọc DHT20, quy tắc sentinel và hiệu chỉnh đất của Node 1.
Arduino không cho hai sketch dùng chung file `.cpp`, nên một bản sao riêng sẽ sớm lệch với firmware thật.

Khi `COLLECT_MODE=1`, `taskSensor` in thêm mỗi 2000 ms một dòng (các log khác vẫn còn, script bỏ qua):

```
D,<millis>,<temperature>,<humidity>,<soil_percent>
```

Giá trị đã áp sentinel (T = −40, H = 0 khi DHT20 lỗi hoặc ngoài dải), giống hệt dữ liệu mà `task_tinyml` thấy.

## Build và nạp

**arduino-cli** (không cần sửa code):

```bash
arduino-cli compile -b OhStem:esp32:yolo_uno --library common \
  --build-property "compiler.cpp.extra_flags=-DCOLLECT_MODE=1" \
  --build-path build/node1_collect node1_sensor
arduino-cli upload -b OhStem:esp32:yolo_uno -p COM5 --input-dir build/node1_collect
```

**Arduino IDE**: trong `node1_sensor/config.h` đổi `#define COLLECT_MODE 0` thành `1`, nạp, thu xong thì **đổi lại 0** (không commit bản `1`).

## Thu dữ liệu

Đóng Serial Monitor (nó chiếm cổng), rồi chạy:

```bash
cd ml
python collect_serial.py --port COM5 --label 0 --session s01 --minutes 8 --note "để yên, phòng 28 °C"
```

Kế hoạch phiên: `TINYML_GUIDE.md` mục 5 (mỗi lớp ≥ 3 phiên, mỗi file = một phiên + một nhãn).
