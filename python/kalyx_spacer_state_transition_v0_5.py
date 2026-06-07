#!/usr/bin/env python3
"""
KALYX Spacer-State Transition Decoder v0.5

Input:
  Decode_chr17_v04_real/cluster_v04_block_sequences.csv

Purpose:
  Lift v0.4 block/spacer extraction into a state-transition model:
    block_i.spacer_state -> delta_family -> block_{i+1}.spacer_state

Boundary:
  This tool analyzes observed sequence/transition structure only.
  It does not infer natural or artificial origin. Origin claims require
  external controls: RepeatMasker/segmental duplication context, GC-matched
  shuffles, cross-chromosome scans, genome-build replication, and independent
  lab reproduction.
"""
from __future__ import annotations

import argparse
import csv
import json
import math
import statistics
from collections import Counter, defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


def shannon_bits(seq: str) -> float:
    if not seq:
        return 0.0
    n = len(seq)
    c = Counter(seq)
    return -sum((v / n) * math.log2(v / n) for v in c.values())


def gc_rate(seq: str) -> float:
    if not seq:
        return 0.0
    return sum(1 for c in seq if c in "GC") / len(seq)


def hamming(a: str, b: str) -> int:
    if len(a) != len(b):
        return max(len(a), len(b))
    return sum(x != y for x, y in zip(a, b))


def consensus(seqs: list[str]) -> str:
    if not seqs:
        return ""
    n = max(len(s) for s in seqs)
    out = []
    for i in range(n):
        col = Counter(s[i] for s in seqs if i < len(s))
        out.append(col.most_common(1)[0][0] if col else "N")
    return "".join(out)


@dataclass(frozen=True)
class Block:
    cluster_id: str
    start0: int
    end0: int
    span: int
    window: str
    spacer: str
    full47: str
    next_delta: str
    delta_family: str
    validation: str


def read_blocks(path: Path, require_ok: bool = True) -> list[Block]:
    if not path.exists():
        raise FileNotFoundError(f"Missing input CSV: {path}")
    rows: list[Block] = []
    with path.open("rt", encoding="utf-8-sig", newline="") as f:
        r = csv.DictReader(f)
        required = {"cluster_id","start0","end0","span","window","spacer_16bp","full_47bp","next_delta_family_v04","validation"}
        missing = required - set(r.fieldnames or [])
        if missing:
            raise ValueError(f"Input CSV missing columns: {sorted(missing)}")
        for d in r:
            if require_ok and d.get("validation") != "ok":
                continue
            rows.append(Block(
                cluster_id=d["cluster_id"],
                start0=int(d["start0"]),
                end0=int(d["end0"]),
                span=int(d["span"]),
                window=d["window"],
                spacer=d["spacer_16bp"],
                full47=d.get("full_47bp", ""),
                next_delta=d.get("next_delta", ""),
                delta_family=d.get("next_delta_family_v04", ""),
                validation=d.get("validation", ""),
            ))
    rows.sort(key=lambda b: (b.start0, b.end0, b.cluster_id))
    return rows


def write_csv(path: Path, fieldnames: list[str], rows: Iterable[dict[str, object]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("wt", encoding="utf-8", newline="") as f:
        w = csv.DictWriter(f, fieldnames=fieldnames)
        w.writeheader()
        for row in rows:
            w.writerow(row)


def safe_float(v: float) -> str:
    return f"{v:.12f}"


def build_state_ids(spacers: list[str]) -> dict[str, str]:
    counts = Counter(spacers)
    ordered = [sp for sp, _ in counts.most_common()]
    return {sp: f"S{idx:03d}" for idx, sp in enumerate(ordered, 1)}


def build_state_table(blocks: list[Block], state_ids: dict[str, str]) -> list[dict[str, object]]:
    total = len(blocks) or 1
    counts = Counter(b.spacer for b in blocks)
    first_pos = {}
    last_pos = {}
    windows = defaultdict(Counter)
    full_variants = defaultdict(Counter)
    for b in blocks:
        first_pos.setdefault(b.spacer, b.start0)
        last_pos[b.spacer] = b.start0
        windows[b.spacer][b.window] += 1
        full_variants[b.spacer][b.full47] += 1

    # consensus among top states is useful for mutation family.
    top_spacers = [sp for sp, _ in counts.most_common(min(10, len(counts)))]
    cons = consensus(top_spacers)

    rows = []
    for rank, (sp, count) in enumerate(counts.most_common(), 1):
        sid = state_ids[sp]
        dom_win, dom_win_count = windows[sp].most_common(1)[0]
        dom_full, dom_full_count = full_variants[sp].most_common(1)[0]
        rows.append({
            "state_id": sid,
            "rank": rank,
            "spacer_16bp": sp,
            "count": count,
            "rate": safe_float(count / total),
            "gc": safe_float(gc_rate(sp)),
            "entropy_bits": safe_float(shannon_bits(sp)),
            "hamming_to_top_consensus": hamming(sp, cons) if cons else "",
            "dominant_window": dom_win,
            "dominant_window_count": dom_win_count,
            "first_start0": first_pos[sp],
            "last_start0": last_pos[sp],
            "full47_variants": len(full_variants[sp]),
            "dominant_full47": dom_full,
            "dominant_full47_count": dom_full_count,
        })
    return rows


def block_state_rows(blocks: list[Block], state_ids: dict[str, str]) -> list[dict[str, object]]:
    rows = []
    for i, b in enumerate(blocks):
        next_b = blocks[i + 1] if i + 1 < len(blocks) else None
        prev_b = blocks[i - 1] if i > 0 else None
        sid = state_ids[b.spacer]
        next_sid = state_ids[next_b.spacer] if next_b else ""
        prev_sid = state_ids[prev_b.spacer] if prev_b else ""
        observed_delta = next_b.start0 - b.start0 if next_b else ""
        rows.append({
            "index": i,
            "cluster_id": b.cluster_id,
            "start0": b.start0,
            "end0": b.end0,
            "window": b.window,
            "state_id": sid,
            "spacer_16bp": b.spacer,
            "prev_state_id": prev_sid,
            "next_state_id": next_sid,
            "next_spacer_16bp": next_b.spacer if next_b else "",
            "next_delta_observed": observed_delta,
            "next_delta_from_v04": b.next_delta,
            "next_delta_family": b.delta_family,
            "validation": b.validation,
        })
    return rows


def transition_rows(blocks: list[Block], state_ids: dict[str, str]) -> tuple[list[dict[str, object]], list[dict[str, object]]]:
    raw = []
    aggregate = Counter()
    aggregate_state = Counter()
    total = max(1, len(blocks) - 1)
    for i in range(len(blocks) - 1):
        a = blocks[i]
        b = blocks[i + 1]
        sid_a = state_ids[a.spacer]
        sid_b = state_ids[b.spacer]
        delta = b.start0 - a.start0
        fam = a.delta_family or "unknown"
        raw.append({
            "index": i,
            "from_cluster_id": a.cluster_id,
            "to_cluster_id": b.cluster_id,
            "from_start0": a.start0,
            "to_start0": b.start0,
            "delta": delta,
            "delta_family": fam,
            "from_state_id": sid_a,
            "to_state_id": sid_b,
            "from_spacer": a.spacer,
            "to_spacer": b.spacer,
        })
        aggregate[(sid_a, sid_b, fam)] += 1
        aggregate_state[(sid_a, sid_b)] += 1

    agg_rows = []
    by_from = Counter()
    for (from_sid, to_sid, fam), c in aggregate.items():
        by_from[(from_sid, fam)] += c
    for rank, ((from_sid, to_sid, fam), c) in enumerate(aggregate.most_common(), 1):
        denom = by_from[(from_sid, fam)] or 1
        agg_rows.append({
            "rank": rank,
            "from_state_id": from_sid,
            "to_state_id": to_sid,
            "delta_family": fam,
            "count": c,
            "rate_all": safe_float(c / total),
            "rate_within_from_state_delta": safe_float(c / denom),
        })
    return raw, agg_rows


def transition_matrix_rows(blocks: list[Block], state_ids: dict[str, str], top_n: int) -> tuple[list[str], list[dict[str, object]]]:
    counts = Counter(b.spacer for b in blocks)
    top_spacers = [sp for sp, _ in counts.most_common(top_n)]
    top_states = [state_ids[sp] for sp in top_spacers]
    trans = Counter()
    row_tot = Counter()
    for i in range(len(blocks) - 1):
        a = state_ids[blocks[i].spacer]
        b = state_ids[blocks[i + 1].spacer]
        if a in top_states and b in top_states:
            trans[(a, b)] += 1
            row_tot[a] += 1
    rows = []
    for a in top_states:
        row = {"from_state_id": a, "row_total": row_tot[a]}
        for b in top_states:
            row[b] = trans[(a, b)]
        rows.append(row)
    return ["from_state_id", "row_total", *top_states], rows


def state_delta_rows(blocks: list[Block], state_ids: dict[str, str]) -> list[dict[str, object]]:
    c = Counter()
    total_by_state = Counter()
    for b in blocks[:-1]:
        sid = state_ids[b.spacer]
        fam = b.delta_family or "unknown"
        c[(sid, fam)] += 1
        total_by_state[sid] += 1
    rows = []
    for rank, ((sid, fam), count) in enumerate(c.most_common(), 1):
        rows.append({
            "rank": rank,
            "state_id": sid,
            "delta_family": fam,
            "count": count,
            "rate_within_state": safe_float(count / max(1, total_by_state[sid])),
        })
    return rows


def run_length_rows(blocks: list[Block], state_ids: dict[str, str]) -> tuple[list[dict[str, object]], list[dict[str, object]]]:
    runs = []
    if not blocks:
        return [], []
    cur_sid = state_ids[blocks[0].spacer]
    cur_sp = blocks[0].spacer
    start_i = 0
    for i in range(1, len(blocks)):
        sid = state_ids[blocks[i].spacer]
        if sid != cur_sid:
            runs.append((cur_sid, cur_sp, start_i, i - 1))
            cur_sid = sid
            cur_sp = blocks[i].spacer
            start_i = i
    runs.append((cur_sid, cur_sp, start_i, len(blocks) - 1))

    rows = []
    hist = Counter()
    for rid, (sid, sp, a, b) in enumerate(runs, 1):
        length = b - a + 1
        hist[(sid, length)] += 1
        rows.append({
            "run_id": rid,
            "state_id": sid,
            "spacer_16bp": sp,
            "run_length": length,
            "index_start": a,
            "index_end": b,
            "start0": blocks[a].start0,
            "end0": blocks[b].end0,
            "span_genomic": blocks[b].end0 - blocks[a].start0,
            "window_start": blocks[a].window,
            "window_end": blocks[b].window,
        })
    hist_rows = []
    for (sid, length), count in hist.most_common():
        hist_rows.append({"state_id": sid, "run_length": length, "count": count})
    return rows, hist_rows


def ngram_rows(blocks: list[Block], state_ids: dict[str, str], n: int) -> list[dict[str, object]]:
    seq = [state_ids[b.spacer] for b in blocks]
    c = Counter(tuple(seq[i:i+n]) for i in range(max(0, len(seq)-n+1)))
    total = sum(c.values()) or 1
    rows = []
    for rank, (ng, count) in enumerate(c.most_common(), 1):
        rows.append({
            "rank": rank,
            "n": n,
            "state_ngram": "→".join(ng),
            "count": count,
            "rate": safe_float(count / total),
        })
    return rows


def main() -> int:
    ap = argparse.ArgumentParser(description="KALYX Spacer-State Transition Decoder v0.5")
    ap.add_argument("--sequence-dir", required=True, help="Directory containing cluster_v04_block_sequences.csv")
    ap.add_argument("--out-dir", required=True)
    ap.add_argument("--top-states", type=int, default=12)
    ap.add_argument("--ngram", type=int, default=3)
    ap.add_argument("--require-ok", action="store_true", default=True)
    args = ap.parse_args()

    sequence_dir = Path(args.sequence_dir)
    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    input_csv = sequence_dir / "cluster_v04_block_sequences.csv"
    blocks = read_blocks(input_csv, require_ok=args.require_ok)
    spacers = [b.spacer for b in blocks]
    state_ids = build_state_ids(spacers)

    states = build_state_table(blocks, state_ids)
    bstates = block_state_rows(blocks, state_ids)
    raw_trans, agg_trans = transition_rows(blocks, state_ids)
    mx_fields, mx_rows = transition_matrix_rows(blocks, state_ids, args.top_states)
    sdelta = state_delta_rows(blocks, state_ids)
    runs, run_hist = run_length_rows(blocks, state_ids)
    ngrams = ngram_rows(blocks, state_ids, args.ngram)

    write_csv(out_dir / "cluster_v05_states.csv", [
        "state_id","rank","spacer_16bp","count","rate","gc","entropy_bits","hamming_to_top_consensus",
        "dominant_window","dominant_window_count","first_start0","last_start0",
        "full47_variants","dominant_full47","dominant_full47_count"
    ], states)
    write_csv(out_dir / "cluster_v05_block_states.csv", [
        "index","cluster_id","start0","end0","window","state_id","spacer_16bp","prev_state_id",
        "next_state_id","next_spacer_16bp","next_delta_observed","next_delta_from_v04",
        "next_delta_family","validation"
    ], bstates)
    write_csv(out_dir / "cluster_v05_transitions_raw.csv", [
        "index","from_cluster_id","to_cluster_id","from_start0","to_start0","delta","delta_family",
        "from_state_id","to_state_id","from_spacer","to_spacer"
    ], raw_trans)
    write_csv(out_dir / "cluster_v05_transitions.csv", [
        "rank","from_state_id","to_state_id","delta_family","count","rate_all","rate_within_from_state_delta"
    ], agg_trans)
    write_csv(out_dir / "cluster_v05_transition_matrix.csv", mx_fields, mx_rows)
    write_csv(out_dir / "cluster_v05_state_delta_matrix.csv", [
        "rank","state_id","delta_family","count","rate_within_state"
    ], sdelta)
    write_csv(out_dir / "cluster_v05_runs.csv", [
        "run_id","state_id","spacer_16bp","run_length","index_start","index_end","start0","end0",
        "span_genomic","window_start","window_end"
    ], runs)
    write_csv(out_dir / "cluster_v05_run_lengths.csv", ["state_id","run_length","count"], run_hist)
    write_csv(out_dir / f"cluster_v05_state_ngrams_n{args.ngram}.csv", ["rank","n","state_ngram","count","rate"], ngrams)

    top_state = states[0] if states else {}
    top_trans = agg_trans[0] if agg_trans else {}
    top_run = max((r["run_length"] for r in runs), default=0)
    manifest = {
        "tool": "KALYX Spacer-State Transition Decoder",
        "version": "0.5",
        "boundary": "state-transition analysis only; no origin inference",
        "input_csv": str(input_csv.resolve()),
        "out_dir": str(out_dir.resolve()),
        "blocks": len(blocks),
        "states": len(states),
        "transitions_raw": len(raw_trans),
        "transitions_aggregated": len(agg_trans),
        "top_state": top_state,
        "top_transition": top_trans,
        "max_run_length": top_run,
        "top_states_for_matrix": args.top_states,
    }
    (out_dir / "cluster_v05_manifest.json").write_text(json.dumps(manifest, indent=2, ensure_ascii=False), encoding="utf-8")

    lines = [
        "# KALYX Spacer-State Transition Decoder v0.5",
        "",
        "## Boundary",
        "",
        "Dieses Artefakt modelliert Spacer als Zustände und Blockabstände als Übergangskanten.",
        "Es beweist keinen natürlichen oder künstlichen Ursprung. Ursprungsaussagen benötigen externe Kontrollen.",
        "",
        "## Summary",
        "",
        f"- input blocks: `{len(blocks)}`",
        f"- unique spacer states: `{len(states)}`",
        f"- raw transitions: `{len(raw_trans)}`",
        f"- aggregated transitions: `{len(agg_trans)}`",
        f"- max same-state run length: `{top_run}`",
    ]
    if top_state:
        lines += [
            f"- top state: `{top_state['state_id']}` spacer=`{top_state['spacer_16bp']}` count=`{top_state['count']}` rate=`{top_state['rate']}`",
        ]
    if top_trans:
        lines += [
            f"- top transition: `{top_trans['from_state_id']} --{top_trans['delta_family']}--> {top_trans['to_state_id']}` count=`{top_trans['count']}`",
        ]
    lines += [
        "",
        "## Top states",
        "",
        "| state | spacer | count | rate | dominant_window |",
        "|---|---|---:|---:|---:|",
    ]
    for row in states[:20]:
        lines.append(f"| {row['state_id']} | `{row['spacer_16bp']}` | {row['count']} | {row['rate']} | {row['dominant_window']} |")
    lines += [
        "",
        "## Top transitions",
        "",
        "| rank | transition | delta_family | count | rate_all | conditional_rate |",
        "|---:|---|---|---:|---:|---:|",
    ]
    for row in agg_trans[:30]:
        lines.append(f"| {row['rank']} | `{row['from_state_id']}→{row['to_state_id']}` | `{row['delta_family']}` | {row['count']} | {row['rate_all']} | {row['rate_within_from_state_delta']} |")
    lines += [
        "",
        "## Top run lengths",
        "",
        "| state | run_length | count |",
        "|---|---:|---:|",
    ]
    for row in run_hist[:30]:
        lines.append(f"| {row['state_id']} | {row['run_length']} | {row['count']} |")
    lines += [
        "",
        "## Output files",
        "",
        "```text",
        "cluster_v05_states.csv",
        "cluster_v05_block_states.csv",
        "cluster_v05_transitions_raw.csv",
        "cluster_v05_transitions.csv",
        "cluster_v05_transition_matrix.csv",
        "cluster_v05_state_delta_matrix.csv",
        "cluster_v05_runs.csv",
        "cluster_v05_run_lengths.csv",
        f"cluster_v05_state_ngrams_n{args.ngram}.csv",
        "cluster_v05_manifest.json",
        "cluster_v05_report.md",
        "```",
        "",
        "## Interpretation guardrail",
        "",
        "Wenn einzelne Übergänge hohe bedingte Wahrscheinlichkeiten haben, ist die Spacer-Folge geordnet.",
        "Wenn Run-Längen und N-Gramme stark konzentriert sind, entsteht ein echter Zustandsautomat.",
        "Erst nach RepeatMasker-/GC-/Cross-Chromosom-Kontrollen darf man über Ursprung sprechen.",
        "",
    ]
    (out_dir / "cluster_v05_report.md").write_text("\n".join(lines), encoding="utf-8")
    print(f"KALYX spacer-state transition decoder complete: {out_dir}")
    print(f"  blocks={len(blocks)} states={len(states)} transitions={len(raw_trans)}")
    print(f"  report={out_dir / 'cluster_v05_report.md'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
