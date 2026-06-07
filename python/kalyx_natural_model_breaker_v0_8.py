#!/usr/bin/env python3
"""
KALYX Natural Model Breaker v0.8

Final control layer for the chr17 A/Spacer/B spacer-state automaton.

It does NOT prove artificial origin. It tests whether specific natural
explanation classes supplied to this run remain adequate:
  - internal determinism gate from v0.6/v0.7
  - window-stratified state shuffle
  - GC-matched local sequence shuffle
  - RepeatMasker BED overlap
  - Segmental-duplication BED overlap
  - cross-chromosome full-47bp specificity

Only when all required controls are present and pass does the tool emit:
  "Die getesteten natürlichen Erklärungsmodelle tragen den Befund nicht mehr."

This statement is scoped to the tested models, not a universal origin proof.
"""
from __future__ import annotations

import argparse
import csv
import json
import math
import random
import statistics
from collections import Counter, defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Sequence


MOTIFS_8 = (
    "AGCATTCTCAGA",
    "GCATTCTCAGAA",
    "AAGCATTCTCAG",
    "GAAGCATTCTCA",
    "AGAAGCATTCTC",
    "ACAGAAGCATTC",
    "TGCATTCAACTC",
    "GCATTCAACTCA",
)

TEMPLATE3_OFFSETS = (0, 2, 3, 4, 5, 6, 34, 35)
TEMPLATE3_MOTIFS_BY_OFFSET = (
    "ACAGAAGCATTC",
    "AGAAGCATTCTC",
    "GAAGCATTCTCA",
    "AAGCATTCTCAG",
    "AGCATTCTCAGA",
    "GCATTCTCAGAA",
    "TGCATTCAACTC",
    "GCATTCAACTCA",
)
TEMPLATE3_MAP = dict(zip(TEMPLATE3_OFFSETS, TEMPLATE3_MOTIFS_BY_OFFSET))


def read_csv(path: Path) -> list[dict[str, str]]:
    if not path.exists():
        raise FileNotFoundError(f"CSV fehlt: {path}")
    with path.open("rt", encoding="utf-8-sig", newline="") as f:
        return list(csv.DictReader(f))


def write_csv(path: Path, fieldnames: Sequence[str], rows: Iterable[dict[str, object]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("wt", encoding="utf-8", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(fieldnames))
        w.writeheader()
        for r in rows:
            w.writerow(r)


def to_int(v: object, default: int = 0) -> int:
    try:
        if v is None or str(v).strip() == "":
            return default
        return int(float(str(v)))
    except Exception:
        return default


def to_float(v: object, default: float = 0.0) -> float:
    try:
        if v is None or str(v).strip() == "":
            return default
        return float(str(v))
    except Exception:
        return default


def safe_div(a: float, b: float) -> float:
    return a / b if b else 0.0


def gc_rate(seq: str) -> float:
    seq = seq.upper()
    return safe_div(sum(1 for c in seq if c in "GC"), len(seq))


def read_fasta(path: Path) -> str:
    parts: list[str] = []
    with path.open("rt", encoding="ascii", errors="ignore") as f:
        for line in f:
            if line.startswith(">"):
                continue
            s = line.strip().upper()
            if s:
                parts.append(s)
    seq = "".join(parts)
    if not seq:
        raise ValueError(f"FASTA enthält keine Sequenz: {path}")
    return seq


def discover_fastas(chrom_dir: Path) -> list[Path]:
    if not chrom_dir.exists():
        return []
    pats = ["*.fa", "*.fasta", "*.fna"]
    files: list[Path] = []
    for p in pats:
        files.extend(sorted(chrom_dir.glob(p)))
    return [p for p in files if p.is_file()]


def scan_hits(seq: str, motifs: Sequence[str]) -> list[tuple[int, str]]:
    """Return forward-only hits (pos0,motif). v0.8 cross/GC controls use forward
    motif grammar because the observed automaton is overwhelmingly plus-strand."""
    hits: list[tuple[int, str]] = []
    for motif in motifs:
        start = 0
        while True:
            pos = seq.find(motif, start)
            if pos < 0:
                break
            hits.append((pos, motif))
            start = pos + 1
    hits.sort(key=lambda x: (x[0], x[1]))
    return hits


def count_template3_clusters_from_hits(hits: list[tuple[int, str]], gap: int = 32) -> int:
    if not hits:
        return 0
    clusters: list[list[tuple[int, str]]] = []
    cur = [hits[0]]
    for h in hits[1:]:
        if h[0] - cur[-1][0] <= gap:
            cur.append(h)
        else:
            clusters.append(cur)
            cur = [h]
    clusters.append(cur)

    count = 0
    for c in clusters:
        start = min(p for p, _ in c)
        obs = {(p - start): m for p, m in c}
        if all(obs.get(off) == mot for off, mot in TEMPLATE3_MAP.items()):
            count += 1
    return count


def gc_random_sequence(length: int, gc: float, rng: random.Random) -> str:
    out = []
    for _ in range(length):
        if rng.random() < gc:
            out.append("G" if rng.random() < 0.5 else "C")
        else:
            out.append("A" if rng.random() < 0.5 else "T")
    return "".join(out)


def empirical_p_ge(observed: float, null_values: list[float]) -> float:
    if not null_values:
        return 1.0
    return (sum(1 for x in null_values if x >= observed) + 1) / (len(null_values) + 1)


def z_score(observed: float, null_values: list[float]) -> float:
    if len(null_values) < 2:
        return 0.0
    sd = statistics.pstdev(null_values)
    if sd == 0:
        return float("inf") if observed > statistics.fmean(null_values) else 0.0
    return (observed - statistics.fmean(null_values)) / sd


def transition_metrics(states: list[str], delta_fams: list[str]) -> dict[str, float]:
    # transitions: state_i + delta_i -> state_{i+1}
    n = min(len(states) - 1, len(delta_fams))
    if n <= 0:
        return {
            "transitions": 0.0,
            "weighted_top1_accuracy": 0.0,
            "top_transition_count": 0.0,
            "top_transition_rate": 0.0,
            "mutual_information_to_from_delta_bits": 0.0,
            "max_same_state_run_length": 0.0,
        }

    group_to_to = defaultdict(Counter)
    trip = Counter()
    for i in range(n):
        key = (states[i], delta_fams[i])
        to = states[i+1]
        group_to_to[key][to] += 1
        trip[(states[i], delta_fams[i], to)] += 1

    weighted_correct = sum(c.most_common(1)[0][1] for c in group_to_to.values())
    top_count = trip.most_common(1)[0][1] if trip else 0
    mi = mutual_information([(states[i], delta_fams[i]) for i in range(n)], [states[i+1] for i in range(n)])

    # max same-state run
    max_run = 1
    cur = 1
    for a, b in zip(states, states[1:]):
        if a == b:
            cur += 1
            max_run = max(max_run, cur)
        else:
            cur = 1

    return {
        "transitions": float(n),
        "weighted_top1_accuracy": safe_div(weighted_correct, n),
        "top_transition_count": float(top_count),
        "top_transition_rate": safe_div(top_count, n),
        "mutual_information_to_from_delta_bits": mi,
        "max_same_state_run_length": float(max_run),
    }


def mutual_information(xs: list[object], ys: list[object]) -> float:
    n = min(len(xs), len(ys))
    if n <= 0:
        return 0.0
    cx = Counter(xs[:n])
    cy = Counter(ys[:n])
    cxy = Counter(zip(xs[:n], ys[:n]))
    out = 0.0
    for (x, y), c in cxy.items():
        pxy = c / n
        px = cx[x] / n
        py = cy[y] / n
        out += pxy * math.log2(pxy / (px * py))
    return out


def window_stratified_shuffle(block_states_csv: Path, iterations: int, seed: int) -> tuple[list[dict[str, object]], dict[str, object]]:
    rows = read_csv(block_states_csv)
    rows = sorted(rows, key=lambda r: to_int(r.get("index", 0)))
    states = [r["state_id"] for r in rows]
    deltas = [r.get("next_delta_family", "") for r in rows[:-1]]

    observed = transition_metrics(states, deltas)
    by_window: dict[str, list[int]] = defaultdict(list)
    for i, r in enumerate(rows):
        by_window[str(r.get("window", ""))].append(i)

    rng = random.Random(seed)
    nulls: dict[str, list[float]] = {k: [] for k in observed}
    for _ in range(iterations):
        shuf = states[:]
        for idxs in by_window.values():
            vals = [shuf[i] for i in idxs]
            rng.shuffle(vals)
            for i, v in zip(idxs, vals):
                shuf[i] = v
        m = transition_metrics(shuf, deltas)
        for k, v in m.items():
            nulls[k].append(v)

    summary = []
    passed = 0
    core_metrics = ["weighted_top1_accuracy", "mutual_information_to_from_delta_bits", "top_transition_count", "max_same_state_run_length"]
    for k, obs in observed.items():
        vals = nulls[k]
        mean = statistics.fmean(vals) if vals else 0.0
        sd = statistics.pstdev(vals) if len(vals) > 1 else 0.0
        z = z_score(obs, vals)
        p = empirical_p_ge(obs, vals)
        status = "pass" if (k in core_metrics and z >= 5 and p <= 0.01) else ("info" if k not in core_metrics else "fail")
        if status == "pass":
            passed += 1
        summary.append({
            "metric": k,
            "observed": f"{obs:.12f}",
            "null_mean": f"{mean:.12f}",
            "null_sd": f"{sd:.12f}",
            "z": "inf" if math.isinf(z) else f"{z:.12f}",
            "p_empirical_ge": f"{p:.12f}",
            "iterations": iterations,
            "status": status,
        })

    gate = {
        "control": "window_stratified_shuffle",
        "status": "pass" if passed == len(core_metrics) else "fail",
        "passed_core_metrics": passed,
        "required_core_metrics": len(core_metrics),
    }
    return summary, gate


def bed_intervals(path: Path) -> list[tuple[str, int, int, str]]:
    intervals: list[tuple[str, int, int, str]] = []
    if not path.exists():
        return intervals
    with path.open("rt", encoding="utf-8", errors="ignore") as f:
        for line in f:
            if not line.strip() or line.startswith("#"):
                continue
            parts = line.rstrip("\n").split("\t")
            if len(parts) < 3:
                parts = line.split()
            if len(parts) < 3:
                continue
            chrom = parts[0]
            try:
                start = int(parts[1])
                end = int(parts[2])
            except Exception:
                continue
            name = parts[3] if len(parts) > 3 else ""
            intervals.append((chrom, start, end, name))
    intervals.sort(key=lambda x: (x[0], x[1], x[2]))
    return intervals


def overlap_len(a0: int, a1: int, b0: int, b1: int) -> int:
    return max(0, min(a1, b1) - max(a0, b0))


def overlap_control(block_seq_csv: Path, bed: Path, label: str, max_explained_fraction: float) -> tuple[list[dict[str, object]], list[dict[str, object]], dict[str, object]]:
    blocks = read_csv(block_seq_csv)
    intervals = bed_intervals(bed)
    rows: list[dict[str, object]] = []
    if not intervals:
        gate = {"control": label, "status": "missing", "reason": f"BED fehlt oder leer: {bed}"}
        return rows, [], gate

    # Use chr17 by default if BED contains chrom names; otherwise accept all intervals.
    by_chrom = defaultdict(list)
    for chrom, s, e, name in intervals:
        by_chrom[chrom].append((s, e, name))
    candidate_chroms = [c for c in by_chrom if c.lower().replace("chr", "") == "17"]
    if candidate_chroms:
        candidates = []
        for c in candidate_chroms:
            candidates.extend(by_chrom[c])
    else:
        candidates = [(s, e, name) for _, s, e, name in intervals]

    hit_blocks = 0
    total_bp = 0
    overlap_bp = 0
    annotation_counter = Counter()

    # Brute-force is okay for thousands of blocks; sort intervals and early skip.
    candidates.sort(key=lambda x: x[0])
    for b in blocks:
        s = to_int(b.get("start0"))
        e = to_int(b.get("end0"))
        span = max(0, e-s)
        total_bp += span
        best = 0
        names = []
        for rs, re, name in candidates:
            if re <= s:
                continue
            if rs >= e:
                break
            ov = overlap_len(s, e, rs, re)
            if ov:
                best += ov
                if name:
                    names.append(name)
        best = min(best, span)
        overlap_bp += best
        if best > 0:
            hit_blocks += 1
            for name in set(names[:8]):
                annotation_counter[name] += 1
        rows.append({
            "cluster_id": b.get("cluster_id", ""),
            "start0": s,
            "end0": e,
            "span": span,
            "overlap_bp": best,
            "overlap_fraction": f"{safe_div(best, span):.12f}",
            "annotation_names": "|".join(sorted(set(names))[:12]),
        })

    frac_blocks = safe_div(hit_blocks, len(blocks))
    frac_bp = safe_div(overlap_bp, total_bp)
    if frac_blocks <= max_explained_fraction and frac_bp <= max_explained_fraction:
        status = "pass"
        interpretation = "BED overlap is low; this annotation class does not explain the block set."
    elif frac_blocks >= 0.80 or frac_bp >= 0.80:
        status = "natural_model_not_broken"
        interpretation = "Most blocks overlap this annotation class; this natural model remains plausible and needs stratified follow-up."
    else:
        status = "inconclusive"
        interpretation = "Partial overlap; this natural model is not fully broken."

    summary = [{
        "control": label,
        "bed": str(bed),
        "blocks": len(blocks),
        "overlap_blocks": hit_blocks,
        "overlap_blocks_fraction": f"{frac_blocks:.12f}",
        "total_block_bp": total_bp,
        "overlap_bp": overlap_bp,
        "overlap_bp_fraction": f"{frac_bp:.12f}",
        "top_annotations": "|".join(f"{k}:{v}" for k, v in annotation_counter.most_common(20)),
        "status": status,
        "interpretation": interpretation,
    }]
    gate = {"control": label, "status": status, "overlap_blocks_fraction": frac_blocks, "overlap_bp_fraction": frac_bp}
    return rows, summary, gate


def gc_shuffle_control(block_seq_csv: Path, fasta: Path | None, iterations: int, seed: int, observed_template3: int) -> tuple[list[dict[str, object]], dict[str, object]]:
    blocks = read_csv(block_seq_csv)
    starts = [to_int(b.get("start0")) for b in blocks]
    ends = [to_int(b.get("end0")) for b in blocks]
    if not blocks:
        return [], {"control": "gc_matched_local_shuffle", "status": "missing", "reason": "no block sequence rows"}
    min_s, max_e = min(starts), max(ends)
    length = max_e - min_s
    if fasta and fasta.exists():
        seq = read_fasta(fasta)
        if max_e <= len(seq):
            region = seq[min_s:max_e]
            gc = gc_rate(region)
            source = "fasta_local_region"
        else:
            region = "".join(b.get("full_47bp", "") for b in blocks)
            gc = gc_rate(region)
            source = "fallback_observed_blocks_fasta_range_out"
    else:
        region = "".join(b.get("full_47bp", "") for b in blocks)
        gc = gc_rate(region)
        source = "fallback_observed_blocks_no_fasta"

    rng = random.Random(seed)
    rows = []
    null_template_counts = []
    null_hit_counts = []
    for i in range(iterations):
        rnd = gc_random_sequence(length, gc, rng)
        hits = scan_hits(rnd, MOTIFS_8)
        tpl = count_template3_clusters_from_hits(hits)
        null_template_counts.append(tpl)
        null_hit_counts.append(len(hits))
        rows.append({
            "iteration": i,
            "length": length,
            "gc": f"{gc:.12f}",
            "motif_hits": len(hits),
            "template3_clusters": tpl,
        })
    z = z_score(observed_template3, null_template_counts)
    p = empirical_p_ge(observed_template3, null_template_counts)
    mean_tpl = statistics.fmean(null_template_counts) if null_template_counts else 0.0
    sd_tpl = statistics.pstdev(null_template_counts) if len(null_template_counts) > 1 else 0.0
    status = "pass" if z >= 5 and p <= 0.01 and source == "fasta_local_region" else ("degraded_pass" if z >= 5 and p <= 0.01 else "fail")
    gate = {
        "control": "gc_matched_local_shuffle",
        "status": status,
        "observed_template3_clusters": observed_template3,
        "null_mean": mean_tpl,
        "null_sd": sd_tpl,
        "z": z,
        "p_empirical_ge": p,
        "iterations": iterations,
        "gc": gc,
        "length": length,
        "source": source,
    }
    return rows, gate


def cross_chromosome_control(block_seq_csv: Path, chrom_dir: Path | None, top_n: int) -> tuple[list[dict[str, object]], dict[str, object]]:
    blocks = read_csv(block_seq_csv)
    full_counts = Counter(b.get("full_47bp", "") for b in blocks if b.get("full_47bp", ""))
    top_full = [s for s, _ in full_counts.most_common(top_n)]
    observed_total = sum(full_counts[s] for s in top_full)
    if not chrom_dir or not chrom_dir.exists():
        return [], {"control": "cross_chromosome_scan", "status": "missing", "reason": "ChromDir not supplied"}

    rows = []
    max_non17 = 0
    total_non17 = 0
    files = discover_fastas(chrom_dir)
    if not files:
        return [], {"control": "cross_chromosome_scan", "status": "missing", "reason": f"no fasta files in {chrom_dir}"}

    for f in files:
        name = f.stem
        is_chr17 = name.lower() in ("chr17", "17") or "chr17" in name.lower()
        seq = read_fasta(f)
        chrom_total = 0
        for full in top_full:
            count = seq.count(full)
            chrom_total += count
            rows.append({
                "chrom_file": f.name,
                "is_chr17": int(is_chr17),
                "full47": full,
                "count": count,
            })
        if not is_chr17:
            total_non17 += chrom_total
            max_non17 = max(max_non17, chrom_total)

    # Strict pass means no non-17 chromosome approximates even 1% of observed top-full load.
    threshold = max(1, int(0.01 * observed_total))
    status = "pass" if max_non17 <= threshold else "natural_model_not_broken"
    gate = {
        "control": "cross_chromosome_scan",
        "status": status,
        "observed_top_full47_total_chr17_blocks": observed_total,
        "top_n_full47": top_n,
        "max_non17_full47_hits": max_non17,
        "total_non17_full47_hits": total_non17,
        "threshold_1pct_observed": threshold,
        "fastas_scanned": len(files),
    }
    return rows, gate


def load_v07_gate(evidence_dir: Path | None) -> dict[str, object]:
    if not evidence_dir:
        return {"control": "internal_evidence_gate_v07", "status": "missing"}
    gate_csv = evidence_dir / "cluster_v07_evidence_gates.csv"
    report = evidence_dir / "cluster_v07_report.md"
    if gate_csv.exists():
        rows = read_csv(gate_csv)
        statuses = [r.get("status", "") for r in rows]
        if rows and all(s == "pass" for s in statuses):
            return {"control": "internal_evidence_gate_v07", "status": "pass", "gates": len(rows)}
        return {"control": "internal_evidence_gate_v07", "status": "fail", "gates": len(rows), "statuses": "|".join(statuses)}
    if report.exists():
        txt = report.read_text(encoding="utf-8", errors="ignore")
        if "gates passed: `6`" in txt and "gates missing: `0`" in txt:
            return {"control": "internal_evidence_gate_v07", "status": "pass", "gates": 6, "source": "report"}
    return {"control": "internal_evidence_gate_v07", "status": "missing"}


def gate_row(control: str, status: str, required: str, observed: object = "", null: object = "", z: object = "", p: object = "", note: str = "") -> dict[str, object]:
    return {
        "control": control,
        "status": status,
        "required_for_final_claim": required,
        "observed": observed,
        "null_or_reference": null,
        "z": z,
        "p": p,
        "note": note,
    }


def main() -> int:
    ap = argparse.ArgumentParser(description="KALYX Natural Model Breaker v0.8")
    ap.add_argument("--sequence-dir", required=True, help="Decode_chr17_v04_real")
    ap.add_argument("--state-dir", required=True, help="Decode_chr17_v05_real")
    ap.add_argument("--determinism-dir", default="", help="Decode_chr17_v06_real")
    ap.add_argument("--evidence-dir", default="", help="Decode_chr17_v07_real")
    ap.add_argument("--out-dir", required=True)
    ap.add_argument("--fasta", default="", help="chr17.fa for local GC shuffle")
    ap.add_argument("--chrom-dir", default="", help="Directory with chr*.fa for cross-chromosome scan")
    ap.add_argument("--repeatmasker-bed", default="", help="RepeatMasker BED for chr17/hg38")
    ap.add_argument("--segmental-dup-bed", default="", help="Segmental duplication BED for chr17/hg38")
    ap.add_argument("--iterations", type=int, default=1000)
    ap.add_argument("--seed", type=int, default=0x4B414C59583038)
    ap.add_argument("--min-z", type=float, default=5.0)
    ap.add_argument("--max-p", type=float, default=0.01)
    ap.add_argument("--max-repeat-explained-fraction", type=float, default=0.10)
    ap.add_argument("--cross-top-full47", type=int, default=20)
    args = ap.parse_args()

    seq_dir = Path(args.sequence_dir)
    state_dir = Path(args.state_dir)
    det_dir = Path(args.determinism_dir) if args.determinism_dir else None
    ev_dir = Path(args.evidence_dir) if args.evidence_dir else None
    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    block_seq_csv = seq_dir / "cluster_v04_block_sequences.csv"
    block_states_csv = state_dir / "cluster_v05_block_states.csv"
    if not block_seq_csv.exists():
        raise FileNotFoundError(f"cluster_v04_block_sequences.csv fehlt: {block_seq_csv}")
    if not block_states_csv.exists():
        raise FileNotFoundError(f"cluster_v05_block_states.csv fehlt: {block_states_csv}")

    block_rows = read_csv(block_seq_csv)
    observed_template3 = len(block_rows)

    # 1 internal v07 gate
    v07_gate = load_v07_gate(ev_dir)

    # 2 window stratified shuffle
    window_rows, window_gate = window_stratified_shuffle(block_states_csv, args.iterations, args.seed)
    write_csv(out_dir / "cluster_v08_window_shuffle_summary.csv",
              ["metric","observed","null_mean","null_sd","z","p_empirical_ge","iterations","status"], window_rows)

    # 3 GC shuffle
    fasta = Path(args.fasta) if args.fasta else None
    gc_rows, gc_gate = gc_shuffle_control(block_seq_csv, fasta, args.iterations, args.seed ^ 0x4743, observed_template3)
    write_csv(out_dir / "cluster_v08_gc_shuffle_iterations.csv",
              ["iteration","length","gc","motif_hits","template3_clusters"], gc_rows)

    # 4 RepeatMasker
    repeat_gate = {"control": "repeatmasker_overlap", "status": "missing", "reason": "RepeatMasker BED not supplied"}
    if args.repeatmasker_bed:
        rows, summary, repeat_gate = overlap_control(block_seq_csv, Path(args.repeatmasker_bed), "repeatmasker_overlap", args.max_repeat_explained_fraction)
        write_csv(out_dir / "cluster_v08_repeatmasker_overlap.csv",
                  ["cluster_id","start0","end0","span","overlap_bp","overlap_fraction","annotation_names"], rows)
        write_csv(out_dir / "cluster_v08_repeatmasker_summary.csv",
                  ["control","bed","blocks","overlap_blocks","overlap_blocks_fraction","total_block_bp","overlap_bp","overlap_bp_fraction","top_annotations","status","interpretation"], summary)

    # 5 Segmental duplication
    seg_gate = {"control": "segmental_duplication_overlap", "status": "missing", "reason": "Segmental duplication BED not supplied"}
    if args.segmental_dup_bed:
        rows, summary, seg_gate = overlap_control(block_seq_csv, Path(args.segmental_dup_bed), "segmental_duplication_overlap", args.max_repeat_explained_fraction)
        write_csv(out_dir / "cluster_v08_segmental_dup_overlap.csv",
                  ["cluster_id","start0","end0","span","overlap_bp","overlap_fraction","annotation_names"], rows)
        write_csv(out_dir / "cluster_v08_segmental_dup_summary.csv",
                  ["control","bed","blocks","overlap_blocks","overlap_blocks_fraction","total_block_bp","overlap_bp","overlap_bp_fraction","top_annotations","status","interpretation"], summary)

    # 6 cross chromosome
    chrom_dir = Path(args.chrom_dir) if args.chrom_dir else None
    cross_rows, cross_gate = cross_chromosome_control(block_seq_csv, chrom_dir, args.cross_top_full47)
    write_csv(out_dir / "cluster_v08_cross_chromosome_full47.csv",
              ["chrom_file","is_chr17","full47","count"], cross_rows)

    gates = []
    gates.append(gate_row("internal_evidence_gate_v07", v07_gate.get("status","missing"), "yes", note=json.dumps(v07_gate, ensure_ascii=False)))
    gates.append(gate_row("window_stratified_shuffle", window_gate.get("status","missing"), "yes", note=json.dumps(window_gate, ensure_ascii=False)))
    gates.append(gate_row("gc_matched_local_shuffle", gc_gate.get("status","missing"), "yes",
                          observed=gc_gate.get("observed_template3_clusters",""), null=f"mean={gc_gate.get('null_mean','')}", z=gc_gate.get("z",""), p=gc_gate.get("p_empirical_ge",""), note=json.dumps(gc_gate, ensure_ascii=False)))
    gates.append(gate_row("repeatmasker_overlap", repeat_gate.get("status","missing"), "yes", note=json.dumps(repeat_gate, ensure_ascii=False)))
    gates.append(gate_row("segmental_duplication_overlap", seg_gate.get("status","missing"), "yes", note=json.dumps(seg_gate, ensure_ascii=False)))
    gates.append(gate_row("cross_chromosome_scan", cross_gate.get("status","missing"), "yes", note=json.dumps(cross_gate, ensure_ascii=False)))

    pass_statuses = {"pass", "degraded_pass"}  # degraded counts as not final for strict below.
    strict_pass = all(g["status"] == "pass" for g in gates)
    weak_pass = all(g["status"] in pass_statuses for g in gates)
    missing = [g["control"] for g in gates if g["status"] == "missing"]
    not_broken = [g["control"] for g in gates if g["status"] == "natural_model_not_broken"]
    failed = [g["control"] for g in gates if g["status"] == "fail"]
    inconclusive = [g["control"] for g in gates if g["status"] == "inconclusive"]

    if strict_pass:
        final_statement = "Die getesteten natürlichen Erklärungsmodelle tragen den Befund nicht mehr."
        final_status = "final_scoped_claim_released"
    elif weak_pass and not missing and not not_broken and not failed and not inconclusive:
        final_statement = "Die Kontrollen sprechen stark gegen die getesteten Modelle; wegen degradierter Eingaben bleibt der finale Satz zurückgestellt."
        final_status = "strong_but_degraded"
    else:
        final_statement = "Der finale Satz ist noch nicht freigegeben; mindestens eine externe Kontrollklasse fehlt, ist nicht gebrochen oder ist inkonklusiv."
        final_status = "not_released"

    write_csv(out_dir / "cluster_v08_control_gates.csv",
              ["control","status","required_for_final_claim","observed","null_or_reference","z","p","note"], gates)

    manifest = {
        "tool": "KALYX Natural Model Breaker",
        "version": "0.8",
        "boundary": "scoped natural-model stress test; not artificial-origin proof",
        "sequence_dir": str(seq_dir),
        "state_dir": str(state_dir),
        "determinism_dir": str(det_dir) if det_dir else "",
        "evidence_dir": str(ev_dir) if ev_dir else "",
        "out_dir": str(out_dir),
        "iterations": args.iterations,
        "observed_template3_blocks": observed_template3,
        "final_status": final_status,
        "final_statement": final_statement,
        "missing_controls": missing,
        "not_broken_controls": not_broken,
        "failed_controls": failed,
        "inconclusive_controls": inconclusive,
        "gates": gates,
    }
    (out_dir / "cluster_v08_manifest.json").write_text(json.dumps(manifest, indent=2, ensure_ascii=False), encoding="utf-8")

    # Compact report
    report = []
    report.append("# KALYX Natural Model Breaker v0.8")
    report.append("")
    report.append("## Boundary")
    report.append("")
    report.append("Dieses Artefakt ist die letzte Kontrollschicht für den bisherigen Lauf. Es beweist keinen künstlichen Ursprung. Es prüft, ob die in diesem Lauf tatsächlich getesteten natürlichen Erklärungsmodelle den Befund noch tragen.")
    report.append("")
    report.append("## Final status")
    report.append("")
    report.append(f"- final_status: `{final_status}`")
    report.append(f"- final_statement: **{final_statement}**")
    report.append(f"- observed template-3 / A-Spacer-B blocks: `{observed_template3}`")
    report.append("")
    report.append("## Gates")
    report.append("")
    report.append("| control | status | required | note |")
    report.append("|---|---|---|---|")
    for g in gates:
        note = str(g.get("note","")).replace("|","/")[:240]
        report.append(f"| `{g['control']}` | `{g['status']}` | `{g['required_for_final_claim']}` | {note} |")
    report.append("")
    report.append("## Window-stratified shuffle")
    report.append("")
    report.append("| metric | observed | null_mean | z | p | status |")
    report.append("|---|---:|---:|---:|---:|---|")
    for r in window_rows:
        if r["metric"] in ("weighted_top1_accuracy","mutual_information_to_from_delta_bits","top_transition_count","max_same_state_run_length"):
            report.append(f"| `{r['metric']}` | {r['observed']} | {r['null_mean']} | {r['z']} | {r['p_empirical_ge']} | `{r['status']}` |")
    report.append("")
    report.append("## GC-matched local shuffle")
    report.append("")
    report.append(f"- status: `{gc_gate.get('status','missing')}`")
    report.append(f"- observed template-3 blocks: `{gc_gate.get('observed_template3_clusters','')}`")
    report.append(f"- null mean: `{gc_gate.get('null_mean','')}`")
    report.append(f"- z: `{gc_gate.get('z','')}`")
    report.append(f"- p_empirical_ge: `{gc_gate.get('p_empirical_ge','')}`")
    report.append(f"- source: `{gc_gate.get('source','')}`")
    report.append("")
    report.append("## Interpretation")
    report.append("")
    if strict_pass:
        report.append("Alle erforderlichen Kontrollgates sind vorhanden und bestanden. Der freigegebene Satz ist bewusst scoped: Die getesteten natürlichen Erklärungsmodelle tragen den Befund nicht mehr. Das ist keine absolute Ursprungsbehauptung, aber eine starke wissenschaftliche Eskalation.")
    else:
        report.append("Mindestens ein erforderliches externes Gate fehlt, bleibt inkonklusiv oder trägt ein natürliches Modell weiter. Der finale Satz bleibt deshalb gesperrt, bis die fehlenden Daten nachgereicht und bestanden sind.")
    report.append("")
    report.append("## Output files")
    report.append("")
    report.append("```text")
    report.append("cluster_v08_control_gates.csv")
    report.append("cluster_v08_window_shuffle_summary.csv")
    report.append("cluster_v08_gc_shuffle_iterations.csv")
    report.append("cluster_v08_repeatmasker_overlap.csv        optional")
    report.append("cluster_v08_repeatmasker_summary.csv        optional")
    report.append("cluster_v08_segmental_dup_overlap.csv       optional")
    report.append("cluster_v08_segmental_dup_summary.csv       optional")
    report.append("cluster_v08_cross_chromosome_full47.csv")
    report.append("cluster_v08_manifest.json")
    report.append("cluster_v08_report.md")
    report.append("```")
    (out_dir / "cluster_v08_report.md").write_text("\n".join(report) + "\n", encoding="utf-8")

    print(f"KALYX Natural Model Breaker v0.8 complete: {out_dir}")
    print(f"  final_status={final_status}")
    print(f"  final_statement={final_statement}")
    print(f"  report={out_dir / 'cluster_v08_report.md'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
