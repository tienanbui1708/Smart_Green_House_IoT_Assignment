"""Trich dong [STATS] (heap, stack) tu log Serial -> CSV + do thi heap (T07/do on dinh, TASKS_SPEC 7).

Vi du (log chay lien tuc >= 2 gio):
  python tools/parse_stats_log.py --log docs/evidence/M9/node1_2h.txt --out docs/measurements/heap_node1.csv
Dong mau: [STATS] up=120s heap=180000 minheap=170000 stack_free: sensor=1234 display=900 ...
Can matplotlib de ve PNG (ml/venv co san); khong co thi chi ghi CSV.
"""
import argparse
import csv
import os
import re
import sys

LINE_RE = re.compile(r"\[STATS\] up=(\d+)s heap=(\d+) minheap=(\d+) stack_free:(.*)")
TASK_RE = re.compile(r"(\w+)=(\d+)")


def parse(path):
    rows, tasks = [], []
    with open(path, encoding="utf-8", errors="ignore") as f:
        for line in f:
            m = LINE_RE.search(line)
            if not m:
                continue
            stacks = dict((k, int(v)) for k, v in TASK_RE.findall(m.group(4)))
            for k in stacks:
                if k not in tasks:
                    tasks.append(k)
            rows.append({"up_s": int(m.group(1)), "heap": int(m.group(2)), "minheap": int(m.group(3)), **stacks})
    return rows, tasks


def plot(rows, png, title):
    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except ImportError:
        print("Khong co matplotlib: bo qua PNG")
        return
    t = [r["up_s"] / 60 for r in rows]
    plt.figure(figsize=(8, 3.5))
    plt.plot(t, [r["heap"] for r in rows], label="heap trong")
    plt.plot(t, [r["minheap"] for r in rows], label="min heap tu luc boot")
    plt.xlabel("uptime (phut)")
    plt.ylabel("byte")
    plt.title(title)
    plt.legend()
    plt.grid(alpha=0.3)
    plt.tight_layout()
    plt.savefig(png, dpi=150)
    plt.close()
    print(f"Da ghi {png}")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--log", required=True)
    ap.add_argument("--out", required=True, help="vd docs/measurements/heap_node1.csv (PNG cung ten)")
    ap.add_argument("--title", default="")
    a = ap.parse_args()
    rows, tasks = parse(a.log)
    if not rows:
        print(f"Khong co du lieu: {a.log} khong co dong [STATS]")
        return 1
    if any(b["up_s"] < x["up_s"] for x, b in zip(rows, rows[1:])):
        print("CANH BAO: uptime giam giua log -> thiet bi da khoi dong lai trong luc do (ghi vao bao cao).")
    os.makedirs(os.path.dirname(a.out) or ".", exist_ok=True)
    cols = ["up_s", "heap", "minheap"] + [f"stack_{k}" for k in tasks]
    with open(a.out, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(cols)
        for r in rows:
            w.writerow([r["up_s"], r["heap"], r["minheap"]] + [r.get(k, "") for k in tasks])
    span_h = (rows[-1]["up_s"] - rows[0]["up_s"]) / 3600
    print(f"Da ghi {a.out}: N={len(rows)} dong, {span_h:.2f} gio")
    print(f"heap dau={rows[0]['heap']} cuoi={rows[-1]['heap']} nho nhat={min(r['heap'] for r in rows)} "
          f"minheap cuoi={rows[-1]['minheap']}")
    for k in tasks:
        vals = [r[k] for r in rows if k in r]
        print(f"stack con trong nho nhat {k}: {min(vals)} byte")
    plot(rows, os.path.splitext(a.out)[0] + ".png", a.title or os.path.basename(a.log))
    return 0


if __name__ == "__main__":
    sys.exit(main())
