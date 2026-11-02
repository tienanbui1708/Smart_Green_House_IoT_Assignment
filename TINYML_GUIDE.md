# TINYML_GUIDE.md – T04 + T05: thu dữ liệu, huấn luyện, export, chạy trên Node 1

> Phục vụ **T04** (thông tin nội suy từ AI chạy trên thiết bị) và **T05** (công cụ thu thập dữ liệu, huấn luyện, export model TinyML). Tổng 2.0 điểm.
> Đọc cùng `PROJECT.md` mục 8.1, `CLAUDE.md` mục 4.5 và 5, `SETUP_FIRMWARE.md` (M0–M2 phải xong trước).
> `CẦN XÁC MINH` = chưa chạy thử trên kit thật. Mã Python trong file này là khung chuẩn, **chưa được chạy**: bạn phải chạy thử và sửa lỗi nếu có.

---

## 0. Tóm tắt quyết định

| Hạng mục | Quyết định |
|---|---|
| Bài toán | Phân loại trạng thái nhà kính từ chuỗi cảm biến: 4 lớp |
| Đầu vào | Cửa sổ trượt 5 mẫu × 3 đặc trưng (nhiệt độ, độ ẩm không khí, độ ẩm đất) = 15 giá trị. Lấy mẫu 2 s nên mỗi cửa sổ phủ 8 s |
| Mô hình | MLP nhỏ: 15 → 32 → 16 → 4 (ReLU, Softmax). Rất nhỏ, vài nghìn tham số |
| Dữ liệu | **Thu từ chính kit** bằng `collect_serial.py`; có thêm dữ liệu tổng hợp cho lớp "cảm biến lỗi", **tách bạch** trong báo cáo |
| Chia tập | **Theo phiên thu** (không chia ngẫu nhiên từng mẫu, tránh rò rỉ) |
| Export | Bước 1 an toàn: **TFLite float32**. Bước 2 nâng cấp: **INT8** nếu chạy được trên thiết bị. Báo cáo ghi rõ bản nào chạy thật |
| Chạy trên thiết bị | `Chirale_TensorFlowLite` (cài từ Library Manager, có kiến trúc esp32) và/hoặc `ArduTFLite` (lớp bọc API đơn giản) |
| Kiểm chứng | **Parity test**: cùng vector đầu vào, kết quả trên thiết bị phải khớp Python |
| Tác động lên hệ thống | Nhãn `ai_class` vào telemetry; khi `ai_class = 3` (cảm biến lỗi) **không gửi** giá trị đo hỏng lên Cloud |

Lý do chọn float32 trước: mô hình nhỏ (khoảng 1.1 nghìn trọng số ≈ vài KB) không cần nén để vừa bộ nhớ ESP32-S3. INT8 vẫn nên làm để có số liệu so sánh kích thước/độ chính xác/thời gian cho Chương 4, nhưng không để nó chặn T04.

---

## 1. Chuẩn bị môi trường Python (trên PC)

```bash
python -m venv venv
# Windows: venv\Scripts\activate      Linux/macOS: source venv/bin/activate
pip install numpy pandas scikit-learn matplotlib pyserial tensorflow
pip freeze > ml/requirements.txt      # ghim phiên bản vào repo
```

Ghi phiên bản Python/TensorFlow vào README. Nếu `pip install tensorflow` lỗi trên phiên bản Python của bạn: dùng Python 3.10 hoặc 3.11 (**CẦN XÁC MINH** phiên bản hỗ trợ).

Cấu trúc thư mục `ml/`:

```
ml/
├─ collect_serial.py
├─ train.py
├─ export_tflite.py
├─ requirements.txt
├─ data/          # CSV thu được (giữ nguyên bản gốc, không sửa tay)
└─ out/           # kết quả train/export (ảnh, model, meta)
```

---

## 2. Thiết kế bài toán

### 2.1 Bốn lớp

| Lớp | Tên | Ý nghĩa | Cách tạo điều kiện khi thu |
|---|---|---|---|
| 0 | `normal` | Vi khí hậu bình thường | Để yên trong phòng; cảm biến đất ở mức ẩm vừa (đất ẩm thật hoặc nhúng đầu dò vừa phải) |
| 1 | `soil_dry` | Đất khô cần tưới | Cảm biến đất trong không khí / đất khô; DHT20 ở điều kiện bình thường |
| 2 | `hot_humid` | Quá nóng hoặc quá ẩm | Đưa nguồn ấm lại gần DHT20 (cốc nước ấm, không dùng nguồn nóng nguy hiểm), hoặc hà hơi/đặt khăn ẩm gần để tăng độ ẩm |
| 3 | `sensor_fault` | Cảm biến lỗi/dữ liệu phi lý | **Thật:** rút dây DHT20 khi đang chạy (firmware gửi giá trị sentinel). **Tổng hợp:** giá trị phi lý (100 °C, 100 %), kẹt giá trị, nhảy vọt |

Lưu ý an toàn: không dùng máy sấy tóc ở khoảng cách gần, không đưa nguồn nhiệt vào sát linh kiện, không làm ướt mạch.

Ngưỡng thực tế của lớp 0/1/2 phải **dựa trên số đo thật** của bạn (ví dụ phòng bạn 27–31 °C): chọn điều kiện khi thu sao cho lớp có ý nghĩa nhưng vẫn khác biệt rõ. Ghi vào bảng (điền sau khi thu):

| Lớp | Khoảng T (°C) | Khoảng H (%) | Khoảng soil (%) |
|---|---|---|---|
| 0 | | | |
| 1 | | | |
| 2 | | | |
| 3 | (sentinel hoặc phi lý) | | |

### 2.2 Chuẩn hóa đặc trưng (hằng số cố định, không học từ dữ liệu)

```
t_norm = (T - 25) / 25
h_norm = (H - 50) / 50
s_norm = (soil - 50) / 50
```

Dùng hằng số cố định để firmware và Python **y hệt nhau** và không có "rò rỉ" thống kê. Thứ tự vector đầu vào (theo thời gian, cũ → mới):

```
[t_{k-4}, h_{k-4}, s_{k-4},  t_{k-3}, h_{k-3}, s_{k-3},  ...,  t_k, h_k, s_k]
```

### 2.3 Giá trị sentinel khi cảm biến lỗi

Khi `DHT.read()` thất bại hoặc ngoài dải hợp lý (T < −10 hoặc > 80; H < 0 hoặc > 100), firmware đặt **T = −40, H = 0** cho mẫu đó (soil vẫn đọc bình thường). Đây là "chữ ký lỗi" nhất quán cho cả thu dữ liệu lẫn suy luận. Ghi vào `config.h` (`SENTINEL_T`, `SENTINEL_H`).

### 2.4 Trung thực về giới hạn (nội dung cho báo cáo)

- Nhiệt độ/độ ẩm trong phòng thay đổi chậm; trong cửa sổ 8 s mô hình chủ yếu dựa vào **giá trị tuyệt đối** hơn là xu hướng. Vì vậy một luật ngưỡng đơn giản cũng có thể làm khá tốt. Hãy tính **baseline luật ngưỡng** để so sánh và nêu trung thực trong báo cáo (mục 6.4).
- Dữ liệu từ ít phiên thu, một phòng, một khoảng thời gian → khả năng tổng quát hóa hạn chế. Nói rõ.
- Lớp "lỗi" có một phần tổng hợp; đừng dùng nó để tuyên bố độ chính xác tuyệt đối.

---

## 3. Firmware chế độ THU DỮ LIỆU (Node 1)

Mục tiêu: Node 1 in ra Serial mỗi 2 s một dòng CSV để Python thu. Làm một sketch riêng `tools/collect_firmware` hoặc cờ `COLLECT_MODE` trong `config.h` (khuyến nghị: sketch riêng cho đơn giản, dùng lại đoạn đọc cảm biến ở M1).

**Định dạng dòng (bắt buộc):**

```
D,<millis>,<temperature>,<humidity>,<soil_percent>
```

ví dụ `D,123456,28.4,64.2,41`. Dòng không bắt đầu bằng `D,` (log khác) sẽ bị script bỏ qua. Chu kỳ 2000 ms, **cùng chu kỳ với lúc chạy thật** (rất quan trọng: nếu thu mỗi 1 s mà chạy thật mỗi 2 s thì cửa sổ 5 mẫu sẽ không khớp).

Phác thảo vòng lặp (viết gọn, bạn tích hợp vào task sensor của M2):

```cpp
// đọc DHT20 + soil → áp dụng quy tắc sentinel → in 1 dòng
float t = NAN, h = NAN; bool ok = (dht.read() == DHT20_OK);
if (ok) { t = dht.getTemperature(); h = dht.getHumidity(); }
if (!ok || t < -10 || t > 80 || h < 0 || h > 100) { t = SENTINEL_T; h = SENTINEL_H; }
int soil = readSoilPercent();                 // đã hiệu chỉnh ở M1
Serial.printf("D,%lu,%.1f,%.1f,%d\n", millis(), t, h, soil);
vTaskDelay(pdMS_TO_TICKS(2000));
```

(Lưu ý `DHT20_OK` là tên hằng của thư viện Rob Tillaart; nếu thư viện bạn dùng khác thì kiểm tra trả về theo thư viện đó. **CẦN XÁC MINH**.)

---

## 4. `ml/collect_serial.py`

```python
"""Thu dữ liệu từ Serial của Node 1 -> CSV có nhãn.
Ví dụ: python collect_serial.py --port COM5 --label 0 --session s01 --minutes 8
"""
import argparse, csv, os, time
import serial


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", required=True)
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--label", type=int, required=True, choices=[0, 1, 2, 3])
    ap.add_argument("--session", required=True, help="tên phiên, ví dụ s01")
    ap.add_argument("--minutes", type=float, default=8)
    ap.add_argument("--note", default="", help="ghi chú điều kiện thu")
    ap.add_argument("--outdir", default="data")
    a = ap.parse_args()

    os.makedirs(a.outdir, exist_ok=True)
    path = os.path.join(a.outdir, f"{a.session}_L{a.label}.csv")
    if os.path.exists(path):
        raise SystemExit(f"File đã tồn tại, không ghi đè: {path}")

    end = time.time() + a.minutes * 60
    good = bad = 0
    with serial.Serial(a.port, a.baud, timeout=3) as ser, open(path, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["session", "label", "pc_time", "ms", "t", "h", "soil"])
        ser.reset_input_buffer()
        print(f"Thu {a.minutes} phút -> {path}  (Ctrl+C để dừng sớm)")
        try:
            while time.time() < end:
                line = ser.readline().decode(errors="ignore").strip()
                if not line.startswith("D,"):
                    continue
                parts = line.split(",")
                try:
                    ms, t, h, s = int(parts[1]), float(parts[2]), float(parts[3]), float(parts[4])
                except (ValueError, IndexError):
                    bad += 1
                    continue
                w.writerow([a.session, a.label, f"{time.time():.3f}", ms, t, h, s])
                f.flush()
                good += 1
                if good % 15 == 0:
                    print(f"  {good} mẫu (lỗi dòng: {bad})  T={t} H={h} soil={s}")
        except KeyboardInterrupt:
            print("Dừng sớm.")
    with open(os.path.join(a.outdir, f"{a.session}_L{a.label}.note.txt"), "w", encoding="utf-8") as nf:
        nf.write(f"label={a.label}\nminutes={a.minutes}\nsamples={good}\nbad_lines={bad}\nnote={a.note}\n")
    print(f"Xong: {good} mẫu, {bad} dòng lỗi.")


if __name__ == "__main__":
    main()
```

Quy tắc khi thu:
- Một file CSV = **một phiên, một nhãn**, điều kiện ổn định trong suốt phiên. Muốn đổi điều kiện → kết thúc phiên, bắt đầu phiên mới (tên mới).
- Không sửa tay file CSV gốc. Nếu phát hiện phiên hỏng, đổi tên thêm hậu tố `_BAD` (đừng xóa) và nói rõ trong báo cáo.
- Ghi `--note` đủ để người khác lặp lại (ví dụ "cốc nước 40 °C đặt cách DHT20 5 cm").

---

## 5. Kế hoạch thu dữ liệu

Chu kỳ 2 s → 8 phút ≈ 240 mẫu ≈ 236 cửa sổ. Mục tiêu tối thiểu (có thể tăng):

| Lớp | Số phiên (khác nhau) | Thời lượng mỗi phiên | Mẫu ước tính |
|---|---|---|---|
| 0 normal | ≥ 4 (ví dụ sáng/trưa/tối, hôm khác nhau) | 8–10 phút | ~1000 |
| 1 soil_dry | ≥ 3 | 8 phút | ~700 |
| 2 hot_humid | ≥ 4 (nóng và ẩm tách phiên) | 8 phút | ~900 |
| 3 sensor_fault (thật) | ≥ 3 (rút/cắm DHT20 nhiều lần trong phiên) | 5 phút | ~300 |

Mỗi lớp **cần ≥ 3 phiên** để chia train/val/test theo phiên (script train kiểm tra điều này). Thu vào các thời điểm/điều kiện khác nhau (ánh sáng, nhiệt độ phòng khác nhau) để mô hình bớt "học thuộc" một lần đo. Lưu `docs/evidence/T05/collect_*.png` (ảnh thiết lập và màn hình chạy script).

Bảng theo dõi phiên (điền khi thu, đưa vào báo cáo):

| Phiên | Nhãn | Thời điểm | Điều kiện | Số mẫu |
|---|---|---|---|---|
| s01 | 0 | | | |

---

## 6. `ml/train.py`

```python
"""Huấn luyện MLP nhỏ phân loại 4 lớp. Chạy: python train.py"""
import glob, json, os, random
import numpy as np, pandas as pd
import tensorflow as tf
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from sklearn.metrics import confusion_matrix, classification_report
from sklearn.utils.class_weight import compute_class_weight

W = 5                                   # số mẫu trong cửa sổ
LABELS = ["normal", "soil_dry", "hot_humid", "sensor_fault"]
SENT_T, SENT_H = -40.0, 0.0
SEED = 42
random.seed(SEED); np.random.seed(SEED); tf.random.set_seed(SEED)
os.makedirs("out", exist_ok=True)


def norm(t, h, s):
    return (t - 25.0) / 25.0, (h - 50.0) / 50.0, (s - 50.0) / 50.0


def windows(df):
    t, h, s = norm(df.t.values.astype(float), df.h.values.astype(float), df.soil.values.astype(float))
    feat = np.stack([t, h, s], axis=1)
    return np.array([feat[i - W + 1:i + 1].reshape(-1) for i in range(W - 1, len(df))], np.float32)


def load_real():
    sess = {}                           # file -> (X, label)
    for p in sorted(glob.glob("data/*_L?.csv")):
        if "_BAD" in p:
            continue
        df = pd.read_csv(p)
        if len(df) < W + 5:
            continue
        sess[p] = (windows(df), int(df["label"].iloc[0]))
    return sess


def split_by_session(sess):
    train, val, test = [], [], []
    for lab in range(4):
        files = [p for p, (_, l) in sess.items() if l == lab]
        if len(files) < 3:
            raise SystemExit(f"Lớp {lab} ({LABELS[lab]}) cần >= 3 phiên, hiện có {len(files)}")
        random.shuffle(files)
        test.append(files[0]); val.append(files[1]); train += files[2:]
        test_f, val_f = files[0], files[1]
    return train, val, test


def stack(files, sess):
    X = np.concatenate([sess[p][0] for p in files]); y = np.concatenate([[sess[p][1]] * len(sess[p][0]) for p in files])
    return X, y


def synthetic_faults(n):
    """Cửa sổ lỗi TỔNG HỢP (phải nêu riêng trong báo cáo)."""
    out = []
    for _ in range(n):
        kind = random.choice(["spike", "stuck", "sentinel", "negative", "jump"])
        T = np.random.uniform(24, 34, W); H = np.random.uniform(40, 85, W); S = np.random.uniform(10, 80, W)
        if kind == "spike":
            T[:] = 100.0; H[:] = 100.0
        elif kind == "stuck":
            T[:] = T[0]; H[:] = H[0]; S[:] = S[0]
        elif kind == "sentinel":
            k = random.randint(1, W); T[-k:] = SENT_T; H[-k:] = SENT_H
        elif kind == "negative":
            T[:] = np.random.uniform(-40, -5, W); H[:] = np.random.uniform(-10, 0, W)
        else:
            i = random.randint(1, W - 1); T[i:] += np.random.choice([-1, 1]) * np.random.uniform(25, 60)
        t, h, s = norm(T, H, S)
        out.append(np.stack([t, h, s], axis=1).reshape(-1))
    return np.array(out, np.float32)


def main():
    sess = load_real()
    tr_f, va_f, te_f = split_by_session(sess)
    Xtr, ytr = stack(tr_f, sess); Xva, yva = stack(va_f, sess); Xte, yte = stack(te_f, sess)

    # dữ liệu tổng hợp cho lớp 3, tách riêng khỏi dữ liệu thật
    Xsyn = synthetic_faults(600); ysyn = np.full(len(Xsyn), 3)
    ntr = int(0.6 * len(Xsyn)); nva = int(0.2 * len(Xsyn))
    Xtr = np.concatenate([Xtr, Xsyn[:ntr]]); ytr = np.concatenate([ytr, ysyn[:ntr]])
    Xva = np.concatenate([Xva, Xsyn[ntr:ntr + nva]]); yva = np.concatenate([yva, ysyn[ntr:ntr + nva]])
    Xsyn_te, ysyn_te = Xsyn[ntr + nva:], ysyn[ntr + nva:]

    print("train/val/test(real):", len(Xtr), len(Xva), len(Xte), "| test(synthetic):", len(Xsyn_te))
    cw = compute_class_weight("balanced", classes=np.arange(4), y=ytr)
    model = tf.keras.Sequential([
        tf.keras.layers.Input(shape=(W * 3,)),
        tf.keras.layers.Dense(32, activation="relu"),
        tf.keras.layers.Dense(16, activation="relu"),
        tf.keras.layers.Dense(4, activation="softmax"),
    ])
    model.compile(optimizer="adam", loss="sparse_categorical_crossentropy", metrics=["accuracy"])
    es = tf.keras.callbacks.EarlyStopping(patience=8, restore_best_weights=True)
    hist = model.fit(Xtr, ytr, validation_data=(Xva, yva), epochs=100, batch_size=32,
                     class_weight=dict(enumerate(cw)), callbacks=[es], verbose=2)
    model.save("out/model.keras")
    model.summary()

    # đồ thị
    fig, ax = plt.subplots(1, 2, figsize=(10, 3.5))
    ax[0].plot(hist.history["accuracy"], label="train"); ax[0].plot(hist.history["val_accuracy"], label="val"); ax[0].set_title("Accuracy"); ax[0].legend()
    ax[1].plot(hist.history["loss"], label="train"); ax[1].plot(hist.history["val_loss"], label="val"); ax[1].set_title("Loss"); ax[1].legend()
    plt.tight_layout(); plt.savefig("out/curves.png", dpi=150); plt.close()

    # đánh giá trên dữ liệu thật và tổng hợp (tách riêng)
    report = []
    for name, X, y in [("TEST_REAL", Xte, yte), ("TEST_SYNTHETIC_FAULT", Xsyn_te, ysyn_te)]:
        pred = model.predict(X, verbose=0).argmax(1)
        acc = float((pred == y).mean())
        cm = confusion_matrix(y, pred, labels=[0, 1, 2, 3])
        report.append(f"== {name}: n={len(y)} accuracy={acc:.4f}\n{cm}\n" +
                      classification_report(y, pred, labels=[0, 1, 2, 3], target_names=LABELS, zero_division=0))
        plt.figure(figsize=(4.5, 4)); plt.imshow(cm, cmap="Blues"); plt.title(f"Confusion - {name}")
        plt.xticks(range(4), LABELS, rotation=30); plt.yticks(range(4), LABELS)
        for i in range(4):
            for j in range(4):
                plt.text(j, i, cm[i, j], ha="center", va="center")
        plt.tight_layout(); plt.savefig(f"out/confusion_{name.lower()}.png", dpi=150); plt.close()

    # baseline luật ngưỡng (để so sánh trung thực) - CHỈNH NGƯỠNG THEO SỐ ĐO THẬT CỦA BẠN
    def rule(x):
        t = x[-3] * 25 + 25; h = x[-2] * 50 + 50; s = x[-1] * 50 + 50
        if t < -10 or t > 80 or h < 0 or h > 100: return 3
        if s < 20: return 1
        if t > 33 or h > 80: return 2
        return 0
    base = np.array([rule(x) for x in Xte]); base_acc = float((base == yte).mean())
    report.append(f"== BASELINE rule-threshold on TEST_REAL: accuracy={base_acc:.4f}")
    open("out/report.txt", "w", encoding="utf-8").write("\n".join(report))
    print("\n".join(report))

    np.savez("out/splits.npz", Xtr=Xtr, ytr=ytr, Xte=Xte, yte=yte, Xsyn_te=Xsyn_te, ysyn_te=ysyn_te)
    json.dump({"window": W, "labels": LABELS, "norm": {"t": [25, 25], "h": [50, 50], "s": [50, 50]},
               "sentinel": {"t": SENT_T, "h": SENT_H}, "train_files": tr_f, "val_files": va_f, "test_files": te_f},
              open("out/meta.json", "w", encoding="utf-8"), ensure_ascii=False, indent=2)


if __name__ == "__main__":
    main()
```

### 6.1 Cách chạy

```bash
cd ml
python train.py
```

### 6.2 Kết quả mong đợi

- Dòng `train/val/test(real)` cho biết số cửa sổ mỗi tập.
- Có `out/curves.png`, `out/confusion_test_real.png`, `out/confusion_test_synthetic_fault.png`, `out/report.txt`, `out/meta.json`, `out/model.keras`.
- Nếu độ chính xác tập test thật ≈ 100 % → **nghi ngờ trước khi vui**: kiểm tra có chia theo phiên chưa, các lớp có quá dễ tách (ví dụ nhiệt độ khác hẳn giữa các lớp)? Đây là điểm báo cáo mẫu khóa trước bị đánh giá thấp. Nói thẳng nếu bài toán dễ.

### 6.3 Nếu lỗi/không ổn

| Hiện tượng | Xử lý |
|---|---|
| `Lớp k cần >= 3 phiên` | Thu thêm phiên cho lớp đó |
| Lớp 1 hoặc 2 nhầm lẫn nhiều | Điều kiện thu chưa đủ khác biệt; thu lại với khác biệt rõ hơn |
| Loss không giảm | Kiểm tra chuẩn hóa, dữ liệu rỗng/NaN, nhãn sai |
| Val ≠ test rất khác | Quá ít phiên; thu thêm vào các thời điểm khác nhau |

### 6.4 Baseline luật ngưỡng

Chỉnh hàm `rule()` theo ngưỡng thật của bạn, so sánh `accuracy` mô hình với baseline. Trong báo cáo, nếu baseline ngang mô hình: nói rõ "bài toán phân biệt chủ yếu bằng ngưỡng; giá trị của TinyML ở đây là minh họa quy trình và khả năng mở rộng sang đặc trưng phức tạp hơn". Nếu mô hình vượt baseline (ví dụ ở các ca biên hoặc lỗi kẹt giá trị), nêu số liệu cụ thể.

---

## 7. `ml/export_tflite.py`

```python
"""Export TFLite float32 + INT8, so sánh, sinh model.h và test_vectors.h.
Chạy: python export_tflite.py --use float     (hoặc --use int8)
"""
import argparse, os
import numpy as np, tensorflow as tf

ap = argparse.ArgumentParser()
ap.add_argument("--use", choices=["float", "int8"], default="float", help="bản ghi vào model.h")
a = ap.parse_args()
os.makedirs("out", exist_ok=True)

model = tf.keras.models.load_model("out/model.keras")
d = np.load("out/splits.npz")
Xtr, Xte, yte = d["Xtr"], d["Xte"], d["yte"]

# ---- float32 ----
conv = tf.lite.TFLiteConverter.from_keras_model(model)
f32 = conv.convert(); open("out/model_float.tflite", "wb").write(f32)

# ---- INT8 (full integer) ----
def rep():
    for i in range(min(500, len(Xtr))):
        yield [Xtr[i:i + 1].astype(np.float32)]
conv8 = tf.lite.TFLiteConverter.from_keras_model(model)
conv8.optimizations = [tf.lite.Optimize.DEFAULT]
conv8.representative_dataset = rep
conv8.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
conv8.inference_input_type = tf.int8
conv8.inference_output_type = tf.int8
i8 = conv8.convert(); open("out/model_int8.tflite", "wb").write(i8)


def run_tflite(buf, X, quant):
    it = tf.lite.Interpreter(model_content=buf); it.allocate_tensors()
    inp, outd = it.get_input_details()[0], it.get_output_details()[0]
    preds = []
    for x in X:
        v = x[None, :].astype(np.float32)
        if quant:
            s, z = inp["quantization"]; v = np.round(v / s + z).astype(np.int8)
        it.set_tensor(inp["index"], v); it.invoke()
        o = it.get_tensor(outd["index"])[0]
        preds.append(int(np.argmax(o)))
    return np.array(preds), inp["quantization"], outd["quantization"]


keras_pred = model.predict(Xte, verbose=0).argmax(1)
pf, _, _ = run_tflite(f32, Xte, False)
pq, qin, qout = run_tflite(i8, Xte, True)
print(f"Keras acc     : {(keras_pred == yte).mean():.4f}")
print(f"TFLite float  : {(pf == yte).mean():.4f}  size={len(f32)} bytes")
print(f"TFLite int8   : {(pq == yte).mean():.4f}  size={len(i8)} bytes  input(scale,zp)={qin} output(scale,zp)={qout}")
print(f"Khớp float vs Keras: {(pf == keras_pred).mean():.4f}  | int8 vs Keras: {(pq == keras_pred).mean():.4f}")


def write_header(path, data, name):
    with open(path, "w") as f:
        f.write("#pragma once\n")
        f.write(f"alignas(8) const unsigned char {name}[] = {{\n")
        for i in range(0, len(data), 12):
            f.write("  " + ", ".join(f"0x{b:02x}" for b in data[i:i + 12]) + ",\n")
        f.write(f"}};\nconst unsigned int {name}_len = {len(data)};\n")


chosen = f32 if a.use == "float" else i8
write_header("out/model.h", chosen, "model_tflite")
print(f"Đã ghi out/model.h ({a.use}), {len(chosen)} bytes")

# ---- vector kiểm tra cho parity test trên thiết bị ----
n = 20
idx = np.random.RandomState(1).choice(len(Xte), n, replace=False)
with open("out/test_vectors.h", "w") as f:
    f.write("#pragma once\nconst int TV_N = %d;\nconst int TV_DIM = %d;\n" % (n, Xte.shape[1]))
    f.write("const float TV_X[%d][%d] = {\n" % (n, Xte.shape[1]))
    for i in idx:
        f.write("  {" + ", ".join(f"{v:.6f}f" for v in Xte[i]) + "},\n")
    f.write("};\nconst int TV_EXPECT[%d] = {" % n + ", ".join(str(int(keras_pred[i])) for i in idx) + "};\n")
print("Đã ghi out/test_vectors.h")
```

Kết quả mong đợi: in ra độ chính xác Keras, TFLite float, TFLite INT8 cùng kích thước từng bản. `model_float.tflite` thường vài KB; INT8 nhỏ hơn. Lưu số này làm bảng "float32 vs INT8" cho Chương 4. Nếu INT8 tụt độ chính xác đáng kể so với float: **dùng float trên thiết bị** và ghi rõ lý do.

Chép `out/model.h` sang `node1_sensor/model.h` và `out/test_vectors.h` sang thư mục sketch parity test. **Đừng** commit `model.h` nếu nó rất lớn (với mô hình này thì vài KB, commit được).

---

## 8. Chạy trên thiết bị (Node 1)

### 8.1 Cài thư viện (Library Manager)

Tìm và cài **`Chirale_TensorFlowLite`** (thư viện TensorFlow Lite Micro cho kiến trúc esp32, tác giả Chirale) và, nếu muốn API đơn giản, **`ArduTFLite`** (lớp bọc của cùng nhóm, API kiểu Arduino). Ghi phiên bản. Hai thư viện được liệt kê trong danh mục Arduino và hỗ trợ kiến trúc esp32; **CẦN XÁC MINH** biên dịch được với core OhStem hiện có (có thể gặp lỗi tùy phiên bản core). Nếu một thư viện không biên dịch được, thử thư viện còn lại hoặc xem mục 11.

### 8.2 Sketch kiểm tra song song (parity test)

Tạo sketch `tools/tinyml_parity` với `model.h` và `test_vectors.h` đặt cạnh. Khung dùng `ArduTFLite` (**API theo trí nhớ, CẦN XÁC MINH tên hàm bằng ví dụ đi kèm thư viện**):

```cpp
#include <ArduTFLite.h>
#include "model.h"
#include "test_vectors.h"

constexpr int kArenaSize = 16 * 1024;           // tăng nếu AllocateTensors thất bại
alignas(16) static uint8_t tensorArena[kArenaSize];

void setup() {
  Serial.begin(115200); delay(1500);
  if (!modelInit(model_tflite, tensorArena, kArenaSize)) { Serial.println("modelInit FAILED"); while (1) delay(1000); }
  int ok = 0;
  uint32_t total_us = 0;
  for (int i = 0; i < TV_N; i++) {
    for (int k = 0; k < TV_DIM; k++) modelSetInput(TV_X[i][k], k);
    uint32_t t0 = micros();
    modelRunInference();
    uint32_t dt = micros() - t0; total_us += dt;
    int best = 0; float bv = -1e9;
    for (int c = 0; c < 4; c++) { float v = modelGetOutput(c); if (v > bv) { bv = v; best = c; } }
    bool match = (best == TV_EXPECT[i]);
    ok += match;
    Serial.printf("vec %2d: device=%d expect=%d %s  (%u us)\n", i, best, TV_EXPECT[i], match ? "OK" : "MISMATCH", dt);
  }
  Serial.printf("PARITY: %d/%d khớp, TB %u us/lần suy luận\n", ok, TV_N, total_us / TV_N);
}
void loop() {}
```

**Kết quả mong đợi:** `PARITY: 20/20 khớp` (với bản float). Với INT8, vài vector biên có thể lệch; ghi lại. Ghi **thời gian suy luận** (µs) và kích thước `tensorArena` đủ dùng (tăng/giảm để tìm giá trị nhỏ nhất chạy được rồi + dư 20 %). Lưu log → `docs/evidence/T04/parity_log.txt`.

Lưu ý: nếu dùng mô hình INT8 với ArduTFLite, nó có thể không tự lượng tử hóa đầu vào float. Khi đó dùng thẳng API của `Chirale_TensorFlowLite` (đọc `scale/zero_point` từ tensor đầu vào, quy đổi `q = round(x/scale + zp)` rồi ghi vào bộ đệm int8). **CẦN XÁC MINH**. Nếu rắc rối, ở lại với float32.

### 8.3 `task_tinyml` trong firmware (đặc tả cho Claude làm ở M7)

| Mục | Quy định |
|---|---|
| Đầu vào | Mỗi lần `task_sensor` ghi mẫu mới → give semaphore `new_data` |
| Bộ đệm | Vòng 5 mẫu `{t,h,soil}` (đã áp sentinel khi lỗi), **cùng chuẩn hóa** như mục 2.2 |
| Khởi động | 4 mẫu đầu chưa đủ cửa sổ → `ai_class = 255` ("đang khởi động") |
| Suy luận | Khi đủ 5 mẫu: nạp 15 giá trị → chạy → lấy `argmax`; log thời gian (µs), nhãn, xác suất |
| Làm mượt | Chọn nhãn theo **đa số của 3 lần gần nhất** để tránh nhấp nháy; vẫn log nhãn thô |
| Kết quả | Ghi `ai_class` vào `shared_data` (mutex); đặt cờ `data_reliable = (ai_class != 3)` |
| Hệ quả | Telemetry luôn gửi `ai_class`. Khi `ai_class == 3` **không gửi** `temperature/humidity/soil` hỏng lên Cloud, gửi thêm `sensor_fault: true`. LCD Node 1 hiện `FAULT` |
| Bộ nhớ | `tensorArena` tĩnh `alignas(16)`; log kích thước khi khởi tạo |
| Ưu tiên | 2; stack bắt đầu 8192; dùng `vTaskDelay`/chờ semaphore có timeout |

Cập nhật liên quan: `PROJECT.md` mục 8.1 (hệ quả `ai_class = 3`) và mục 7.2 (khóa `sensor_fault`).

---

## 9. Đo đạc và minh chứng

| Chỉ số | Cách lấy | File |
|---|---|---|
| Số mẫu/phiên/lớp | Đếm từ `data/*.csv` | `docs/measurements/dataset_summary.csv` |
| Accuracy, confusion matrix (dữ liệu thật) | `out/report.txt`, `out/confusion_test_real.png` | `docs/evidence/T05/` |
| Accuracy (dữ liệu lỗi tổng hợp) | `out/confusion_test_synthetic_fault.png` | `docs/evidence/T05/` |
| Đường cong học | `out/curves.png` | `docs/evidence/T05/` |
| So sánh baseline luật ngưỡng | `out/report.txt` | |
| Kích thước model float/INT8, độ chính xác từng bản | in ra từ `export_tflite.py` | `docs/measurements/model_sizes.txt` |
| Thời gian suy luận trên thiết bị | parity log (µs trung bình) | `docs/evidence/T04/parity_log.txt` |
| Kích thước `tensorArena` tối thiểu | thử nghiệm giảm dần | ghi trong báo cáo |
| Khớp thiết bị vs Python | `PARITY: n/20` | `docs/evidence/T04/parity_log.txt` |
| Video suy luận thời gian thực | quay: đổi điều kiện (làm ấm, cho đất vào nước, rút dây DHT20) → `ai_class` đổi trên LCD/dashboard; khi lỗi cảm biến, dữ liệu hỏng không lên Cloud | `docs/evidence/T04/inference_demo.mp4` |

Bằng chứng T05 phải cho thấy **chuỗi công cụ chạy lại được** từ đầu: `README` ghi rõ lệnh `collect → train → export`.

---

## 10. Nội dung trung thực cho báo cáo (T04, T05)

- Mô tả dữ liệu: bao nhiêu phiên, điều kiện, thời điểm; phần **thật** và phần **tổng hợp** (chỉ lớp lỗi).
- Cách chia tập: theo phiên; số cửa sổ mỗi tập.
- Kết quả trên dữ liệu thật và dữ liệu tổng hợp **trình bày riêng**.
- So sánh với baseline luật ngưỡng và nhận xét thẳng.
- Giới hạn: một phòng, ít phiên, điều kiện đơn giản; chưa kiểm thử dài hạn.
- Quy trình triển khai: float32 vs INT8 (kích thước, độ chính xác, thời gian), bản nào chạy trên thiết bị.
- Tác động hệ thống: dữ liệu lỗi bị chặn khỏi Cloud (nêu số lần chặn trong video/log nếu có).
- Không dùng từ tuyệt đối ("hoàn hảo", "100 %") khi chưa kiểm tra rò rỉ.

---

## 11. Sự cố thường gặp

| Triệu chứng | Nguyên nhân/Cách xử lý |
|---|---|
| `pip install tensorflow` lỗi | Dùng Python 3.10/3.11; cập nhật pip |
| `collect_serial.py` không thấy dữ liệu | Sai cổng; Serial Monitor Arduino đang chiếm cổng (đóng nó); dòng không bắt đầu bằng `D,` |
| Số mẫu quá ít/không đều | Chu kỳ in không đúng 2 s; kiểm tra firmware thu |
| `AllocateTensors() failed` | Tensor arena nhỏ → tăng `kArenaSize`; thiếu toán tử (đăng ký op resolver đúng với thư viện) |
| Biên dịch lỗi thư viện TFLM với core | Thử thư viện còn lại (`Chirale_TensorFlowLite` ↔ cách khác); đổi phiên bản thư viện; giữ phương án dự phòng: suy luận MLP viết tay bằng C với trọng số float xuất từ Python (ghi rõ trong báo cáo) |
| Parity lệch nhiều | Chuẩn hóa khác nhau giữa Python và firmware; thứ tự đặc trưng sai; sai kiểu dữ liệu đầu vào (INT8) |
| Mô hình luôn ra lớp 0 | Mất cân bằng lớp; dữ liệu thu quá giống nhau giữa các lớp; thiếu phiên |
| Thiết bị reset khi suy luận | Stack task nhỏ → tăng lên 8192+; arena quá lớn gây thiếu RAM |
| Nhãn nhấp nháy | Dùng làm mượt đa số 3 lần |

Phương án dự phòng cuối cùng (nếu mọi thư viện TFLM đều không chạy): xuất trọng số từ Keras ra mảng C và tự cài `dense_relu`, `dense_softmax` trong firmware (phép nhân ma trận 15×32, 32×16, 16×4). Việc này vẫn đúng tinh thần "mô hình ML chạy trên thiết bị" nhưng **phải nói rõ** là không dùng TFLM. Hỏi thầy nếu muốn chắc.

---

## 12. Prompt mẫu giao cho Claude

**M7a – firmware thu dữ liệu:**
> Đọc `PROJECT.md`, `CLAUDE.md`, `TINYML_GUIDE.md`. Làm **M7a**: sketch `tools/collect_firmware` cho Node 1 in dòng `D,<ms>,<t>,<h>,<soil>` mỗi 2000 ms, áp quy tắc sentinel (T=−40, H=0 khi lỗi). Dùng địa chỉ LCD `0x__`, `RAW_DRY=__`, `RAW_WET=__`. Chỉ làm sketch này.

**M7b – công cụ Python:**
> Làm **M7b**: rà soát và chạy thử `ml/collect_serial.py`, `train.py`, `export_tflite.py` theo mục 4, 6, 7 của `TINYML_GUIDE.md`. Dữ liệu của tôi đã có trong `ml/data/` (`<mô tả>`). Sửa lỗi nếu có, giải thích kết quả, **không** tự điều chỉnh để ép độ chính xác cao.

**M7c – chạy trên thiết bị:**
> Làm **M7c**: tạo `tools/tinyml_parity` theo mục 8.2, rồi `task_tinyml` theo mục 8.3 cho Node 1. Thư viện: `<Chirale_TensorFlowLite|ArduTFLite>` phiên bản `<...>`. Bản model: `<float|int8>`. Giữ nguyên chuẩn hóa mục 2.2. Nêu kế hoạch ngắn trước, đưa đầy đủ từng file, cách kiểm tra, minh chứng cần lưu.

---

## 13. Checklist T04 + T05

- [ ] ≥ 3 phiên cho mỗi lớp, CSV gốc không bị sửa tay.
- [ ] `train.py` chạy, có đồ thị, confusion matrix (thật và tổng hợp), `report.txt`.
- [ ] Đã so sánh với baseline luật ngưỡng và nhận xét trung thực.
- [ ] `export_tflite.py` chạy, có số liệu float vs INT8.
- [ ] Parity test trên thiết bị đạt, có log thời gian suy luận.
- [ ] `task_tinyml` chạy trong firmware; `ai_class` lên dashboard; dữ liệu lỗi bị chặn khỏi Cloud.
- [ ] Video suy luận thời gian thực.
- [ ] README có lệnh `collect → train → export` chạy lại được.
- [ ] Báo cáo tách rõ dữ liệu thật/tổng hợp và nêu giới hạn.

---

## Tài liệu tham khảo cho phần này

- Chirale_TensorFlowLite (thư viện TFLM cho Arduino, kiến trúc esp32): https://github.com/spaziochirale/Chirale_TensorFlowLite
- ArduTFLite (lớp bọc API kiểu Arduino): https://github.com/spaziochirale/ArduTFLite
- TensorFlow Lite for Microcontrollers: https://www.tensorflow.org/lite/microcontrollers
- Hướng dẫn chuyển đổi và lượng tử hóa TensorFlow Lite (post-training quantization).
