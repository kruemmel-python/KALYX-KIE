
param(
  [Parameter(Mandatory=$true)][string]$MotifRoot,
  [Parameter(Mandatory=$true)][string]$OutRoot,
  [string]$KValues = "12,16,20",
  [int]$Hamming = 2,
  [UInt64]$MinCount = 1,
  [UInt64]$MaxRows = 20000,
  [int]$TopFamilies = 15,
  [string]$Exe = ".\build_vs\Release\kalyx_motif_family.exe",
  [switch]$RcLink,
  [switch]$Strict
)

$ErrorActionPreference = "Stop"
function Fail($m) { throw $m }
function Read-CsvSmart($Path) {
  $raw = Get-Content $Path -Raw
  if ($raw -match ';') { return Import-Csv $Path -Delimiter ';' }
  return Import-Csv $Path
}

if (-not (Test-Path $MotifRoot)) { Fail "MotifRoot fehlt: $MotifRoot" }
New-Item -ItemType Directory -Force -Path $OutRoot | Out-Null

$Dirs = Get-ChildItem $MotifRoot -Directory | Where-Object { Test-Path (Join-Path $_.FullName "kalyx_origin_v0_7_top_kmers_all.csv") } | Sort-Object Name
if ($Dirs.Count -eq 0) {
  $single = Join-Path $MotifRoot "kalyx_origin_v0_7_top_kmers_all.csv"
  if (Test-Path $single) {
    $Dirs = @([pscustomobject]@{ Name = "single"; FullName = (Resolve-Path $MotifRoot).Path })
  } else {
    Fail "Keine v0.7 top-kmers CSV gefunden unter: $MotifRoot"
  }
}

$Summary = Join-Path $OutRoot "KALYX_ORIGIN_V0_8_MULTISCALE_FAMILY_SUMMARY.md"
$Lines = New-Object System.Collections.Generic.List[string]
$Lines.Add("# KALYX-ORIGIN v0.8 Multiscale Motif-Family Summary")
$Lines.Add("")
$Lines.Add("Motif-family clustering across v0.7 multiscale outputs.")
$Lines.Add("")

foreach ($d in $Dirs) {
  $name = $d.Name
  $top = Join-Path $d.FullName "kalyx_origin_v0_7_top_kmers_all.csv"
  $out = Join-Path $OutRoot $name
  Write-Host "=== KALYX-ORIGIN v0.8 family clustering: $name ==="
  .\scripts\kalyx_origin_family_v0_8.ps1 `
    -TopKmersCsv $top `
    -OutDir $out `
    -KValues $KValues `
    -Hamming $Hamming `
    -MinCount $MinCount `
    -MaxRows $MaxRows `
    -TopFamilies $TopFamilies `
    -Exe $Exe `
    -RcLink:$RcLink `
    -Strict:$Strict

  $fam = Join-Path $out "kalyx_origin_v0_8_families_all.csv"
  $topFamily = $null
  if (Test-Path $fam) {
    $rows = @(Read-CsvSmart $fam | Sort-Object {[UInt64]($_.total_count)} -Descending | Select-Object -First 1)
    if ($rows.Count -gt 0) { $topFamily = $rows[0] }
  }
  $Lines.Add("## $name")
  $Lines.Add("")
  $Lines.Add("- TopKmers: ``$top``")
  $Lines.Add("- Output: ``$out``")
  if ($null -ne $topFamily) {
    $Lines.Add("- Top consensus: ``$($topFamily.consensus)``")
    $Lines.Add("- k: ``$($topFamily.k)``")
    $Lines.Add("- total_count: ``$($topFamily.total_count)``")
    $Lines.Add("- size: ``$($topFamily.size)``")
    $Lines.Add("- rc_balance: ``$($topFamily.rc_balance)``")
  }
  $Lines.Add("")
}

$Lines | Set-Content -Encoding UTF8 $Summary
Write-Host "KALYX-ORIGIN v0.8 multiscale family complete:"
Write-Host "  $Summary"
