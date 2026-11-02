#!/usr/bin/env bash
# Cai OhStem Boards tren Linux. Zip cua goi co thu muc quyen 0555 nen arduino-cli khong giai nen duoc;
# script tai bang arduino-cli, kiem SHA-256 theo index, roi tu giai nen voi quyen dung.
set -euo pipefail
VERSION="3.1.3"
DATA="$HOME/.arduino15"
DEST="$DATA/packages/OhStem/hardware/esp32/$VERSION"
export VERSION DATA DEST

if [ -d "$DEST" ]; then echo "Da co $DEST"; exit 0; fi
arduino-cli core update-index
# Lenh nay tai goi va cai tool, roi that bai o buoc giai nen; loi do duoc xu ly ben duoi.
arduino-cli core install "OhStem:esp32@$VERSION" || true

python3 -I - <<'PY'
import hashlib, json, os, shutil, tempfile, zipfile
ver, data, dest = os.environ["VERSION"], os.environ["DATA"], os.environ["DEST"]
index = json.load(open(f"{data}/package_xcon_index.json"))
package = index["packages"][0]
platform = next(p for p in package["platforms"] if p["version"] == ver)
archive = f"{data}/staging/packages/{platform['archiveFileName']}"
digest = hashlib.sha256(open(archive, "rb").read()).hexdigest()
if f"SHA-256:{digest}" != platform["checksum"]:
    raise SystemExit("Sai checksum goi OhStem")

tmp = tempfile.mkdtemp(dir=data)
with zipfile.ZipFile(archive) as z:
    for info in z.infolist():
        path = os.path.join(tmp, info.filename)
        if not os.path.realpath(path).startswith(os.path.realpath(tmp) + os.sep):
            raise SystemExit(f"Duong dan khong hop le: {info.filename}")
        if info.is_dir():
            os.makedirs(path, exist_ok=True)
            continue
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "wb") as out:
            out.write(z.read(info))
        os.chmod(path, 0o755 if (info.external_attr >> 16) & 0o111 else 0o644)

os.makedirs(os.path.dirname(dest), exist_ok=True)
shutil.move(os.path.join(tmp, f"ohstem_esp32-{ver}"), dest)
shutil.rmtree(tmp)
meta = {k: v for k, v in package.items() if k not in ("platforms", "tools")}
json.dump({"packages": [{**meta, "platforms": [platform], "tools": []}]}, open(f"{dest}/installed.json", "w"))
PY
arduino-cli core list
