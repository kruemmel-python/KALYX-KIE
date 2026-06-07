#!/usr/bin/env python3
"""
KALYX Signature Cluster Analyzer v0.2

Input:
  decode_hits.csv from KALYX Signature Decoder v0.1

Purpose:
  Convert raw motif hits into spatial motif clusters, template classes,
  cluster-start delta histograms, and a scientific report.

Boundary:
  This tool analyzes syntax/periodicity/coordinate structure only.
  It does not infer natural or artificial origin.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
from collections import Counter, defaultdict
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

SUPERSEQ = "TGCATTCAACTCACAGAAGCATTCTCAGAA"
DEFAULT_CLUSTER_GAP = 32
DEFAULT_WINDOW = 1_048_576


@dataclass(frozen=True)
class Hit:
    motif: str
    strand: str
    pos0: int
    pos1: int
    window: int


def read_hits(path: Path) -> list[Hit]:
    hits: list[Hit] = []
    with path.open("r", encoding="utf-8", newline="") as f:
        r = csv.DictReader(f)
        required = {"motif", "strand", "pos0", "pos1", "window"}
        missing = required - set(r.fieldnames or [])
        if missing:
            raise ValueError(f"decode_hits.csv missing columns: {sorted(missing)}")
        for row in r:
            hits.append(
                Hit(
                    motif=row["motif"],
                    strand=row["strand"],
                    pos0=int(row["pos0"]),
                    pos1=int(row["pos1"]),
                    window=int(row["window"]),
                )
            )
    hits.sort(key=lambda h: (h.pos0, h.pos1, h.motif, h.strand))
    return hits


def sha12(s: str) -> str:
    return hashlib.sha256(s.encode("utf-8")).hexdigest()[:12]


def cluster_hits(hits: list[Hit], gap: int) -> list[list[Hit]]:
    if not hits:
        return []
    clusters: list[list[Hit]] = []
    cur: list[Hit] = [hits[0]]
    for h in hits[1:]:
        if h.pos0 - cur[-1].pos0 <= gap:
            cur.append(h)
        else:
            clusters.append(cur)
            cur = [h]
    clusters.append(cur)
    return clusters


def template_of(cluster: list[Hit]) -> tuple[tuple[int, str, str], ...]:
    start = min(h.pos0 for h in cluster)
    return tuple((h.pos0 - start, h.motif, h.strand) for h in sorted(cluster, key=lambda x: (x.pos0, x.motif, x.strand)))


def write_csv(path: Path, rows: list[dict], fieldnames: list[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as f:
        w = csv.DictWriter(f, fieldnames=fieldnames)
        w.writeheader()
        for row in rows:
            w.writerow(row)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--decode-dir", required=True, help="Directory containing decode_hits.csv")
    ap.add_argument("--out-dir", required=True, help="Output directory")
    ap.add_argument("--cluster-gap", type=int, default=DEFAULT_CLUSTER_GAP)
    ap.add_argument("--window-size", type=int, default=DEFAULT_WINDOW)
    args = ap.parse_args()

    decode_dir = Path(args.decode_dir)
    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    hits_path = decode_dir / "decode_hits.csv"
    hits = read_hits(hits_path)
    clusters = cluster_hits(hits, args.cluster_gap)

    tpl_counter: Counter[tuple[tuple[int, str, str], ...]] = Counter(template_of(c) for c in clusters)
    tpl_rank = {tpl: i + 1 for i, (tpl, _) in enumerate(tpl_counter.most_common())}

    cluster_rows: list[dict] = []
    for cid, c in enumerate(clusters):
        start = min(h.pos0 for h in c)
        end = max(h.pos1 for h in c)
        strands = "".join(sorted({h.strand for h in c}))
        motifs = sorted({h.motif for h in c})
        tpl = template_of(c)
        cluster_rows.append(
            {
                "cluster_id": cid,
                "start0": start,
                "end0": end,
                "span": end - start,
                "n_hits": len(c),
                "n_motifs": len(motifs),
                "strand_set": strands,
                "window": start // args.window_size,
                "template_rank": tpl_rank[tpl],
                "template_sha12": sha12(str(tpl)),
                "motifs": "|".join(motifs),
            }
        )

    template_rows: list[dict] = []
    for tpl, count in tpl_counter.most_common():
        rank = tpl_rank[tpl]
        strand_set = "".join(sorted({x[2] for x in tpl}))
        offsets = "|".join(str(x[0]) for x in tpl)
        motifs = "|".join(x[1] for x in tpl)
        strands = "|".join(x[2] for x in tpl)
        template_rows.append(
            {
                "template_rank": rank,
                "count": count,
                "n_hits": len(tpl),
                "n_motifs": len({x[1] for x in tpl}),
                "strand_set": strand_set,
                "template_sha12": sha12(str(tpl)),
                "offsets": offsets,
                "motifs_by_offset": motifs,
                "strands_by_offset": strands,
            }
        )

    # Delta histogram on cluster starts, global and core-band.
    starts = [r["start0"] for r in cluster_rows]
    deltas = [starts[i] - starts[i - 1] for i in range(1, len(starts))]
    delta_counts = Counter(deltas)
    delta_rows = [{"delta": d, "count": c} for d, c in delta_counts.most_common()]

    # Window summary.
    win_counter: dict[int, dict[str, int]] = defaultdict(lambda: {"clusters": 0, "hits": 0, "complete_8": 0})
    for r in cluster_rows:
        w = int(r["window"])
        win_counter[w]["clusters"] += 1
        win_counter[w]["hits"] += int(r["n_hits"])
        if int(r["n_hits"]) == 8 and int(r["n_motifs"]) == 8:
            win_counter[w]["complete_8"] += 1
    window_rows = []
    for w in sorted(win_counter):
        start = w * args.window_size
        window_rows.append(
            {
                "window": w,
                "start0": start,
                "end0": start + args.window_size,
                "clusters": win_counter[w]["clusters"],
                "hits": win_counter[w]["hits"],
                "complete_8_clusters": win_counter[w]["complete_8"],
            }
        )

    # Motif/strand summary from hits.
    motif_counter: Counter[tuple[str, str]] = Counter((h.motif, h.strand) for h in hits)
    motif_rows = []
    for (motif, strand), count in sorted(motif_counter.items(), key=lambda x: (-x[1], x[0])):
        motif_rows.append({"motif": motif, "strand": strand, "hits": count})

    # Write outputs.
    write_csv(out_dir / "cluster_v02_clusters.csv", cluster_rows,
              ["cluster_id", "start0", "end0", "span", "n_hits", "n_motifs", "strand_set", "window",
               "template_rank", "template_sha12", "motifs"])
    write_csv(out_dir / "cluster_v02_templates.csv", template_rows,
              ["template_rank", "count", "n_hits", "n_motifs", "strand_set", "template_sha12",
               "offsets", "motifs_by_offset", "strands_by_offset"])
    write_csv(out_dir / "cluster_v02_delta_hist.csv", delta_rows, ["delta", "count"])
    write_csv(out_dir / "cluster_v02_windows.csv", window_rows,
              ["window", "start0", "end0", "clusters", "hits", "complete_8_clusters"])
    write_csv(out_dir / "cluster_v02_motif_strand.csv", motif_rows, ["motif", "strand", "hits"])

    top_windows = sorted(window_rows, key=lambda x: int(x["hits"]), reverse=True)[:10]
    core_windows = [r for r in window_rows if 21 <= int(r["window"]) <= 25]
    core_hits = sum(int(r["hits"]) for r in core_windows)
    core_clusters = sum(int(r["clusters"]) for r in core_windows)
    complete8 = sum(int(r["complete_8_clusters"]) for r in core_windows)
    plus_hits = sum(c for (m, s), c in motif_counter.items() if s == "+")
    minus_hits = sum(c for (m, s), c in motif_counter.items() if s == "-")

    manifest = {
        "version": "KALYX_SIGNATURE_CLUSTER_ANALYZER_V0_2",
        "boundary": "syntax/coordinate/periodicity analysis only; no origin claim",
        "input_hits": str(hits_path),
        "cluster_gap": args.cluster_gap,
        "window_size": args.window_size,
        "total_hits": len(hits),
        "total_clusters": len(clusters),
        "total_templates": len(tpl_counter),
        "plus_hits": plus_hits,
        "minus_hits": minus_hits,
        "core_windows_21_25_hits": core_hits,
        "core_windows_21_25_clusters": core_clusters,
        "core_windows_21_25_complete8_clusters": complete8,
        "top_delta_counts": delta_rows[:20],
        "top_templates": template_rows[:10],
    }
    (out_dir / "cluster_v02_manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")

    report = []
    report.append("# KALYX Signature Cluster Analyzer v0.2")
    report.append("")
    report.append("## Boundary")
    report.append("Dieses Artefakt analysiert Motiv-Cluster, Offset-Schablonen und Cluster-Periodik. Es beweist keinen natürlichen oder künstlichen Ursprung.")
    report.append("")
    report.append("## Summary")
    report.append(f"- input hits: `{len(hits)}`")
    report.append(f"- clusters: `{len(clusters)}` using cluster_gap={args.cluster_gap}")
    report.append(f"- templates: `{len(tpl_counter)}`")
    report.append(f"- strand balance: plus={plus_hits} minus={minus_hits} plus_rate={plus_hits / max(1, len(hits)):.12f}")
    report.append(f"- core windows 21..25: hits={core_hits} clusters={core_clusters} complete_8_clusters={complete8}")
    report.append("")
    report.append("## Top windows")
    report.append("")
    report.append("| window | start0 | end0 | hits | clusters | complete_8 |")
    report.append("|---:|---:|---:|---:|---:|---:|")
    for r in top_windows:
        report.append(f"| {r['window']} | {r['start0']} | {r['end0']} | {r['hits']} | {r['clusters']} | {r['complete_8_clusters']} |")
    report.append("")
    report.append("## Top cluster-start deltas")
    report.append("")
    report.append("| delta | count |")
    report.append("|---:|---:|")
    for r in delta_rows[:25]:
        report.append(f"| {r['delta']} | {r['count']} |")
    report.append("")
    report.append("## Top templates")
    report.append("")
    report.append("| rank | count | n_hits | n_motifs | strand | offsets | motifs_by_offset |")
    report.append("|---:|---:|---:|---:|---|---|---|")
    for r in template_rows[:10]:
        report.append(f"| {r['template_rank']} | {r['count']} | {r['n_hits']} | {r['n_motifs']} | {r['strand_set']} | `{r['offsets']}` | `{r['motifs_by_offset']}` |")
    report.append("")
    report.append("## Interpretation guardrail")
    report.append("Der stärkste Befund ist nicht Bedeutungs-Semantik, sondern eine wiederkehrende Offset-Schablone. Das ist ein Kandidat für ein lokales Repeat-/Regulations-/Strukturmotiv und muss gegen RepeatMasker, GC-kontrollierte Shuffles und andere Chromosomen geprüft werden.")
    (out_dir / "cluster_v02_report.md").write_text("\n".join(report) + "\n", encoding="utf-8")

    print(f"KALYX signature cluster analyzer complete: {out_dir}")
    print(f"  hits={len(hits)} clusters={len(clusters)} templates={len(tpl_counter)}")
    print(f"  report={out_dir / 'cluster_v02_report.md'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
