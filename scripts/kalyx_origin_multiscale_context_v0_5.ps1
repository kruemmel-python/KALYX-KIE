
param(
  [Parameter(Mandatory=$true)][string]$Fasta,
  [Parameter(Mandatory=$true)][string]$MultiScaleDir,
  [Parameter(Mandatory=$true)][string]$OutRoot,
  [UInt32]$K = 16,
  [Int32]$Top = 10,
  [string]$Exe = ".\build_vs\Release\kalyx_fasta_context.exe",
  [switch]$Strict
)
$ErrorActionPreference = "Stop"
function Fail($m) { throw $m }

if (-not (Test-Path $Fasta)) { Fail "FASTA fehlt: $Fasta" }
if (-not (Test-Path $MultiScaleDir)) { Fail "MultiScaleDir fehlt: $MultiScaleDir" }

New-Item -ItemType Directory -Force $OutRoot | Out-Null
$summary = Join-Path $OutRoot "KALYX_ORIGIN_V0_5_MULTISCALE_CONTEXT_SUMMARY.md"
$lines = @()
$lines += "# KALYX-ORIGIN v0.5 Multiscale FASTA Context Summary"
$lines += ""

$dirs = Get-ChildItem $MultiScaleDir -Directory | Where-Object { Test-Path (Join-Path $_.FullName "kalyx_origin_v0_4_ranking.csv") } | Sort-Object Name
foreach ($d in $dirs) {
  $ranking = Join-Path $d.FullName "kalyx_origin_v0_4_ranking.csv"
  $w = 0
  if ($d.Name -match "w(\d+)") { $w = [UInt64]$Matches[1] }
  if ($w -eq 0) { continue }

  $outDir = Join-Path $OutRoot $d.Name
  & .\scripts\kalyx_origin_context_v0_5.ps1 `
    -Fasta $Fasta `
    -RankingCsv $ranking `
    -OutDir $outDir `
    -K $K `
    -WindowSymbols $w `
    -Top $Top `
    -Exe $Exe `
    -Strict:$Strict

  $ctxCsv = Join-Path $outDir "kalyx_origin_v0_5_fasta_context.csv"
  $ctx = Import-Csv $ctxCsv
  $first = $ctx | Select-Object -First 1
  $lines += "## $($d.Name)"
  $lines += ""
  $lines += "- Ranking: ``$ranking``"
  $lines += "- Context: ``$ctxCsv``"
  if ($first) {
    $lines += "- Top label: ``$($first.label)``"
    $lines += "- Top base range 0-based: ``$($first.base_start_0)..$($first.base_end_0)``"
    $lines += "- Top GC: ``$($first.gc_rate)``"
    $lines += "- Top N-rate: ``$($first.n_rate)``"
  }
  $lines += ""
}

[System.IO.File]::WriteAllLines($summary, $lines, [System.Text.UTF8Encoding]::new($false))
Write-Host "KALYX-ORIGIN v0.5 multiscale context complete:"
Write-Host "  $summary"
