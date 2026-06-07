param(
  [string]$OutDir = ".\Controls_UCSC_hg38",
  [string]$Chrom = "chr17",
  [switch]$Download,
  [switch]$Force,
  [string]$RmskTxtGz = "",
  [string]$SuperDupsTxtGz = "",
  [string]$Python = "python"
)

$ErrorActionPreference = "Stop"

$script = Join-Path $PSScriptRoot "..\python\kalyx_build_ucsc_control_beds_v0_8_1.py"
if (-not (Test-Path $script)) {
  throw "BED builder fehlt: $script"
}

$args = @(
  $script,
  "--out-dir", $OutDir,
  "--chrom", $Chrom
)

if ($Download) { $args += "--download" }
if ($Force) { $args += "--force" }
if ($RmskTxtGz -ne "") {
  $args += "--rmsk-txt-gz"
  $args += $RmskTxtGz
}
if ($SuperDupsTxtGz -ne "") {
  $args += "--superdups-txt-gz"
  $args += $SuperDupsTxtGz
}

& $Python @args
if ($LASTEXITCODE -ne 0) {
  throw "KALYX UCSC Control BED Builder fehlgeschlagen: exit=$LASTEXITCODE"
}
