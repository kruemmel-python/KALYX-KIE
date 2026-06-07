param(
  [Parameter(Mandatory=$true)][string]$StateDir,
  [string]$OutDir = ".\Decode_chr17_v06_real",
  [int]$Iterations = 1000,
  [int]$MinGroupCount = 10,
  [int]$NGram = 3,
  [string]$Python = "python"
)

$ErrorActionPreference = "Stop"

$script = Join-Path $PSScriptRoot "..\python\kalyx_spacer_state_determinism_v0_6.py"
if (-not (Test-Path $script)) {
  throw "Python analyzer fehlt: $script"
}
if (-not (Test-Path $StateDir)) {
  throw "StateDir fehlt: $StateDir"
}
if (-not (Test-Path (Join-Path $StateDir "cluster_v05_transitions_raw.csv"))) {
  throw "cluster_v05_transitions_raw.csv fehlt in: $StateDir"
}
if (-not (Test-Path (Join-Path $StateDir "cluster_v05_block_states.csv"))) {
  throw "cluster_v05_block_states.csv fehlt in: $StateDir"
}

& $Python $script `
  --state-dir $StateDir `
  --out-dir $OutDir `
  --iterations $Iterations `
  --min-group-count $MinGroupCount `
  --ngram $NGram

if ($LASTEXITCODE -ne 0) {
  throw "KALYX Spacer-State Determinism v0.6 fehlgeschlagen: exit=$LASTEXITCODE"
}
