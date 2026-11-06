"""Do do tre Webserver cuc bo (T07, TASKS_SPEC 4.7) -> docs/measurements/web_latency.csv.

Vi du (PC cung mang voi node):
  python tools/web_latency.py --host 192.168.1.50 --node 1 --n 30 --condition "laptop Wi-Fi, cach 3 m"

Do hai loai request:
  state : GET /api/state
  mode  : POST /api/mode voi dung gia tri 'auto' hien tai (khong lam doi trang thai thiet bi)
Chi dung thu vien chuan. Ghi them vao CSV (append) de gop nhieu lan do.
"""
import argparse
import csv
import datetime
import json
import os
import statistics
import time
import urllib.request


def timed(req, timeout):
    t0 = time.perf_counter()
    with urllib.request.urlopen(req, timeout=timeout) as r:
        body = r.read()
        code = r.status
    return (time.perf_counter() - t0) * 1000.0, code, body


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", required=True)
    ap.add_argument("--node", type=int, choices=[1, 2], required=True)
    ap.add_argument("--n", type=int, default=20)
    ap.add_argument("--gap", type=float, default=0.5, help="giay giua hai request")
    ap.add_argument("--timeout", type=float, default=5.0)
    ap.add_argument("--condition", default="")
    ap.add_argument("--out", default="docs/measurements/web_latency.csv")
    a = ap.parse_args()

    base = f"http://{a.host}"
    rows, fails = [], 0
    for i in range(a.n):
        for kind in ("state", "mode"):
            try:
                if kind == "state":
                    ms, code, body = timed(urllib.request.Request(base + "/api/state"), a.timeout)
                    auto = json.loads(body)["auto"]
                else:
                    data = json.dumps({"auto": auto}).encode()
                    req = urllib.request.Request(base + "/api/mode", data=data, headers={"Content-Type": "application/json"})
                    ms, code, _ = timed(req, a.timeout)
                rows.append((kind, i, round(ms, 1), code))
            except Exception as e:  # mat goi/timeout: ghi nhan, khong bia so
                fails += 1
                print(f"{kind} #{i} loi: {e}")
            time.sleep(a.gap)

    if not rows:
        print("Khong co du lieu (khong request nao thanh cong).")
        return
    os.makedirs(os.path.dirname(a.out) or ".", exist_ok=True)
    new = not os.path.exists(a.out)
    stamp = datetime.datetime.now().isoformat(timespec="seconds")
    with open(a.out, "a", newline="", encoding="utf-8") as f:
        w = csv.writer(f)
        if new:
            w.writerow(["timestamp", "node", "host", "kind", "i", "latency_ms", "http_code", "condition"])
        for kind, i, ms, code in rows:
            w.writerow([stamp, a.node, a.host, kind, i, ms, code, a.condition])
    print(f"Da ghi {len(rows)} dong vao {a.out}; loi={fails}")
    for kind in ("state", "mode"):
        v = sorted(ms for k, _, ms, _ in rows if k == kind)
        if v:
            p95 = v[min(len(v) - 1, int(round(0.95 * (len(v) - 1))))]
            print(f"{kind}: N={len(v)} mean={statistics.fmean(v):.1f} ms p95={p95:.1f} ms max={v[-1]:.1f} ms")


if __name__ == "__main__":
    main()
