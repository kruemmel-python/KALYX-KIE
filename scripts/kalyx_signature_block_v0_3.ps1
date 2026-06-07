param(
  [Parameter(Mandatory=$true)][string]$DecodeDir,
  [string]$OutDir = ".\Decode_chr17_v03",
  [int]$TargetRank = 3,
  [string]$Periods = "170,171,337,341,342,506,507,508,513,2378,2379,2380",
  [string]$FamilyCenters = "137,168,170,171,200,205,341,342,506,507,508,513,2379",
  [int]$FamilyTolerance = 3,
  [string]$Python = "python"
)

$ErrorActionPreference = "Stop"

$script = Join-Path $PSScriptRoot "..\python\kalyx_signature_block_v0_3.py"
if (-not (Test-Path $script)) {
  throw "Python analyzer fehlt: $script"
}
if (-not (Test-Path $DecodeDir)) {
  throw "DecodeDir fehlt: $DecodeDir"
}
if (-not (Test-Path (Join-Path $DecodeDir "cluster_v02_clusters.csv"))) {
  throw "cluster_v02_clusters.csv fehlt in: $DecodeDir"
}
if (-not (Test-Path (Join-Path $DecodeDir "cluster_v02_templates.csv"))) {
  throw "cluster_v02_templates.csv fehlt in: $DecodeDir"
}

& $Python $script `
  --decode-dir $DecodeDir `
  --out-dir $OutDir `
  --target-rank $TargetRank `
  --periods $Periods `
  --family-centers $FamilyCenters `
  --family-tolerance $FamilyTolerance

if ($LASTEXITCODE -ne 0) {
  throw "KALYX Signature Block Analyzer v0.3 fehlgeschlagen: exit=$LASTEXITCODE"
}
