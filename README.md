# KALYX

**KALYX** is a low-level KDNA/KFIELD research substrate for turning ordered data into testable transition grammars.  
It was created by **Ralf Krümmel** and is released under the **Apache License 2.0**.

KALYX is not a CRUD framework, not a machine-learning wrapper, and not a visualization demo. It is a deterministic data-substrate system: raw ordered signals are converted into compact symbol streams, projected into a KDNA representation, induced into KGRAM transition grammars, and evaluated through self/cross matrices and null models.

The central operational idea is simple:

```text
ordered source field
→ canonical symbols
→ KDNA projection
→ KGRAM transition grammar
→ self/cross KFIELD matrix
→ null models
→ report / library artifact
```

KALYX currently contains two major validated application tracks:

1. **Genome substrate analysis**: full chromosome FASTA streams are transformed into k-mer symbol streams, KDNA streams, chromosome self-grammars, genome-wide self/cross matrices, and KGLIB enterprise libraries.
2. **Language substrate analysis**: Universal Dependencies CoNLL-U corpora are transformed into linguistic symbol streams, KDNA streams, language self-grammars, self/cross language matrices, true null models, and KLLIB language libraries.

The repository still uses the historical executable prefix `kdna_*`. The system name is **KALYX**.

---

## Why KALYX Exists

Most analytical pipelines collapse ordered structure into aggregate features too early. KALYX keeps order as a first-class substrate.

For genomes, that means it does not only count k-mers. It builds transition grammars over projected k-mer streams and asks whether one chromosome grammar explains another chromosome stream.

For languages, it does not only count parts of speech. It builds transition grammars over UPOS/DEPREL-derived symbol streams and asks whether one language grammar explains another language stream better than shuffled, rotated, or block-boundary null models.

KALYX is designed around:

- fixed binary ABIs,
- explicit memory layouts,
- reproducible stream artifacts,
- CPU reference paths,
- optional OpenCL acceleration,
- self/cross differential measurement,
- true null models,
- auditable library files.

---

## Core Pipeline

```text
                ┌────────────────────────┐
                │ source data             │
                │ FASTA / CoNLL-U / CSV   │
                └───────────┬────────────┘
                            │
                            ▼
                ┌────────────────────────┐
                │ KSTREAM / raw symbols   │
                │ uint64[n] + summary     │
                └───────────┬────────────┘
                            │
                            ▼
                ┌────────────────────────┐
                │ KDNA projection         │
                │ affine KDNA symbol map  │
                └───────────┬────────────┘
                            │
                            ▼
                ┌────────────────────────┐
                │ KGRAM01                 │
                │ transition grammar      │
                └───────────┬────────────┘
                            │
                            ▼
                ┌────────────────────────┐
                │ KFIELD / KGENOME matrix │
                │ self + cross coupling   │
                └───────────┬────────────┘
                            │
                            ▼
                ┌────────────────────────┐
                │ null models + reports   │
                │ libraries: KGLIB/KLLIB  │
                └────────────────────────┘
```

---

## Repository Layout

```text
include/        Stable C headers and binary ABI definitions
src/            KDNA core implementation
kernels/        OpenCL kernels
tools/          Command-line tools
tests/          CTest test programs
scripts/        PowerShell orchestration scripts
python/         Python helper bindings
```

Important binary interfaces:

```text
KGRAM01   fixed-size transition grammar
KGENOME1  self/cross matrix record format
KGLIB001  genome library artifact
KSTREAM1  generic stream summary header
KLLIB001  language library artifact
```

---

## Build Requirements

### Windows

Recommended environment:

- Windows 10/11
- Visual Studio 2022 Build Tools
- CMake 3.20+
- PowerShell 5.1 or newer
- OpenCL runtime and device driver, optional

```powershell
cd H:\path\to\KALYX

$CMAKE_EXE = "C:\Program Files\CMake\bin\cmake.exe"
$CTEST_EXE = "C:\Program Files\CMake\bin\ctest.exe"

& $CMAKE_EXE -S . -B build_vs `
  -G "Visual Studio 17 2022" `
  -A x64

& $CMAKE_EXE --build build_vs --config Release --parallel

& $CTEST_EXE --test-dir build_vs -C Release --output-on-failure
```

### Linux / Unix-like systems

The core is C11 and CMake based. The PowerShell orchestration scripts are Windows-oriented, but the C tools can be built with a standard CMake toolchain.

```bash
cmake -S . -B build
cmake --build build --config Release -j
ctest --test-dir build --output-on-failure
```

---

## Main Executables

### Core KDNA

| Tool | Purpose |
|---|---|
| `kdna_project` | Projects a `uint64` symbol stream into a KDNA symbol stream. |
| `kdna_probe` | Inspects binary artifacts and prints magic/version/hash metadata. |
| `kdna_symbol_gram` | Builds a KGRAM01 transition grammar from a KDNA stream. |
| `kdna_genome_matrix` | Builds a self/cross KFIELD/KGENOME matrix from streams and grammars. |
| `kdna_field_null_matrix` | Builds true null-model matrices for KFIELD/KLANG experiments. |

### KALYX Interaction Substrate

The repository now also contains the KALYX Interaction Envelope layer described by `Anweisung_Bauplan.md`. This layer is a deterministic authority boundary for LLM interaction:

```text
application state
-> .kie.md interaction envelope
-> LLM or offline transport
-> .kresp.md response
-> deterministic validation
-> .kaudit.json audit ledger
```

Core properties:

- fixed C ABI headers in `include/kalyx_*.h`
- deterministic Markdown envelope emitter for `.kie.md`
- controlled Markdown/JSON response scanner for `.kresp.md`
- explicit response schema, risk, confirmation and context-use checks
- allowed-action and forbidden-action validation
- SHA-256 envelope and response hashing
- provider-neutral audit ledger generation
- CLI tools and Python transport bridge
- CTest regression coverage for determinism, rejection, allowed actions, forbidden actions, audits and domains

Build and test with the project-standard command:

```powershell
$CMAKE_EXE = "C:\Program Files\CMake\bin\cmake.exe"; $CTEST_EXE = "C:\Program Files\CMake\bin\ctest.exe"; & $CMAKE_EXE -S . -B build_vs -G "Visual Studio 17 2022" -A x64; & $CMAKE_EXE --build build_vs --config Release --parallel; & $CTEST_EXE --test-dir build_vs -C Release --output-on-failure
```

Generate and validate an app envelope:

```powershell
.\build_vs\Release\kalyx_make_envelope.exe --domain app --intent export_document --state .\examples\app_state.json --request .\examples\app_request.txt --out .\out\app.kie.md
.\build_vs\Release\kalyx_validate_response.exe --envelope .\out\app.kie.md --response .\examples\app_response_valid.kresp.md --audit .\out\app.kaudit.json
.\build_vs\Release\kalyx_audit_print.exe .\out\app.kaudit.json
```

Use the provider-neutral Python bridge in offline mode:

```powershell
python .\python\kalyx_llm_bridge.py --envelope .\out\app.kie.md --response .\out\bridge.kresp.md --audit .\out\bridge.kaudit.json --validator .\build_vs\Release\kalyx_validate_response.exe
```

### Genome Track

| Tool | Purpose |
|---|---|
| `kdna_fasta_symbols` | Converts FASTA sequences into k-mer `uint64` streams. |
| `kdna_genome_matrix` | Computes genome self/cross grammar matrices. |
| `kdna_genome_library` | Builds KGLIB001 enterprise genome libraries from existing artifacts. |

### KSTREAM / KLANG Track

| Tool | Purpose |
|---|---|
| `kdna_stream_bytes` | Converts byte streams into KSTREAM-compatible symbols. |
| `kdna_stream_tokens` | Converts token streams into KSTREAM-compatible symbols. |
| `kdna_stream_csv` | Converts CSV columns into KSTREAM-compatible symbols. |
| `kdna_stream_conllu` | Converts Universal Dependencies CoNLL-U files into linguistic KSTREAM symbols. |
| `kdna_stream_probe` | Inspects KSTREAM summary headers. |
| `kdna_klang_library` | Builds KLLIB001 language libraries from KLANG suite artifacts. |

---

## Genome Analysis

### Objective

The genome track asks:

```text
Does a chromosome-specific KGRAM explain only its own KDNA stream,
or does it also explain other chromosome streams?
```

KALYX processes chromosomes as ordered fields. For the human genome reference runs, chromosomes `chr1` through `chr22`, `chrX`, and `chrY` were treated as independent sources.

### Genome Artifact Chain

```text
chrN.fa
→ chrN_k16.u64
→ chrN_k16.kfsum
→ chrN_k16_kdna.u64
→ chrN_k16_self.kgram
→ human_chr1_22_XY_full_k16.kgenome
→ human_chr1_22_XY_full_k16.csv
→ human_kdna_library.kglib
→ human_kdna_library.json
→ human_kdna_library.csv
```

### Genome Semantics

| Artifact | Meaning |
|---|---|
| `*_k16.u64` | Raw k=16 DNA k-mer stream. |
| `*.kfsum` | FASTA stream summary. |
| `*_kdna.u64` | KDNA-projected chromosome stream. |
| `*_self.kgram` | Self-induced chromosome grammar. |
| `*.kgenome` | Binary self/cross matrix. |
| `*.csv` | Matrix metrics for inspection/reporting. |
| `*.kglib` | Enterprise genome library index. |

### Genome Metrics

`kdna_genome_matrix` emits one row/column cell per stream/grammar pair.

Important metrics:

| Metric | Meaning |
|---|---|
| `baseline_accuracy` | Accuracy expected from raw variant frequency. |
| `kgram_accuracy` | Fraction of test transitions explained by the grammar. |
| `lift` | `kgram_accuracy - baseline_accuracy`. |
| `surprise_rate` | Fraction of transitions not explained by grammar. |
| `out_of_grammar` | Count of test transitions outside the grammar. |
| `grammar_edges` | Number of KGRAM rules in the column grammar. |

### What Was Achieved With Genomes

KALYX was used to build a full chromosome-level pipeline for the human genome:

- all autosomes and sex chromosomes were processed as separate streams,
- k=16 symbol streams were generated from FASTA,
- KDNA-projected streams were generated,
- self KGRAMs were built per chromosome,
- a genome-wide self/cross matrix was produced,
- an enterprise chromosome library layer was built to index existing artifacts without recomputation.

The important architectural outcome is that the genome is no longer just a set of sequence files. It becomes an auditable KFIELD library of projected streams, grammars, cross-relations, hashes, and matrix metrics.

---

## Language Analysis: KLANG

KLANG is the language application layer inside KALYX.

It maps Universal Dependencies CoNLL-U corpora into KSTREAM symbols and then uses the same KDNA/KGRAM/KFIELD path that the genome track uses.

### Language Pipeline

```text
*.conllu
→ kdna_stream_conllu
→ *_upos_wN.u64
→ *_upos_wN.kstream
→ kdna_project
→ *_upos_wN_kdna.u64
→ kdna_symbol_gram
→ *_upos_wN_self.kgram
→ kdna_genome_matrix / KFIELD
→ kdna_field_null_matrix
→ KLANG report
→ KLLIB001
```

### Supported CoNLL-U Fields

`kdna_stream_conllu` supports:

```text
upos
form
lemma
feats
deprel
upos_feats
upos_deprel
lemma_upos
```

Important options:

```powershell
--field upos
--window 2
--stride 1
--sentence-boundary reset
--symbol-bits 32
```

### Why `--symbol-bits 32` Matters

Language symbols are hashed. A direct 64-bit symbol space can be poorly matched to the current KDNA affine projection configuration. KLANG therefore supports folded 32-bit symbols:

```text
linguistic key
→ FNV1a64
→ fold32
→ uint64 symbol compatible with source:[0, 2^32-1]
```

This avoids degenerate grammars and produces real transition structures.

Example validated KGRAM rule counts:

```text
DE UPOS w2: 3039 rules
EN UPOS w2: 4066 rules
AR UPOS w2: 2556 rules
```

---

## Language Reference Run: German ↔ English

### Data

The validated language run used:

```text
de_gsd-ud-train.conllu
en_ewt-ud-train.conllu
```

These are not parallel translation corpora. That is important: the experiment tests whether structural transition grammar transfers across independent German and English corpora, not whether translated content aligns.

### UPOS w2 Matrix

```text
DE → DE  accuracy: 0.9966128365
DE → EN  accuracy: 0.9808771953
EN → DE  accuracy: 0.9381357403
EN → EN  accuracy: 0.9895330672
```

With true null models:

```text
DE → EN observed_minus_null: 0.6898256189
EN → DE observed_minus_null: 0.7523635943
```

Interpretation:

```text
German and English share a strong ordered UPOS-w2 transition structure
in the KDNA/KGRAM space. The effect remains far above symbol_shuffle,
rotation, and block_boundary null models.
```

### Multi-Resolution KLANG Suite

The suite was run over:

```text
upos_w1
upos_w2
upos_w3
upos_deprel_w2
```

Observed behavior:

| Resolution | Interpretation |
|---|---|
| `upos_w1` | Very general POS stream; high accuracy but weak delta because nulls also perform strongly. |
| `upos_w2` | Strong local POS transition signal. |
| `upos_w3` | Harder local syntax; cross accuracy drops but remains far above null. |
| `upos_deprel_w2` | POS plus dependency role; strongest structural specificity. |

Representative observed-minus-null values:

| Config | Direction | observed_minus_null |
|---|---:|---:|
| `upos_w2` | DE → EN | 0.6898256189 |
| `upos_w2` | EN → DE | 0.7523635943 |
| `upos_w3` | DE → EN | 0.7928200061 |
| `upos_w3` | EN → DE | 0.6615928745 |
| `upos_deprel_w2` | DE → EN | 0.6033018176 |
| `upos_deprel_w2` | EN → DE | 0.5471156628 |

### KLLIB001 Language Library

The KLANG suite was compiled into a KLLIB001 language library:

```text
magic: KLLIB001
version: 1
configs: 4
cells: 16
artifact_bytes: 36,864,144
hash: 0x4f95d1382b6bdd9b

total_de_symbols: 999,856
total_en_symbols: 768,657
total_de_kgram_rules: 42,322
total_en_kgram_rules: 46,325

avg_self_acc:   0.961483684013
avg_cross_acc:  0.840148995040
avg_self_delta: 0.615501985309
avg_cross_delta:0.506771403716
```

This turns the language experiment into an auditable binary library, not just a report.

---

## Language Reference Run: German / English / Arabic Triad

### Data

The triad run used:

```text
de_gsd-ud-train.conllu
en_ewt-ud-train.conllu
ar_padt-ud-train.conllu
```

Configuration:

```text
field: upos
window: 2
symbol_bits: 32
train: 0.70
null modes: symbol_shuffle | rotation | block_boundary
sample_per_cell: 200000
```

### 3×3 Observed Matrix

```text
DE → DE  accuracy: 0.9966128365
DE → EN  accuracy: 0.9808771953
DE → AR  accuracy: 0.7141580765

EN → DE  accuracy: 0.9381357403
EN → EN  accuracy: 0.9895330672
EN → AR  accuracy: 0.7698663426

AR → DE  accuracy: 0.8686888783
AR → EN  accuracy: 0.8991750968
AR → AR  accuracy: 0.9948424420
```

### 3×3 Observed vs Null

| Direction | observed_accuracy | avg_null_accuracy | observed_minus_null |
|---|---:|---:|---:|
| DE → EN | 0.9808771953 | 0.2806055999 | 0.7002715954 |
| DE → AR | 0.7141580765 | 0.1190307912 | 0.5951272853 |
| EN → DE | 0.9381357403 | 0.1738934213 | 0.7642423190 |
| EN → AR | 0.7698663426 | 0.1215356130 | 0.6483307296 |
| AR → DE | 0.8686888783 | 0.2214076920 | 0.6472811864 |
| AR → EN | 0.8991750968 | 0.2511950128 | 0.6479800840 |

Interpretation:

```text
Arabic is clearly more distant from German/English in raw cross accuracy,
but it does not collapse to noise. All cross-language directions remain far
above true null models. The asymmetry suggests that KALYX measures directed
grammar compatibility rather than simple symmetric similarity.
```

---

## Null Models

KALYX uses true sequence-order controls, not only label sampling.

Implemented null modes:

| Null mode | Meaning |
|---|---|
| `symbol_shuffle` | Shuffles symbols while preserving gross symbol inventory. |
| `rotation` | Rotates sequence order to disrupt training/test alignment while retaining local continuity. |
| `block_boundary` | Tests block-level boundary disruption. |

A result is considered structurally interesting only when observed metrics remain above these null models.

---

## Methodological Boundaries

KALYX does **not** claim that it has proven biological causality or universal grammar.

What KALYX can claim from the current runs:

```text
It builds deterministic transition grammars from ordered substrates.
It measures whether a grammar induced from one source explains another source.
It compares observed coupling against null models.
It can preserve and audit results as binary library artifacts.
```

What KALYX does not yet claim:

```text
It does not infer biological function from chromosome coupling alone.
It does not prove linguistic universal grammar.
It does not replace controlled corpus design.
It does not remove the need for additional languages, domains, and replications.
```

---

## Reproducing the Language Suite

### DE/EN smoke run

```powershell
.\scripts\klang_2x2_experiment.ps1 `
  -DeConllu .\de_gsd-ud-train.conllu `
  -EnConllu .\en_ewt-ud-train.conllu `
  -OutDir .\LangOut_smoke `
  -Field upos `
  -Window 2 `
  -SymbolBits 32 `
  -Train 0.70 `
  -Backend cpu `
  -NullSamples 0 `
  -MaxAutoNullSamples 10000
```

### DE/EN full v1.2 run

```powershell
.\scripts\klang_full_v1_2.ps1 `
  -DeConllu .\de_gsd-ud-train.conllu `
  -EnConllu .\en_ewt-ud-train.conllu `
  -OutDir .\LangOut `
  -Field upos `
  -Window 2 `
  -SymbolBits 32 `
  -Train 0.70 `
  -Backend cpu `
  -NullSamples 0 `
  -MaxAutoNullSamples 200000
```

### DE/EN multi-resolution suite

```powershell
.\scripts\klang_suite_v1_2.ps1 `
  -DeConllu .\de_gsd-ud-train.conllu `
  -EnConllu .\en_ewt-ud-train.conllu `
  -OutRoot .\LangSuiteOut `
  -Train 0.70 `
  -Backend cpu `
  -NullSamples 0 `
  -MaxAutoNullSamples 200000
```

### Build KLLIB001

```powershell
.\scripts\klang_library_from_suite.ps1 `
  -SuiteDir .\LangSuiteOut `
  -Out .\LangSuiteOut\klang_library.kllib `
  -Json .\LangSuiteOut\klang_library.json `
  -Csv .\LangSuiteOut\klang_library_cells.csv `
  -Exe .\build_vs\Release\kdna_klang_library.exe `
  -Strict
```

---

## Reproducing the Genome Track

A typical chromosome-level path is:

```powershell
# FASTA → k=16 symbol stream
.\build_vs\Release\kdna_fasta_symbols.exe `
  --in .\Genome\chr17.fa `
  --out .\GenomeOutFull\chr17_k16.u64 `
  --summary .\GenomeOutFull\chr17_k16.kfsum `
  --k 16

# k=16 symbol stream → KDNA projection
$N = [UInt64]((Get-Item ".\GenomeOutFull\chr17_k16.u64").Length / 8)

.\build_vs\Release\kdna_project.exe `
  --symbols .\GenomeOutFull\chr17_k16.u64 `
  --n $N `
  --out-symbols .\GenomeOutFull\chr17_k16_kdna.u64 `
  --mode affine `
  --k 16 `
  --xmin -8 `
  --xmax 8 `
  --backend opencl

# KDNA stream → chromosome grammar
$N = [UInt64]((Get-Item ".\GenomeOutFull\chr17_k16_kdna.u64").Length / 8)

.\build_vs\Release\kdna_symbol_gram.exe `
  --symbols .\GenomeOutFull\chr17_k16_kdna.u64 `
  --n $N `
  --out .\GenomeOutFull\chr17_k16_self.kgram `
  --train 0.70
```

The full genome matrix is built from all chromosome entries:

```powershell
.\build_vs\Release\kdna_genome_matrix.exe `
  --entry chr1  .\GenomeOutFull\chr1_k16_kdna.u64  .\GenomeOutFull\chr1_k16_self.kgram `
  --entry chr2  .\GenomeOutFull\chr2_k16_kdna.u64  .\GenomeOutFull\chr2_k16_self.kgram `
  ...
  --entry chrX  .\GenomeOutFull\chrX_k16_kdna.u64  .\GenomeOutFull\chrX_k16_self.kgram `
  --entry chrY  .\GenomeOutFull\chrY_k16_kdna.u64  .\GenomeOutFull\chrY_k16_self.kgram `
  --out .\GenomeOutFull\human_chr1_22_XY_full_k16.kgenome `
  --csv .\GenomeOutFull\human_chr1_22_XY_full_k16.csv `
  --train 0.70 `
  --bins 32
```

---

## Testing

Run the full test suite:

```powershell
$CTEST_EXE = "C:\Program Files\CMake\bin\ctest.exe"

& $CTEST_EXE --test-dir build_vs -C Release --output-on-failure
```

Run only KSTREAM/KLANG tests:

```powershell
& $CTEST_EXE --test-dir build_vs -C Release -R "klang|kstream" --output-on-failure
```

Run KLLIB help test:

```powershell
& $CTEST_EXE --test-dir build_vs -C Release -R "klang_library_help" --output-on-failure
```

---

## Design Principles

KALYX follows these engineering constraints:

```text
Substrate before UI.
Runtime before interface.
ABI before convenience.
Tests before claims.
Determinism before aesthetics.
Reports after null models.
Libraries after artifact validation.
```

---

## Glossary

### ABI

Application Binary Interface. In KALYX this means fixed binary layouts with static size checks, such as 128-byte KGRAM headers or 512-byte KLLIB cell records.

### Baseline Accuracy

Accuracy expected from raw symbol frequency without grammar structure.

### Block-Boundary Null

A null model that disrupts block-level sequence boundaries to test whether observed grammar coupling depends on real order.

### CoNLL-U

A tabular format used by Universal Dependencies corpora. KALYX reads CoNLL-U for language experiments.

### FASTA

A common biological sequence file format. KALYX reads chromosome FASTA files for genome experiments.

### FNV1a

A deterministic non-cryptographic hash used by KALYX for symbol and artifact hashing.

### KALYX

The full system: a substrate architecture for converting ordered data into KDNA/KGRAM/KFIELD artifacts and libraries.

### KDNA

The projected representation used by KALYX. Raw symbols are mapped into KDNA symbols before grammar induction.

### KFIELD

A self/cross measurement field. Rows are streams; columns are grammars. Each cell measures how well a grammar explains a stream.

### KGENOME

Genome-specific KFIELD matrix format. Stores chromosome self/cross metrics.

### KGLIB001

Enterprise genome library artifact. Indexes chromosome streams, grammars, matrices, hashes, and file metadata.

### KGRAM01

Fixed ABI transition grammar extracted from adjacent KDNA transitions.

### KLANG

Language application layer inside KALYX. Converts CoNLL-U linguistic streams into KDNA/KGRAM/KFIELD experiments.

### KLLIB001

Enterprise language library artifact. Indexes KLANG suite results, cell metrics, null models, report hashes, and language grammar metadata.

### KSTREAM1

Generic stream summary format. Describes symbol streams from bytes, tokens, CSV, CoNLL-U, or future adapters.

### Lift

Difference between grammar accuracy and baseline accuracy.

```text
lift = kgram_accuracy - baseline_accuracy
```

### Null Model

A controlled corruption or transformation used to test whether observed structure depends on real order.

### Observed Minus Null

Difference between observed grammar accuracy and average null-model accuracy.

```text
observed_minus_null = observed_accuracy - avg_null_accuracy
```

### Out of Grammar

Number of test transitions not found in a grammar.

### Surprise Rate

Fraction of test transitions outside the grammar.

### UPOS

Universal part-of-speech tag from Universal Dependencies.

### UPOS w2

A window of two adjacent UPOS tags. This is the main local POS transition test used in KLANG.

### UPOS w3

A window of three adjacent UPOS tags. This is a harder local syntax test.

### UPOS+DEPREL

A combined linguistic symbol using universal POS and dependency relation.

---

## License

Copyright © Ralf Krümmel.

Licensed under the **Apache License, Version 2.0**.  
You may obtain a copy of the License at:

```text
https://www.apache.org/licenses/LICENSE-2.0
```

Unless required by applicable law or agreed to in writing, software distributed under the Apache License is distributed on an **"AS IS" BASIS**, without warranties or conditions of any kind.

---

## KALYX-KIE v0.6 — Document Input Adapter

v0.6 erlaubt direkte Dokumenteingabe ohne vorherigen PowerShell- oder Python-JSON-Wrapper.

Beispiel:

```powershell
.\build_vs\Release\kalyx_make_envelope.exe --domain app --intent summarize_document --document .\README.md --document-type markdown --request .\out\readme_request.txt --out .\out\readme.kie.md
```

Der alte JSON-State-Weg bleibt gültig:

```powershell
.\build_vs\Release\kalyx_make_envelope.exe --domain app --intent summarize_document --state .\out\readme_state.json --request .\out\readme_request.txt --out .\out\readme.kie.md
```

`--state` und `--document` dürfen nicht gleichzeitig verwendet werden. Ein Envelope hat genau eine autoritative Eingabequelle.

v0.6 schreibt KIE-Kontrollblöcke vor eingebettete Dokumentdaten und härtet den Section-Parser gegen Markdown-Codeblöcke und kollidierende Überschriften innerhalb von Dokumentinhalten.

---

## KALYX-KIE v0.7 — LM Studio Runtime Hardening + OpenCL Test Guard

v0.7 erweitert die LLM-Bridge und stabilisiert den Testlauf.

### LM Studio direkt verwenden

```powershell
python .\python\kalyx_llm_bridge.py --mode lmstudio --envelope .\out\readme_direct.kie.md --response .\out\readme_lmstudio.kresp.md --audit .\out\readme_lmstudio.kaudit.json --validator .\build_vs\Release\kalyx_validate_response.exe --model auto --timeout 300 --max-tokens 4096 --repair-attempts 1
```

Die Bridge nutzt standardmäßig `http://127.0.0.1:1234/v1/chat/completions`, liest bei `--model auto` den ersten Modellnamen aus `/v1/models` und versucht bei ungültiger KRESP01-Antwort eine Reparaturrunde.

### OpenCL-Tests

OpenCL-Laufzeittests sind standardmäßig deaktiviert, damit instabile Treiber/ICDs den CPU/KIE-Testlauf nicht per Segfault abbrechen. Zum Aktivieren:

```powershell
$CMAKE_EXE = "C:\Program Files\CMake\bin\cmake.exe"; $CTEST_EXE = "C:\Program Files\CMake\bin\ctest.exe"; & $CMAKE_EXE -S . -B build_vs -G "Visual Studio 17 2022" -A x64 -DKALYX_ENABLE_OPENCL_TESTS=ON; & $CMAKE_EXE --build build_vs --config Release --parallel; & $CTEST_EXE --test-dir build_vs -C Release --output-on-failure
```

## KALYX-KIE v0.9.4 — Profile Registry + Custom Profile Loader

v0.9.4 ergänzt den Prompt Author um externe `KPROFILE01` Profile.

```powershell
.\build_vs\Release\kalyx_prompt_author.exe --list-profiles --profile-dir .\profiles
.\build_vs\Release\kalyx_prompt_author.exe --profile-dir .\profiles --profile custom.summary_export --goal "Fasse README.md zusammen" --pack .\out\custom_profile.kpromptpack
.\build_vs\Release\kalyx_prompt_author.exe --profile-file .\profiles\custom.summary_export.kprofile.json --pack .\out\custom_profile_file.kpromptpack
```

Damit können Teams eigene Prompt-/Contract-/Policy-Profile versionieren, ohne KALYX neu zu kompilieren.

---

## KALYX-KIE v1.0 Final — Documentation Pack + User-Facing CLI Polish

v1.0 Final ergänzt v0.9.4 um eine produktreife Dokumentations- und CLI-Schicht.

### Neue Dokumentation

```text
docs/KALYX_KIE_v0_9_5_USER_GUIDE.md
docs/KALYX_KIE_SCHEMA_REFERENCE.md
docs/KALYX_KIE_CLI_REFERENCE.md
docs/KALYX_KIE_DEMO_RUNBOOK.md
docs/KALYX_KIE_TROUBLESHOOTING.md
```

### Neue Skripte

```powershell
.\scripts\run_kalyx_v0_9_5_offline_reference.ps1
.\scripts\run_kalyx_v0_9_5_lmstudio_live.ps1
```

### CLI-Polish

Die zentralen KIE-Tools haben erweiterte `--help`-Ausgaben und unterstützen `--version`:

```powershell
.\build_vs\Release\kalyx_prompt_author.exe --help
.\build_vs\Release\kalyx_make_envelope.exe --help
.\build_vs\Release\kalyx_validate_response.exe --help
.\build_vs\Release\kalyx_host_dispatch_demo.exe --help
```

### Empfohlener Build unter Windows

```powershell
$CMAKE_EXE = "C:\Program Files\CMake\bin\cmake.exe"; $CTEST_EXE = "C:\Program Files\CMake\bin\ctest.exe"; Remove-Item .\build_vs -Recurse -Force -ErrorAction SilentlyContinue; Remove-Item .\out -Recurse -Force -ErrorAction SilentlyContinue; & $CMAKE_EXE -S . -B build_vs -G "Visual Studio 17 2022" -A x64; & $CMAKE_EXE --build build_vs --config Release --parallel 1 -- /m:1 /p:CL_MPCount=1; & $CTEST_EXE --test-dir build_vs -C Release --output-on-failure
```



---

## KALYX-KIE v1.0 Final — Release Candidate

v1.0 Final friert die KALYX-KIE-Oberfläche für Release-Candidate-Tests ein.

Offizielle Einstiegspunkte:

```powershell
.\scripts
un_kalyx_v1_0_final_offline_reference.ps1
.\scripts
un_kalyx_v1_0_final_lmstudio_live.ps1
```

Wichtige RC-Dateien:

```text
KALYX_KIE_v1_0_Final_MANIFEST.json
KALYX_KIE_v1_0_Final_RELEASE_NOTES.md
docs/KALYX_KIE_v1_0_Final_QUICKSTART.md
docs/KALYX_KIE_v1_0_Final_TEST_MATRIX.md
docs/KALYX_KIE_v1_0_Final_KNOWN_ISSUES.md
docs/KALYX_KIE_v1_0_Final_SCHEMA_FREEZE.md
```

Version der zentralen Tools:

```text
1.0-kie-v1.0-final
```


## KALYX-KIE v1.0 Final — Release Candidate Stabilization

Final ist ein Stabilisierungsschnitt auf RC1. Die Runtime-Funktionen bleiben eingefroren; verbessert wurden Manifest, Dokumentation, offizielle Runbooks und Final-Smoke-Checks.

Offizielle Läufe:

```powershell
.\scripts\run_kalyx_v1_0_final_offline_reference.ps1
.\scripts\run_kalyx_v1_0_final_lmstudio_live.ps1
.\scripts\run_kalyx_v1_0_final_version_check.ps1
```

Version:

```text
1.0-kie-v1.0-final
```


## KALYX-KIE v1.0 Final — Stable Release

v1.0 Final ist der stabile Schnitt nach RC2. Die Runtime-Funktionen bleiben gegenüber RC2 eingefroren; finalisiert wurden Versionsstring, Manifest, Dokumentation und offizielle Runbooks.

Offizielle Einstiegspunkte:

```powershell
.\scriptsun_kalyx_v1_0_final_offline_reference.ps1
.\scriptsun_kalyx_v1_0_final_lmstudio_live.ps1
.\scriptsun_kalyx_v1_0_final_version_check.ps1
```

Erwartete Version:

```text
1.0-kie-v1.0-final
```
