"""Hang so va tien xu ly dung chung train/export (TINYML_GUIDE 2.2-2.3).

PHAI khop firmware: node1_sensor/config.h (AI_NORM_*), common/protocol.h (SENTINEL_T/H).
"""
import numpy as np

W = 5                                   # so mau trong cua so (chu ky 2 s -> 8 s)
N_FEAT = 3                              # t, h, soil
LABELS = ["normal", "soil_dry", "hot_humid", "sensor_fault"]
FAULT = 3
SENT_T, SENT_H = -40.0, 0.0
T_MIN, T_MAX, H_MIN, H_MAX = -10.0, 80.0, 0.0, 100.0   # ngoai dai -> loi (giong task_sensor)
NORM = {"t": (25.0, 25.0), "h": (50.0, 50.0), "s": (50.0, 50.0)}  # (offset, scale)


def norm(t, h, s):
    return ((t - NORM["t"][0]) / NORM["t"][1], (h - NORM["h"][0]) / NORM["h"][1], (s - NORM["s"][0]) / NORM["s"][1])


def denorm_last(x):
    """Mau moi nhat (3 gia tri cuoi) cua mot cua so -> (T, H, soil) don vi that."""
    return (x[-3] * NORM["t"][1] + NORM["t"][0], x[-2] * NORM["h"][1] + NORM["h"][0], x[-1] * NORM["s"][1] + NORM["s"][0])


def is_fault_sample(t, h):
    """Mau loi: sentinel hoac ngoai dai hop ly (TINYML_GUIDE 2.3)."""
    t = np.asarray(t, float)
    h = np.asarray(h, float)
    return (t < T_MIN) | (t > T_MAX) | (h < H_MIN) | (h > H_MAX) | ((t == SENT_T) & (h == SENT_H))


def windows(t, h, s):
    """Cua so truot, thu tu cu -> moi: [t,h,s]_{k-4} ... [t,h,s]_k (giong task_tinyml)."""
    tn, hn, sn = norm(np.asarray(t, float), np.asarray(h, float), np.asarray(s, float))
    feat = np.stack([tn, hn, sn], axis=1)
    if len(feat) < W:
        return np.zeros((0, W * N_FEAT), np.float32)
    return np.array([feat[i - W + 1:i + 1].reshape(-1) for i in range(W - 1, len(feat))], np.float32)


def rule_baseline(x):
    """Baseline luat nguong: loi neu bat ky mau nao trong cua so loi; con lai xet mau moi nhat.
    CHINH NGUONG THEO SO DO THAT (TINYML_GUIDE 6.4)."""
    tw = x[0::3] * NORM["t"][1] + NORM["t"][0]
    hw = x[1::3] * NORM["h"][1] + NORM["h"][0]
    if is_fault_sample(tw, hw).any():
        return FAULT
    t, h, s = denorm_last(x)
    if s < 20:
        return 1
    if t > 33 or h > 80:
        return 2
    return 0
