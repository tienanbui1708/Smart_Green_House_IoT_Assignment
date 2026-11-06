"""Export TFLite float32 + INT8, so sanh, sinh model.h va test_vectors.h (T05, TINYML_GUIDE 7).

  python export_tflite.py --use float            # ghi out/model.h (ban float)
  python export_tflite.py --use float --install  # + chep sang node1_sensor/ va tools/tinyml_parity/

model.h: mang g_model / g_model_len (alignas 16) + macro MODEL_IS_INT8, MODEL_IS_DEMO.
test_vectors.h: 20 vector (15 tu test that + 5 tu loi tong hop), dap an = du doan cua ban TFLite duoc chon.
"""
import argparse
import json
import os
import shutil
import tempfile

import numpy as np

os.environ.setdefault("TF_CPP_MIN_LOG_LEVEL", "2")
import tensorflow as tf  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
N_VEC_REAL, N_VEC_SYN = 15, 5


def converter_for(model):
    """Keras 3 doi khi khong convert truc tiep duoc: di qua SavedModel."""
    try:
        conv = tf.lite.TFLiteConverter.from_keras_model(model)
        conv.convert()
        return lambda: tf.lite.TFLiteConverter.from_keras_model(model)
    except Exception as e:  # noqa: BLE001
        print(f"from_keras_model loi ({type(e).__name__}), dung SavedModel")
        tmp = tempfile.mkdtemp()
        model.export(tmp)
        return lambda: tf.lite.TFLiteConverter.from_saved_model(tmp)


def run_tflite(buf, X):
    it = tf.lite.Interpreter(model_content=buf)
    it.allocate_tensors()
    inp, outd = it.get_input_details()[0], it.get_output_details()[0]
    quant = inp["dtype"] == np.int8
    preds, probs = [], []
    for x in X:
        v = x[None, :].astype(np.float32)
        if quant:
            s, z = inp["quantization"]
            v = np.clip(np.round(v / s + z), -128, 127).astype(np.int8)
        it.set_tensor(inp["index"], v)
        it.invoke()
        o = it.get_tensor(outd["index"])[0].astype(np.float32)
        if quant:
            s, z = outd["quantization"]
            o = (o - z) * s
        preds.append(int(np.argmax(o)))
        probs.append(o)
    return np.array(preds), inp["quantization"], outd["quantization"]


def write_model_h(path, data, is_int8, is_demo):
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write("// model.h - SINH TU DONG boi ml/export_tflite.py. KHONG sua tay.\n")
        if is_demo:
            f.write("// !!! MODEL DEMO: train tu du lieu TONG HOP (ml/make_demo_data.py), chi de chay thu duong ong.\n"
                    "// !!! Thu du lieu that roi chay lai train.py + export_tflite.py --install truoc khi bao cao.\n")
        f.write("#pragma once\n#include <stdint.h>\n\n")
        f.write(f"#define MODEL_IS_INT8 {1 if is_int8 else 0}\n#define MODEL_IS_DEMO {1 if is_demo else 0}\n\n")
        f.write("alignas(16) const unsigned char g_model[] = {\n")
        for i in range(0, len(data), 12):
            f.write("  " + ", ".join(f"0x{b:02x}" for b in data[i:i + 12]) + ",\n")
        f.write(f"}};\nconst unsigned int g_model_len = {len(data)};\n")


def write_vectors_h(path, X, expect):
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write("// test_vectors.h - SINH TU DONG boi ml/export_tflite.py (vector da chuan hoa).\n#pragma once\n")
        f.write(f"const int TV_N = {len(X)};\nconst int TV_DIM = {X.shape[1]};\n")
        f.write(f"const float TV_X[{len(X)}][{X.shape[1]}] = {{\n")
        for x in X:
            f.write("  {" + ", ".join(f"{v:.6f}f" for v in x) + "},\n")
        f.write(f"}};\nconst int TV_EXPECT[{len(X)}] = {{" + ", ".join(str(int(e)) for e in expect) + "};\n")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--use", choices=["float", "int8"], default="float", help="ban ghi vao model.h")
    ap.add_argument("--out", default="out")
    ap.add_argument("--install", action="store_true", help="chep model.h/test_vectors.h vao firmware")
    a = ap.parse_args()
    out = os.path.join(HERE, a.out)
    with open(os.path.join(out, "meta.json"), encoding="utf-8") as f:
        meta = json.load(f)
    model = tf.keras.models.load_model(os.path.join(out, "model.keras"))
    d = np.load(os.path.join(out, "splits.npz"))
    Xtr, Xte, yte, Xsyn, ysyn = d["Xtr"], d["Xte"], d["yte"], d["Xsyn_te"], d["ysyn_te"]

    make = converter_for(model)
    f32 = make().convert()

    def rep():
        for i in np.random.RandomState(0).permutation(len(Xtr))[:500]:
            yield [Xtr[i:i + 1].astype(np.float32)]

    c8 = make()
    c8.optimizations = [tf.lite.Optimize.DEFAULT]
    c8.representative_dataset = rep
    c8.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
    c8.inference_input_type = tf.int8
    c8.inference_output_type = tf.int8
    i8 = c8.convert()
    for name, buf in (("model_float.tflite", f32), ("model_int8.tflite", i8)):
        with open(os.path.join(out, name), "wb") as f:
            f.write(buf)

    keras_pred = model.predict(Xte, verbose=0).argmax(1)
    pf, _, _ = run_tflite(f32, Xte)
    pq, qin, qout = run_tflite(i8, Xte)
    pf_syn, _, _ = run_tflite(f32, Xsyn)
    pq_syn, _, _ = run_tflite(i8, Xsyn)
    lines = [
        "DU LIEU DEMO TONG HOP - KHONG DUNG DE BAO CAO" if meta.get("demo_data") else "Du lieu that (ml/data)",
        f"Keras        acc(test that)={(keras_pred == yte).mean():.4f}",
        f"TFLite float acc(test that)={(pf == yte).mean():.4f} acc(loi tong hop)={(pf_syn == ysyn).mean():.4f} size={len(f32)} B",
        f"TFLite int8  acc(test that)={(pq == yte).mean():.4f} acc(loi tong hop)={(pq_syn == ysyn).mean():.4f} size={len(i8)} B "
        f"in(scale,zp)={qin} out(scale,zp)={qout}",
        f"Khop float vs Keras: {(pf == keras_pred).mean():.4f} | int8 vs Keras: {(pq == keras_pred).mean():.4f}",
    ]
    report = "\n".join(lines)
    print(report)
    with open(os.path.join(out, "model_sizes.txt"), "w", encoding="utf-8") as f:
        f.write(report + "\n")

    chosen = f32 if a.use == "float" else i8
    write_model_h(os.path.join(out, "model.h"), chosen, a.use == "int8", bool(meta.get("demo_data")))
    rs = np.random.RandomState(1)
    Xv = np.concatenate([Xte[rs.choice(len(Xte), N_VEC_REAL, replace=False)], Xsyn[rs.choice(len(Xsyn), N_VEC_SYN, replace=False)]])
    expect, _, _ = run_tflite(chosen, Xv)
    write_vectors_h(os.path.join(out, "test_vectors.h"), Xv, expect)
    print(f"Da ghi out/model.h ({a.use}, {len(chosen)} B) va out/test_vectors.h")

    if a.install:
        shutil.copy(os.path.join(out, "model.h"), os.path.join(ROOT, "node1_sensor", "model.h"))
        parity = os.path.join(ROOT, "tools", "tinyml_parity")
        os.makedirs(parity, exist_ok=True)
        for name in ("model.h", "test_vectors.h"):
            shutil.copy(os.path.join(out, name), os.path.join(parity, name))
        print("Da chep model.h -> node1_sensor/, model.h + test_vectors.h -> tools/tinyml_parity/")


if __name__ == "__main__":
    main()
