param(
  [Parameter(Mandatory=$true)][string]$SequenceDir,
  [Parameter(Mandatory=$true)][string]$StateDir,
  [string]$DeterminismDir = ".\Decode_chr17_v06_real",
  [string]$EvidenceDir = ".\Decode_chr17_v07_real",
  [string]$OutDir = ".\Decode_chr17_v08_real",
  [string]$Fasta = ".\chr17.fa",
  [string]$ChromDir = "",
  [string]$RepeatMaskerBed = "",
  [string]$SegmentalDupBed = "",
  [int]$Iterations = 1000,
  [string]$Python = "python"
)

$ErrorActionPreference = "Stop"

$script = Join-Path $PSScriptRoot "..\python\kalyx_natural_model_breaker_v0_8.py"
if (-not (Test-Path $script)) { throw "Python analyzer fehlt: $script" }
if (-not (Test-Path $SequenceDir)) { throw "SequenceDir fehlt: $SequenceDir" }
if (-not (Test-Path (Join-Path $SequenceDir "cluster_v04_block_sequences.csv"))) { throw "cluster_v04_block_sequences.csv fehlt in $SequenceDir" }
if (-not (Test-Path $StateDir)) { throw "StateDir fehlt: $StateDir" }
if (-not (Test-Path (Join-Path $StateDir "cluster_v05_block_states.csv"))) { throw "cluster_v05_block_states.csv fehlt in $StateDir" }

$args = @(
  $script,
  "--sequence-dir", $SequenceDir,
  "--state-dir", $StateDir,
  "--determinism-dir", $DeterminismDir,
  "--evidence-dir", $EvidenceDir,
  "--out-dir", $OutDir,
  "--iterations", $Iterations
)

if ($Fasta -ne "" -and (Test-Path $Fasta)) {
  $args += "--fasta"
  $args += $Fasta
}

if ($ChromDir -ne "" -and (Test-Path $ChromDir)) {
  $args += "--chrom-dir"
  $args += $ChromDir
}

if ($RepeatMaskerBed -ne "" -and (Test-Path $RepeatMaskerBed)) {
  $args += "--repeatmasker-bed"
  $args += $RepeatMaskerBed
}

if ($SegmentalDupBed -ne "" -and (Test-Path $SegmentalDupBed)) {
  $args += "--segmental-dup-bed"
  $args += $SegmentalDupBed
}

& $Python @args

if ($LASTEXITCODE -ne 0) {
  throw "KALYX Natural Model Breaker v0.8 fehlgeschlagen: exit=$LASTEXITCODE"
}
