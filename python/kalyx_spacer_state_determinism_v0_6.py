#!/usr/bin/env python3
"""
KALYX Spacer-State Determinism / Null Model Decoder v0.6

Input:
  Decode_chr17_v05_real/
    cluster_v05_block_states.csv
    cluster_v05_transitions_raw.csv
    cluster_v05_transitions.csv

Purpose:
  Test whether the spacer-state transition system is more deterministic than
  a simple state-frequency / shuffled-order null model.

Boundary:
  This is an internal null-model and determinism test. It does not prove
  natural or artificial origin. It upgrades the claim from "ordered-looking"
  to "quantified against a defined null".
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
from typing import Iterable


def read_csv(path: Path) -> list[dict[str, str]]:
    if not path.exists():
        raise FileNotFoundError(f"Missing input CSV: {path}")
    with path.open("rt", encoding="utf-8-sig", newline="") as f:
        return list(csv.DictReader(f))


def write_csv(path: Path, fieldnames: list[str], rows: Iterable[dict[str, object]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("wt", encoding="utf-8", newline="") as f:
        w = csv.DictWriter(f, fieldnames=fieldnames)
        w.writeheader()
        for row in rows:
            w.writerow(row)


def f12(x: float) -> str:
    if math.isnan(x) or math.isinf(x):
        return ""
    return f"{x:.12f}"


def entropy(counter: Counter[str]) -> float:
    total = sum(counter.values())
    if total <= 0:
        return 0.0
    out = 0.0
    for c in counter.values():
        p = c / total
        out -= p * math.log2(p)
    return out


def group_transition_counts(raw: list[dict[str, str]], to_override: list[str] | None = None) -> dict[tuple[str, str], Counter[str]]:
    groups: dict[tuple[str, str], Counter[str]] = defaultdict(Counter)
    for i, r in enumerate(raw):
        from_state = r["from_state_id"]
        delta_family = r["delta_family"]
        to_state = to_override[i] if to_override is not None else r["to_state_id"]
        groups[(from_state, delta_family)][to_state] += 1
    return groups


def determinism_metrics(raw: list[dict[str, str]], to_override: list[str] | None = None) -> dict[str, float]:
    groups = group_transition_counts(raw, to_override)
    total = len(raw)
    if total == 0:
        return {
            "transitions": 0,
            "groups": 0,
            "weighted_top1_accuracy": 0.0,
            "mean_group_top1_accuracy": 0.0,
            "mean_group_entropy_bits": 0.0,
            "top_transition_count": 0,
            "top_transition_rate": 0.0,
            "mutual_information_to_from_delta_bits": 0.0,
        }

    top_hits = 0
    group_top_rates = []
    group_entropies = []
    transition_counts = Counter()
    for (from_state, delta_family), counter in groups.items():
        n = sum(counter.values())
        top = counter.most_common(1)[0][1]
        top_hits += top
        group_top_rates.append(top / n)
        group_entropies.append(entropy(counter))
        for to_state, c in counter.items():
            transition_counts[(from_state, delta_family, to_state)] += c

    to_counts = Counter()
    joint_counts = Counter()
    group_counts = Counter()
    for i, r in enumerate(raw):
        to_state = to_override[i] if to_override is not None else r["to_state_id"]
        g = (r["from_state_id"], r["delta_family"])
        to_counts[to_state] += 1
        group_counts[g] += 1
        joint_counts[(g, to_state)] += 1

    # I(To ; Group) = sum p(g,to) log2 p(g,to)/(p(g)p(to))
    mi = 0.0
    for (g, to_state), c in joint_counts.items():
        p_joint = c / total
        p_g = group_counts[g] / total
        p_to = to_counts[to_state] / total
        if p_joint > 0 and p_g > 0 and p_to > 0:
            mi += p_joint * math.log2(p_joint / (p_g * p_to))

    top_transition_count = transition_counts.most_common(1)[0][1] if transition_counts else 0
    return {
        "transitions": total,
        "groups": len(groups),
        "weighted_top1_accuracy": top_hits / total,
        "mean_group_top1_accuracy": statistics.fmean(group_top_rates) if group_top_rates else 0.0,
        "mean_group_entropy_bits": statistics.fmean(group_entropies) if group_entropies else 0.0,
        "top_transition_count": float(top_transition_count),
        "top_transition_rate": top_transition_count / total,
        "mutual_information_to_from_delta_bits": mi,
    }


def max_run_length(states: list[str]) -> int:
    if not states:
        return 0
    best = 1
    cur = 1
    for a, b in zip(states, states[1:]):
        if a == b:
            cur += 1
            best = max(best, cur)
        else:
            cur = 1
    return best


def ngram_counts(states: list[str], n: int) -> Counter[tuple[str, ...]]:
    c = Counter()
    if n <= 0 or len(states) < n:
        return c
    for i in range(len(states) - n + 1):
        c[tuple(states[i:i+n])] += 1
    return c


def null_stats(values: list[float], observed: float, greater_equal: bool = True) -> dict[str, float]:
    if not values:
        return {"mean": 0.0, "sd": 0.0, "z": 0.0, "p_empirical": 1.0}
    mean = statistics.fmean(values)
    sd = statistics.pstdev(values) if len(values) > 1 else 0.0
    z = (observed - mean) / sd if sd > 0 else 0.0
    if greater_equal:
        extreme = sum(1 for v in values if v >= observed)
    else:
        extreme = sum(1 for v in values if v <= observed)
    p = (extreme + 1) / (len(values) + 1)
    return {"mean": mean, "sd": sd, "z": z, "p_empirical": p}


def build_determinism_rows(raw: list[dict[str, str]], min_count: int) -> list[dict[str, object]]:
    groups = group_transition_counts(raw)
    rows = []
    rank = 1
    for (from_state, delta_family), counter in sorted(groups.items(), key=lambda kv: sum(kv[1].values()), reverse=True):
        n = sum(counter.values())
        if n < min_count:
            continue
        top_to, top_count = counter.most_common(1)[0]
        rows.append({
            "rank": rank,
            "from_state_id": from_state,
            "delta_family": delta_family,
            "n": n,
            "top_to_state_id": top_to,
            "top_count": top_count,
            "top_rate": f12(top_count / n),
            "entropy_bits": f12(entropy(counter)),
            "alternatives": len(counter),
            "to_distribution": "|".join(f"{k}:{v}" for k, v in counter.most_common(12)),
        })
        rank += 1
    return rows


def enrichment_rows(raw: list[dict[str, str]], min_count: int) -> list[dict[str, object]]:
    total = len(raw)
    to_counts = Counter(r["to_state_id"] for r in raw)
    trans_counts = Counter((r["from_state_id"], r["delta_family"], r["to_state_id"]) for r in raw)
    group_counts = Counter((r["from_state_id"], r["delta_family"]) for r in raw)
    rows = []
    for rank, ((from_state, delta_family, to_state), obs) in enumerate(trans_counts.most_common(), 1):
        if obs < min_count:
            continue
        expected = group_counts[(from_state, delta_family)] * (to_counts[to_state] / total) if total else 0.0
        enrichment = obs / expected if expected > 0 else 0.0
        rows.append({
            "rank": rank,
            "from_state_id": from_state,
            "delta_family": delta_family,
            "to_state_id": to_state,
            "observed": obs,
            "group_n": group_counts[(from_state, delta_family)],
            "to_state_global_n": to_counts[to_state],
            "expected_independent": f12(expected),
            "enrichment": f12(enrichment),
            "observed_minus_expected": f12(obs - expected),
        })
    return rows


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--state-dir", required=True, help="Directory containing v0.5 outputs")
    ap.add_argument("--out-dir", required=True)
    ap.add_argument("--iterations", type=int, default=1000)
    ap.add_argument("--seed", type=int, default=0x4B414C5958563036)
    ap.add_argument("--min-group-count", type=int, default=10)
    ap.add_argument("--ngram", type=int, default=3)
    args = ap.parse_args()

    state_dir = Path(args.state_dir)
    out_dir = Path(args.out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    raw = read_csv(state_dir / "cluster_v05_transitions_raw.csv")
    block_states = read_csv(state_dir / "cluster_v05_block_states.csv")

    states_seq = [r["state_id"] for r in block_states]
    to_states = [r["to_state_id"] for r in raw]
    observed = determinism_metrics(raw)
    observed_run = max_run_length(states_seq)
    ngrams = ngram_counts(states_seq, args.ngram)
    observed_top_ngram = ngrams.most_common(1)[0][1] if ngrams else 0

    rng = random.Random(args.seed)
    null_metrics = defaultdict(list)
    null_run = []
    null_ngram = []
    for _ in range(args.iterations):
        shuffled_to = to_states[:]
        rng.shuffle(shuffled_to)
        m = determinism_metrics(raw, to_override=shuffled_to)
        for k, v in m.items():
            if k not in ("transitions", "groups"):
                null_metrics[k].append(float(v))
        shuffled_states = states_seq[:]
        rng.shuffle(shuffled_states)
        null_run.append(float(max_run_length(shuffled_states)))
        nc = ngram_counts(shuffled_states, args.ngram)
        null_ngram.append(float(nc.most_common(1)[0][1] if nc else 0))

    summary_rows = []
    for metric, obs_value in observed.items():
        if metric in ("transitions", "groups"):
            summary_rows.append({
                "metric": metric,
                "observed": f12(float(obs_value)),
                "null_mean": "",
                "null_sd": "",
                "z": "",
                "p_empirical_ge": "",
                "iterations": args.iterations,
            })
        else:
            ns = null_stats(null_metrics[metric], float(obs_value), greater_equal=True)
            summary_rows.append({
                "metric": metric,
                "observed": f12(float(obs_value)),
                "null_mean": f12(ns["mean"]),
                "null_sd": f12(ns["sd"]),
                "z": f12(ns["z"]),
                "p_empirical_ge": f12(ns["p_empirical"]),
                "iterations": args.iterations,
            })

    ns_run = null_stats(null_run, float(observed_run), greater_equal=True)
    summary_rows.append({
        "metric": "max_same_state_run_length",
        "observed": f12(float(observed_run)),
        "null_mean": f12(ns_run["mean"]),
        "null_sd": f12(ns_run["sd"]),
        "z": f12(ns_run["z"]),
        "p_empirical_ge": f12(ns_run["p_empirical"]),
        "iterations": args.iterations,
    })
    ns_ng = null_stats(null_ngram, float(observed_top_ngram), greater_equal=True)
    summary_rows.append({
        "metric": f"top_state_ngram_n{args.ngram}_count",
        "observed": f12(float(observed_top_ngram)),
        "null_mean": f12(ns_ng["mean"]),
        "null_sd": f12(ns_ng["sd"]),
        "z": f12(ns_ng["z"]),
        "p_empirical_ge": f12(ns_ng["p_empirical"]),
        "iterations": args.iterations,
    })

    det_rows = build_determinism_rows(raw, args.min_group_count)
    enr_rows = enrichment_rows(raw, args.min_group_count)

    # State dominance / coverage.
    state_counts = Counter(states_seq)
    total_states = len(states_seq) or 1
    coverage_rows = []
    cumulative = 0
    for rank, (state, count) in enumerate(state_counts.most_common(), 1):
        cumulative += count
        coverage_rows.append({
            "rank": rank,
            "state_id": state,
            "count": count,
            "rate": f12(count / total_states),
            "cumulative_count": cumulative,
            "cumulative_rate": f12(cumulative / total_states),
        })

    write_csv(out_dir / "cluster_v06_null_summary.csv",
              ["metric","observed","null_mean","null_sd","z","p_empirical_ge","iterations"], summary_rows)
    write_csv(out_dir / "cluster_v06_group_determinism.csv",
              ["rank","from_state_id","delta_family","n","top_to_state_id","top_count","top_rate","entropy_bits","alternatives","to_distribution"], det_rows)
    write_csv(out_dir / "cluster_v06_transition_enrichment.csv",
              ["rank","from_state_id","delta_family","to_state_id","observed","group_n","to_state_global_n","expected_independent","enrichment","observed_minus_expected"], enr_rows)
    write_csv(out_dir / "cluster_v06_state_coverage.csv",
              ["rank","state_id","count","rate","cumulative_count","cumulative_rate"], coverage_rows)

    manifest = {
        "version": "KALYX_SPACER_STATE_DETERMINISM_V0_6",
        "boundary": "internal null-model determinism test only; no origin proof",
        "state_dir": str(state_dir.resolve()),
        "out_dir": str(out_dir.resolve()),
        "transitions": len(raw),
        "blocks": len(block_states),
        "states": len(state_counts),
        "iterations": args.iterations,
        "seed": args.seed,
        "ngram": args.ngram,
        "observed_metrics": observed,
        "observed_max_run": observed_run,
        "observed_top_ngram_count": observed_top_ngram,
    }
    (out_dir / "cluster_v06_manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")

    def row_for(metric: str) -> dict[str, str]:
        for r in summary_rows:
            if r["metric"] == metric:
                return r
        return {}

    top_metrics = [
        "weighted_top1_accuracy",
        "mean_group_top1_accuracy",
        "mean_group_entropy_bits",
        "top_transition_count",
        "mutual_information_to_from_delta_bits",
        "max_same_state_run_length",
        f"top_state_ngram_n{args.ngram}_count",
    ]

    report = [
        "# KALYX Spacer-State Determinism / Null Model Decoder v0.6",
        "",
        "## Boundary",
        "",
        "Dieses Artefakt testet die v0.5-Zustandsmaschine gegen interne Nullmodelle.",
        "Es beweist keinen natürlichen oder künstlichen Ursprung. Es quantifiziert nur,",
        "ob die Übergänge stärker geordnet sind als eine zustandshäufigkeits-erhaltende",
        "Shuffle-Baseline.",
        "",
        "## Summary",
        "",
        f"- transitions: `{len(raw)}`",
        f"- blocks: `{len(block_states)}`",
        f"- states: `{len(state_counts)}`",
        f"- null iterations: `{args.iterations}`",
        f"- ngram: `{args.ngram}`",
        "",
        "## Null-model metrics",
        "",
        "| metric | observed | null_mean | null_sd | z | p_empirical_ge |",
        "|---|---:|---:|---:|---:|---:|",
    ]
    for metric in top_metrics:
        r = row_for(metric)
        if r:
            report.append(f"| `{metric}` | {r['observed']} | {r['null_mean']} | {r['null_sd']} | {r['z']} | {r['p_empirical_ge']} |")

    report += [
        "",
        "## Top group determinism",
        "",
        "| rank | from | delta_family | n | top_to | top_count | top_rate | alternatives |",
        "|---:|---|---|---:|---|---:|---:|---:|",
    ]
    for r in det_rows[:30]:
        report.append(f"| {r['rank']} | `{r['from_state_id']}` | `{r['delta_family']}` | {r['n']} | `{r['top_to_state_id']}` | {r['top_count']} | {r['top_rate']} | {r['alternatives']} |")

    report += [
        "",
        "## Top transition enrichment",
        "",
        "| rank | from | delta | to | observed | expected | enrichment |",
        "|---:|---|---|---|---:|---:|---:|",
    ]
    for r in enr_rows[:30]:
        report.append(f"| {r['rank']} | `{r['from_state_id']}` | `{r['delta_family']}` | `{r['to_state_id']}` | {r['observed']} | {r['expected_independent']} | {r['enrichment']} |")

    report += [
        "",
        "## Output files",
        "",
        "```text",
        "cluster_v06_null_summary.csv",
        "cluster_v06_group_determinism.csv",
        "cluster_v06_transition_enrichment.csv",
        "cluster_v06_state_coverage.csv",
        "cluster_v06_manifest.json",
        "cluster_v06_report.md",
        "```",
        "",
        "## Interpretation guardrail",
        "",
        "Wenn die beobachteten Determinismusmetriken weit außerhalb der Shuffle-Null liegen,",
        "ist die Spacer-State-Folge intern nicht als einfache zufällige Anordnung der",
        "beobachteten Zustände erklärbar. Ursprungsaussagen bleiben trotzdem externen",
        "Kontrollen vorbehalten: RepeatMasker, Segmental-Duplication-Kontext, GC-Shuffles,",
        "Cross-Chromosom-Scans und Build-/Populationsreplikation.",
        "",
    ]
    (out_dir / "cluster_v06_report.md").write_text("\n".join(report), encoding="utf-8")

    print(f"KALYX spacer-state determinism v0.6 complete: {out_dir}")
    print(f"  transitions={len(raw)} states={len(state_counts)} iterations={args.iterations}")
    print(f"  report={out_dir / 'cluster_v06_report.md'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
