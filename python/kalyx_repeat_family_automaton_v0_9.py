#!/usr/bin/env python3
"""
KALYX Repeat-Family Conditioned Automaton Test v0.9

Purpose
-------
v0.8 showed that the signal sits inside RepeatMasker context. v0.9 therefore
does NOT ask "RepeatMasker yes/no" anymore. It asks a stricter question:

  Within the same RepeatMasker family/name/class, is the A/Spacer/B block and
  its spacer-state transition automaton still exceptional?

Inputs
------
- cluster_v04_block_sequences.csv       (from v0.4)
- repeatmasker_chr17.bed                (from v0.8.1 UCSC builder)
- chr17.fa                              (optional but recommended for scanning)
- optional target repeat filters

Outputs
-------
- cluster_v09_block_repeat_annotations.csv
- cluster_v09_repeat_group_summary.csv
- cluster_v09_family_skeleton_blocks.csv
- cluster_v09_family_spacer_states.csv
- cluster_v09_family_transitions.csv
- cluster_v09_family_shuffle_summary.csv
- cluster_v09_control_gates.csv
- cluster_v09_manifest.json
- cluster_v09_report.md

Boundary
--------
This script tests a repeat-family-conditioned null model. It does not prove
natural or artificial origin. If the family-conditioned shuffle breaks, the
allowed claim is:

  The observed automaton is not explained by simple membership in the tested
  RepeatMasker family alone.
"""
from __future__ import annotations

import argparse
import csv
import json
import math
import random
import statistics
from collections import Counter, defaultdict
from dataclasses import dataclass, asdict
from pathlib import Path
from typing import Iterable, Sequence


A_EXPECT = "ACAGAAGCATTCTCAGAA"
B_EXPECT = "TGCATTCAACTCA"
BLOCK_LEN = 47
SPACER_LEN = 16


@dataclass(frozen=True)
class Block:
    cluster_id: str
    start0: int
    end0: int
    span: int
    window: str
    template_rank: str
    strand_set: str
    spacer: str
    full47: str
    next_delta: str
    next_delta_family: str


@dataclass(frozen=True)
class BedRepeat:
    chrom: str
    start0: int
    end0: int
    name: str
    score: str
    strand: str
    rep_class: str
    rep_family: str
    rep_name: str

    @property
    def group_key(self) -> str:
        return f"{self.rep_name}|{self.rep_class}|{self.rep_family}"

    @property
    def length(self) -> int:
        return max(0, self.end0 - self.start0)


@dataclass(frozen=True)
class SkeletonBlock:
    chrom: str
    start0: int
    end0: int
    repeat_key: str
    spacer: str
    full47: str
    source: str


def read_csv_dicts(path: Path) -> list[dict[str, str]]:
    if not path.exists():
        raise FileNotFoundError(path)
    with path.open("r", encoding="utf-8-sig", newline="") as f:
        return list(csv.DictReader(f))


def read_blocks(path: Path) -> list[Block]:
    rows = read_csv_dicts(path)
    out: list[Block] = []
    required = {"cluster_id", "start0", "end0", "span", "window", "template_rank",
                "strand_set", "spacer_16bp", "full_47bp", "next_delta", "next_delta_family_v04"}
    missing = required - set(rows[0].keys() if rows else [])
    if missing:
        raise ValueError(f"{path} missing columns: {sorted(missing)}")
    for r in rows:
        out.append(Block(
            cluster_id=r["cluster_id"],
            start0=int(r["start0"]),
            end0=int(r["end0"]),
            span=int(r["span"]),
            window=r["window"],
            template_rank=r["template_rank"],
            strand_set=r["strand_set"],
            spacer=r["spacer_16bp"],
            full47=r["full_47bp"],
            next_delta=r["next_delta"],
            next_delta_family=r["next_delta_family_v04"],
        ))
    out.sort(key=lambda b: (b.start0, b.end0, b.cluster_id))
    return out


def parse_bed(path: Path, chrom_filter: str = "chr17") -> list[BedRepeat]:
    out: list[BedRepeat] = []
    with path.open("r", encoding="utf-8-sig", newline="") as f:
        for line in f:
            if not line.strip() or line.startswith("#"):
                continue
            parts = line.rstrip("\n").split("\t")
            if len(parts) < 3:
                continue
            chrom = parts[0]
            if chrom_filter and chrom != chrom_filter:
                continue
            start = int(parts[1])
            end = int(parts[2])
            name = parts[3] if len(parts) > 3 else "."
            score = parts[4] if len(parts) > 4 else "0"
            strand = parts[5] if len(parts) > 5 else "."
            # v0.8.1 BED: chrom start end name score strand rep_class rep_family rep_name
            if len(parts) >= 9:
                rep_class = parts[6]
                rep_family = parts[7]
                rep_name = parts[8]
            else:
                # Fallback: parse name as repName|repClass|repFamily if possible
                toks = name.split("|")
                rep_name = toks[0] if len(toks) > 0 else name
                rep_class = toks[1] if len(toks) > 1 else "."
                rep_family = toks[2] if len(toks) > 2 else "."
            out.append(BedRepeat(chrom, start, end, name, score, strand, rep_class, rep_family, rep_name))
    out.sort(key=lambda x: (x.start0, x.end0, x.group_key))
    return out


def read_fasta(path: Path) -> str:
    seq: list[str] = []
    with path.open("r", encoding="ascii", errors="replace") as f:
        for line in f:
            if line.startswith(">"):
                continue
            s = line.strip().upper()
            if s:
                seq.append(s)
    s = "".join(seq)
    if not s:
        raise ValueError(f"empty FASTA: {path}")
    return s


def overlap_len(a0: int, a1: int, b0: int, b1: int) -> int:
    return max(0, min(a1, b1) - max(a0, b0))


def annotate_blocks(blocks: Sequence[Block], repeats: Sequence[BedRepeat]) -> list[dict[str, object]]:
    # two-pointer over sorted repeats/blocks
    rows: list[dict[str, object]] = []
    j = 0
    n = len(repeats)
    for b in blocks:
        while j < n and repeats[j].end0 <= b.start0:
            j += 1
        k = j
        overlaps: list[tuple[int, BedRepeat]] = []
        while k < n and repeats[k].start0 < b.end0:
            ov = overlap_len(b.start0, b.end0, repeats[k].start0, repeats[k].end0)
            if ov > 0:
                overlaps.append((ov, repeats[k]))
            k += 1
        if not overlaps:
            rows.append({
                "cluster_id": b.cluster_id, "start0": b.start0, "end0": b.end0, "span": b.span,
                "spacer_16bp": b.spacer, "full_47bp": b.full47, "next_delta": b.next_delta,
                "next_delta_family": b.next_delta_family, "overlap_bp": 0, "overlap_fraction": 0.0,
                "repeat_key": "", "rep_name": "", "rep_class": "", "rep_family": "",
                "repeat_start0": "", "repeat_end0": "",
            })
            continue
        # strongest single overlap row
        ov, r = max(overlaps, key=lambda x: (x[0], x[1].length))
        rows.append({
            "cluster_id": b.cluster_id, "start0": b.start0, "end0": b.end0, "span": b.span,
            "spacer_16bp": b.spacer, "full_47bp": b.full47, "next_delta": b.next_delta,
            "next_delta_family": b.next_delta_family, "overlap_bp": ov,
            "overlap_fraction": ov / max(1, b.span),
            "repeat_key": r.group_key, "rep_name": r.rep_name, "rep_class": r.rep_class,
            "rep_family": r.rep_family, "repeat_start0": r.start0, "repeat_end0": r.end0,
        })
    return rows


def write_csv(path: Path, fieldnames: Sequence[str], rows: Iterable[dict[str, object]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(fieldnames))
        w.writeheader()
        for r in rows:
            w.writerow(r)


def window_shuffle_metrics(blocks: Sequence[Block], states: Sequence[str] | None = None, seed: int = 0) -> dict[str, float]:
    """Compute transition determinism for block order; if states supplied, use them."""
    if states is None:
        states = [b.spacer for b in blocks]
    delta_fams = [b.next_delta_family for b in blocks[:-1]]
    from_states = states[:-1]
    to_states = states[1:]
    groups: dict[tuple[str, str], Counter[str]] = defaultdict(Counter)
    for s, d, t in zip(from_states, delta_fams, to_states):
        groups[(s, d)][t] += 1
    total = sum(sum(c.values()) for c in groups.values())
    if total == 0:
        return {"weighted_top1_accuracy": 0.0, "top_transition_count": 0.0, "mutual_information_bits": 0.0}
    weighted = 0
    top = 0
    for counter in groups.values():
        m = counter.most_common(1)[0][1]
        weighted += m
        top = max(top, m)
    # MI between (from_state, delta) and to_state
    x_counts: Counter[tuple[str, str]] = Counter()
    y_counts: Counter[str] = Counter()
    xy_counts: Counter[tuple[tuple[str, str], str]] = Counter()
    for s, d, t in zip(from_states, delta_fams, to_states):
        x = (s, d)
        x_counts[x] += 1
        y_counts[t] += 1
        xy_counts[(x, t)] += 1
    mi = 0.0
    for (x, y), c in xy_counts.items():
        pxy = c / total
        px = x_counts[x] / total
        py = y_counts[y] / total
        mi += pxy * math.log2(pxy / (px * py))
    return {
        "weighted_top1_accuracy": weighted / total,
        "top_transition_count": float(top),
        "mutual_information_bits": mi,
    }


def empirical_right_tail(obs: float, nulls: Sequence[float]) -> float:
    # conservative +1 empirical p
    ge = sum(1 for x in nulls if x >= obs)
    return (ge + 1) / (len(nulls) + 1)


def zscore(obs: float, vals: Sequence[float]) -> float:
    if not vals:
        return 0.0
    mean = statistics.fmean(vals)
    if len(vals) < 2:
        return 0.0
    sd = statistics.pstdev(vals)
    if sd == 0:
        return float("inf") if obs > mean else 0.0
    return (obs - mean) / sd


def scan_skeleton_blocks(seq: str, repeats: Sequence[BedRepeat], group_filter: set[str] | None = None) -> list[SkeletonBlock]:
    out: list[SkeletonBlock] = []
    seen: set[tuple[int, str]] = set()
    for r in repeats:
        if group_filter is not None and r.group_key not in group_filter:
            continue
        start = max(0, r.start0)
        end = min(len(seq), r.end0)
        if end - start < BLOCK_LEN:
            continue
        # scan A within interval such that B at +34 is also in interval
        region = seq[start:end]
        pos = region.find(A_EXPECT)
        while pos != -1:
            gpos = start + pos
            if gpos + BLOCK_LEN <= end:
                bseq = seq[gpos+34:gpos+47]
                if bseq == B_EXPECT:
                    full = seq[gpos:gpos+47]
                    spacer = seq[gpos+18:gpos+34]
                    key = (gpos, r.group_key)
                    if key not in seen:
                        seen.add(key)
                        out.append(SkeletonBlock(r.chrom, gpos, gpos+47, r.group_key, spacer, full, "repeat_family_scan"))
            pos = region.find(A_EXPECT, pos + 1)
    out.sort(key=lambda x: (x.start0, x.repeat_key))
    return out


def gc_fraction(s: str) -> float:
    return (s.count("G") + s.count("C")) / len(s) if s else 0.0


def main() -> int:
    ap = argparse.ArgumentParser(description="KALYX Repeat-Family Conditioned Automaton Test v0.9")
    ap.add_argument("--sequence-dir", required=True, help="Decode_chr17_v04_real directory")
    ap.add_argument("--repeatmasker-bed", required=True, help="repeatmasker_chr17.bed from v0.8.1")
    ap.add_argument("--fasta", required=True, help="chr17.fa")
    ap.add_argument("--out-dir", required=True)
    ap.add_argument("--chrom", default="chr17")
    ap.add_argument("--iterations", type=int, default=250)
    ap.add_argument("--seed", type=int, default=0x4B414C59583039)
    ap.add_argument("--min-overlap-fraction", type=float, default=0.80)
    ap.add_argument("--dominant-group", default="", help="Optional repeat_key, e.g. ALR/Alpha|Satellite|centr. Empty: infer from observed blocks.")
    ap.add_argument("--scan-family", action="store_true", help="Scan whole dominant repeat family intervals for A/Spacer/B skeletons.")
    args = ap.parse_args()

    seq_dir = Path(args.sequence_dir)
    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    blocks_path = seq_dir / "cluster_v04_block_sequences.csv"
    blocks = read_blocks(blocks_path)
    repeats = parse_bed(Path(args.repeatmasker_bed), chrom_filter=args.chrom)
    seq = read_fasta(Path(args.fasta))

    annotations = annotate_blocks(blocks, repeats)
    ann_fields = ["cluster_id","start0","end0","span","spacer_16bp","full_47bp","next_delta","next_delta_family",
                  "overlap_bp","overlap_fraction","repeat_key","rep_name","rep_class","rep_family","repeat_start0","repeat_end0"]
    write_csv(out_dir / "cluster_v09_block_repeat_annotations.csv", ann_fields, annotations)

    # group summary only sufficiently overlapping blocks
    group_blocks: dict[str, list[dict[str, object]]] = defaultdict(list)
    no_group = 0
    for a in annotations:
        if float(a["overlap_fraction"]) >= args.min_overlap_fraction and a["repeat_key"]:
            group_blocks[str(a["repeat_key"])].append(a)
        else:
            no_group += 1

    group_rows: list[dict[str, object]] = []
    total_blocks = len(blocks)
    for key, rows in sorted(group_blocks.items(), key=lambda kv: len(kv[1]), reverse=True):
        spacers = Counter(str(r["spacer_16bp"]) for r in rows)
        fulls = Counter(str(r["full_47bp"]) for r in rows)
        starts = [int(r["start0"]) for r in rows]
        rep_name, rep_class, rep_family = (key.split("|") + ["", "", ""])[:3]
        group_rows.append({
            "repeat_key": key,
            "rep_name": rep_name,
            "rep_class": rep_class,
            "rep_family": rep_family,
            "blocks": len(rows),
            "block_fraction": len(rows)/total_blocks if total_blocks else 0.0,
            "unique_spacers": len(spacers),
            "top_spacer": spacers.most_common(1)[0][0] if spacers else "",
            "top_spacer_count": spacers.most_common(1)[0][1] if spacers else 0,
            "unique_full47": len(fulls),
            "start_min": min(starts) if starts else "",
            "start_max": max(starts) if starts else "",
        })
    write_csv(out_dir / "cluster_v09_repeat_group_summary.csv",
              ["repeat_key","rep_name","rep_class","rep_family","blocks","block_fraction","unique_spacers",
               "top_spacer","top_spacer_count","unique_full47","start_min","start_max"], group_rows)

    if args.dominant_group:
        dominant_group = args.dominant_group
    elif group_rows:
        dominant_group = str(group_rows[0]["repeat_key"])
    else:
        dominant_group = ""

    dom_rows = group_blocks.get(dominant_group, [])
    dom_start_set = {int(r["start0"]) for r in dom_rows}
    dom_blocks = [b for b in blocks if b.start0 in dom_start_set]

    # Family skeleton scan
    skeletons: list[SkeletonBlock] = []
    if args.scan_family and dominant_group:
        skeletons = scan_skeleton_blocks(seq, repeats, {dominant_group})
    skel_rows = [asdict(s) for s in skeletons]
    write_csv(out_dir / "cluster_v09_family_skeleton_blocks.csv",
              ["chrom","start0","end0","repeat_key","spacer","full47","source"], skel_rows)

    # Spacer states in dominant group and skeleton scan
    spacer_counter = Counter(b.spacer for b in dom_blocks)
    skel_spacer_counter = Counter(s.spacer for s in skeletons)
    state_rows: list[dict[str, object]] = []
    all_spacers = sorted(set(spacer_counter) | set(skel_spacer_counter), key=lambda s: (-spacer_counter[s], s))
    for i, sp in enumerate(all_spacers, 1):
        state_rows.append({
            "state_rank": i,
            "spacer_16bp": sp,
            "observed_blocks": spacer_counter[sp],
            "family_skeleton_blocks": skel_spacer_counter[sp],
            "gc": gc_fraction(sp),
        })
    write_csv(out_dir / "cluster_v09_family_spacer_states.csv",
              ["state_rank","spacer_16bp","observed_blocks","family_skeleton_blocks","gc"], state_rows)

    # Transitions in dominant family, same logic as v0.5 but conditioned to group.
    transition_counter: Counter[tuple[str, str, str]] = Counter()
    dom_blocks_sorted = sorted(dom_blocks, key=lambda b: b.start0)
    for a, b in zip(dom_blocks_sorted, dom_blocks_sorted[1:]):
        transition_counter[(a.spacer, a.next_delta_family, b.spacer)] += 1
    trans_rows = []
    from_delta_totals = Counter((a, d) for a, d, b in transition_counter)
    for (a, d, b), c in transition_counter.most_common():
        denom = from_delta_totals[(a,d)] if from_delta_totals[(a,d)] else 1
        trans_rows.append({
            "from_spacer": a, "delta_family": d, "to_spacer": b, "count": c,
            "conditional_rate": c/denom,
        })
    write_csv(out_dir / "cluster_v09_family_transitions.csv",
              ["from_spacer","delta_family","to_spacer","count","conditional_rate"], trans_rows)

    # Family-conditioned shuffle: same selected family, same deltas/order positions, permute spacers.
    obs_metrics = window_shuffle_metrics(dom_blocks_sorted)
    rnd = random.Random(args.seed)
    state_list = [b.spacer for b in dom_blocks_sorted]
    null_metrics: dict[str, list[float]] = defaultdict(list)
    for i in range(args.iterations):
        shuffled = state_list[:]
        rnd.shuffle(shuffled)
        m = window_shuffle_metrics(dom_blocks_sorted, states=shuffled)
        for k, v in m.items():
            null_metrics[k].append(v)

    null_rows: list[dict[str, object]] = []
    for metric, obs in obs_metrics.items():
        vals = null_metrics.get(metric, [])
        null_rows.append({
            "metric": metric,
            "observed": obs,
            "null_mean": statistics.fmean(vals) if vals else "",
            "null_sd": statistics.pstdev(vals) if len(vals) > 1 else "",
            "z": zscore(obs, vals),
            "empirical_p_right_tail": empirical_right_tail(obs, vals) if vals else "",
            "iterations": len(vals),
        })
    write_csv(out_dir / "cluster_v09_family_shuffle_summary.csv",
              ["metric","observed","null_mean","null_sd","z","empirical_p_right_tail","iterations"], null_rows)

    # Gates
    gates: list[dict[str, object]] = []
    def gate(name: str, status: str, value: object, threshold: object, rationale: str):
        gates.append({"gate": name, "status": status, "value": value, "threshold": threshold, "rationale": rationale})

    dominant_frac = (len(dom_blocks) / total_blocks) if total_blocks else 0
    gate("repeat_family_identified", "pass" if dominant_group else "fail", dominant_group, "non-empty",
         "Dominant RepeatMasker group assigned to observed blocks.")
    gate("dominant_family_coverage", "pass" if dominant_frac >= 0.80 else "fail", dominant_frac, ">=0.80",
         "Most observed blocks must be in one repeat family for family-conditioned test.")
    gate("family_skeleton_scan", "pass" if (not args.scan_family or len(skeletons) >= len(dom_blocks)) else "warn",
         len(skeletons), f">= observed family blocks ({len(dom_blocks)})",
         "Whole-family scan should recover at least observed A/Spacer/B blocks if enabled.")
    # Break if metrics remain above null with z>=10 and p<=0.01
    for row in null_rows:
        metric = str(row["metric"])
        z = float(row["z"]) if row["z"] != "" else 0.0
        p = float(row["empirical_p_right_tail"]) if row["empirical_p_right_tail"] != "" else 1.0
        status = "pass" if z >= 10.0 and p <= 0.01 else "fail"
        gate(f"family_conditioned_shuffle_{metric}", status, f"z={z};p={p}", "z>=10 and p<=0.01",
             "Observed automaton remains exceptional after conditioning on RepeatMasker family and preserving state counts.")

    final_release = all(g["status"] == "pass" for g in gates if str(g["gate"]).startswith("family_conditioned_shuffle_"))
    claim = ("repeat_family_alone_not_sufficient" if final_release else "not_released")
    gate("v09_scoped_claim", "pass" if final_release else "not_released", claim,
         "all family-conditioned shuffle gates pass",
         "Release only the scoped claim that RepeatMasker family membership alone does not explain the observed automaton.")

    write_csv(out_dir / "cluster_v09_control_gates.csv",
              ["gate","status","value","threshold","rationale"], gates)

    manifest = {
        "version": "KALYX_REPEAT_FAMILY_CONDITIONED_AUTOMATON_V0_9",
        "boundary": "No origin proof. Tests whether RepeatMasker family membership alone explains the observed automaton.",
        "sequence_dir": str(seq_dir),
        "repeatmasker_bed": str(Path(args.repeatmasker_bed)),
        "fasta": str(Path(args.fasta)),
        "out_dir": str(out_dir),
        "blocks": len(blocks),
        "repeats": len(repeats),
        "dominant_group": dominant_group,
        "dominant_group_blocks": len(dom_blocks),
        "dominant_group_fraction": dominant_frac,
        "family_skeleton_blocks": len(skeletons),
        "iterations": args.iterations,
        "claim_status": claim,
        "observed_metrics": obs_metrics,
    }
    (out_dir / "cluster_v09_manifest.json").write_text(json.dumps(manifest, indent=2, ensure_ascii=False), encoding="utf-8")

    # Report
    top_groups = "\n".join(
        f"| `{r['repeat_key']}` | {r['blocks']} | {float(r['block_fraction']):.12f} | {r['unique_spacers']} | `{r['top_spacer']}` |"
        for r in group_rows[:12]
    )
    top_trans = "\n".join(
        f"| `{r['from_spacer']}` | `{r['delta_family']}` | `{r['to_spacer']}` | {r['count']} | {float(r['conditional_rate']):.12f} |"
        for r in trans_rows[:20]
    )
    shuffle_lines = "\n".join(
        f"| {r['metric']} | {float(r['observed']):.12f} | {float(r['null_mean']):.12f} | {float(r['z']):.6f} | {float(r['empirical_p_right_tail']):.12f} |"
        for r in null_rows
    )
    gate_lines = "\n".join(
        f"| {g['gate']} | {g['status']} | `{g['value']}` | `{g['threshold']}` |"
        for g in gates
    )

    report = f"""# KALYX Repeat-Family Conditioned Automaton Test v0.9

## Boundary

Dieses Artefakt beweist keinen natürlichen oder künstlichen Ursprung. Es prüft
eine präzisere Frage als v0.8:

> Erklärt die Zugehörigkeit zur dominanten RepeatMasker-Familie allein den
> beobachteten A/Spacer/B-Spacer-State-Automaten?

## Summary

- observed blocks: `{len(blocks)}`
- repeat annotations: `{len(repeats)}`
- dominant repeat group: `{dominant_group}`
- dominant group blocks: `{len(dom_blocks)}`
- dominant group fraction: `{dominant_frac:.12f}`
- family skeleton scan enabled: `{bool(args.scan_family)}`
- family skeleton A/Spacer/B blocks: `{len(skeletons)}`
- shuffle iterations: `{args.iterations}`
- claim_status: `{claim}`

## Repeat group summary

| repeat_key | blocks | block_fraction | unique_spacers | top_spacer |
|---|---:|---:|---:|---|
{top_groups}

## Dominant-family transitions

| from_spacer | delta_family | to_spacer | count | conditional_rate |
|---|---|---|---:|---:|
{top_trans}

## Family-conditioned shuffle summary

| metric | observed | null_mean | z | empirical_p_right_tail |
|---|---:|---:|---:|---:|
{shuffle_lines}

## Control gates

| gate | status | value | threshold |
|---|---|---|---|
{gate_lines}

## Scoped interpretation

If `v09_scoped_claim = pass`, the allowed claim is:

```text
RepeatMasker family membership alone does not explain the observed
A/Spacer/B spacer-state automaton.
```

This still does not prove artificial origin. It moves the remaining natural
model from "known repeat family" to a more specific question:

```text
What process inside this repeat family produces the observed deterministic
spacer-state transition grammar?
```

## Output files

```text
cluster_v09_block_repeat_annotations.csv
cluster_v09_repeat_group_summary.csv
cluster_v09_family_skeleton_blocks.csv
cluster_v09_family_spacer_states.csv
cluster_v09_family_transitions.csv
cluster_v09_family_shuffle_summary.csv
cluster_v09_control_gates.csv
cluster_v09_manifest.json
cluster_v09_report.md
```
"""
    (out_dir / "cluster_v09_report.md").write_text(report, encoding="utf-8")

    print(f"KALYX repeat-family conditioned automaton test complete: {out_dir}")
    print(f"  blocks={len(blocks)} dominant_group={dominant_group!r} dominant_blocks={len(dom_blocks)}")
    print(f"  claim_status={claim}")
    print(f"  report={out_dir / 'cluster_v09_report.md'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
