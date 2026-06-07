param(
  [Parameter(Mandatory=$true)][string]$SequenceDir,
  [Parameter(Mandatory=$true)][string]$V09Dir,
  [string]$OutDir = ".\Decode_chr17_v10_real",
  [int]$TargetRank = 3,
  [int]$Iterations = 250,
  [int]$HorPeriod = 2380,
  [int]$HorSlots = 14,
  [string]$Python = "python",
  [switch]$KeepAllRepeatGroups
)

$ErrorActionPreference = "Stop"

$script = Join-Path $PSScriptRoot "..\python\kalyx_v091_ratefix_hor_v10.py"
if (-not (Test-Path $script)) {
  throw "Python analyzer fehlt: $script"
}
if (-not (Test-Path $SequenceDir)) {
  throw "SequenceDir fehlt: $SequenceDir"
}
if (-not (Test-Path (Join-Path $SequenceDir "cluster_v04_block_sequences.csv"))) {
  throw "cluster_v04_block_sequences.csv fehlt in: $SequenceDir"
}
if (-not (Test-Path $V09Dir)) {
  throw "V09Dir fehlt: $V09Dir"
}
if (-not (Test-Path (Join-Path $V09Dir "cluster_v09_block_repeat_annotations.csv"))) {
  Write-Warning "cluster_v09_block_repeat_annotations.csv fehlt in V09Dir. Lauf geht weiter, aber ohne Dominant-Repeat-Group-Filter."
}

$args = @(
  $script,
  "--sequence-dir", $SequenceDir,
  "--v09-dir", $V09Dir,
  "--out-dir", $OutDir,
  "--target-rank", "$TargetRank",
  "--iterations", "$Iterations",
  "--hor-period", "$HorPeriod",
  "--hor-slots", "$HorSlots"
)

if ($KeepAllRepeatGroups) {
  $args += "--keep-all-repeat-groups"
}

& $Python @args

if ($LASTEXITCODE -ne 0) {
  throw "KALYX v0.9.1/v1.0 Lauf fehlgeschlagen: exit=$LASTEXITCODE"
}
