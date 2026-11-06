"""Tong hop CSV so do that trong docs/measurements/ -> summary.md cho Chuong 4 (TASKS_SPEC 7).

  python tools/summarize_measurements.py
Chi doc file co that; file thieu -> ghi 'khong co du lieu'. Khong uoc luong, khong dien so.
File nhan dang:
  espnow_rtt*.csv  (seq,rtt_us)                     <- tools/parse_now_log.py
  web_latency.csv  (...,kind,...,latency_ms,...)    <- tools/web_latency.py
  heap_node*.csv   (up_s,heap,minheap,...)          <- tools/parse_stats_log.py
  ota_time.csv     (run,node,bin_bytes,seconds,...) <- ghi tay tu log/video OTA (COREIOT_SETUP.md 10.6)
  rpc_latency.csv  (...,latency_ms,...)             <- tuy chon
"""
import csv
import glob
import os
import statistics

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MEAS = os.path.join(ROOT, "docs", "measurements")


def read(path):
    with open(path, encoding="utf-8") as f:
        return list(csv.DictReader(f))


def pct(v, p):
    v = sorted(v)
    k = (len(v) - 1) * p / 100
    lo = int(k)
    hi = min(lo + 1, len(v) - 1)
    return v[lo] + (v[hi] - v[lo]) * (k - lo)


def stat_row(name, vals, src):
    if not vals:
        return f"| {name} | 0 | khong co du lieu | | | | `{src}` |"
    return (f"| {name} | {len(vals)} | {statistics.fmean(vals):.1f} | {pct(vals, 50):.1f} | {pct(vals, 95):.1f} | "
            f"{max(vals):.1f} | `{src}` |")


def floats(rows, key):
    out = []
    for r in rows:
        try:
            out.append(float(r[key]))
        except (KeyError, ValueError, TypeError):
            pass
    return out


def section_espnow():
    lines = []
    files = sorted(glob.glob(os.path.join(MEAS, "espnow_rtt*.csv")))
    for p in files:
        rows = read(p)
        rtt_ms = [v / 1000 for v in floats(rows, "rtt_us")]
        lines.append(stat_row(f"ESP-NOW RTT (ms) {os.path.basename(p)}", rtt_ms, os.path.relpath(p, ROOT)))
        lines.append(stat_row(f"ESP-NOW tre mot chieu RTT/2 (ms) {os.path.basename(p)}", [v / 2 for v in rtt_ms],
                              os.path.relpath(p, ROOT)))
    return lines or [stat_row("ESP-NOW RTT (ms)", [], "docs/measurements/espnow_rtt*.csv")]


def section_web():
    p = os.path.join(MEAS, "web_latency.csv")
    rows = read(p) if os.path.exists(p) else []
    return [stat_row(f"Web {k} (ms)", floats([r for r in rows if r.get("kind") == k], "latency_ms"),
                     "docs/measurements/web_latency.csv") for k in ("state", "mode")]


def section_simple(fname, key, label, scale=1.0):
    p = os.path.join(MEAS, fname)
    rows = read(p) if os.path.exists(p) else []
    return [stat_row(label, [v * scale for v in floats(rows, key)], f"docs/measurements/{fname}")]


def section_heap():
    out = []
    for p in sorted(glob.glob(os.path.join(MEAS, "heap_node*.csv"))):
        rows = read(p)
        heap = floats(rows, "heap")
        up = floats(rows, "up_s")
        if not heap:
            continue
        hours = (up[-1] - up[0]) / 3600 if len(up) > 1 else 0
        out.append(f"| {os.path.basename(p)} | {len(heap)} | {hours:.2f} h | dau {heap[0]:.0f} B | cuoi {heap[-1]:.0f} B | "
                   f"nho nhat {min(heap):.0f} B | `{os.path.relpath(p, ROOT)}` |")
    return out or ["| heap | 0 | khong co du lieu | | | | `docs/measurements/heap_node*.csv` |"]


def main():
    md = ["# Tong hop so do (tu dong, tools/summarize_measurements.py)", "",
          "Chi gom so do that trong `docs/measurements/`. Dieu kien do (khoang cach, mang, ngay gio) xem trong tung CSV/ghi chu.", "",
          "| Chi so | N | Trung binh | p50 | p95 | Max | Nguon |", "|---|---|---|---|---|---|---|"]
    md += section_espnow()
    md += section_web()
    md += section_simple("ota_time.csv", "seconds", "OTA: gan firmware -> UPDATED (s)")
    md += section_simple("rpc_latency.csv", "latency_ms", "RPC dashboard -> thiet bi (ms)")
    md += ["", "## Do on dinh bo nho", "", "| File | N | Thoi gian | Heap | | | Nguon |", "|---|---|---|---|---|---|---|"]
    md += section_heap()
    out = os.path.join(MEAS, "summary.md")
    os.makedirs(MEAS, exist_ok=True)
    with open(out, "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(md) + "\n")
    print("\n".join(md))
    print(f"\nDa ghi {os.path.relpath(out, ROOT)}")


if __name__ == "__main__":
    main()
