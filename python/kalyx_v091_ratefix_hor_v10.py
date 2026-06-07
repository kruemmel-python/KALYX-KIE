#!/usr/bin/env python3
"""
KALYX v0.9.1 + v1.0 Combined Runner

v0.9.1:
  Fixes transition-rate semantics from v0.9. v0.9 counted rates against the
  number of distinct observed target states in a (from_state, delta_family)
  group, which could produce values > 1. v0.9.1 emits two bounded rates:

    conditional_rate_from_state
      count / all outgoing transitions from the same from_spacer

    conditional_rate_from_state_and_delta
      count / all outgoing transitions from the same from_spacer and delta_family

v1.0:
  Tests whether the observed spacer-state automaton remains exceptional after
  conditioning on a coarse Alpha-Satellite / HOR structure. It builds HOR slots
  from genomic position modulo a candidate HOR period (default 2380) and shuffles
  spacer states within each slot, preserving slot-specific state composition.

Boundary:
  This tests repeat-family/HOR-conditioned null models. It does not prove
  artificial origin. It can release only scoped claims.
"""
from __future__ import annotations

import argparse
import csv
import json
import math
import random
import statistics
from collections import Counter, defaultdict
from pathlib import Path
from typing import Iterable, Sequence


def read_csv_dicts(path: Path) -> list[dict[str, str]]:
    if not path.exists():
        raise FileNotFoundError(f"missing CSV: {path}")
    with path.open("r", encoding="utf-8-sig", newline="") as f:
        return list(csv.DictReader(f))


def write_csv(path: Path, rows: list[dict[str, object]], fieldnames: Sequence[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(fieldnames))
        w.writeheader()
        for row in rows:
            w.writerow(row)


def to_int(raw: object, default: int = 0) -> int:
    try:
        if raw is None or str(raw).strip() == "":
            return default
        return int(float(str(raw).strip()))
    except Exception:
        return default


def to_float(raw: object, default: float = 0.0) -> float:
    try:
        if raw is None or str(raw).strip() == "":
            return default
        return float(str(raw).strip())
    except Exception:
        return default


def zscore(obs: float, vals: Sequence[float]) -> float:
    if len(vals) <= 1:
        return 0.0
    sd = statistics.pstdev(vals)
    if sd == 0:
        if obs > statistics.fmean(vals):
            return float("inf")
        if obs < statistics.fmean(vals):
            return float("-inf")
        return 0.0
    return (obs - statistics.fmean(vals)) / sd


def empirical_right_tail(obs: float, vals: Sequence[float]) -> float:
    if not vals:
        return 1.0
    return (sum(1 for v in vals if v >= obs) + 1) / (len(vals) + 1)


def entropy_bits(counter: Counter[str]) -> float:
    total = sum(counter.values())
    if total <= 0:
        return 0.0
    out = 0.0
    for c in counter.values():
        p = c / total
        if p > 0:
            out -= p * math.log2(p)
    return out


def mutual_information_from_delta_to_state(transitions: list[tuple[str, str, str]]) -> float:
    # X = from_spacer + "|" + delta_family, Y = to_spacer
    n = len(transitions)
    if n == 0:
        return 0.0
    xy = Counter((a + "|" + d, b) for a, d, b in transitions)
    x = Counter(a + "|" + d for a, d, b in transitions)
    y = Counter(b for a, d, b in transitions)
    mi = 0.0
    for (xv, yv), c in xy.items():
        pxy = c / n
        px = x[xv] / n
        py = y[yv] / n
        mi += pxy * math.log2(pxy / (px * py))
    return mi


def weighted_top1_accuracy(transitions: list[tuple[str, str, str]]) -> float:
    # For each (from, delta), count dominant to-state; weighted by group size.
    if not transitions:
        return 0.0
    groups: dict[tuple[str, str], Counter[str]] = defaultdict(Counter)
    for a, d, b in transitions:
        groups[(a, d)][b] += 1
    correct = sum(counter.most_common(1)[0][1] for counter in groups.values())
    return correct / len(transitions)


def top_transition_count(transitions: list[tuple[str, str, str]]) -> int:
    if not transitions:
        return 0
    return Counter(transitions).most_common(1)[0][1]


def state_run_max(states: list[str]) -> int:
    if not states:
        return 0
    best = cur = 1
    for a, b in zip(states, states[1:]):
        if a == b:
            cur += 1
        else:
            best = max(best, cur)
            cur = 1
    return max(best, cur)


def transition_metrics(states: list[str], deltas: list[str]) -> dict[str, float]:
    transitions = [(states[i], deltas[i], states[i + 1]) for i in range(min(len(deltas), len(states)-1))]
    return {
        "weighted_top1_accuracy": weighted_top1_accuracy(transitions),
        "top_transition_count": float(top_transition_count(transitions)),
        "mutual_information_to_from_delta_bits": mutual_information_from_delta_to_state(transitions),
        "max_same_state_run_length": float(state_run_max(states)),
        "states": float(len(states)),
        "transitions": float(len(transitions)),
    }


def fixed_transition_rates(block_rows: list[dict[str, str]]) -> tuple[list[dict[str, object]], list[str], list[str]]:
    # Uses v0.4 block sequence rows; avoids relying on v0.9 buggy rates.
    rows = sorted(block_rows, key=lambda r: (to_int(r.get("start0")), to_int(r.get("end0")), str(r.get("cluster_id", ""))))
    states = [r.get("spacer_16bp", "") for r in rows]
    deltas = [r.get("next_delta_family_v04", "") for r in rows[:-1]]
    transitions = [(states[i], deltas[i], states[i+1]) for i in range(min(len(deltas), len(states)-1))]

    counter = Counter(transitions)
    from_totals = Counter()
    from_delta_totals = Counter()
    delta_totals = Counter()
    for a, d, b in transitions:
        from_totals[a] += 1
        from_delta_totals[(a, d)] += 1
        delta_totals[d] += 1

    out: list[dict[str, object]] = []
    for (a, d, b), c in counter.most_common():
        out.append({
            "from_spacer": a,
            "delta_family": d,
            "to_spacer": b,
            "count": c,
            "from_state_total": from_totals[a],
            "from_state_delta_total": from_delta_totals[(a, d)],
            "delta_total": delta_totals[d],
            "conditional_rate_from_state": c / from_totals[a] if from_totals[a] else 0.0,
            "conditional_rate_from_state_and_delta": c / from_delta_totals[(a, d)] if from_delta_totals[(a, d)] else 0.0,
            "share_of_delta_family": c / delta_totals[d] if delta_totals[d] else 0.0,
        })
    return out, states, deltas


def classify_hor_slot(start0: int, hor_period: int, slots: int) -> int:
    if hor_period <= 0 or slots <= 0:
        return 0
    phase = start0 % hor_period
    # Slot by uniform division, default 14 slots approximates 14 x 170 bp.
    return min(slots - 1, int(phase / hor_period * slots))


def load_block_repeat_groups(v09_dir: Path) -> dict[str, str]:
    # Optional. Maps cluster_id -> repeat group key from v0.9 annotations.
    candidates = [
        v09_dir / "cluster_v09_block_repeat_annotations.csv",
        v09_dir / "cluster_v09_block_repeat_annotation.csv",
    ]
    path = next((p for p in candidates if p.exists()), None)
    if path is None:
        return {}
    out = {}
    for r in read_csv_dicts(path):
        cid = r.get("cluster_id", "")
        # v0.9 writer may use group_key, repeat_group, best_group.
        group = r.get("group_key") or r.get("repeat_group") or r.get("best_repeat_group") or r.get("dominant_group") or r.get("repeat_key") or ""
        if cid and group:
            out[cid] = group
    return out


def hor_conditioned_shuffle(states: list[str], slot_ids: list[int], rnd: random.Random) -> list[str]:
    # Shuffle states only within each HOR slot to preserve slot-specific composition.
    out = states[:]
    by_slot: dict[int, list[int]] = defaultdict(list)
    for i, slot in enumerate(slot_ids):
        by_slot[slot].append(i)
    for idxs in by_slot.values():
        vals = [out[i] for i in idxs]
        rnd.shuffle(vals)
        for i, val in zip(idxs, vals):
            out[i] = val
    return out


def run(args: argparse.Namespace) -> int:
    seq_dir = Path(args.sequence_dir)
    v09_dir = Path(args.v09_dir)
    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    block_path = seq_dir / "cluster_v04_block_sequences.csv"
    if not block_path.exists():
        raise FileNotFoundError(f"v0.4 block sequence CSV fehlt: {block_path}")

    block_rows_all = read_csv_dicts(block_path)
    # Keep target rank and valid blocks.
    block_rows = [
        r for r in block_rows_all
        if str(r.get("template_rank", "")) == str(args.target_rank)
        and str(r.get("validation", "ok")) == "ok"
        and r.get("spacer_16bp", "")
    ]
    block_rows = sorted(block_rows, key=lambda r: (to_int(r.get("start0")), to_int(r.get("end0"))))

    repeat_by_cluster = load_block_repeat_groups(v09_dir)
    if repeat_by_cluster:
        # Keep only dominant group unless disabled.
        groups = Counter(repeat_by_cluster.get(r.get("cluster_id", ""), "") for r in block_rows)
        if "" in groups:
            del groups[""]
        dominant_group, dominant_count = groups.most_common(1)[0] if groups else ("", 0)
    else:
        dominant_group, dominant_count = ("", 0)

    if repeat_by_cluster and not args.keep_all_repeat_groups:
        block_rows = [r for r in block_rows if repeat_by_cluster.get(r.get("cluster_id", ""), "") == dominant_group]

    fixed_rows, states, deltas = fixed_transition_rates(block_rows)
    write_csv(out_dir / "cluster_v091_fixed_transitions.csv", fixed_rows, [
        "from_spacer", "delta_family", "to_spacer", "count",
        "from_state_total", "from_state_delta_total", "delta_total",
        "conditional_rate_from_state", "conditional_rate_from_state_and_delta",
        "share_of_delta_family"
    ])

    # Per-state outgoing summary.
    out_groups: dict[str, Counter[tuple[str, str]]] = defaultdict(Counter)
    for r in fixed_rows:
        out_groups[str(r["from_spacer"])][(str(r["delta_family"]), str(r["to_spacer"]))] += to_int(r["count"])
    state_summary = []
    for state, counter in sorted(out_groups.items(), key=lambda kv: -sum(kv[1].values())):
        total = sum(counter.values())
        top_key, top_count = counter.most_common(1)[0]
        state_summary.append({
            "from_spacer": state,
            "outgoing_total": total,
            "top_delta": top_key[0],
            "top_to_spacer": top_key[1],
            "top_count": top_count,
            "top_rate_from_state": top_count / total if total else 0.0,
            "outgoing_entropy_bits": entropy_bits(Counter({f"{d}->{b}": c for (d,b), c in counter.items()})),
        })
    write_csv(out_dir / "cluster_v091_state_outgoing_summary.csv", state_summary, [
        "from_spacer", "outgoing_total", "top_delta", "top_to_spacer",
        "top_count", "top_rate_from_state", "outgoing_entropy_bits"
    ])

    # v1.0 HOR slots.
    starts = [to_int(r.get("start0")) for r in block_rows]
    slot_ids = [classify_hor_slot(s, args.hor_period, args.hor_slots) for s in starts]
    hor_rows = []
    for i, r in enumerate(block_rows):
        hor_rows.append({
            "cluster_id": r.get("cluster_id", ""),
            "start0": starts[i],
            "end0": to_int(r.get("end0")),
            "spacer_16bp": states[i],
            "hor_period": args.hor_period,
            "hor_slots": args.hor_slots,
            "hor_phase": starts[i] % args.hor_period if args.hor_period else 0,
            "hor_slot": slot_ids[i],
            "repeat_group": repeat_by_cluster.get(r.get("cluster_id", ""), dominant_group),
            "next_delta_family": r.get("next_delta_family_v04", ""),
            "next_delta": r.get("next_delta", ""),
        })
    write_csv(out_dir / "cluster_v10_hor_slots.csv", hor_rows, [
        "cluster_id", "start0", "end0", "spacer_16bp",
        "hor_period", "hor_slots", "hor_phase", "hor_slot",
        "repeat_group", "next_delta_family", "next_delta"
    ])

    # HOR slot summary.
    slot_summary = []
    for slot in sorted(set(slot_ids)):
        idxs = [i for i, x in enumerate(slot_ids) if x == slot]
        c = Counter(states[i] for i in idxs)
        top_state, top_count = c.most_common(1)[0] if c else ("", 0)
        slot_summary.append({
            "hor_slot": slot,
            "blocks": len(idxs),
            "unique_states": len(c),
            "top_state": top_state,
            "top_state_count": top_count,
            "top_state_rate": top_count / len(idxs) if idxs else 0.0,
            "state_entropy_bits": entropy_bits(c),
        })
    write_csv(out_dir / "cluster_v10_hor_slot_summary.csv", slot_summary, [
        "hor_slot", "blocks", "unique_states", "top_state", "top_state_count", "top_state_rate", "state_entropy_bits"
    ])

    # HOR-conditioned observed metrics and null.
    obs_metrics = transition_metrics(states, deltas)
    rnd = random.Random(args.seed)
    null: dict[str, list[float]] = defaultdict(list)
    for _ in range(args.iterations):
        shuf_states = hor_conditioned_shuffle(states, slot_ids, rnd)
        m = transition_metrics(shuf_states, deltas)
        for k, v in m.items():
            null[k].append(v)

    shuffle_rows = []
    for metric, obs in obs_metrics.items():
        vals = null.get(metric, [])
        shuffle_rows.append({
            "metric": metric,
            "observed": obs,
            "null_mean": statistics.fmean(vals) if vals else "",
            "null_sd": statistics.pstdev(vals) if len(vals) > 1 else "",
            "z": zscore(obs, vals) if vals else "",
            "empirical_p_right_tail": empirical_right_tail(obs, vals) if vals else "",
            "iterations": len(vals),
            "null_model": f"shuffle_states_within_hor_slot_period_{args.hor_period}_slots_{args.hor_slots}",
        })
    write_csv(out_dir / "cluster_v10_hor_shuffle_summary.csv", shuffle_rows, [
        "metric", "observed", "null_mean", "null_sd", "z",
        "empirical_p_right_tail", "iterations", "null_model"
    ])

    # HOR transition rows from fixed transitions for convenience.
    write_csv(out_dir / "cluster_v10_hor_conditioned_transitions.csv", fixed_rows, [
        "from_spacer", "delta_family", "to_spacer", "count",
        "from_state_total", "from_state_delta_total", "delta_total",
        "conditional_rate_from_state", "conditional_rate_from_state_and_delta",
        "share_of_delta_family"
    ])

    # Gates.
    gates = []
    def add_gate(name: str, status: str, value: object, threshold: object, rationale: str) -> None:
        gates.append({"gate": name, "status": status, "value": value, "threshold": threshold, "rationale": rationale})

    add_gate("v091_rate_fix", "pass" if all(0 <= float(r["conditional_rate_from_state_and_delta"]) <= 1 for r in fixed_rows) else "fail",
             "bounded_rates", "all rates in [0,1]", "v0.9.1 corrected transition denominators.")
    # Find core metrics.
    def metric_row(name: str) -> dict[str, object]:
        return next((r for r in shuffle_rows if r["metric"] == name), {})

    wta = metric_row("weighted_top1_accuracy")
    mi = metric_row("mutual_information_to_from_delta_bits")
    top = metric_row("top_transition_count")
    # Pass thresholds: z >= 5 and p <= 0.01 for WTA/MI; top transition z>=5.
    for name, row in [("weighted_top1_accuracy", wta), ("mutual_information_to_from_delta_bits", mi), ("top_transition_count", top)]:
        z = to_float(row.get("z", 0))
        p = to_float(row.get("empirical_p_right_tail", 1))
        add_gate(f"hor_conditioned_{name}",
                 "pass" if z >= args.z_threshold and p <= args.p_threshold else "natural_model_not_broken",
                 f"z={z};p={p}", f"z>={args.z_threshold};p<={args.p_threshold}",
                 "Observed automaton remains stronger than HOR-slot-conditioned shuffle.")
    scoped_pass = all(g["status"] == "pass" for g in gates)

    add_gate("v10_scoped_claim",
             "pass" if scoped_pass else "not_released",
             "HOR-conditioned repeat-family null" if scoped_pass else "open",
             "all v1.0 gates pass",
             "If pass: HOR-slot-conditioned repeat family structure alone is not sufficient.")

    write_csv(out_dir / "cluster_v10_control_gates.csv", gates, ["gate", "status", "value", "threshold", "rationale"])

    manifest = {
        "version": "KALYX_v0.9.1_rate_fix_plus_v1.0_HOR_conditioned_test",
        "sequence_dir": str(seq_dir),
        "v09_dir": str(v09_dir),
        "out_dir": str(out_dir),
        "target_rank": args.target_rank,
        "blocks_used": len(block_rows),
        "dominant_repeat_group": dominant_group,
        "dominant_repeat_group_blocks_in_v09": dominant_count,
        "hor_period": args.hor_period,
        "hor_slots": args.hor_slots,
        "iterations": args.iterations,
        "claim_status": "hor_conditioned_repeat_model_not_sufficient" if scoped_pass else "not_released",
        "boundary": "Scoped model-break test; no artificial-origin proof."
    }
    (out_dir / "cluster_v10_manifest.json").write_text(json.dumps(manifest, indent=2, ensure_ascii=False), encoding="utf-8")

    top_fixed = "\n".join(
        f"| `{r['from_spacer']}` | `{r['delta_family']}` | `{r['to_spacer']}` | {r['count']} | {float(r['conditional_rate_from_state']):.12f} | {float(r['conditional_rate_from_state_and_delta']):.12f} |"
        for r in fixed_rows[:25]
    )
    shuffle_lines = "\n".join(
        f"| {r['metric']} | {r['observed']} | {r['null_mean']} | {r['z']} | {r['empirical_p_right_tail']} |"
        for r in shuffle_rows
    )
    gate_lines = "\n".join(
        f"| {g['gate']} | {g['status']} | {g['value']} | {g['threshold']} |"
        for g in gates
    )
    slot_lines = "\n".join(
        f"| {r['hor_slot']} | {r['blocks']} | {r['unique_states']} | `{r['top_state']}` | {r['top_state_count']} | {float(r['top_state_rate']):.12f} |"
        for r in slot_summary
    )
    report = f"""# KALYX v0.9.1 Rate-Fix + v1.0 Alpha-Satellite/HOR-conditioned Automaton Test

## Boundary

Dieser Lauf korrigiert zuerst die v0.9-Transition-Rates und testet danach,
ob der Spacer-State-Automat auch nach Konditionierung auf eine grobe HOR-Struktur
außergewöhnlich bleibt. Er beweist keinen natürlichen oder künstlichen Ursprung.

## Inputs

- v0.4 sequence dir: `{seq_dir}`
- v0.9 repeat-family dir: `{v09_dir}`
- target rank: `{args.target_rank}`
- blocks used: `{len(block_rows)}`
- dominant repeat group: `{dominant_group}`
- HOR period: `{args.hor_period}`
- HOR slots: `{args.hor_slots}`
- iterations: `{args.iterations}`

## v0.9.1 Rate-Fix

Die alte Spalte `conditional_rate` aus v0.9 wird ersetzt durch:

```text
conditional_rate_from_state
conditional_rate_from_state_and_delta
```

Beide liegen per Definition in `[0,1]`.

## Top corrected transitions

| from | delta | to | count | rate_from_state | rate_from_state_and_delta |
|---|---|---|---:|---:|---:|
{top_fixed}

## v1.0 HOR-slot summary

| hor_slot | blocks | unique_states | top_state | top_count | top_rate |
|---:|---:|---:|---|---:|---:|
{slot_lines}

## HOR-conditioned shuffle summary

| metric | observed | null_mean | z | empirical_p_right_tail |
|---|---:|---:|---:|---:|
{shuffle_lines}

## Control gates

| gate | status | value | threshold |
|---|---|---|---|
{gate_lines}

## Scoped claim

If `v10_scoped_claim = pass`, the allowed claim is:

```text
Die grobe Alpha-Satellite/HOR-Slot-Struktur allein erklärt den beobachteten
A/Spacer/B-Spacer-State-Automaten nicht.
```

This is not an artificial-origin proof. It narrows the remaining natural model
from "RepeatMasker family" to a more specific HOR/Alpha-Satellite mechanism that
must reproduce the same state grammar.

## Output files

```text
cluster_v091_fixed_transitions.csv
cluster_v091_state_outgoing_summary.csv
cluster_v10_hor_slots.csv
cluster_v10_hor_slot_summary.csv
cluster_v10_hor_conditioned_transitions.csv
cluster_v10_hor_shuffle_summary.csv
cluster_v10_control_gates.csv
cluster_v10_manifest.json
cluster_v10_report.md
```
"""
    (out_dir / "cluster_v10_report.md").write_text(report, encoding="utf-8")

    print(f"KALYX v0.9.1 + v1.0 complete: {out_dir}")
    print(f"  blocks_used={len(block_rows)} hor_period={args.hor_period} hor_slots={args.hor_slots}")
    print(f"  claim_status={manifest['claim_status']}")
    print(f"  report={out_dir / 'cluster_v10_report.md'}")
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description="KALYX v0.9.1 Rate-Fix + v1.0 HOR-conditioned Automaton Test")
    ap.add_argument("--sequence-dir", required=True, help="Directory containing v0.4 cluster_v04_block_sequences.csv")
    ap.add_argument("--v09-dir", required=True, help="Directory containing v0.9 outputs")
    ap.add_argument("--out-dir", required=True)
    ap.add_argument("--target-rank", type=int, default=3)
    ap.add_argument("--iterations", type=int, default=250)
    ap.add_argument("--seed", type=int, default=0x4B414C5958484F52)
    ap.add_argument("--hor-period", type=int, default=2380)
    ap.add_argument("--hor-slots", type=int, default=14)
    ap.add_argument("--z-threshold", type=float, default=5.0)
    ap.add_argument("--p-threshold", type=float, default=0.01)
    ap.add_argument("--keep-all-repeat-groups", action="store_true",
                    help="Do not restrict to dominant v0.9 repeat group.")
    raise SystemExit(run(ap.parse_args()))


if __name__ == "__main__":
    main()
