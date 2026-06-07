param(
  [Parameter(Mandatory=$true)][string]$SequenceDir,
  [string]$OutDir = ".\Decode_chr17_v05_real",
  [int]$TopStates = 12,
  [int]$NGram = 3,
  [string]$Python = "python"
)

$ErrorActionPreference = "Stop"

$script = Join-Path $PSScriptRoot "..\python\kalyx_spacer_state_transition_v0_5.py"
if (-not (Test-Path $script)) {
  throw "Python analyzer fehlt: $script"
}
if (-not (Test-Path $SequenceDir)) {
  throw "SequenceDir fehlt: $SequenceDir"
}
if (-not (Test-Path (Join-Path $SequenceDir "cluster_v04_block_sequences.csv"))) {
  throw "cluster_v04_block_sequences.csv fehlt in: $SequenceDir"
}

& $Python $script `
  --sequence-dir $SequenceDir `
  --out-dir $OutDir `
  --top-states $TopStates `
  --ngram $NGram

if ($LASTEXITCODE -ne 0) {
  throw "KALYX Spacer-State Transition Decoder v0.5 fehlgeschlagen: exit=$LASTEXITCODE"
}
