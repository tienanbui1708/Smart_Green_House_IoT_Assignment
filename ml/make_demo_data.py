"""Sinh du lieu DEMO (TONG HOP) chi de chay thu duong ong collect -> train -> export khi chua co du lieu that.

KHONG phai du lieu thu tu kit. Ghi vao ml/data_demo/ (tach rieng ml/data/), ten file bat dau 'DEMO_'.
Model train tu du lieu nay KHONG duoc dung de bao cao do chinh xac (TASKS_SPEC 5.2).
  python make_demo_data.py            ->  ml/data_demo/DEMO_sXX_L<k>.csv
"""
import argparse
import csv
import os

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
SESSIONS_PER_CLASS = 4
SAMPLES = 120          # 4 phut x 2 s
PERIOD_MS = 2000

# (T, H, soil) trung binh cho moi lop; moi phien lech them mot chut de cac phien khac nhau.
CENTERS = {0: (28.5, 62.0, 48.0), 1: (28.5, 60.0, 10.0), 2: (34.5, 84.0, 45.0)}


def gen_session(rng, label, idx):
    if label == 3:  # cam bien loi: xen ke doan doc tot va doan sentinel (rut/cam day)
        t = rng.normal(28.0, 0.3, SAMPLES)
        h = rng.normal(62.0, 1.0, SAMPLES)
        s = rng.normal(45.0, 1.5, SAMPLES)
        on = np.zeros(SAMPLES, bool)
        for start in range(10, SAMPLES, 30):
            on[start:start + rng.integers(8, 18)] = True
        t[on], h[on] = -40.0, 0.0
        return t, h, s
    ct, ch, cs = CENTERS[label]
    ct += rng.normal(0, 0.8)
    ch += rng.normal(0, 2.5)
    cs += rng.normal(0, 3.0)
    drift = np.linspace(0, rng.normal(0, 0.6), SAMPLES)
    t = ct + drift + rng.normal(0, 0.15, SAMPLES)
    h = ch + 2 * drift + rng.normal(0, 0.6, SAMPLES)
    s = cs + rng.normal(0, 1.2, SAMPLES)
    return t, h, np.clip(s, 0, 100)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--outdir", default=os.path.join(HERE, "data_demo"))
    ap.add_argument("--seed", type=int, default=7)
    a = ap.parse_args()
    rng = np.random.default_rng(a.seed)
    os.makedirs(a.outdir, exist_ok=True)
    n = 0
    for label in range(4):
        for k in range(SESSIONS_PER_CLASS):
            name = f"DEMO_s{label}{k}"
            t, h, s = gen_session(rng, label, k)
            with open(os.path.join(a.outdir, f"{name}_L{label}.csv"), "w", newline="", encoding="utf-8") as f:
                w = csv.writer(f)
                w.writerow(["session", "label", "pc_time", "ms", "t", "h", "soil"])
                for i in range(SAMPLES):
                    w.writerow([name, label, 0, i * PERIOD_MS, round(t[i], 1), round(h[i], 1), int(round(s[i]))])
            n += 1
    print(f"Da sinh {n} phien DEMO (tong hop) vao {a.outdir}. KHONG dung de bao cao do chinh xac.")


if __name__ == "__main__":
    main()
