"""Huan luyen MLP 15->32->16->4 phan loai trang thai nha kinh (T05, TINYML_GUIDE 6).

  python train.py                       # du lieu that trong ml/data/
  python train.py --data data_demo      # chi de chay thu duong ong (du lieu DEMO tong hop)

Quy tac (TASKS_SPEC 5.2):
- Moi CSV = mot phien + mot nhan; chia train/val/test THEO PHIEN (moi lop >= 3 phien).
- Loi TONG HOP (lop 3) tach rieng khoi du lieu that va danh gia rieng.
- Co baseline luat nguong de so sanh. Do chinh xac that ~100 % -> canh bao kiem tra ro ri.
Dau ra (ml/out/): model.keras, curves.png, confusion_*.png, report.txt, meta.json, splits.npz, dataset_summary.csv
"""
import argparse
import glob
import json
import os
import random

import numpy as np
import pandas as pd

os.environ.setdefault("TF_CPP_MIN_LOG_LEVEL", "2")
import tensorflow as tf  # noqa: E402
import matplotlib  # noqa: E402

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402
from sklearn.metrics import classification_report, confusion_matrix  # noqa: E402
from sklearn.utils.class_weight import compute_class_weight  # noqa: E402

from features import FAULT, LABELS, N_FEAT, NORM, SENT_H, SENT_T, W, is_fault_sample, rule_baseline, windows  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
SEED = 42
N_SYNTH = 600


def load_sessions(data_dir):
    """file -> (X, label). Phien loi (lop 3) chi giu cua so co it nhat mot mau loi (doan cam lai day khong phai 'loi')."""
    sess, summary = {}, []
    for p in sorted(glob.glob(os.path.join(data_dir, "*_L?.csv"))):
        if "_BAD" in p:
            continue
        df = pd.read_csv(p)
        label = int(df["label"].iloc[0])
        X = windows(df.t.values, df.h.values, df.soil.values)
        dropped = 0
        if label == FAULT and len(X):
            keep = np.array([is_fault_sample(x[0::3] * NORM["t"][1] + NORM["t"][0],
                                             x[1::3] * NORM["h"][1] + NORM["h"][0]).any() for x in X])
            dropped = int((~keep).sum())
            X = X[keep]
        summary.append({"file": os.path.basename(p), "label": label, "samples": len(df), "windows": len(X),
                        "dropped_fault_windows_without_fault": dropped})
        if len(X) >= 5:
            sess[p] = (X, label)
    return sess, pd.DataFrame(summary)


def split_by_session(sess, rng):
    train, val, test = [], [], []
    for lab in range(len(LABELS)):
        files = sorted(p for p, (_, l) in sess.items() if l == lab)
        if len(files) < 3:
            raise SystemExit(f"Lop {lab} ({LABELS[lab]}) can >= 3 phien de chia theo phien, hien co {len(files)}")
        rng.shuffle(files)
        test.append(files[0])
        val.append(files[1])
        train += files[2:]
    return train, val, test


def stack(files, sess):
    X = np.concatenate([sess[p][0] for p in files])
    y = np.concatenate([[sess[p][1]] * len(sess[p][0]) for p in files]).astype(np.int64)
    return X, y


def synthetic_faults(n, rng):
    """Cua so loi TONG HOP (bao cao rieng). Khong dung kieu 'ket gia tri': phong on dinh co the
    cho 5 mau giong het o do phan giai 0.1 -> se day mo hinh bao loi gia tren du lieu binh thuong."""
    out = []
    for _ in range(n):
        kind = rng.choice(["spike", "sentinel", "negative", "jump"])
        T, H, S = rng.uniform(24, 34, W), rng.uniform(40, 85, W), rng.uniform(10, 80, W)
        if kind == "spike":
            T[:], H[:] = rng.uniform(81, 120), rng.uniform(100.5, 120)
        elif kind == "sentinel":
            k = int(rng.integers(1, W + 1))
            T[-k:], H[-k:] = SENT_T, SENT_H
        elif kind == "negative":
            T[:], H[:] = rng.uniform(-40, -11, W), rng.uniform(-10, -0.5, W)
        else:  # nhay vot phi ly giua hai mau 2 s, ra ngoai dai hop ly
            i = int(rng.integers(1, W))
            T[i:] = rng.choice([-1, 1]) * rng.uniform(85, 120)
        out.append(windows(T, H, S)[0])
    return np.array(out, np.float32)


def build_model():
    return tf.keras.Sequential([
        tf.keras.layers.Input(shape=(W * N_FEAT,)),
        tf.keras.layers.Dense(32, activation="relu"),
        tf.keras.layers.Dense(16, activation="relu"),
        tf.keras.layers.Dense(len(LABELS), activation="softmax"),
    ])


def plot_curves(hist, out):
    fig, ax = plt.subplots(1, 2, figsize=(10, 3.5))
    for i, key in enumerate(["accuracy", "loss"]):
        ax[i].plot(hist.history[key], label="train")
        ax[i].plot(hist.history["val_" + key], label="val")
        ax[i].set_title(key.capitalize())
        ax[i].legend()
    plt.tight_layout()
    plt.savefig(os.path.join(out, "curves.png"), dpi=150)
    plt.close()


def evaluate(model, name, X, y, out, demo):
    pred = model.predict(X, verbose=0).argmax(1)
    acc = float((pred == y).mean())
    cm = confusion_matrix(y, pred, labels=list(range(len(LABELS))))
    plt.figure(figsize=(4.8, 4.2))
    plt.imshow(cm, cmap="Blues")
    plt.title(f"Confusion - {name}" + (" (DEMO)" if demo else ""))
    plt.xticks(range(4), LABELS, rotation=30)
    plt.yticks(range(4), LABELS)
    for i in range(4):
        for j in range(4):
            plt.text(j, i, cm[i, j], ha="center", va="center")
    plt.tight_layout()
    plt.savefig(os.path.join(out, f"confusion_{name.lower()}.png"), dpi=150)
    plt.close()
    rep = classification_report(y, pred, labels=list(range(4)), target_names=LABELS, zero_division=0)
    return acc, f"== {name}: n={len(y)} accuracy={acc:.4f}\n{cm}\n{rep}"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--data", default="data", help="thu muc CSV (tuong doi ml/)")
    ap.add_argument("--out", default="out")
    ap.add_argument("--epochs", type=int, default=100)
    a = ap.parse_args()
    data_dir = os.path.join(HERE, a.data)
    out = os.path.join(HERE, a.out)
    os.makedirs(out, exist_ok=True)
    random.seed(SEED)
    np.random.seed(SEED)
    tf.random.set_seed(SEED)
    rng = np.random.default_rng(SEED)

    sess, summary = load_sessions(data_dir)
    if not sess:
        raise SystemExit(f"Khong co du lieu trong {data_dir} (can *_L<k>.csv tu collect_serial.py)")
    demo = any(os.path.basename(p).startswith("DEMO_") for p in sess)
    summary.to_csv(os.path.join(out, "dataset_summary.csv"), index=False)
    tr_f, va_f, te_f = split_by_session(sess, rng)
    Xtr, ytr = stack(tr_f, sess)
    Xva, yva = stack(va_f, sess)
    Xte, yte = stack(te_f, sess)

    Xsyn = synthetic_faults(N_SYNTH, rng)
    ntr, nva = int(0.6 * N_SYNTH), int(0.2 * N_SYNTH)
    Xtr, ytr = np.concatenate([Xtr, Xsyn[:ntr]]), np.concatenate([ytr, np.full(ntr, FAULT)])
    Xva, yva = np.concatenate([Xva, Xsyn[ntr:ntr + nva]]), np.concatenate([yva, np.full(nva, FAULT)])
    Xsyn_te = Xsyn[ntr + nva:]
    ysyn_te = np.full(len(Xsyn_te), FAULT)
    print(f"cua so train/val/test(that): {len(Xtr)}/{len(Xva)}/{len(Xte)} | test(tong hop loi): {len(Xsyn_te)}")

    model = build_model()
    model.compile(optimizer="adam", loss="sparse_categorical_crossentropy", metrics=["accuracy"])
    cw = compute_class_weight("balanced", classes=np.arange(4), y=ytr)
    es = tf.keras.callbacks.EarlyStopping(patience=8, restore_best_weights=True)
    hist = model.fit(Xtr, ytr, validation_data=(Xva, yva), epochs=a.epochs, batch_size=32,
                     class_weight=dict(enumerate(cw)), callbacks=[es], verbose=2)
    model.save(os.path.join(out, "model.keras"))
    plot_curves(hist, out)

    tag = "DU LIEU DEMO TONG HOP - KHONG DUNG DE BAO CAO\n" if demo else ""
    acc_real, rep_real = evaluate(model, "TEST_REAL", Xte, yte, out, demo)
    _, rep_syn = evaluate(model, "TEST_SYNTHETIC_FAULT", Xsyn_te, ysyn_te, out, demo)
    base = np.array([rule_baseline(x) for x in Xte])
    base_syn = np.array([rule_baseline(x) for x in Xsyn_te])
    lines = [tag + rep_real, rep_syn,
             f"== BASELINE luat nguong tren TEST_REAL: accuracy={(base == yte).mean():.4f}",
             f"== BASELINE luat nguong tren TEST_SYNTHETIC_FAULT: accuracy={(base_syn == ysyn_te).mean():.4f}",
             f"train_files={[os.path.basename(f) for f in tr_f]}",
             f"val_files={[os.path.basename(f) for f in va_f]}",
             f"test_files={[os.path.basename(f) for f in te_f]}"]
    if acc_real >= 0.995:
        lines.append("CANH BAO: accuracy test that ~100 %: kiem tra ro ri (chia theo phien? cac lop qua de tach?) "
                     "va so voi baseline truoc khi bao cao (TASKS_SPEC 5.2).")
    report = "\n".join(lines)
    with open(os.path.join(out, "report.txt"), "w", encoding="utf-8") as f:
        f.write(report + "\n")
    print(report)

    np.savez(os.path.join(out, "splits.npz"), Xtr=Xtr, ytr=ytr, Xte=Xte, yte=yte, Xsyn_te=Xsyn_te, ysyn_te=ysyn_te)
    meta = {"demo_data": demo, "data_dir": a.data, "window": W, "labels": LABELS, "norm": NORM,
            "sentinel": {"t": SENT_T, "h": SENT_H}, "n_synthetic_fault": N_SYNTH,
            "train_files": tr_f, "val_files": va_f, "test_files": te_f,
            "accuracy_test_real": acc_real, "tensorflow": tf.__version__}
    with open(os.path.join(out, "meta.json"), "w", encoding="utf-8") as f:
        json.dump(meta, f, ensure_ascii=False, indent=2, default=str)


if __name__ == "__main__":
    main()
