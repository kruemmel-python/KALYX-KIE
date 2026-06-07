param(
  [Parameter(Mandatory=$true)][string]$BlockDir,
  [Parameter(Mandatory=$true)][string]$Fasta,
  [string]$OutDir = ".\Decode_chr17_v04",
  [int]$TargetRank = 3,
  [string]$Python = "python"
)

$ErrorActionPreference = "Stop"

$script = Join-Path $PSScriptRoot "..\python\kalyx_signature_sequence_v0_4.py"
if (-not (Test-Path $script)) {
  throw "Python analyzer fehlt: $script"
}
if (-not (Test-Path $BlockDir)) {
  throw "BlockDir fehlt: $BlockDir"
}
if (-not (Test-Path (Join-Path $BlockDir "cluster_v03_blocks.csv"))) {
  throw "cluster_v03_blocks.csv fehlt in: $BlockDir"
}
if (-not (Test-Path $Fasta)) {
  throw "FASTA fehlt: $Fasta"
}

& $Python $script `
  --block-dir $BlockDir `
  --fasta $Fasta `
  --out-dir $OutDir `
  --target-rank $TargetRank

if ($LASTEXITCODE -ne 0) {
  throw "KALYX Signature Block Sequence Decoder v0.4 fehlgeschlagen: exit=$LASTEXITCODE"
}
