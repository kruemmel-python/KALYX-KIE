#!/usr/bin/env python3
"""
KALYX UCSC Control BED Builder v0.8.1

Creates the external control BED files required by KALYX Natural Model Breaker v0.8:

  - repeatmasker_chr17.bed          from UCSC hg38 database/rmsk.txt.gz
  - segmental_dup_chr17.bed         from UCSC hg38 database/genomicSuperDups.txt.gz

It can also keep all chromosomes if --chrom all is used.

Scientific boundary:
  This script does not decide whether the KALYX signal is natural or artificial.
  It only materializes external annotation controls so v0.8 can test overlap gates.

Data sources:
  UCSC hg38 database dump directory:
    https://hgdownload.soe.ucsc.edu/goldenPath/hg38/database/
  Tables:
    rmsk.txt.gz
    genomicSuperDups.txt.gz
"""
from __future__ import annotations

import argparse
import csv
import gzip
import json
import shutil
import sys
import urllib.request
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Iterator


UCSC_HG38_DB = "https://hgdownload.soe.ucsc.edu/goldenPath/hg38/database"
RMSK_URL = f"{UCSC_HG38_DB}/rmsk.txt.gz"
SUPERDUPS_URL = f"{UCSC_HG38_DB}/genomicSuperDups.txt.gz"


@dataclass(frozen=True)
class BuildStats:
    table: str
    source: str
    output: str
    chrom_filter: str
    rows_read: int
    rows_written: int


def download(url: str, out: Path, force: bool = False) -> None:
    out.parent.mkdir(parents=True, exist_ok=True)
    if out.exists() and not force:
        print(f"[keep] {out}")
        return
    tmp = out.with_suffix(out.suffix + ".tmp")
    print(f"[download] {url}")
    with urllib.request.urlopen(url, timeout=120) as r, tmp.open("wb") as f:
        shutil.copyfileobj(r, f)
    tmp.replace(out)
    print(f"[ok] {out}")


def open_text_maybe_gzip(path: Path):
    if path.suffix == ".gz":
        return gzip.open(path, "rt", encoding="utf-8", errors="replace", newline="")
    return path.open("rt", encoding="utf-8", errors="replace", newline="")


def chrom_accept(chrom: str, wanted: str) -> bool:
    if wanted.lower() in ("all", "*"):
        return chrom.startswith("chr") and "_" not in chrom
    return chrom == wanted


def write_bed(path: Path, rows: Iterable[list[str]]) -> int:
    path.parent.mkdir(parents=True, exist_ok=True)
    count = 0
    with path.open("wt", encoding="utf-8", newline="") as f:
        w = csv.writer(f, delimiter="\t", lineterminator="\n")
        for row in rows:
            w.writerow(row)
            count += 1
    return count


def iter_rmsk_bed(source: Path, chrom: str) -> Iterator[list[str]]:
    """UCSC rmsk.txt.gz columns, hg38:

    0 bin
    1 swScore
    2 milliDiv
    3 milliDel
    4 milliIns
    5 genoName
    6 genoStart
    7 genoEnd
    8 genoLeft
    9 strand
    10 repName
    11 repClass
    12 repFamily
    13 repStart
    14 repEnd
    15 repLeft
    16 id
    """
    with open_text_maybe_gzip(source) as f:
        for line in f:
            if not line.strip() or line.startswith("#"):
                continue
            fields = line.rstrip("\n").split("\t")
            if len(fields) < 13:
                continue
            geno = fields[5]
            if not chrom_accept(geno, chrom):
                continue
            start, end = fields[6], fields[7]
            rep_name = fields[10]
            rep_class = fields[11]
            rep_family = fields[12]
            strand = fields[9]
            score = fields[1]
            # BED6 + extra annotations
            name = f"{rep_name}|{rep_class}|{rep_family}"
            yield [geno, start, end, name, score, strand, rep_class, rep_family, rep_name]


def count_rows(source: Path) -> int:
    n = 0
    with open_text_maybe_gzip(source) as f:
        for line in f:
            if line.strip() and not line.startswith("#"):
                n += 1
    return n


def iter_superdups_bed(source: Path, chrom: str) -> Iterator[list[str]]:
    """UCSC genomicSuperDups.txt.gz columns, hg38:

    0 bin
    1 chrom
    2 chromStart
    3 chromEnd
    4 name
    5 score
    6 strand
    7 otherChrom
    8 otherStart
    9 otherEnd
    10 otherSize
    11 uid
    12 posBasesHit
    13 testResult
    14 verdict
    15 chits
    16 ccov
    17 alignfile
    18 alignL
    19 indelN
    20 indelS
    21 alignB
    22 matchB
    23 mismatchB
    24 transitionsB
    25 transversionsB
    26 fracMatch
    27 fracMatchIndel
    28 jcK
    29 k2K
    """
    with open_text_maybe_gzip(source) as f:
        for line in f:
            if not line.strip() or line.startswith("#"):
                continue
            fields = line.rstrip("\n").split("\t")
            if len(fields) < 10:
                continue
            c = fields[1]
            if not chrom_accept(c, chrom):
                continue
            start, end = fields[2], fields[3]
            name = fields[4] if fields[4] else f"{c}:{start}-{end}"
            score = fields[5] if len(fields) > 5 else "0"
            strand = fields[6] if len(fields) > 6 else "."
            other = f"{fields[7]}:{fields[8]}-{fields[9]}" if len(fields) > 9 else ""
            frac = fields[26] if len(fields) > 26 else ""
            yield [c, start, end, name, score, strand, other, frac]


def build(args: argparse.Namespace) -> int:
    out_dir = Path(args.out_dir)
    cache_dir = Path(args.cache_dir) if args.cache_dir else out_dir / "_ucsc_cache"
    out_dir.mkdir(parents=True, exist_ok=True)
    cache_dir.mkdir(parents=True, exist_ok=True)

    rmsk_src = Path(args.rmsk_txt_gz) if args.rmsk_txt_gz else cache_dir / "rmsk.txt.gz"
    super_src = Path(args.superdups_txt_gz) if args.superdups_txt_gz else cache_dir / "genomicSuperDups.txt.gz"

    if args.download:
        download(RMSK_URL, rmsk_src, force=args.force)
        download(SUPERDUPS_URL, super_src, force=args.force)

    if not rmsk_src.exists():
        raise FileNotFoundError(f"rmsk source missing: {rmsk_src}. Use --download or --rmsk-txt-gz.")
    if not super_src.exists():
        raise FileNotFoundError(f"genomicSuperDups source missing: {super_src}. Use --download or --superdups-txt-gz.")

    chrom = args.chrom
    suffix = chrom if chrom.lower() != "all" else "all"
    rmsk_bed = out_dir / f"repeatmasker_{suffix}.bed"
    super_bed = out_dir / f"segmental_dup_{suffix}.bed"

    rmsk_read = count_rows(rmsk_src)
    rmsk_written = write_bed(rmsk_bed, iter_rmsk_bed(rmsk_src, chrom))
    super_read = count_rows(super_src)
    super_written = write_bed(super_bed, iter_superdups_bed(super_src, chrom))

    stats = [
        BuildStats("rmsk", str(rmsk_src), str(rmsk_bed), chrom, rmsk_read, rmsk_written),
        BuildStats("genomicSuperDups", str(super_src), str(super_bed), chrom, super_read, super_written),
    ]

    manifest = {
        "tool": "KALYX UCSC Control BED Builder",
        "version": "0.8.1",
        "assembly": "hg38",
        "chrom_filter": chrom,
        "ucsc_database": UCSC_HG38_DB,
        "sources": {
            "rmsk": RMSK_URL,
            "genomicSuperDups": SUPERDUPS_URL,
        },
        "outputs": [s.__dict__ for s in stats],
        "bed_schemas": {
            "repeatmasker": "chrom,start,end,name,score,strand,repClass,repFamily,repName",
            "segmental_dup": "chrom,start,end,name,score,strand,other_locus,fracMatch",
        },
    }
    (out_dir / "ucsc_control_beds_manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")

    report = [
        "# KALYX UCSC Control BED Builder v0.8.1",
        "",
        "## Boundary",
        "",
        "Dieses Artefakt erstellt externe Kontroll-BEDs für v0.8. Es bewertet den Befund nicht selbst.",
        "",
        "## Outputs",
        "",
        "| table | source | output | rows_read | rows_written |",
        "|---|---|---|---:|---:|",
    ]
    for s in stats:
        report.append(f"| {s.table} | `{s.source}` | `{s.output}` | {s.rows_read} | {s.rows_written} |")
    report += [
        "",
        "## v0.8 usage",
        "",
        "```powershell",
        f".\\scripts\\kalyx_natural_model_breaker_v0_8.ps1 `",
        f"  -SequenceDir .\\Decode_chr17_v04_real `",
        f"  -StateDir .\\Decode_chr17_v05_real `",
        f"  -DeterminismDir .\\Decode_chr17_v06_real `",
        f"  -EvidenceDir .\\Decode_chr17_v07_real `",
        f"  -Fasta .\\chr17.fa `",
        f"  -ChromDir .\\Genome `",
        f"  -RepeatMaskerBed {rmsk_bed} `",
        f"  -SegmentalDupBed {super_bed} `",
        f"  -OutDir .\\Decode_chr17_v08_final `",
        f"  -Iterations 250",
        "```",
        "",
    ]
    (out_dir / "KALYX_UCSC_CONTROL_BEDS_V0_8_1_REPORT.md").write_text("\n".join(report), encoding="utf-8")

    print("KALYX UCSC control BED builder complete")
    for s in stats:
        print(f"  {s.table}: rows_read={s.rows_read} rows_written={s.rows_written} output={s.output}")
    print(f"  manifest={out_dir / 'ucsc_control_beds_manifest.json'}")
    return 0


def main() -> int:
    p = argparse.ArgumentParser(description="Build RepeatMasker and SegmentalDup BED controls for KALYX v0.8.")
    p.add_argument("--out-dir", default="Controls_UCSC_hg38", help="Output directory for BEDs and report")
    p.add_argument("--cache-dir", default="", help="Cache directory for UCSC .txt.gz files")
    p.add_argument("--chrom", default="chr17", help="Chromosome filter, e.g. chr17 or all")
    p.add_argument("--download", action="store_true", help="Download UCSC rmsk.txt.gz and genomicSuperDups.txt.gz")
    p.add_argument("--force", action="store_true", help="Re-download if cached files already exist")
    p.add_argument("--rmsk-txt-gz", default="", help="Existing local rmsk.txt.gz or rmsk.txt")
    p.add_argument("--superdups-txt-gz", default="", help="Existing local genomicSuperDups.txt.gz or .txt")
    return build(p.parse_args())


if __name__ == "__main__":
    raise SystemExit(main())
