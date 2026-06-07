#!/usr/bin/env python3
"""
KALYX Signature Block Analyzer v0.3

Reads v0.2 cluster/template artifacts and lifts the analysis from "hit clusters"
to "repeat blocks": module offsets, A/B spacer, cluster-start deltas and phase
lattices. It is a structural decoder only; it does not infer origin.
"""
from __future__ import annotations

import argparse
import csv
import json
import math
import os
import statistics
from collections import Counter, defaultdict
from dataclasses import dataclass, asdict
from pathlib import Path
from typing import Iterable, Optional


@dataclass(frozen=True)
class Template:
    rank: int
    count: int
    n_hits: int
    n_motifs: int
    strand_set: str
    sha12: str
    offsets: tuple[int, ...]
    motifs_by_offset: tuple[str, ...]
    strands_by_offset: tuple[str, ...]

    @property
    def span_from_offsets(self) -> int:
        if not self.offsets:
            return 0
        return max(self.offsets) + 12 - min(self.offsets)

    @property
    def min_a_offset(self) -> Optional[int]:
        vals = [o for o in self.offsets if o <= 12]
        return min(vals) if vals else None

    @property
    def max_a_offset(self) -> Optional[int]:
        vals = [o for o in self.offsets if o <= 12]
        return max(vals) if vals else None

    @property
    def min_b_offset(self) -> Optional[int]:
        vals = [o for o in self.offsets if o >= 24]
        return min(vals) if vals else None

    @property
    def max_b_offset(self) -> Optional[int]:
        vals = [o for o in self.offsets if o >= 24]
        return max(vals) if vals else None

    @property
    def has_a(self) -> bool:
        return self.min_a_offset is not None and self.max_a_offset is not None

    @property
    def has_b(self) -> bool:
        return self.min_b_offset is not None and self.max_b_offset is not None

    @property
    def spacer_length(self) -> Optional[int]:
        if not (self.has_a and self.has_b):
            return None
        # A-module genomic end = start + maxA + 12, B-module start = start + minB.
        return int(self.min_b_offset) - (int(self.max_a_offset) + 12)


@dataclass(frozen=True)
class Cluster:
    cluster_id: int
    start0: int
    end0: int
    span: int
    n_hits: int
    n_motifs: int
    strand_set: str
    window: int
    template_rank: int
    template_sha12: str
    motifs: str


def read_csv_dicts(path: Path) -> list[dict[str, str]]:
    if not path.exists():
        raise FileNotFoundError(f"CSV fehlt: {path}")
    with path.open("r", encoding="utf-8-sig", newline="") as f:
        return list(csv.DictReader(f))


def int_field(row: dict[str, str], key: str, default: int = 0) -> int:
    raw = row.get(key, "")
    if raw is None or str(raw).strip() == "":
        return default
    return int(str(raw).strip())


def parse_pipe_ints(raw: str) -> tuple[int, ...]:
    if not raw:
        return tuple()
    return tuple(int(x) for x in raw.split("|") if x != "")


def parse_pipe_strs(raw: str) -> tuple[str, ...]:
    if not raw:
        return tuple()
    return tuple(x for x in raw.split("|"))


def load_templates(path: Path) -> dict[int, Template]:
    out: dict[int, Template] = {}
    for row in read_csv_dicts(path):
        rank = int_field(row, "template_rank")
        out[rank] = Template(
            rank=rank,
            count=int_field(row, "count"),
            n_hits=int_field(row, "n_hits"),
            n_motifs=int_field(row, "n_motifs"),
            strand_set=row.get("strand_set", ""),
            sha12=row.get("template_sha12", ""),
            offsets=parse_pipe_ints(row.get("offsets", "")),
            motifs_by_offset=parse_pipe_strs(row.get("motifs_by_offset", "")),
            strands_by_offset=parse_pipe_strs(row.get("strands_by_offset", "")),
        )
    return out


def load_clusters(path: Path) -> list[Cluster]:
    clusters: list[Cluster] = []
    for row in read_csv_dicts(path):
        clusters.append(
            Cluster(
                cluster_id=int_field(row, "cluster_id"),
                start0=int_field(row, "start0"),
                end0=int_field(row, "end0"),
                span=int_field(row, "span"),
                n_hits=int_field(row, "n_hits"),
                n_motifs=int_field(row, "n_motifs"),
                strand_set=row.get("strand_set", ""),
                window=int_field(row, "window"),
                template_rank=int_field(row, "template_rank"),
                template_sha12=row.get("template_sha12", ""),
                motifs=row.get("motifs", ""),
            )
        )
    clusters.sort(key=lambda c: (c.start0, c.end0, c.cluster_id))
    return clusters


def write_csv(path: Path, rows: list[dict[str, object]], fieldnames: list[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as f:
        w = csv.DictWriter(f, fieldnames=fieldnames)
        w.writeheader()
        for r in rows:
            w.writerow(r)


def nearest_family(delta: int, centers: Iterable[int], tolerance: int) -> str:
    best_center = None
    best_dist = 10**9
    for c in centers:
        d = abs(delta - c)
        if d < best_dist:
            best_dist = d
            best_center = c
    if best_center is None or best_dist > tolerance:
        return "other"
    sign = "+" if delta - best_center >= 0 else ""
    return f"{best_center}{sign}{delta-best_center}"


def dominant_mod(starts: list[int], period: int) -> tuple[int, int, float]:
    if not starts:
        return (0, 0, 0.0)
    c = Counter(s % period for s in starts)
    phase, count = c.most_common(1)[0]
    return (phase, count, count / len(starts))


def run(args: argparse.Namespace) -> int:
    decode_dir = Path(args.decode_dir)
    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    clusters_csv = Path(args.clusters) if args.clusters else decode_dir / "cluster_v02_clusters.csv"
    templates_csv = Path(args.templates) if args.templates else decode_dir / "cluster_v02_templates.csv"

    templates = load_templates(templates_csv)
    clusters = load_clusters(clusters_csv)

    target_rank = int(args.target_rank)
    target_clusters = [c for c in clusters if c.template_rank == target_rank]
    target_clusters.sort(key=lambda c: c.start0)
    target_template = templates.get(target_rank)

    periods = [int(x) for x in args.periods.split(",") if x.strip()]
    family_centers = [int(x) for x in args.family_centers.split(",") if x.strip()]
    family_tolerance = int(args.family_tolerance)

    # Per-cluster block lift.
    block_rows: list[dict[str, object]] = []
    for idx, c in enumerate(target_clusters):
        t = templates.get(c.template_rank)
        next_start = target_clusters[idx + 1].start0 if idx + 1 < len(target_clusters) else None
        prev_start = target_clusters[idx - 1].start0 if idx > 0 else None
        next_delta = (next_start - c.start0) if next_start is not None else ""
        prev_delta = (c.start0 - prev_start) if prev_start is not None else ""

        a_start = a_end = b_start = b_end = ""
        spacer = ""
        if t:
            if t.has_a:
                a_start = c.start0 + int(t.min_a_offset)
                a_end = c.start0 + int(t.max_a_offset) + 12
            if t.has_b:
                b_start = c.start0 + int(t.min_b_offset)
                b_end = c.start0 + int(t.max_b_offset) + 12
            if t.spacer_length is not None:
                spacer = t.spacer_length

        block_rows.append({
            "cluster_id": c.cluster_id,
            "start0": c.start0,
            "end0": c.end0,
            "span": c.span,
            "window": c.window,
            "template_rank": c.template_rank,
            "template_sha12": c.template_sha12,
            "strand_set": c.strand_set,
            "n_hits": c.n_hits,
            "n_motifs": c.n_motifs,
            "a_start0": a_start,
            "a_end0": a_end,
            "b_start0": b_start,
            "b_end0": b_end,
            "spacer_length": spacer,
            "prev_delta": prev_delta,
            "next_delta": next_delta,
            "next_delta_family": nearest_family(int(next_delta), family_centers, family_tolerance) if next_delta != "" else "",
        })

    write_csv(out_dir / "cluster_v03_blocks.csv", block_rows, [
        "cluster_id","start0","end0","span","window","template_rank","template_sha12","strand_set",
        "n_hits","n_motifs","a_start0","a_end0","b_start0","b_end0","spacer_length",
        "prev_delta","next_delta","next_delta_family"
    ])

    # Spacer/template table.
    spacer_rows: list[dict[str, object]] = []
    for rank, t in sorted(templates.items()):
        spacer_rows.append({
            "template_rank": rank,
            "count": t.count,
            "n_hits": t.n_hits,
            "n_motifs": t.n_motifs,
            "strand_set": t.strand_set,
            "span_from_offsets": t.span_from_offsets,
            "has_a": int(t.has_a),
            "has_b": int(t.has_b),
            "min_a_offset": "" if t.min_a_offset is None else t.min_a_offset,
            "max_a_offset": "" if t.max_a_offset is None else t.max_a_offset,
            "min_b_offset": "" if t.min_b_offset is None else t.min_b_offset,
            "max_b_offset": "" if t.max_b_offset is None else t.max_b_offset,
            "spacer_length": "" if t.spacer_length is None else t.spacer_length,
            "offsets": "|".join(map(str, t.offsets)),
            "motifs_by_offset": "|".join(t.motifs_by_offset),
        })
    write_csv(out_dir / "cluster_v03_spacers.csv", spacer_rows, [
        "template_rank","count","n_hits","n_motifs","strand_set","span_from_offsets",
        "has_a","has_b","min_a_offset","max_a_offset","min_b_offset","max_b_offset",
        "spacer_length","offsets","motifs_by_offset"
    ])

    # Delta histogram for target only.
    deltas = [target_clusters[i + 1].start0 - target_clusters[i].start0 for i in range(len(target_clusters) - 1)]
    delta_counter = Counter(deltas)
    delta_rows = []
    for delta, count in delta_counter.most_common():
        delta_rows.append({
            "delta": delta,
            "count": count,
            "family": nearest_family(delta, family_centers, family_tolerance),
        })
    write_csv(out_dir / "cluster_v03_target_delta_hist.csv", delta_rows, ["delta","count","family"])

    # Family histogram.
    fam_counter = Counter(nearest_family(d, family_centers, family_tolerance) for d in deltas)
    fam_rows = [{"family": fam, "count": cnt, "rate": cnt / len(deltas) if deltas else 0.0} for fam, cnt in fam_counter.most_common()]
    write_csv(out_dir / "cluster_v03_delta_families.csv", fam_rows, ["family","count","rate"])

    # Phase lattice.
    starts = [c.start0 for c in target_clusters]
    phase_rows = []
    for p in periods:
        phase, count, rate = dominant_mod(starts, p)
        phase_rows.append({
            "template_rank": target_rank,
            "period": p,
            "clusters": len(starts),
            "dominant_phase": phase,
            "dominant_phase_count": count,
            "dominant_phase_rate": f"{rate:.12f}",
        })
    write_csv(out_dir / "cluster_v03_phase_lattice.csv", phase_rows, [
        "template_rank","period","clusters","dominant_phase","dominant_phase_count","dominant_phase_rate"
    ])

    # Window stats for target.
    win_counts = Counter(c.window for c in target_clusters)
    win_rows = []
    for w, cnt in win_counts.most_common():
        starts_w = [c.start0 for c in target_clusters if c.window == w]
        win_rows.append({
            "window": w,
            "target_clusters": cnt,
            "start_min": min(starts_w) if starts_w else "",
            "start_max": max(starts_w) if starts_w else "",
        })
    write_csv(out_dir / "cluster_v03_target_windows.csv", win_rows, ["window","target_clusters","start_min","start_max"])

    # Basic delta stats.
    stats = {}
    if deltas:
        stats = {
            "min": min(deltas),
            "max": max(deltas),
            "mean": statistics.fmean(deltas),
            "median": statistics.median(deltas),
            "top_delta": delta_counter.most_common(1)[0][0],
            "top_delta_count": delta_counter.most_common(1)[0][1],
        }

    manifest = {
        "version": "KALYX_SIGNATURE_BLOCK_V0_3",
        "boundary": "Structural decoder only; no natural/artificial origin inference.",
        "decode_dir": str(decode_dir),
        "out_dir": str(out_dir),
        "clusters_csv": str(clusters_csv),
        "templates_csv": str(templates_csv),
        "total_clusters": len(clusters),
        "templates": len(templates),
        "target_rank": target_rank,
        "target_clusters": len(target_clusters),
        "target_template": asdict(target_template) if target_template else None,
        "delta_stats": stats,
        "periods": periods,
        "family_centers": family_centers,
        "family_tolerance": family_tolerance,
    }
    with (out_dir / "cluster_v03_manifest.json").open("w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=2, ensure_ascii=False)

    # Report.
    top_delta_lines = "\n".join(
        f"| {r['delta']} | {r['count']} | {r['family']} |" for r in delta_rows[:25]
    )
    top_family_lines = "\n".join(
        f"| {r['family']} | {r['count']} | {float(r['rate']):.12f} |" for r in fam_rows[:25]
    )
    phase_lines = "\n".join(
        f"| {r['period']} | {r['dominant_phase']} | {r['dominant_phase_count']} | {r['dominant_phase_rate']} |"
        for r in phase_rows
    )
    template_block = ""
    if target_template:
        template_block = f"""
## Target template

- rank: `{target_template.rank}`
- count in v0.2: `{target_template.count}`
- offsets: `{"|".join(map(str, target_template.offsets))}`
- motifs_by_offset: `{"|".join(target_template.motifs_by_offset)}`
- span_from_offsets: `{target_template.span_from_offsets}`
- A offset range: `{target_template.min_a_offset}`..`{target_template.max_a_offset}`
- B offset range: `{target_template.min_b_offset}`..`{target_template.max_b_offset}`
- spacer_length: `{target_template.spacer_length}`
"""
    report = f"""# KALYX Signature Block Analyzer v0.3

## Boundary

Dieses Artefakt hebt v0.2-Cluster auf Blockebene: A-Modul, B-Modul, Spacer,
Cluster-Start-Deltas und Phasenraster. Es beweist keinen natürlichen oder
künstlichen Ursprung.

## Summary

- input clusters: `{len(clusters)}`
- templates: `{len(templates)}`
- target template rank: `{target_rank}`
- target clusters: `{len(target_clusters)}`
- output blocks: `{len(block_rows)}`
- top target delta: `{stats.get('top_delta', '')}` count=`{stats.get('top_delta_count', '')}`
- target delta median: `{stats.get('median', '')}`

{template_block}

## Top target cluster-start deltas

| delta | count | family |
|---:|---:|---|
{top_delta_lines}

## Delta families

| family | count | rate |
|---|---:|---:|
{top_family_lines}

## Phase lattice

| period | dominant_phase | count | rate |
|---:|---:|---:|---:|
{phase_lines}

## Output files

```text
cluster_v03_blocks.csv
cluster_v03_spacers.csv
cluster_v03_target_delta_hist.csv
cluster_v03_delta_families.csv
cluster_v03_phase_lattice.csv
cluster_v03_target_windows.csv
cluster_v03_manifest.json
cluster_v03_report.md
```

## Interpretation guardrail

Wenn Template rank {target_rank} stabil span=47 und spacer=16 zeigt, ist der
stärkste aktuelle Befund ein wiederholtes A/Spacer/B-Blocksubstrat. Bedeutung,
Ursprung und biologische Funktion bleiben bis zu RepeatMasker-/GC-/Cross-
Chromosom-Kontrollen offen.
"""
    (out_dir / "cluster_v03_report.md").write_text(report, encoding="utf-8")

    print(f"KALYX signature block analyzer complete: {out_dir}")
    print(f"  clusters={len(clusters)} target_rank={target_rank} target_clusters={len(target_clusters)}")
    print(f"  report={out_dir / 'cluster_v03_report.md'}")
    return 0


def main() -> int:
    ap = argparse.ArgumentParser(description="KALYX Signature Block Analyzer v0.3")
    ap.add_argument("--decode-dir", required=True, help="Directory containing v0.2 cluster outputs")
    ap.add_argument("--out-dir", required=True, help="Output directory")
    ap.add_argument("--clusters", default="", help="Explicit cluster_v02_clusters.csv path")
    ap.add_argument("--templates", default="", help="Explicit cluster_v02_templates.csv path")
    ap.add_argument("--target-rank", type=int, default=3, help="Template rank to lift as A/Spacer/B block")
    ap.add_argument("--periods", default="170,171,337,341,342,506,507,508,513,2378,2379,2380",
                    help="Comma-separated phase periods")
    ap.add_argument("--family-centers", default="137,168,170,171,200,205,341,342,506,507,508,513,2379",
                    help="Comma-separated delta family centers")
    ap.add_argument("--family-tolerance", type=int, default=3, help="Delta family +/- tolerance")
    return run(ap.parse_args())


if __name__ == "__main__":
    raise SystemExit(main())
