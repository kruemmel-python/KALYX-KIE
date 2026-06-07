#!/usr/bin/env python3
"""
KALYX Signature Block Sequence Decoder v0.4

Reads v0.3 block coordinates and a chr17 FASTA, extracts the real 47-bp
A/Spacer/B block sequence for each target block, and summarizes spacer/full
sequence states.

Boundary:
This is structural sequence analysis only. It does not prove natural or
artificial origin.
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


A_EXPECT = "ACAGAAGCATTCTCAGAA"
B_EXPECT = "TGCATTCAACTCA"


@dataclass(frozen=True)
class BlockRow:
    cluster_id: str
    start0: int
    end0: int
    span: int
    window: str
    template_rank: str
    template_sha12: str
    strand_set: str
    n_hits: int
    n_motifs: int
    a_start0: int
    a_end0: int
    b_start0: int
    b_end0: int
    spacer_length: int
    prev_delta: str
    next_delta: str
    next_delta_family: str


def read_fasta(path: Path) -> str:
    seq_parts: list[str] = []
    with path.open("rt", encoding="ascii", errors="strict") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith(">"):
                continue
            seq_parts.append(line.upper())
    seq = "".join(seq_parts)
    if not seq:
        raise ValueError(f"FASTA contains no sequence: {path}")
    # Keep N and IUPAC chars, but later validation marks non-ACGTN.
    return seq


def read_blocks(path: Path) -> list[BlockRow]:
    rows: list[BlockRow] = []
    with path.open("rt", encoding="utf-8-sig", newline="") as f:
        r = csv.DictReader(f)
        required = {
            "cluster_id","start0","end0","span","window","template_rank","template_sha12",
            "strand_set","n_hits","n_motifs","a_start0","a_end0","b_start0","b_end0",
            "spacer_length","prev_delta","next_delta","next_delta_family"
        }
        missing = required - set(r.fieldnames or [])
        if missing:
            raise ValueError(f"Missing required columns in {path}: {sorted(missing)}")
        for d in r:
            rows.append(BlockRow(
                cluster_id=d["cluster_id"],
                start0=int(d["start0"]),
                end0=int(d["end0"]),
                span=int(d["span"]),
                window=d["window"],
                template_rank=d["template_rank"],
                template_sha12=d["template_sha12"],
                strand_set=d["strand_set"],
                n_hits=int(d["n_hits"]),
                n_motifs=int(d["n_motifs"]),
                a_start0=int(d["a_start0"]),
                a_end0=int(d["a_end0"]),
                b_start0=int(d["b_start0"]),
                b_end0=int(d["b_end0"]),
                spacer_length=int(d["spacer_length"]),
                prev_delta=d["prev_delta"],
                next_delta=d["next_delta"],
                next_delta_family=d["next_delta_family"],
            ))
    rows.sort(key=lambda x: (x.start0, x.end0, x.cluster_id))
    return rows


def gc_rate(seq: str) -> float:
    if not seq:
        return 0.0
    return sum(1 for c in seq if c in "GC") / len(seq)


def at_rate(seq: str) -> float:
    if not seq:
        return 0.0
    return sum(1 for c in seq if c in "AT") / len(seq)


def shannon_bits(seq: str) -> float:
    if not seq:
        return 0.0
    n = len(seq)
    c = Counter(seq)
    return -sum((v/n) * math.log2(v/n) for v in c.values())


def hamming(a: str, b: str) -> int:
    if len(a) != len(b):
        return max(len(a), len(b))
    return sum(1 for x, y in zip(a, b) if x != y)


def is_simple_low_complexity(seq: str) -> bool:
    # Conservative flag only: mononucleotide-heavy or dinucleotide-like.
    if not seq:
        return True
    c = Counter(seq)
    if c.most_common(1)[0][1] / len(seq) >= 0.75:
        return True
    # repeated 1-4mer exact cyclic repetition
    for k in range(1, 5):
        unit = seq[:k]
        if (unit * ((len(seq) + k - 1) // k))[:len(seq)] == seq:
            return True
    return False


def family_of_delta(delta: str) -> str:
    if delta == "":
        return ""
    try:
        d = int(delta)
    except ValueError:
        return "invalid"
    centers = [137,168,170,171,200,205,341,342,506,507,508,513,2379,2380]
    best = min(centers, key=lambda c: abs(d-c))
    if abs(d-best) <= 3:
        sign = d - best
        return f"{best}{sign:+d}"
    return "other"


def write_csv(path: Path, fieldnames: list[str], rows: Iterable[dict[str, object]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("wt", encoding="utf-8", newline="") as f:
        w = csv.DictWriter(f, fieldnames=fieldnames)
        w.writeheader()
        for row in rows:
            w.writerow(row)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--block-dir", required=True, help="Directory containing cluster_v03_blocks.csv")
    ap.add_argument("--fasta", required=True, help="chr17 FASTA file")
    ap.add_argument("--out-dir", required=True)
    ap.add_argument("--target-rank", type=int, default=3)
    ap.add_argument("--top", type=int, default=40)
    args = ap.parse_args()

    block_dir = Path(args.block_dir)
    fasta_path = Path(args.fasta)
    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    block_csv = block_dir / "cluster_v03_blocks.csv"
    if not block_csv.exists():
        raise FileNotFoundError(f"Missing v0.3 blocks CSV: {block_csv}")
    if not fasta_path.exists():
        raise FileNotFoundError(f"Missing FASTA: {fasta_path}")

    seq = read_fasta(fasta_path)
    blocks = read_blocks(block_csv)

    target_blocks = [b for b in blocks if b.template_rank == str(args.target_rank)]
    sequence_rows: list[dict[str, object]] = []
    spacer_counter: Counter[str] = Counter()
    full_counter: Counter[str] = Counter()
    a_counter: Counter[str] = Counter()
    b_counter: Counter[str] = Counter()
    delta_spacer_counter: Counter[tuple[str, str]] = Counter()
    window_spacer_counter: Counter[tuple[str, str]] = Counter()
    spacer_next_counter: Counter[tuple[str, str]] = Counter()
    validation_counter: Counter[str] = Counter()

    for b in target_blocks:
        if b.end0 > len(seq):
            validation = "out_of_range"
            full = a = spacer = bseq = ""
        else:
            full = seq[b.start0:b.end0]
            a = seq[b.a_start0:b.a_end0]
            spacer = seq[b.a_end0:b.b_start0]
            bseq = seq[b.b_start0:b.b_end0]
            checks = []
            if len(full) != b.span:
                checks.append("full_len")
            if len(a) != 18:
                checks.append("a_len")
            if len(spacer) != b.spacer_length:
                checks.append("spacer_len")
            if len(bseq) != 13:
                checks.append("b_len")
            if hamming(a, A_EXPECT) != 0:
                checks.append(f"a_ham={hamming(a,A_EXPECT)}")
            if hamming(bseq, B_EXPECT) != 0:
                checks.append(f"b_ham={hamming(bseq,B_EXPECT)}")
            if any(ch not in "ACGTN" for ch in full):
                checks.append("non_acgtn")
            validation = "ok" if not checks else ";".join(checks)

        validation_counter[validation] += 1
        spacer_counter[spacer] += 1
        full_counter[full] += 1
        a_counter[a] += 1
        b_counter[bseq] += 1
        delta_family = family_of_delta(b.next_delta)
        delta_spacer_counter[(delta_family, spacer)] += 1
        window_spacer_counter[(b.window, spacer)] += 1
        spacer_next_counter[(spacer, delta_family)] += 1

        sequence_rows.append({
            "cluster_id": b.cluster_id,
            "start0": b.start0,
            "end0": b.end0,
            "span": b.span,
            "window": b.window,
            "template_rank": b.template_rank,
            "strand_set": b.strand_set,
            "a_start0": b.a_start0,
            "a_end0": b.a_end0,
            "b_start0": b.b_start0,
            "b_end0": b.b_end0,
            "spacer_length": b.spacer_length,
            "prev_delta": b.prev_delta,
            "next_delta": b.next_delta,
            "next_delta_family_v03": b.next_delta_family,
            "next_delta_family_v04": delta_family,
            "full_47bp": full,
            "a_sequence": a,
            "spacer_16bp": spacer,
            "b_sequence": bseq,
            "a_hamming_expected": hamming(a, A_EXPECT) if a else "",
            "b_hamming_expected": hamming(bseq, B_EXPECT) if bseq else "",
            "full_gc": f"{gc_rate(full):.12f}" if full else "",
            "spacer_gc": f"{gc_rate(spacer):.12f}" if spacer else "",
            "spacer_entropy_bits": f"{shannon_bits(spacer):.12f}" if spacer else "",
            "spacer_low_complexity": int(is_simple_low_complexity(spacer)),
            "validation": validation,
        })

    # Write rows
    write_csv(out_dir / "cluster_v04_block_sequences.csv", [
        "cluster_id","start0","end0","span","window","template_rank","strand_set",
        "a_start0","a_end0","b_start0","b_end0","spacer_length","prev_delta","next_delta",
        "next_delta_family_v03","next_delta_family_v04","full_47bp","a_sequence",
        "spacer_16bp","b_sequence","a_hamming_expected","b_hamming_expected","full_gc",
        "spacer_gc","spacer_entropy_bits","spacer_low_complexity","validation"
    ], sequence_rows)

    def top_counter_rows(counter: Counter[str], key_name: str) -> list[dict[str, object]]:
        total = sum(counter.values()) or 1
        rows = []
        for rank, (k, c) in enumerate(counter.most_common(), 1):
            rows.append({
                "rank": rank,
                key_name: k,
                "count": c,
                "rate": f"{c/total:.12f}",
                "length": len(k),
                "gc": f"{gc_rate(k):.12f}" if k else "",
                "at": f"{at_rate(k):.12f}" if k else "",
                "entropy_bits": f"{shannon_bits(k):.12f}" if k else "",
                "low_complexity": int(is_simple_low_complexity(k)) if k else "",
            })
        return rows

    write_csv(out_dir / "cluster_v04_spacer_variants.csv",
              ["rank","spacer_16bp","count","rate","length","gc","at","entropy_bits","low_complexity"],
              top_counter_rows(spacer_counter, "spacer_16bp"))

    write_csv(out_dir / "cluster_v04_full47_variants.csv",
              ["rank","full_47bp","count","rate","length","gc","at","entropy_bits","low_complexity"],
              top_counter_rows(full_counter, "full_47bp"))

    write_csv(out_dir / "cluster_v04_a_variants.csv",
              ["rank","a_sequence","count","rate","length","gc","at","entropy_bits","low_complexity"],
              top_counter_rows(a_counter, "a_sequence"))

    write_csv(out_dir / "cluster_v04_b_variants.csv",
              ["rank","b_sequence","count","rate","length","gc","at","entropy_bits","low_complexity"],
              top_counter_rows(b_counter, "b_sequence"))

    # delta family x spacer
    total_ds = sum(delta_spacer_counter.values()) or 1
    ds_rows = []
    for rank, ((fam, sp), c) in enumerate(delta_spacer_counter.most_common(), 1):
        ds_rows.append({
            "rank": rank,
            "next_delta_family": fam,
            "spacer_16bp": sp,
            "count": c,
            "rate": f"{c/total_ds:.12f}",
            "spacer_gc": f"{gc_rate(sp):.12f}" if sp else "",
            "spacer_entropy_bits": f"{shannon_bits(sp):.12f}" if sp else "",
        })
    write_csv(out_dir / "cluster_v04_delta_spacer.csv",
              ["rank","next_delta_family","spacer_16bp","count","rate","spacer_gc","spacer_entropy_bits"], ds_rows)

    # window x spacer
    total_ws = sum(window_spacer_counter.values()) or 1
    ws_rows = []
    for rank, ((win, sp), c) in enumerate(window_spacer_counter.most_common(), 1):
        ws_rows.append({
            "rank": rank,
            "window": win,
            "spacer_16bp": sp,
            "count": c,
            "rate": f"{c/total_ws:.12f}",
        })
    write_csv(out_dir / "cluster_v04_window_spacer.csv",
              ["rank","window","spacer_16bp","count","rate"], ws_rows)

    # validation summary
    val_total = sum(validation_counter.values()) or 1
    val_rows = [{"validation": k, "count": c, "rate": f"{c/val_total:.12f}"}
                for k, c in validation_counter.most_common()]
    write_csv(out_dir / "cluster_v04_validation.csv",
              ["validation","count","rate"], val_rows)

    # Summary report
    spacers_n = len(spacer_counter)
    full_n = len(full_counter)
    top_spacer = spacer_counter.most_common(1)[0] if spacer_counter else ("", 0)
    top_full = full_counter.most_common(1)[0] if full_counter else ("", 0)
    ok_count = validation_counter.get("ok", 0)
    spacer_entropy_values = [shannon_bits(k) for k in spacer_counter.keys() if k]
    median_spacer_gc = statistics.median([gc_rate(row["spacer_16bp"]) for row in sequence_rows if row["spacer_16bp"]]) if sequence_rows else 0.0

    manifest = {
        "tool": "KALYX Signature Block Sequence Decoder",
        "version": "0.4",
        "boundary": "structural sequence extraction; no origin claim",
        "block_dir": str(block_dir.resolve()),
        "fasta": str(fasta_path.resolve()),
        "out_dir": str(out_dir.resolve()),
        "target_rank": args.target_rank,
        "input_blocks": len(blocks),
        "target_blocks": len(target_blocks),
        "ok_blocks": ok_count,
        "unique_spacers": spacers_n,
        "unique_full47": full_n,
        "top_spacer": top_spacer[0],
        "top_spacer_count": top_spacer[1],
        "top_full47": top_full[0],
        "top_full47_count": top_full[1],
        "expected_a": A_EXPECT,
        "expected_b": B_EXPECT,
    }
    (out_dir / "cluster_v04_manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")

    report_lines = [
        "# KALYX Signature Block Sequence Decoder v0.4",
        "",
        "## Boundary",
        "",
        "Dieses Artefakt extrahiert die reale 47-bp-Sequenz aus `chr17.fa` für die v0.3-Blöcke.",
        "Es analysiert A-Modul, 16-bp-Spacer, B-Modul, Varianten und Delta-Kopplungen.",
        "Es beweist keinen natürlichen oder künstlichen Ursprung.",
        "",
        "## Summary",
        "",
        f"- input v0.3 blocks: `{len(blocks)}`",
        f"- target template rank: `{args.target_rank}`",
        f"- target blocks: `{len(target_blocks)}`",
        f"- validation ok: `{ok_count}`",
        f"- unique full_47bp variants: `{full_n}`",
        f"- unique spacer_16bp variants: `{spacers_n}`",
        f"- top spacer: `{top_spacer[0]}` count=`{top_spacer[1]}`",
        f"- top full_47bp: `{top_full[0]}` count=`{top_full[1]}`",
        f"- median spacer GC: `{median_spacer_gc:.12f}`",
        "",
        "## Expected module validation",
        "",
        f"- expected A: `{A_EXPECT}`",
        f"- expected B: `{B_EXPECT}`",
        "",
        "## Top spacer variants",
        "",
        "| rank | spacer | count | rate | GC | entropy |",
        "|---:|---|---:|---:|---:|---:|",
    ]
    total_sp = sum(spacer_counter.values()) or 1
    for rank, (sp, c) in enumerate(spacer_counter.most_common(args.top), 1):
        report_lines.append(
            f"| {rank} | `{sp}` | {c} | {c/total_sp:.12f} | {gc_rate(sp):.12f} | {shannon_bits(sp):.12f} |"
        )
    report_lines += [
        "",
        "## Top full 47-bp variants",
        "",
        "| rank | full_47bp | count | rate |",
        "|---:|---|---:|---:|",
    ]
    total_full = sum(full_counter.values()) or 1
    for rank, (full, c) in enumerate(full_counter.most_common(min(args.top, 25)), 1):
        report_lines.append(f"| {rank} | `{full}` | {c} | {c/total_full:.12f} |")
    report_lines += [
        "",
        "## Top delta-family x spacer states",
        "",
        "| rank | next_delta_family | spacer | count | rate |",
        "|---:|---|---|---:|---:|",
    ]
    for row in ds_rows[:args.top]:
        report_lines.append(f"| {row['rank']} | `{row['next_delta_family']}` | `{row['spacer_16bp']}` | {row['count']} | {row['rate']} |")
    report_lines += [
        "",
        "## Output files",
        "",
        "```text",
        "cluster_v04_block_sequences.csv",
        "cluster_v04_spacer_variants.csv",
        "cluster_v04_full47_variants.csv",
        "cluster_v04_a_variants.csv",
        "cluster_v04_b_variants.csv",
        "cluster_v04_delta_spacer.csv",
        "cluster_v04_window_spacer.csv",
        "cluster_v04_validation.csv",
        "cluster_v04_manifest.json",
        "cluster_v04_report.md",
        "```",
        "",
        "## Interpretation guardrail",
        "",
        "Wenn der Spacer wenige dominante Zustände besitzt, ist er Teil der Syntax.",
        "Wenn der Spacer sehr variabel ist, ist er eher ein Abstandsfeld/Strukturfenster.",
        "Beides bleibt erst nach RepeatMasker-, GC-Shuffle- und Cross-Chromosom-Kontrollen biologisch interpretierbar.",
        "",
    ]
    (out_dir / "cluster_v04_report.md").write_text("\n".join(report_lines), encoding="utf-8")

    print(f"KALYX signature sequence decoder complete: {out_dir}")
    print(f"  blocks={len(blocks)} target_rank={args.target_rank} target_blocks={len(target_blocks)} ok={ok_count}")
    print(f"  unique_spacers={spacers_n} unique_full47={full_n}")
    print(f"  report={out_dir / 'cluster_v04_report.md'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
