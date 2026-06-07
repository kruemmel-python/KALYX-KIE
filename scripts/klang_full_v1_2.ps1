param(
  [Parameter(Mandatory=$true)][string]$DeConllu,
  [Parameter(Mandatory=$true)][string]$EnConllu,
  [string]$OutDir = ".\LangOut",
  [string]$Field = "upos",
  [int]$Window = 2,
  [int]$SymbolBits = 32,
  [double]$Train = 0.70,
  [string]$Backend = "cpu",
  [UInt64]$NullSamples = 0,
  [UInt64]$MaxAutoNullSamples = 200000,
  [UInt64]$MinAutoNullSamples = 10000
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$exp = Join-Path $PSScriptRoot "klang_2x2_experiment.ps1"
$report = Join-Path $PSScriptRoot "klang_report_v1_2.ps1"

if (-not (Test-Path $exp)) { throw "Fehlt: $exp" }
if (-not (Test-Path $report)) { throw "Fehlt: $report" }

& $exp `
  -DeConllu $DeConllu `
  -EnConllu $EnConllu `
  -OutDir $OutDir `
  -Field $Field `
  -Window $Window `
  -SymbolBits $SymbolBits `
  -Train $Train `
  -Backend $Backend `
  -NullSamples $NullSamples `
  -MaxAutoNullSamples $MaxAutoNullSamples `
  -MinAutoNullSamples $MinAutoNullSamples

$matrix = Join-Path $OutDir ("klang_{0}_w{1}.csv" -f $Field,$Window)
$null = Join-Path $OutDir ("klang_{0}_w{1}_null.csv" -f $Field,$Window)

& $report `
  -OutDir $OutDir `
  -MatrixCsv $matrix `
  -NullCsv $null `
  -ReportDir (Join-Path $OutDir "ReportV12") `
  -Title ("KLANG_REPORT v1.2 — {0} w{1}" -f $Field,$Window)

Write-Host "KLANG v1.2 full run complete:"
Write-Host "  $matrix"
Write-Host "  $null"
Write-Host "  $(Join-Path $OutDir 'ReportV12\KLANG_REPORT_V1_2.html')"
