#!/usr/bin/env python3
"""
KALYX Signature Decoder v0.1

Purpose:
  Decode/validate a small DNA motif family as a substrate graph, not as a claim
  about origin. The tool prefers existing KALYX artefacts created by the
  PowerShell pipeline and falls back to parsing Powershell.log / explicit motifs.

Outputs:
  - decode_manifest.json
  - decode_motifs.csv
  - decode_edges.csv
  - decode_core.csv
  - decode_supersequence.txt
  - decode_log_windows.csv       (if log contains v0.9 scan lines)
  - decode_hits.csv              (if FASTA is available)
  - decode_periods.csv           (if hits are available)
  - decode_report.md

Scientific boundary:
  This tool tests a motif transition hypothesis. It does not infer natural or
  artificial origin. It exposes graph structure, strand balance, coordinates and
  periodicity for independent validation.
"""
from __future__ import annotations

import argparse
import csv
import dataclasses
import hashlib
import json
import math
import os
import random
import re
import statistics
from collections import Counter, defaultdict
from pathlib import Path
from typing import Iterable, Iterator, Sequence

DEFAULT_MOTIFS: tuple[str, ...] = (
    "AGCATTCTCAGA",
    "GCATTCTCAGAA",
    "AAGCATTCTCAG",
    "GAAGCATTCTCA",
    "AGAAGCATTCTC",
    "ACAGAAGCATTC",
    "TGCATTCAACTC",
    "GCATTCAACTCA",
)

DNA = set("ACGT")
RC_TABLE = str.maketrans("ACGTacgt", "TGCAtgca")


@dataclasses.dataclass(frozen=True)
class Motif:
    rank: int
    seq: str
    source: str = "input"
    weight: int | None = None
    rc_balance: float | None = None
    support: float | None = None


@dataclasses.dataclass(frozen=True)
class Edge:
    src: str
    dst: str
    overlap: int
    suffix_from_src: str
    prefix_from_dst: str
    kind: str


@dataclasses.dataclass(frozen=True)
class Hit:
    motif: str
    strand: str
    pos0: int
    pos1: int
    window: int | None


def norm_seq(seq: str) -> str:
    s = seq.strip().upper()
    if not s:
        raise ValueError("empty DNA sequence")
    bad = sorted(set(s) - DNA)
    if bad:
        raise ValueError(f"invalid DNA characters in {seq!r}: {bad}")
    return s


def reverse_complement(seq: str) -> str:
    return seq.translate(RC_TABLE)[::-1].upper()


def sha256_text(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def read_fasta(path: Path) -> str:
    chunks: list[str] = []
    with path.open("rt", encoding="utf-8", errors="replace") as f:
        for line in f:
            if line.startswith(">"):
                continue
            s = line.strip().upper()
            if s:
                chunks.append(s)
    seq = "".join(chunks)
    if not seq:
        raise ValueError(f"FASTA contains no sequence: {path}")
    return seq


def write_csv(path: Path, rows: Iterable[dict[str, object]], fieldnames: Sequence[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=list(fieldnames))
        w.writeheader()
        for row in rows:
            w.writerow(row)


def parse_motif_arg(values: Sequence[str] | None) -> list[str]:
    if not values:
        return list(DEFAULT_MOTIFS)
    out: list[str] = []
    for v in values:
        for part in re.split(r"[\s,;]+", v.strip()):
            if part:
                out.append(norm_seq(part))
    return out


def parse_signature_motifs_from_log(log_text: str) -> list[Motif]:
    """Extract motif table from KALYX_ORIGIN_V0_9_SIGNATURE_REPORT log output."""
    motifs: list[Motif] = []
    # Markdown table rows:
    # | 1 | 0 | AGCATTCTCAGA | 10375 | 0.000096... | 1.000... |
    row_re = re.compile(
        r"^\|\s*(\d+)\s*\|\s*(\d+)\s*\|\s*([ACGT]{6,40})\s*\|\s*([0-9]+)\s*\|\s*([0-9.Ee+-]+)\s*\|\s*([0-9.Ee+-]+)\s*\|",
        re.MULTILINE,
    )
    seen: set[str] = set()
    for m in row_re.finditer(log_text):
        seq = m.group(3).upper()
        if seq in seen:
            continue
        seen.add(seq)
        motifs.append(
            Motif(
                rank=int(m.group(1)),
                seq=seq,
                source="powershell_log_report",
                weight=int(m.group(4)),
                rc_balance=float(m.group(5)),
                support=float(m.group(6)),
            )
        )
    return motifs


def parse_scan_windows_from_log(log_text: str) -> list[dict[str, object]]:
    # kalyx_signature_scan v0.9: sig_k12_0000 base=[0,1048576] len=1048576 hits=1 rc=3 density=... rc_balance=... period=1 rate=...
    rx = re.compile(
        r"kalyx_signature_scan v0\.9:\s+(\S+)\s+base=\[(\d+),(\d+)\]\s+len=(\d+)\s+hits=(\d+)\s+rc=(\d+)\s+density=([0-9.Ee+-]+)\s+rc_balance=([0-9.Ee+-]+)\s+period=(\d+)\s+rate=([0-9.Ee+-]+)"
    )
    rows: list[dict[str, object]] = []
    for m in rx.finditer(log_text):
        rows.append(
            {
                "label": m.group(1),
                "base_start0": int(m.group(2)),
                "base_end0": int(m.group(3)),
                "length": int(m.group(4)),
                "hits": int(m.group(5)),
                "rc": int(m.group(6)),
                "density": float(m.group(7)),
                "rc_balance": float(m.group(8)),
                "period": int(m.group(9)),
                "period_rate": float(m.group(10)),
            }
        )
    return rows


def discover_existing_artifacts(root: Path, log_text: str | None) -> dict[str, list[str]]:
    """Discover paths mentioned in the log and paths existing in the current root."""
    keys = {
        "signature_k12_csv": "kalyx_origin_v0_9_signature_k12.csv",
        "signature_scan_csv": "kalyx_origin_v0_9_signature_scan.csv",
        "signature_ranking_csv": "kalyx_origin_v0_9_signature_ranking.csv",
        "signature_report_md": "KALYX_ORIGIN_V0_9_SIGNATURE_REPORT.md",
        "families_v08_csv": "kalyx_origin_v0_8_families_all.csv",
        "motif_v07_csv": "kalyx_origin_v0_7_top_kmers_all.csv",
        "repeat_v06_csv": "kalyx_origin_v0_6_repeat_context.csv",
        "context_v05_csv": "kalyx_origin_v0_5_fasta_context.csv",
    }
    found: dict[str, set[str]] = {k: set() for k in keys}
    # Existing files under root
    for p in root.rglob("*"):
        if p.is_file():
            for key, name in keys.items():
                if p.name == name:
                    found[key].add(str(p))
    if log_text:
        # Windows-like relative or absolute paths ending in the file name.
        for key, name in keys.items():
            pattern = re.compile(r"([A-Za-z]:\\[^\s`|]+?" + re.escape(name) + r"|\.\\[^\s`|]+?" + re.escape(name) + r")")
            for m in pattern.finditer(log_text):
                raw = m.group(1)
                found[key].add(raw)
    return {k: sorted(v) for k, v in found.items()}


def longest_common_substrings(seqs: Sequence[str]) -> list[tuple[str, int]]:
    if not seqs:
        return []
    first = min(seqs, key=len)
    candidates: Counter[str] = Counter()
    for i in range(len(first)):
        for j in range(i + 1, len(first) + 1):
            sub = first[i:j]
            if all(sub in s for s in seqs):
                candidates[sub] = len(sub)
    if not candidates:
        return []
    max_len = max(len(s) for s in candidates)
    return sorted(((s, len(s)) for s in candidates if len(s) == max_len), key=lambda x: x[0])


def max_overlap(a: str, b: str, min_overlap: int = 1) -> int:
    n = min(len(a), len(b))
    best = 0
    for k in range(min_overlap, n + 1):
        if a[-k:] == b[:k]:
            best = k
    return best


def build_edges(seqs: Sequence[str], min_overlap: int) -> list[Edge]:
    edges: list[Edge] = []
    for a in seqs:
        for b in seqs:
            if a == b:
                continue
            ov = max_overlap(a, b, min_overlap=min_overlap)
            if ov >= min_overlap:
                edges.append(
                    Edge(
                        src=a,
                        dst=b,
                        overlap=ov,
                        suffix_from_src=a[-ov:],
                        prefix_from_dst=b[:ov],
                        kind="suffix_prefix",
                    )
                )
    edges.sort(key=lambda e: (-e.overlap, e.src, e.dst))
    return edges


def greedy_superstring(seqs: Sequence[str]) -> str:
    items = list(dict.fromkeys(seqs))
    while len(items) > 1:
        best: tuple[int, int, int, str] | None = None
        for i, a in enumerate(items):
            for j, b in enumerate(items):
                if i == j:
                    continue
                if b in a:
                    candidate = a
                    ov = len(b)
                else:
                    ov = max_overlap(a, b, 1)
                    candidate = a + b[ov:]
                score = (ov, -len(candidate), -i, candidate)
                if best is None or score > best:
                    best = score
                    best_pair = (i, j, candidate)
        assert best is not None
        i, j, merged = best_pair
        new_items = [x for k, x in enumerate(items) if k not in (i, j)]
        new_items.append(merged)
        items = new_items
    return items[0]


def find_hits(seq: str, motifs: Sequence[str], window_size: int | None) -> list[Hit]:
    hits: list[Hit] = []
    for motif in motifs:
        for strand, pat in (("+", motif), ("-", reverse_complement(motif))):
            start = 0
            while True:
                pos = seq.find(pat, start)
                if pos < 0:
                    break
                win = (pos // window_size) if window_size else None
                hits.append(Hit(motif=motif, strand=strand, pos0=pos, pos1=pos + len(pat), window=win))
                start = pos + 1
    hits.sort(key=lambda h: (h.pos0, h.strand, h.motif))
    return hits


def analyse_periods(hits: Sequence[Hit], periods: Sequence[int]) -> list[dict[str, object]]:
    rows: list[dict[str, object]] = []
    by_key: dict[tuple[str, str], list[int]] = defaultdict(list)
    all_positions: list[int] = []
    for h in hits:
        by_key[(h.motif, h.strand)].append(h.pos0)
        all_positions.append(h.pos0)
    for key, pos in list(by_key.items()) + [(("ALL", "*"), sorted(all_positions))]:
        motif, strand = key
        pos = sorted(pos)
        deltas = [b - a for a, b in zip(pos, pos[1:])]
        delta_counts = Counter(deltas)
        top_delta = delta_counts.most_common(1)[0][0] if delta_counts else None
        gcd_val = 0
        for d in deltas:
            gcd_val = math.gcd(gcd_val, d)
        for p in periods:
            if len(pos) < 2:
                same_mod_rate = 0.0
                dominant_mod = None
                dominant_mod_count = 0
            else:
                mods = Counter(x % p for x in pos)
                dominant_mod, dominant_mod_count = mods.most_common(1)[0]
                same_mod_rate = dominant_mod_count / len(pos)
            rows.append(
                {
                    "motif": motif,
                    "strand": strand,
                    "hits": len(pos),
                    "deltas": len(deltas),
                    "gcd_delta": gcd_val,
                    "top_delta": top_delta if top_delta is not None else "",
                    "period": p,
                    "dominant_mod": "" if dominant_mod is None else dominant_mod,
                    "dominant_mod_count": dominant_mod_count,
                    "dominant_mod_rate": f"{same_mod_rate:.12f}",
                }
            )
    return rows


def mono_expected_count(seq: str, motif: str) -> float:
    counts = Counter(c for c in seq if c in DNA)
    n = sum(counts.values())
    if n == 0 or len(seq) < len(motif):
        return 0.0
    prob = 1.0
    for c in motif:
        prob *= counts[c] / n
    return max(0, len(seq) - len(motif) + 1) * prob


def main(argv: Sequence[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description="KALYX motif substrate decoder v0.1")
    ap.add_argument("--root", default=".", help="Project/root directory for artifact discovery")
    ap.add_argument("--log", default="Powershell.log", help="PowerShell log path, relative to root unless absolute")
    ap.add_argument("--fasta", default="", help="Optional FASTA path; if omitted, tries chr17.fa and Genome/chr17.fa")
    ap.add_argument("--outdir", default="Decode_chr17_v01", help="Output directory")
    ap.add_argument("--motif", action="append", help="Motif(s); can be repeated or comma/space separated")
    ap.add_argument("--min-overlap", type=int, default=6, help="Minimum suffix/prefix overlap for graph edges")
    ap.add_argument("--window-bases", type=int, default=1048576, help="Window size for hit scan")
    ap.add_argument("--periods", default="171,342,512,1024,1048576", help="Comma-separated periods to test")
    args = ap.parse_args(argv)

    root = Path(args.root).resolve()
    outdir = Path(args.outdir)
    if not outdir.is_absolute():
        outdir = root / outdir
    outdir.mkdir(parents=True, exist_ok=True)

    log_path = Path(args.log)
    if not log_path.is_absolute():
        log_path = root / log_path
    log_text = log_path.read_text(encoding="utf-8", errors="replace") if log_path.exists() else ""

    motifs_from_arg = parse_motif_arg(args.motif)
    motif_objs = parse_signature_motifs_from_log(log_text)
    if motif_objs:
        motif_seqs = [m.seq for m in motif_objs[: len(motifs_from_arg) if args.motif else 8]]
    else:
        motif_objs = [Motif(rank=i + 1, seq=s, source="default_or_arg") for i, s in enumerate(motifs_from_arg)]
        motif_seqs = [m.seq for m in motif_objs]

    # Normalize/unique while preserving order.
    motif_seqs = list(dict.fromkeys(norm_seq(s) for s in motif_seqs))
    motifs = [Motif(rank=i + 1, seq=s, source="active") for i, s in enumerate(motif_seqs)]

    artifacts = discover_existing_artifacts(root, log_text)
    scan_windows = parse_scan_windows_from_log(log_text)

    write_csv(
        outdir / "decode_log_windows.csv",
        scan_windows,
        ["label", "base_start0", "base_end0", "length", "hits", "rc", "density", "rc_balance", "period", "period_rate"],
    )

    # Motif table.
    motif_rows: list[dict[str, object]] = []
    for i, s in enumerate(motif_seqs, 1):
        motif_rows.append(
            {
                "rank": i,
                "motif": s,
                "k": len(s),
                "gc": f"{(s.count('G') + s.count('C')) / len(s):.12f}",
                "reverse_complement": reverse_complement(s),
                "sha256_12": sha256_text(s)[:12],
                "mono_expected_per_1Mb_gc_local_unknown": "",
            }
        )
    write_csv(outdir / "decode_motifs.csv", motif_rows, ["rank", "motif", "k", "gc", "reverse_complement", "sha256_12", "mono_expected_per_1Mb_gc_local_unknown"])

    # Core table.
    cores = longest_common_substrings(motif_seqs)
    core_rows = [{"core": c, "length": l, "present_in": len(motif_seqs)} for c, l in cores]
    write_csv(outdir / "decode_core.csv", core_rows, ["core", "length", "present_in"])

    # Edges.
    edges = build_edges(motif_seqs, min_overlap=args.min_overlap)
    write_csv(
        outdir / "decode_edges.csv",
        (dataclasses.asdict(e) for e in edges),
        ["src", "dst", "overlap", "suffix_from_src", "prefix_from_dst", "kind"],
    )

    superseq = greedy_superstring(motif_seqs)
    (outdir / "decode_supersequence.txt").write_text(superseq + "\n", encoding="utf-8")

    # Optional FASTA scan.
    fasta_path: Path | None = None
    if args.fasta:
        fp = Path(args.fasta)
        fasta_path = fp if fp.is_absolute() else root / fp
    else:
        for cand in (root / "chr17.fa", root / "Genome" / "chr17.fa", root / "Genome" / "chr17.fa.gz"):
            if cand.exists() and cand.suffix != ".gz":
                fasta_path = cand
                break

    hits: list[Hit] = []
    fasta_len = None
    if fasta_path and fasta_path.exists():
        seq = read_fasta(fasta_path)
        fasta_len = len(seq)
        hits = find_hits(seq, motif_seqs, args.window_bases)
        # fill expectation per whole fasta
        motif_rows2 = []
        for r in motif_rows:
            motif = str(r["motif"])
            rr = dict(r)
            rr["mono_expected_per_1Mb_gc_local_unknown"] = f"{mono_expected_count(seq[: min(len(seq), 1_000_000)], motif):.9f}"
            motif_rows2.append(rr)
        write_csv(outdir / "decode_motifs.csv", motif_rows2, ["rank", "motif", "k", "gc", "reverse_complement", "sha256_12", "mono_expected_per_1Mb_gc_local_unknown"])
    else:
        seq = ""

    write_csv(
        outdir / "decode_hits.csv",
        (dataclasses.asdict(h) for h in hits),
        ["motif", "strand", "pos0", "pos1", "window"],
    )

    periods = [int(x) for x in re.split(r"[\s,;]+", args.periods.strip()) if x]
    period_rows = analyse_periods(hits, periods) if hits else []
    write_csv(
        outdir / "decode_periods.csv",
        period_rows,
        ["motif", "strand", "hits", "deltas", "gcd_delta", "top_delta", "period", "dominant_mod", "dominant_mod_count", "dominant_mod_rate"],
    )

    # Branch after strongest common core
    core = cores[0][0] if cores else ""
    branch_counter: Counter[str] = Counter()
    for s in motif_seqs:
        idx = s.find(core)
        if idx >= 0:
            branch_counter[s[idx + len(core): idx + len(core) + 4]] += 1

    manifest = {
        "version": "KALYX_SIGNATURE_DECODER_V0_1",
        "root": str(root),
        "log": str(log_path) if log_path.exists() else None,
        "fasta": str(fasta_path) if fasta_path and fasta_path.exists() else None,
        "fasta_len": fasta_len,
        "motifs": motif_seqs,
        "common_cores": core_rows,
        "branch_after_primary_core": dict(branch_counter),
        "supersequence": superseq,
        "edges": len(edges),
        "hits": len(hits),
        "scan_windows_from_log": len(scan_windows),
        "discovered_artifacts": artifacts,
        "boundary": "structure/periodicity diagnostic only; no origin inference",
    }
    (outdir / "decode_manifest.json").write_text(json.dumps(manifest, indent=2, ensure_ascii=False), encoding="utf-8")

    report = []
    report.append("# KALYX Signature Decoder v0.1\n")
    report.append("## Boundary\n")
    report.append("Dieses Artefakt dechiffriert die beobachtete Motiv-Syntax als Graph, Periodik und Koordinatenstruktur. Es beweist keinen natürlichen oder künstlichen Ursprung.\n")
    report.append("## Inputs\n")
    report.append(f"- root: `{root}`\n")
    report.append(f"- log: `{log_path if log_path.exists() else 'not found'}`\n")
    report.append(f"- fasta: `{fasta_path if fasta_path and fasta_path.exists() else 'not available'}`\n")
    report.append(f"- motifs: `{len(motif_seqs)}`\n")
    report.append("\n## Motifs\n\n")
    report.append("| rank | motif | reverse complement | GC |\n|---:|---|---|---:|\n")
    for r in motif_rows:
        report.append(f"| {r['rank']} | `{r['motif']}` | `{r['reverse_complement']}` | {r['gc']} |\n")
    report.append("\n## Common core\n\n")
    if cores:
        for c, l in cores[:8]:
            report.append(f"- `{c}` length={l}\n")
    else:
        report.append("- no common core\n")
    report.append("\n## Branches after primary core\n\n")
    if branch_counter:
        for b, n in branch_counter.most_common():
            label = b if b else "<end>"
            report.append(f"- `{label}`: {n}\n")
    else:
        report.append("- no branches\n")
    report.append("\n## Greedy shortest supersequence candidate\n\n")
    report.append(f"`{superseq}`\n")
    report.append("\n## Graph\n\n")
    report.append(f"- edges with overlap >= {args.min_overlap}: {len(edges)}\n")
    report.append("- strongest edges:\n")
    for e in edges[:12]:
        report.append(f"  - `{e.src}` -> `{e.dst}` overlap={e.overlap}\n")
    report.append("\n## Log-derived scan windows\n\n")
    report.append(f"- parsed v0.9 windows from Powershell.log: {len(scan_windows)}\n")
    if scan_windows:
        top = sorted(scan_windows, key=lambda r: (int(r['hits']), float(r['density'])), reverse=True)[:10]
        report.append("\nTop windows by hits:\n\n| label | start | end | hits | rc | density | period | rate |\n|---|---:|---:|---:|---:|---:|---:|---:|\n")
        for r in top:
            report.append(f"| {r['label']} | {r['base_start0']} | {r['base_end0']} | {r['hits']} | {r['rc']} | {r['density']} | {r['period']} | {r['period_rate']} |\n")
    report.append("\n## FASTA scan\n\n")
    if hits:
        report.append(f"- hits forward+RC: {len(hits)}\n")
        by_motif = Counter((h.motif, h.strand) for h in hits)
        report.append("\n| motif | strand | hits |\n|---|---:|---:|\n")
        for (motif, strand), n in sorted(by_motif.items()):
            report.append(f"| `{motif}` | {strand} | {n} |\n")
    else:
        report.append("- FASTA nicht verfügbar; `decode_hits.csv` ist leer. Lege `chr17.fa` oder `Genome/chr17.fa` ins Projekt oder übergib `--fasta`.\n")
    report.append("\n## Next validation steps\n\n")
    report.append("1. `decode_hits.csv` mit echter chr17-FASTA erzeugen.\n")
    report.append("2. Periodik in `decode_periods.csv` gegen RepeatMasker/GC-kontrollierte Nullmodelle vergleichen.\n")
    report.append("3. Die v0.9 CSV-Artefakte aus `Origin_chr17_v09_signature_scan` in denselben Root legen, damit Report und Scanner verglichen werden können.\n")
    (outdir / "decode_report.md").write_text("".join(report), encoding="utf-8")

    print(f"KALYX signature decoder complete: {outdir}")
    print(f"  motifs={len(motif_seqs)} edges={len(edges)} log_windows={len(scan_windows)} hits={len(hits)}")
    print(f"  report={outdir / 'decode_report.md'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
