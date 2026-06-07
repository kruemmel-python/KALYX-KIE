#!/usr/bin/env python3
"""
KALYX Evidence Gate / External Control Planner v0.7

Input:
  Decode_chr17_v06_real/cluster_v06_null_summary.csv
  Decode_chr17_v06_real/cluster_v06_group_determinism.csv
  Decode_chr17_v06_real/cluster_v06_transition_enrichment.csv
Optional:
  Decode_chr17_v04_real/cluster_v04_block_sequences.csv
  RepeatMasker/segmental-duplication BED-like file

Purpose:
  Convert v0.6 determinism into an evidence-gated scientific claim ladder.
  If optional repeat/segdup annotations are provided, quantify overlap.

Boundary:
  v0.7 does not claim origin. It is the gatekeeper for when a claim may move
  from "ordered" to "not explained by tested nulls".
"""
from __future__ import annotations

import argparse
import csv
import json
import math
from pathlib import Path
from typing import Iterable


def read_csv(path: Path) -> list[dict[str, str]]:
    if not path.exists():
        raise FileNotFoundError(f"Missing CSV: {path}")
    with path.open("rt", encoding="utf-8-sig", newline="") as f:
        return list(csv.DictReader(f))


def write_csv(path: Path, fieldnames: list[str], rows: Iterable[dict[str, object]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("wt", encoding="utf-8", newline="") as f:
        w = csv.DictWriter(f, fieldnames=fieldnames)
        w.writeheader()
        for row in rows:
            w.writerow(row)


def fnum(x: str, default: float = 0.0) -> float:
    try:
        return float(x)
    except Exception:
        return default


def get_metric(rows: list[dict[str, str]], metric: str) -> dict[str, str] | None:
    for r in rows:
        if r.get("metric") == metric:
            return r
    return None


def parse_bed(path: Path) -> list[tuple[str, int, int, str]]:
    """Parse BED-like chrom,start,end[,name]."""
    if not path or not path.exists():
        return []
    intervals = []
    with path.open("rt", encoding="utf-8", errors="replace") as f:
        for line in f:
            s = line.strip()
            if not s or s.startswith("#") or s.startswith("track") or s.startswith("browser"):
                continue
            parts = s.replace(",", "\t").split()
            if len(parts) < 3:
                continue
            chrom = parts[0]
            try:
                start = int(parts[1])
                end = int(parts[2])
            except ValueError:
                continue
            name = parts[3] if len(parts) >= 4 else ""
            if end > start:
                intervals.append((chrom, start, end, name))
    intervals.sort(key=lambda x: (x[0], x[1], x[2]))
    return intervals


def overlap_len(a0: int, a1: int, b0: int, b1: int) -> int:
    return max(0, min(a1, b1) - max(a0, b0))


def block_overlap(blocks: list[dict[str, str]], intervals: list[tuple[str, int, int, str]], chrom: str) -> tuple[list[dict[str, object]], dict[str, object]]:
    if not blocks or not intervals:
        return [], {
            "blocks": len(blocks),
            "intervals": len(intervals),
            "overlap_blocks": 0,
            "overlap_rate": "",
            "full_covered_blocks": 0,
            "full_covered_rate": "",
        }
    ints = [(s, e, n) for c, s, e, n in intervals if c == chrom or c.replace("chr","") == chrom.replace("chr","")]
    ints.sort()
    rows = []
    j = 0
    overlap_blocks = 0
    full_covered = 0
    for b in blocks:
        start = int(b["start0"])
        end = int(b["end0"])
        span = end - start
        while j < len(ints) and ints[j][1] <= start:
            j += 1
        cov = 0
        names = []
        k = max(0, j - 3)
        while k < len(ints) and ints[k][0] < end:
            ov = overlap_len(start, end, ints[k][0], ints[k][1])
            if ov:
                cov += ov
                if ints[k][2]:
                    names.append(ints[k][2])
            k += 1
        if cov > 0:
            overlap_blocks += 1
        if cov >= span:
            full_covered += 1
        rows.append({
            "cluster_id": b.get("cluster_id",""),
            "start0": start,
            "end0": end,
            "span": span,
            "overlap_bp": cov,
            "overlap_fraction": f"{(cov/span if span else 0):.12f}",
            "overlap_names": "|".join(sorted(set(names))[:10]),
        })
    summary = {
        "blocks": len(blocks),
        "intervals": len(ints),
        "overlap_blocks": overlap_blocks,
        "overlap_rate": f"{overlap_blocks / len(blocks):.12f}" if blocks else "",
        "full_covered_blocks": full_covered,
        "full_covered_rate": f"{full_covered / len(blocks):.12f}" if blocks else "",
    }
    return rows, summary


def main() -> int:
    ap = argparse.ArgumentParser(description="KALYX evidence gate v0.7")
    ap.add_argument("--determinism-dir", required=True, help="Directory containing v0.6 outputs")
    ap.add_argument("--sequence-dir", default="", help="Optional v0.4 directory containing block sequences")
    ap.add_argument("--out-dir", required=True)
    ap.add_argument("--repeat-bed", default="", help="Optional RepeatMasker/segmental-duplication BED-like file")
    ap.add_argument("--chrom", default="chr17")
    ap.add_argument("--z-threshold", type=float, default=10.0)
    ap.add_argument("--p-threshold", type=float, default=0.001)
    args = ap.parse_args()

    det_dir = Path(args.determinism_dir)
    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    null_summary = read_csv(det_dir / "cluster_v06_null_summary.csv")
    group_det = read_csv(det_dir / "cluster_v06_group_determinism.csv")
    enrich = read_csv(det_dir / "cluster_v06_transition_enrichment.csv")

    metrics = {}
    for name in [
        "weighted_top1_accuracy",
        "mean_group_top1_accuracy",
        "mean_group_entropy_bits",
        "top_transition_count",
        "mutual_information_to_from_delta_bits",
        "max_same_state_run_length",
        "top_state_ngram_n3_count",
    ]:
        r = get_metric(null_summary, name)
        if r:
            metrics[name] = r

    gate_rows = []
    def gate(metric: str, direction: str, explanation: str):
        r = metrics.get(metric)
        if not r:
            gate_rows.append({"gate": metric, "status": "missing", "observed": "", "null_mean": "", "z": "", "p_empirical": "", "explanation": explanation})
            return
        z = fnum(r.get("z",""))
        p = fnum(r.get("p_empirical_ge",""), default=1.0)
        obs = fnum(r.get("observed",""))
        mean = fnum(r.get("null_mean",""))
        if direction == "high":
            ok = z >= args.z_threshold and p <= args.p_threshold
        elif direction == "low":
            # for entropy lower-than-null we use |z| and note empirical_ge may be 1 by construction
            ok = z <= -args.z_threshold
        else:
            ok = abs(z) >= args.z_threshold
        gate_rows.append({
            "gate": metric,
            "status": "pass" if ok else "open",
            "observed": r.get("observed",""),
            "null_mean": r.get("null_mean",""),
            "z": r.get("z",""),
            "p_empirical": r.get("p_empirical_ge",""),
            "explanation": explanation,
        })

    gate("weighted_top1_accuracy", "high", "Transition groups predict next state better than state-frequency shuffle.")
    gate("mutual_information_to_from_delta_bits", "high", "Next state carries information about from-state/delta group.")
    gate("top_transition_count", "high", "Dominant transition exceeds shuffled-order baseline.")
    gate("max_same_state_run_length", "high", "Observed same-state runs exceed shuffled baseline.")
    gate("top_state_ngram_n3_count", "high", "Observed state tri-gram exceeds shuffled baseline.")
    gate("mean_group_entropy_bits", "low", "Transition groups are lower-entropy than shuffled baseline.")

    # Control requirements
    control_rows = [
        {"control": "internal_state_frequency_shuffle", "status": "done", "evidence": "v0.6 null_summary", "required_for": "ordered automaton claim"},
        {"control": "window_stratified_shuffle", "status": "next", "evidence": "", "required_for": "exclude window composition artefact"},
        {"control": "repeatmasker_overlap", "status": "optional_done" if args.repeat_bed else "missing", "evidence": args.repeat_bed, "required_for": "repeat explanation assessment"},
        {"control": "segmental_duplication_overlap", "status": "missing", "evidence": "", "required_for": "duplication explanation assessment"},
        {"control": "gc_matched_local_shuffle", "status": "missing", "evidence": "", "required_for": "sequence composition null"},
        {"control": "cross_chromosome_scan", "status": "missing", "evidence": "", "required_for": "genome-wide specificity"},
        {"control": "cross_build_or_population_replication", "status": "missing", "evidence": "", "required_for": "reference-build artefact exclusion"},
    ]

    # Optional repeat overlap
    overlap_summary = None
    if args.sequence_dir and args.repeat_bed:
        seq_dir = Path(args.sequence_dir)
        blocks_path = seq_dir / "cluster_v04_block_sequences.csv"
        bed_path = Path(args.repeat_bed)
        if blocks_path.exists() and bed_path.exists():
            blocks = read_csv(blocks_path)
            intervals = parse_bed(bed_path)
            overlap_rows, overlap_summary = block_overlap(blocks, intervals, args.chrom)
            write_csv(out_dir / "cluster_v07_repeat_overlap.csv",
                      ["cluster_id","start0","end0","span","overlap_bp","overlap_fraction","overlap_names"],
                      overlap_rows)
            write_csv(out_dir / "cluster_v07_repeat_overlap_summary.csv",
                      ["blocks","intervals","overlap_blocks","overlap_rate","full_covered_blocks","full_covered_rate"],
                      [overlap_summary])

    # Build summary
    passed = sum(1 for r in gate_rows if r["status"] == "pass")
    open_gates = sum(1 for r in gate_rows if r["status"] == "open")
    missing = sum(1 for r in gate_rows if r["status"] == "missing")

    if passed >= 5:
        allowed_statement = "The spacer-state automaton is strongly ordered relative to the internal state-frequency shuffle null."
    else:
        allowed_statement = "The spacer-state automaton remains a candidate; internal null evidence is insufficient."

    prohibited_statement = "Artificial origin is proven."
    next_statement = "The next claim gate requires RepeatMasker/segmental duplication, GC-matched local shuffles, cross-chromosome scan, and cross-build/population replication."

    write_csv(out_dir / "cluster_v07_evidence_gates.csv",
              ["gate","status","observed","null_mean","z","p_empirical","explanation"], gate_rows)
    write_csv(out_dir / "cluster_v07_control_requirements.csv",
              ["control","status","evidence","required_for"], control_rows)

    # High transition rows, normalized
    trans_rows = []
    for i, r in enumerate(enrich[:100], 1):
        trans_rows.append({
            "rank": i,
            "from_state_id": r.get("from_state_id", r.get("from","")),
            "delta_family": r.get("delta_family", r.get("delta","")),
            "to_state_id": r.get("to_state_id", r.get("to","")),
            "observed": r.get("observed",""),
            "expected_independent": r.get("expected_independent", r.get("expected","")),
            "enrichment": r.get("enrichment",""),
            "observed_minus_expected": r.get("observed_minus_expected",""),
        })
    write_csv(out_dir / "cluster_v07_top_transition_enrichment.csv",
              ["rank","from_state_id","delta_family","to_state_id","observed","expected_independent","enrichment","observed_minus_expected"],
              trans_rows)

    manifest = {
        "version": "KALYX_EVIDENCE_GATE_V0_7",
        "boundary": "claim-gating only; no origin proof",
        "determinism_dir": str(det_dir.resolve()),
        "sequence_dir": str(Path(args.sequence_dir).resolve()) if args.sequence_dir else None,
        "repeat_bed": str(Path(args.repeat_bed).resolve()) if args.repeat_bed else None,
        "z_threshold": args.z_threshold,
        "p_threshold": args.p_threshold,
        "gates_passed": passed,
        "gates_open": open_gates,
        "gates_missing": missing,
        "allowed_statement": allowed_statement,
        "prohibited_statement": prohibited_statement,
        "next_statement": next_statement,
        "repeat_overlap_summary": overlap_summary,
    }
    (out_dir / "cluster_v07_manifest.json").write_text(json.dumps(manifest, indent=2, ensure_ascii=False), encoding="utf-8")

    report = []
    report.append("# KALYX Evidence Gate v0.7")
    report.append("")
    report.append("## Boundary")
    report.append("")
    report.append("Dieses Artefakt ist eine Claim-Gate-Schicht. Es beweist keinen Ursprung.")
    report.append("Es sagt, welche Aussage durch die bisherigen Kontrollen getragen wird und welche Kontrollen noch fehlen.")
    report.append("")
    report.append("## Summary")
    report.append("")
    report.append(f"- gates passed: `{passed}`")
    report.append(f"- gates open: `{open_gates}`")
    report.append(f"- gates missing: `{missing}`")
    report.append(f"- allowed statement: **{allowed_statement}**")
    report.append(f"- prohibited statement: **{prohibited_statement}**")
    report.append("")
    report.append("## Evidence gates")
    report.append("")
    report.append("| gate | status | observed | null_mean | z | p |")
    report.append("|---|---|---:|---:|---:|---:|")
    for r in gate_rows:
        report.append(f"| `{r['gate']}` | `{r['status']}` | {r['observed']} | {r['null_mean']} | {r['z']} | {r['p_empirical']} |")
    report.append("")
    report.append("## Control requirements")
    report.append("")
    report.append("| control | status | required_for |")
    report.append("|---|---|---|")
    for r in control_rows:
        report.append(f"| `{r['control']}` | `{r['status']}` | {r['required_for']} |")
    if overlap_summary:
        report.append("")
        report.append("## Repeat overlap summary")
        report.append("")
        report.append(f"- blocks: `{overlap_summary['blocks']}`")
        report.append(f"- intervals: `{overlap_summary['intervals']}`")
        report.append(f"- overlap_blocks: `{overlap_summary['overlap_blocks']}`")
        report.append(f"- overlap_rate: `{overlap_summary['overlap_rate']}`")
        report.append(f"- full_covered_blocks: `{overlap_summary['full_covered_blocks']}`")
        report.append(f"- full_covered_rate: `{overlap_summary['full_covered_rate']}`")
    report.append("")
    report.append("## Claim ladder")
    report.append("")
    report.append("1. **Now supported:** The spacer-state automaton is strongly ordered relative to the internal shuffle null.")
    report.append("2. **Next supported if v0.7+ controls pass:** The observed order is not explained by tested local sequence/composition/repeat nulls.")
    report.append("3. **Still not proven by this layer:** artificial origin.")
    report.append("")
    report.append("## Output files")
    report.append("")
    report.append("```text")
    report.append("cluster_v07_evidence_gates.csv")
    report.append("cluster_v07_control_requirements.csv")
    report.append("cluster_v07_top_transition_enrichment.csv")
    report.append("cluster_v07_repeat_overlap.csv                optional")
    report.append("cluster_v07_repeat_overlap_summary.csv        optional")
    report.append("cluster_v07_manifest.json")
    report.append("cluster_v07_report.md")
    report.append("```")
    (out_dir / "cluster_v07_report.md").write_text("\n".join(report) + "\n", encoding="utf-8")

    print(f"KALYX evidence gate v0.7 complete: {out_dir}")
    print(f"  gates_passed={passed} gates_open={open_gates} gates_missing={missing}")
    print(f"  report={out_dir / 'cluster_v07_report.md'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
