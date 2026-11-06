"""Thu du lieu tu Serial cua Node 1 (COLLECT_MODE=1) -> CSV co nhan (TINYML_GUIDE 4).

Moi file = MOT phien + MOT nhan, dieu kien on dinh suot phien. Khong sua tay CSV goc.
Vi du (Windows):  python collect_serial.py --port COM5 --label 0 --session s01 --minutes 8 --note "de yen, phong 28 do"
Chi doc dong 'D,<millis>,<t>,<h>,<soil>' (tools/collect_firmware/README.md); dong log khac bi bo qua.
"""
import argparse
import csv
import os
import time

import serial

HERE = os.path.dirname(os.path.abspath(__file__))


def parse_line(line):
    """'D,123456,28.4,64.2,41' -> (ms, t, h, soil) hoac None."""
    if not line.startswith("D,"):
        return None
    parts = line.split(",")
    if len(parts) != 5:
        return None
    try:
        return int(parts[1]), float(parts[2]), float(parts[3]), float(parts[4])
    except ValueError:
        return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", required=True)
    ap.add_argument("--baud", type=int, default=115200)
    ap.add_argument("--label", type=int, required=True, choices=[0, 1, 2, 3])
    ap.add_argument("--session", required=True, help="ten phien, vi du s01")
    ap.add_argument("--minutes", type=float, default=8)
    ap.add_argument("--note", default="", help="ghi chu dieu kien thu (de nguoi khac lap lai)")
    ap.add_argument("--outdir", default=os.path.join(HERE, "data"))
    a = ap.parse_args()

    os.makedirs(a.outdir, exist_ok=True)
    path = os.path.join(a.outdir, f"{a.session}_L{a.label}.csv")
    if os.path.exists(path):
        raise SystemExit(f"File da ton tai, khong ghi de: {path}")

    end = time.time() + a.minutes * 60
    good = bad = 0
    with serial.Serial(a.port, a.baud, timeout=3) as ser, open(path, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["session", "label", "pc_time", "ms", "t", "h", "soil"])
        ser.reset_input_buffer()
        print(f"Thu {a.minutes} phut -> {path}  (Ctrl+C de dung som)")
        try:
            while time.time() < end:
                line = ser.readline().decode(errors="ignore").strip()
                if not line.startswith("D,"):
                    continue
                row = parse_line(line)
                if row is None:
                    bad += 1
                    continue
                ms, t, h, s = row
                w.writerow([a.session, a.label, f"{time.time():.3f}", ms, t, h, s])
                f.flush()
                good += 1
                if good % 15 == 0:
                    print(f"  {good} mau (dong loi: {bad})  T={t} H={h} soil={s}")
        except KeyboardInterrupt:
            print("Dung som.")
    with open(os.path.join(a.outdir, f"{a.session}_L{a.label}.note.txt"), "w", encoding="utf-8") as nf:
        nf.write(f"label={a.label}\nminutes={a.minutes}\nsamples={good}\nbad_lines={bad}\nnote={a.note}\n")
    print(f"Xong: {good} mau, {bad} dong loi.")


if __name__ == "__main__":
    main()
