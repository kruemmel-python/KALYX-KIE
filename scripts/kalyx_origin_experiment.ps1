
param(
  [string]$PlantOutDir = ".\PlantOut_chr17_v02",
  [string]$OutDir = "",
  [UInt64]$MaxSymbols = 1048576,
  [switch]$Strict
)

$ErrorActionPreference = "Stop"
$Inv = [System.Globalization.CultureInfo]::InvariantCulture

function Fail($m) { throw $m }
function NeedFile($p) { if (-not (Test-Path $p)) { Fail "Fehlt: $p" } }
function D([object]$x) {
  if ($null -eq $x) { return 0.0 }
  $s = [string]$x
  if ($s.Length -eq 0) { return 0.0 }
  return [double]::Parse($s, $Inv)
}
function F9([double]$x) { return $x.ToString("0.000000000", $Inv) }

NeedFile ".\build_vs\Release\kalyx_origin.exe"

$PlantOutDir = (Resolve-Path $PlantOutDir).Path
if ($OutDir.Length -eq 0) {
  $OutDir = Join-Path $PlantOutDir "Origin"
}
New-Item -ItemType Directory -Force $OutDir | Out-Null
$OutDir = (Resolve-Path $OutDir).Path

$Control = Join-Path $PlantOutDir "carrier_control.u64"
$Planted = Join-Path $PlantOutDir "carrier_planted.u64"
$Pattern = Join-Path $PlantOutDir "plant_pattern.u64"
$Positions = Join-Path $PlantOutDir "plant_positions.csv"
$PlantMatrix = Join-Path $PlantOutDir "kalyx_plant.csv"
$PlantNull = Join-Path $PlantOutDir "kalyx_plant_null.csv"
$PlantReport = Join-Path $PlantOutDir "KALYX_PLANT_REPORT.md"

foreach ($P in @($Control,$Planted,$Pattern,$Positions,$PlantMatrix,$PlantNull)) { NeedFile $P }

Write-Host "=== KALYX-ORIGIN v0.1 diagnostics ==="

$Rows = @()
$TempFiles = @()

$Runs = @(
  @{ name="control"; symbols=$Control; control=$Control; pattern=$Pattern; positions="" },
  @{ name="planted"; symbols=$Planted; control=$Control; pattern=$Pattern; positions=$Positions },
  @{ name="pattern"; symbols=$Pattern; control=$Control; pattern=$Pattern; positions="" }
)

foreach ($R in $Runs) {
  $Tmp = Join-Path $OutDir ("origin_" + $R.name + ".csv")
  $Args = @(
    "--symbols", $R.symbols,
    "--out", $Tmp,
    "--name", $R.name,
    "--control", $R.control,
    "--pattern", $R.pattern,
    "--max-symbols", "$MaxSymbols"
  )
  if ($R.positions.Length -gt 0) {
    $Args += "--positions"
    $Args += $R.positions
  }
  & .\build_vs\Release\kalyx_origin.exe @Args
  if ($LASTEXITCODE -ne 0) { Fail "kalyx_origin fehlgeschlagen: $($R.name)" }
  NeedFile $Tmp
  $TempFiles += $Tmp
}

$MetricsCsv = Join-Path $OutDir "kalyx_origin_metrics.csv"
$First = $true
$Combined = @()
foreach ($T in $TempFiles) {
  $Lines = Get-Content $T
  if ($First) {
    $Combined += $Lines[0]
    $First = $false
  }
  $Combined += $Lines[1]
}
$Utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllLines($MetricsCsv, $Combined, $Utf8NoBom)

$Metrics = Import-Csv $MetricsCsv
$ControlM = $Metrics | Where-Object { $_.name -eq "control" } | Select-Object -First 1
$PlantedM = $Metrics | Where-Object { $_.name -eq "planted" } | Select-Object -First 1
$PatternM = $Metrics | Where-Object { $_.name -eq "pattern" } | Select-Object -First 1

$PlantCells = Import-Csv $PlantMatrix
$PlantedPattern = $PlantCells | Where-Object { $_.row -eq "planted" -and $_.col -eq "pattern" } | Select-Object -First 1
$ControlPattern = $PlantCells | Where-Object { $_.row -eq "control" -and $_.col -eq "pattern" } | Select-Object -First 1
$PatternPattern = $PlantCells | Where-Object { $_.row -eq "pattern" -and $_.col -eq "pattern" } | Select-Object -First 1

$Report = Join-Path $OutDir "KALYX_ORIGIN_REPORT.md"

$Lines = @()
$Lines += "# KALYX-ORIGIN v0.1 Report"
$Lines += ""
$Lines += "Genesis diagnostics for a declared KALYX-PLANT positive-control run."
$Lines += ""
$Lines += "## Scope"
$Lines += ""
$Lines += "KALYX-ORIGIN does not prove natural or artificial origin. It measures origin-relevant diagnostics. Core principle:"
$Lines += ""
$Lines += "> Structure is not origin. KALYX detects transition order; origin requires additional diagnostics."
$Lines += ""
$Lines += "## Inputs"
$Lines += ""
$Lines += ("- PlantOutDir: ``{0}``" -f $PlantOutDir)
$Lines += ("- MaxSymbols: ``{0}``" -f $MaxSymbols)
if (Test-Path $PlantReport) { $Lines += ("- Plant report: ``{0}``" -f $PlantReport) }
$Lines += ""
$Lines += "## Key KFIELD Cells"
$Lines += ""
$Lines += "| Cell | Accuracy | Lift | Surprise |"
$Lines += "|---|---:|---:|---:|"
$Lines += "| control → pattern | $(F9 (D $ControlPattern.kgram_accuracy)) | $(F9 (D $ControlPattern.lift)) | $(F9 (D $ControlPattern.surprise_rate)) |"
$Lines += "| planted → pattern | $(F9 (D $PlantedPattern.kgram_accuracy)) | $(F9 (D $PlantedPattern.lift)) | $(F9 (D $PlantedPattern.surprise_rate)) |"
$Lines += "| pattern → pattern | $(F9 (D $PatternPattern.kgram_accuracy)) | $(F9 (D $PatternPattern.lift)) | $(F9 (D $PatternPattern.surprise_rate)) |"
$Lines += ""
$Lines += "## Origin Diagnostics"
$Lines += ""
$Lines += "| Stream | Entropy bits | Unique symbols | Top32 mass | Control hamming | Pattern edge accuracy | Position density | Plant-like score |"
$Lines += "|---|---:|---:|---:|---:|---:|---:|---:|"
foreach ($M in $Metrics) {
  $Lines += "| $($M.name) | $(F9 (D $M.entropy_bits)) | $($M.unique_symbols) | $(F9 (D $M.top32_mass)) | $(F9 (D $M.control_hamming_rate)) | $(F9 (D $M.pattern_edge_accuracy)) | $(F9 (D $M.position_density)) | $(F9 (D $M.plant_like_score)) |"
}
$Lines += ""
$Lines += "## Planted Schedule Diagnostics"
$Lines += ""
$Lines += "| Metric | Value |"
$Lines += "|---|---:|"
$Lines += "| positions_count | $($PlantedM.positions_count) |"
$Lines += "| position_density | $(F9 (D $PlantedM.position_density)) |"
$Lines += "| gap_mean | $(F9 (D $PlantedM.gap_mean)) |"
$Lines += "| gap_std | $(F9 (D $PlantedM.gap_std)) |"
$Lines += "| gap_min | $($PlantedM.gap_min) |"
$Lines += "| gap_max | $($PlantedM.gap_max) |"
$Lines += "| jitter_abs_mean | $(F9 (D $PlantedM.jitter_abs_mean)) |"
$Lines += ""
$Lines += "## Interpretation"
$Lines += ""
$Lines += "A declared planted stream should show: higher pattern-edge compatibility than control, measurable drift from control, entropy/concentration changes, and a nonzero audited position density. Natural or unknown streams require comparison against controls, relatives, neighboring regions, k/window sweeps, and null-model sweeps before origin claims."
$Lines += ""
$Lines += "## Artefacts"
$Lines += ""
$Lines += ("- ``{0}``" -f $MetricsCsv)
$Lines += ("- ``{0}``" -f $Report)

[System.IO.File]::WriteAllLines($Report, $Lines, $Utf8NoBom)

Write-Host "KALYX-ORIGIN complete:"
Write-Host "  $MetricsCsv"
Write-Host "  $Report"

if ($Strict) {
  $pp = D $PlantedM.pattern_edge_accuracy
  $cp = D $ControlM.pattern_edge_accuracy
  $score = D $PlantedM.plant_like_score
  if ($pp -le $cp) { Fail "Strict fail: planted pattern_edge_accuracy <= control." }
  if ($score -le 0.10) { Fail "Strict fail: plant_like_score too low." }
}
