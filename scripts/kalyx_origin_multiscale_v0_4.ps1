
param(
  [Parameter(Mandatory=$true)][string]$Symbols,
  [Parameter(Mandatory=$true)][string]$OutRoot,
  [UInt64]$CenterStart = 23068672,
  [UInt64[]]$WindowSizes = @(131072,262144,524288,1048576),
  [int]$Windows = 32,
  [switch]$Strict
)

$ErrorActionPreference = "Stop"

function Fail($m) {
  if ($Strict) { throw $m }
  Write-Error $m
}

if (-not (Test-Path $Symbols)) { Fail "Symbols fehlt: $Symbols" }

New-Item -ItemType Directory -Force $OutRoot | Out-Null
$Summary = Join-Path $OutRoot "KALYX_ORIGIN_V0_4_MULTISCALE_SUMMARY.md"
$Lines = @()
$Lines += "# KALYX-ORIGIN v0.4 Multiscale Summary"
$Lines += ""
$Lines += "- Symbols: ``$Symbols``"
$Lines += "- CenterStart: ``$CenterStart``"
$Lines += "- Windows per scale: ``$Windows``"
$Lines += ""

foreach ($W in $WindowSizes) {
  $span = [UInt64]$W * [UInt64]$Windows
  $half = [UInt64]($span / 2)
  $start = if ($CenterStart -gt $half) { [UInt64]($CenterStart - $half) } else { [UInt64]0 }
  $out = Join-Path $OutRoot ("w{0}" -f $W)

  Remove-Item -Recurse -Force $out -ErrorAction SilentlyContinue

  .\scripts\kalyx_origin_zoom_v0_4.ps1 `
    -Symbols $Symbols `
    -OutDir $out `
    -StartSymbols $start `
    -WindowSymbols $W `
    -StepSymbols $W `
    -Windows $Windows `
    -Strict:$Strict

  $rankCsv = Join-Path $out "kalyx_origin_v0_4_ranking.csv"
  if (Test-Path $rankCsv) {
    $top = Import-Csv $rankCsv | Select-Object -First 5
    $Lines += "## WindowSymbols $W"
    $Lines += ""
    $Lines += "| Rank | Label | Skip | Score | Entropy | Top32 | EdgeEntropy | EdgeTop32 |"
    $Lines += "|---:|---|---:|---:|---:|---:|---:|---:|"
    $r = 1
    foreach ($row in $top) {
      $Lines += ("| {0} | {1} | {2} | {3} | {4} | {5} | {6} | {7} |" -f `
        $r,$row.label,$row.skip_symbols,$row.origin_anomaly_score,$row.entropy_bits,$row.top32_mass,$row.edge_entropy_bits,$row.edge_top32_mass)
      $r++
    }
    $Lines += ""
  }
}

Set-Content -Encoding UTF8 $Summary $Lines
Write-Host "KALYX-ORIGIN v0.4 multiscale complete:"
Write-Host "  $Summary"
