"""Phan tich log [NOW] cua ESP-NOW -> CSV RTT + thong ke (T06, TASKS_SPEC 3.4).

Vi du:
  python tools/parse_now_log.py --node1 docs/evidence/T06/node1_1m.txt --node2 docs/evidence/T06/node2_1m.txt \
      --out docs/measurements/espnow_rtt_1m.csv --condition "1 m, khong vat can"

Chi xu ly so do that. Khong co dong RTT -> bao "khong co du lieu", khong ghi CSV.
Do tre mot chieu uoc luong = RTT/2 (dong ho Node 1); KHONG dung rx_ms cua Node 2.
"""
import argparse
import csv
import os
import re
import statistics
import sys

RTT_RE = re.compile(r"\[NOW\] RTT seq=(\d+) rtt_us=(\d+)")
N1_STATS_RE = re.compile(r"\[NOW\] stats sent=(\d+) acked=(\d+)")
N2_STATS_RE = re.compile(r"\[NOW\] stats rx=(\d+) lost=(\d+)")


def read_lines(path):
    with open(path, encoding="utf-8", errors="ignore") as f:
        return f.read().splitlines()


def percentile(sorted_vals, p):
    """Noi suy tuyen tinh giua hai hang (giong numpy 'linear')."""
    if not sorted_vals:
        return float("nan")
    k = (len(sorted_vals) - 1) * p / 100.0
    lo, hi = int(k), min(int(k) + 1, len(sorted_vals) - 1)
    return sorted_vals[lo] + (sorted_vals[hi] - sorted_vals[lo]) * (k - lo)


def parse_rtt(lines):
    rtt = {}
    for line in lines:
        m = RTT_RE.search(line)
        if m:
            rtt[int(m.group(1))] = int(m.group(2))  # seq trung (khong le xay ra) -> giu ban cuoi
    return rtt


def counter_delta(lines, regex):
    """Hieu giua dong stats dau va cuoi (log co the bat dau giua chung)."""
    rows = [tuple(int(g) for g in m.groups()) for m in map(regex.search, lines) if m]
    if len(rows) < 2:
        return None
    first, last = rows[0], rows[-1]
    if any(b < a for a, b in zip(first, last)):  # node khoi dong lai giua log: dung dong cuoi
        return last
    return tuple(b - a for a, b in zip(first, last))


def report_rtt(rtt, condition):
    seqs = sorted(rtt)
    vals = sorted(rtt.values())
    span = seqs[-1] - seqs[0] + 1
    print(f"Dieu kien: {condition or '(chua ghi)'}")
    print(f"N (Ack co RTT) = {len(vals)}  | khoang seq {seqs[0]}..{seqs[-1]} ({span} goi)")
    print(f"Mat khu hoi (khong co Ack trong khoang seq) = {100.0 * (span - len(vals)) / span:.2f} %")
    print(f"RTT us: mean={statistics.fmean(vals):.0f} p50={percentile(vals, 50):.0f} "
          f"p95={percentile(vals, 95):.0f} max={vals[-1]} min={vals[0]}")
    print(f"Tre mot chieu uoc luong (RTT/2) us: mean={statistics.fmean(vals) / 2:.0f} p95={percentile(vals, 95) / 2:.0f}")
    if len(vals) < 200:
        print(f"CANH BAO: moi co {len(vals)} mau, TASKS_SPEC 3.6 yeu cau >= 200 goi.")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--node1", required=True, help="log Serial Node 1 (co dong [NOW] RTT)")
    ap.add_argument("--node2", help="log Serial Node 2 (co dong [NOW] stats rx= lost=)")
    ap.add_argument("--out", default="docs/measurements/espnow_rtt.csv")
    ap.add_argument("--condition", default="", help="khoang cach, vat can, ngay gio")
    a = ap.parse_args()

    n1 = read_lines(a.node1)
    rtt = parse_rtt(n1)
    if not rtt:
        print(f"Khong co du lieu: {a.node1} khong co dong '[NOW] RTT seq=.. rtt_us=..'")
        return 1

    os.makedirs(os.path.dirname(a.out) or ".", exist_ok=True)
    with open(a.out, "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        w.writerow(["seq", "rtt_us"])
        for seq in sorted(rtt):
            w.writerow([seq, rtt[seq]])
    print(f"Da ghi {a.out} ({len(rtt)} dong)")
    report_rtt(rtt, a.condition)

    d1 = counter_delta(n1, N1_STATS_RE)
    if d1 and d1[0] > 0:
        print(f"Node 1 stats: sent={d1[0]} acked={d1[1]} ack_rate={100.0 * d1[1] / d1[0]:.2f} %")
    if a.node2:
        d2 = counter_delta(read_lines(a.node2), N2_STATS_RE)
        if d2 and d2[0] + d2[1] > 0:
            print(f"Node 2 stats: rx={d2[0]} lost={d2[1]} mat goi mot chieu={100.0 * d2[1] / (d2[0] + d2[1]):.2f} %")
        else:
            print(f"Node 2: khong du dong '[NOW] stats rx=..' trong {a.node2} (can >= 2 dong)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
