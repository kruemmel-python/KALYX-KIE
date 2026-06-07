param(
  [Parameter(Mandatory=$true)][string]$DeterminismDir,
  [string]$SequenceDir = "",
  [string]$OutDir = ".\Decode_chr17_v07_real",
  [string]$RepeatBed = "",
  [string]$Chrom = "chr17",
  [double]$ZThreshold = 10.0,
  [double]$PThreshold = 0.001,
  [string]$Python = "python"
)

$ErrorActionPreference = "Stop"

$script = Join-Path $PSScriptRoot "..\python\kalyx_evidence_gate_v0_7.py"
if (-not (Test-Path $script)) {
  throw "Python analyzer fehlt: $script"
}
if (-not (Test-Path $DeterminismDir)) {
  throw "DeterminismDir fehlt: $DeterminismDir"
}
if (-not (Test-Path (Join-Path $DeterminismDir "cluster_v06_null_summary.csv"))) {
  throw "cluster_v06_null_summary.csv fehlt in: $DeterminismDir"
}

$args = @(
  $script,
  "--determinism-dir", $DeterminismDir,
  "--out-dir", $OutDir,
  "--chrom", $Chrom,
  "--z-threshold", "$ZThreshold",
  "--p-threshold", "$PThreshold"
)

if ($SequenceDir -ne "") {
  $args += "--sequence-dir"
  $args += $SequenceDir
}
if ($RepeatBed -ne "") {
  $args += "--repeat-bed"
  $args += $RepeatBed
}

& $Python @args

if ($LASTEXITCODE -ne 0) {
  throw "KALYX Evidence Gate v0.7 fehlgeschlagen: exit=$LASTEXITCODE"
}
