from __future__ import annotations

import ctypes as C
from pathlib import Path
from typing import Iterable


KDNA_FIELDS = 17
KDNA_X = 0
KDNA_K01 = 1
KDNA_K02 = 2
KDNA_K03 = 3
KDNA_K04 = 4
KDNA_K05 = 5
KDNA_EK = 6
KDNA_AK = 7
KDNA_LK = 8
KDNA_RAW = 9
KDNA_DOM = 10
KDNA_S01 = 11
KDNA_S02 = 12
KDNA_S03 = 13
KDNA_S04 = 14
KDNA_S05 = 15
KDNA_DOM_SCORE = 16


class Constants(C.Structure):
    _fields_ = [
        ("cA", C.c_double),
        ("cB", C.c_double),
        ("cC", C.c_double),
        ("cD", C.c_double),
        ("cE", C.c_double),
        ("eps", C.c_double),
        ("exp_min", C.c_double),
        ("exp_max", C.c_double),
        ("hard_max", C.c_double),
    ]


def idx(field: int, n: int, i: int) -> int:
    return field * n + i


def load(lib_path: str | Path) -> C.CDLL:
    lib = C.CDLL(str(lib_path))
    lib.kdna_default_constants.argtypes = [C.POINTER(Constants)]
    lib.kdna_status_str.argtypes = [C.c_int]
    lib.kdna_status_str.restype = C.c_char_p
    lib.kdna_eval_cpu.argtypes = [C.POINTER(C.c_double), C.POINTER(C.c_double), C.c_size_t, C.POINTER(Constants)]
    lib.kdna_eval_opencl.argtypes = [C.POINTER(C.c_double), C.POINTER(C.c_double), C.c_size_t, C.POINTER(Constants), C.c_char_p]
    return lib


def default_constants(lib: C.CDLL) -> Constants:
    c = Constants()
    lib.kdna_default_constants(C.byref(c))
    return c


def eval_cpu(lib: C.CDLL, xs: Iterable[float], constants: Constants | None = None) -> list[dict[str, float | int]]:
    values = [float(x) for x in xs]
    n = len(values)
    if n == 0:
        return []
    c = constants if constants is not None else default_constants(lib)
    xbuf = (C.c_double * n)(*values)
    out = (C.c_double * (KDNA_FIELDS * n))()
    rc = lib.kdna_eval_cpu(xbuf, out, n, C.byref(c))
    if rc != 0:
        raise RuntimeError(lib.kdna_status_str(rc).decode())
    return _decode(out, n)


def eval_opencl(
    lib: C.CDLL,
    xs: Iterable[float],
    kernel_path: str | Path,
    constants: Constants | None = None,
) -> list[dict[str, float | int]]:
    values = [float(x) for x in xs]
    n = len(values)
    if n == 0:
        return []
    c = constants if constants is not None else default_constants(lib)
    xbuf = (C.c_double * n)(*values)
    out = (C.c_double * (KDNA_FIELDS * n))()
    rc = lib.kdna_eval_opencl(xbuf, out, n, C.byref(c), str(kernel_path).encode())
    if rc != 0:
        raise RuntimeError(lib.kdna_status_str(rc).decode())
    return _decode(out, n)


def _decode(out: C.Array[C.c_double], n: int) -> list[dict[str, float | int]]:
    rows: list[dict[str, float | int]] = []
    for i in range(n):
        rows.append({
            "x": out[idx(KDNA_X, n, i)],
            "K01": out[idx(KDNA_K01, n, i)],
            "K02": out[idx(KDNA_K02, n, i)],
            "K03": out[idx(KDNA_K03, n, i)],
            "K04": out[idx(KDNA_K04, n, i)],
            "K05": out[idx(KDNA_K05, n, i)],
            "E_K": out[idx(KDNA_EK, n, i)],
            "A_K": out[idx(KDNA_AK, n, i)],
            "L_K": out[idx(KDNA_LK, n, i)],
            "RAW": int(out[idx(KDNA_RAW, n, i)]),
            "D": int(out[idx(KDNA_DOM, n, i)]),
            "S01": out[idx(KDNA_S01, n, i)],
            "S02": out[idx(KDNA_S02, n, i)],
            "S03": out[idx(KDNA_S03, n, i)],
            "S04": out[idx(KDNA_S04, n, i)],
            "S05": out[idx(KDNA_S05, n, i)],
            "dominanceScore": out[idx(KDNA_DOM_SCORE, n, i)],
        })
    return rows
