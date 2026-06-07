param(
  [Parameter(Mandatory=$true)][string]$SequenceDir,
  [Parameter(Mandatory=$true)][string]$RepeatMaskerBed,
  [Parameter(Mandatory=$true)][string]$Fasta,
  [string]$OutDir = ".\Decode_chr17_v09_real",
  [string]$Chrom = "chr17",
  [int]$Iterations = 250,
  [string]$DominantGroup = "",
  [switch]$ScanFamily,
  [string]$Python = "python"
)

$ErrorActionPreference = "Stop"

$script = Join-Path $PSScriptRoot "..\python\kalyx_repeat_family_automaton_v0_9.py"
if (-not (Test-Path $script)) { throw "v0.9 Python script fehlt: $script" }
if (-not (Test-Path $SequenceDir)) { throw "SequenceDir fehlt: $SequenceDir" }
if (-not (Test-Path (Join-Path $SequenceDir "cluster_v04_block_sequences.csv"))) { throw "cluster_v04_block_sequences.csv fehlt in $SequenceDir" }
if (-not (Test-Path $RepeatMaskerBed)) { throw "RepeatMasker BED fehlt: $RepeatMaskerBed" }
if (-not (Test-Path $Fasta)) { throw "FASTA fehlt: $Fasta" }

$args = @(
  $script,
  "--sequence-dir", $SequenceDir,
  "--repeatmasker-bed", $RepeatMaskerBed,
  "--fasta", $Fasta,
  "--out-dir", $OutDir,
  "--chrom", $Chrom,
  "--iterations", $Iterations
)

if ($DominantGroup -ne "") {
  $args += "--dominant-group"
  $args += $DominantGroup
}
if ($ScanFamily) {
  $args += "--scan-family"
}

& $Python @args

if ($LASTEXITCODE -ne 0) {
  throw "KALYX Repeat-Family Conditioned Automaton v0.9 fehlgeschlagen: exit=$LASTEXITCODE"
}
