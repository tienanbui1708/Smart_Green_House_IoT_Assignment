"""Nhung web/index.html vao firmware dang PROGMEM (TASKS_SPEC 4.6).

Chay tu goc repo moi khi sua web/index.html:
  python tools/embed_web.py
Ghi node1_sensor/web_assets.h va node2_actuator/web_assets.h (cung mot trang; trang tu nhan biet node qua /api/state).
"""
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "web", "index.html")
TARGETS = [os.path.join(ROOT, "node1_sensor", "web_assets.h"), os.path.join(ROOT, "node2_actuator", "web_assets.h")]
DELIM = "rawliteral"
MAX_BYTES = 20 * 1024  # TASKS_SPEC 4.5: moi trang <= ~20 KB

HEADER = """// {name} - SINH TU DONG boi tools/embed_web.py tu web/index.html. KHONG sua tay.
#pragma once
#include <pgmspace.h>

const char INDEX_HTML[] PROGMEM = R"{delim}(
{html}){delim}";
"""


def main():
    with open(SRC, encoding="utf-8") as f:
        html = f.read()
    if f"){DELIM}\"" in html:
        sys.exit(f"web/index.html chua chuoi ket thuc raw literal '){DELIM}\"'")
    size = len(html.encode("utf-8"))
    if size > MAX_BYTES:
        sys.exit(f"web/index.html {size} byte > {MAX_BYTES} byte")
    for path in TARGETS:
        with open(path, "w", encoding="utf-8", newline="\n") as f:
            f.write(HEADER.format(name=os.path.relpath(path, ROOT).replace("\\", "/"), delim=DELIM, html=html))
        print(f"Da ghi {os.path.relpath(path, ROOT)} ({size} byte HTML)")


if __name__ == "__main__":
    main()
