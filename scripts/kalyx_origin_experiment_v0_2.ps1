
param(
  [Parameter(Mandatory=$true)][string]$PlantOutDir,
  [string]$NaturalSymbols = "",
  [string]$OutDir = "",
  [UInt64]$MaxSymbols = 1048576,
  [UInt64]$WindowSymbols = 1048576,
  [int]$NaturalWindows = 8,
  [string]$Exe = ".\build_vs\Release\kalyx_origin.exe",
  [switch]$Strict
)

$ErrorActionPreference = "Stop"
$culture = [System.Globalization.CultureInfo]::InvariantCulture

function Fail([string]$m) {
  if ($Strict) { throw $m }
  Write-Warning $m
}

function ToD([object]$v) {
  if ($null -eq $v) { return 0.0 }
  $s = [string]$v
  if ([string]::IsNullOrWhiteSpace($s)) { return 0.0 }
  return [double]::Parse($s, $culture)
}

function Mean([double[]]$xs) {
  if ($xs.Count -eq 0) { return 0.0 }
  $sum = 0.0
  foreach ($x in $xs) { $sum += $x }
  return $sum / [double]$xs.Count
}

function Std([double[]]$xs) {
  if ($xs.Count -lt 2) { return 0.0 }
  $m = Mean $xs
  $s = 0.0
  foreach ($x in $xs) { $d = $x - $m; $s += $d * $d }
  return [Math]::Sqrt($s / [double]($xs.Count - 1))
}

function MinV([double[]]$xs) {
  if ($xs.Count -eq 0) { return 0.0 }
  $m = $xs[0]
  foreach ($x in $xs) { if ($x -lt $m) { $m = $x } }
  return $m
}

function MaxV([double[]]$xs) {
  if ($xs.Count -eq 0) { return 0.0 }
  $m = $xs[0]
  foreach ($x in $xs) { if ($x -gt $m) { $m = $x } }
  return $m
}

function Fmt([double]$x) {
  return $x.ToString("0.000000000", $culture)
}

if (-not (Test-Path $Exe)) { throw "kalyx_origin.exe fehlt: $Exe" }
if (-not (Test-Path $PlantOutDir)) { throw "PlantOutDir fehlt: $PlantOutDir" }

$PlantOutDir = (Get-Item $PlantOutDir).FullName
if ([string]::IsNullOrWhiteSpace($OutDir)) {
  $OutDir = Join-Path $PlantOutDir "OriginV02"
}
New-Item -ItemType Directory -Force $OutDir | Out-Null
$OutDir = (Get-Item $OutDir).FullName

$Control = Join-Path $PlantOutDir "carrier_control.u64"
$Planted = Join-Path $PlantOutDir "carrier_planted.u64"
$Pattern = Join-Path $PlantOutDir "plant_pattern.u64"
$Positions = Join-Path $PlantOutDir "plant_positions.csv"
$Manifest = Join-Path $PlantOutDir "plant_manifest.csv"

foreach ($p in @($Control,$Planted,$Pattern,$Manifest)) {
  if (-not (Test-Path $p)) { throw "Fehlt: $p" }
}
if (-not (Test-Path $Positions)) { $Positions = "" }

$Metrics = Join-Path $OutDir "kalyx_origin_metrics.csv"
$Baseline = Join-Path $OutDir "kalyx_origin_natural_baseline.csv"
$Report = Join-Path $OutDir "KALYX_ORIGIN_V0_2_REPORT.md"
Remove-Item -Force $Metrics,$Baseline,$Report -ErrorAction SilentlyContinue

function Run-Origin(
  [string]$Label,
  [string]$InputPath,
  [UInt64]$Skip,
  [string]$ControlPath,
  [string]$PatternPath,
  [string]$PositionsPath,
  [string]$ManifestPath
) {
  $args = @(
    "--in", $InputPath,
    "--label", $Label,
    "--out-csv", $Metrics,
    "--max-symbols", "$MaxSymbols",
    "--skip-symbols", "$Skip",
    "--append"
  )
  if (-not [string]::IsNullOrWhiteSpace($ControlPath)) {
    $args += "--control"; $args += $ControlPath
  }
  if (-not [string]::IsNullOrWhiteSpace($PatternPath)) {
    $args += "--pattern"; $args += $PatternPath
  }
  if (-not [string]::IsNullOrWhiteSpace($PositionsPath)) {
    $args += "--positions"; $args += $PositionsPath
  }
  if (-not [string]::IsNullOrWhiteSpace($ManifestPath)) {
    $args += "--plant-manifest"; $args += $ManifestPath
  }
  & $Exe @args
  if ($LASTEXITCODE -ne 0) { throw "kalyx_origin fehlgeschlagen: $Label" }
}

Write-Host "=== KALYX-ORIGIN v0.2: PLANT controls ==="

Run-Origin -Label "plant_control" -InputPath $Control -Skip 0 -ControlPath $Control -PatternPath $Pattern -PositionsPath "" -ManifestPath ""
Run-Origin -Label "plant_planted" -InputPath $Planted -Skip 0 -ControlPath $Control -PatternPath $Pattern -PositionsPath $Positions -ManifestPath $Manifest
Run-Origin -Label "plant_pattern" -InputPath $Pattern -Skip 0 -ControlPath "" -PatternPath $Pattern -PositionsPath "" -ManifestPath ""

$naturalCount = 0
if (-not [string]::IsNullOrWhiteSpace($NaturalSymbols)) {
  if (-not (Test-Path $NaturalSymbols)) { throw "NaturalSymbols fehlt: $NaturalSymbols" }
  $NaturalSymbols = (Get-Item $NaturalSymbols).FullName
  $totalWords = [UInt64]((Get-Item $NaturalSymbols).Length / 8)

  Write-Host "=== KALYX-ORIGIN v0.2: natural baseline windows ==="
  for ($i = 0; $i -lt $NaturalWindows; $i++) {
    $skip = [UInt64]($i * [int64]$WindowSymbols)
    if ($skip -ge $totalWords) { break }
    $remain = $totalWords - $skip
    if ($remain -lt 2) { break }

    $label = "natural_" + $i.ToString("000")
    Run-Origin -Label $label -InputPath $NaturalSymbols -Skip $skip -ControlPath "" -PatternPath $Pattern -PositionsPath "" -ManifestPath ""
    $naturalCount++
  }
}

$rows = Import-Csv $Metrics
$naturalRows = @($rows | Where-Object { $_.label -like "natural_*" })
$plantedRow = $rows | Where-Object { $_.label -eq "plant_planted" } | Select-Object -First 1
$controlRow = $rows | Where-Object { $_.label -eq "plant_control" } | Select-Object -First 1
$patternRow = $rows | Where-Object { $_.label -eq "plant_pattern" } | Select-Object -First 1

$metricsToSummarize = @(
  "entropy_bits",
  "top32_mass",
  "edge_entropy_bits",
  "edge_top32_mass",
  "pattern_edge_accuracy",
  "plant_like_score",
  "unique_symbols",
  "unique_edges"
)

$baseLines = @()
$baseLines += "metric,n,mean,std,min,max,planted,planted_z,planted_over_max"

foreach ($m in $metricsToSummarize) {
  [double[]]$vals = @()
  foreach ($r in $naturalRows) { $vals += (ToD $r.$m) }
  $mean = Mean $vals
  $std = Std $vals
  $min = MinV $vals
  $max = MaxV $vals
  $p = ToD $plantedRow.$m
  $z = 0.0
  if ($std -gt 0.0) { $z = ($p - $mean) / $std }
  $over = $p - $max
  $baseLines += (($m,$vals.Count,(Fmt $mean),(Fmt $std),(Fmt $min),(Fmt $max),(Fmt $p),(Fmt $z),(Fmt $over)) -join ",")
}

Set-Content -Encoding UTF8 $Baseline $baseLines

# Heuristic origin flags.
$plantedPattern = ToD $plantedRow.pattern_edge_accuracy
$controlPattern = ToD $controlRow.pattern_edge_accuracy
$plantedLike = ToD $plantedRow.plant_like_score
$controlLike = ToD $controlRow.plant_like_score
$plantedHamming = ToD $plantedRow.control_hamming
$plantedDensity = ToD $plantedRow.position_density

$natPatternVals = @()
$natLikeVals = @()
foreach ($r in $naturalRows) {
  $natPatternVals += (ToD $r.pattern_edge_accuracy)
  $natLikeVals += (ToD $r.plant_like_score)
}
$natPatternMax = MaxV $natPatternVals
$natLikeMax = MaxV $natLikeVals

$passPlantControl = ($plantedPattern -gt ($controlPattern + 0.05))
$passNaturalEnvelope = $true
if ($naturalRows.Count -gt 0) {
  $passNaturalEnvelope = ($plantedPattern -gt ($natPatternMax + 0.05)) -and ($plantedLike -gt ($natLikeMax + 0.05))
}

$reportLines = @()
$reportLines += "# KALYX-ORIGIN v0.2 Report"
$reportLines += ""
$reportLines += "Natural-baseline origin diagnostics for declared PLANT controls."
$reportLines += ""
$reportLines += "## Result"
$reportLines += ""
$reportLines += "- Plant-vs-control diagnostic pass: " + $passPlantControl
$reportLines += "- Natural-envelope diagnostic pass: " + $passNaturalEnvelope
$reportLines += "- Natural windows: " + $naturalRows.Count
$reportLines += ""
$reportLines += "## Inputs"
$reportLines += ""
$reportLines += "- PlantOutDir: " + $PlantOutDir
if (-not [string]::IsNullOrWhiteSpace($NaturalSymbols)) { $reportLines += "- NaturalSymbols: " + $NaturalSymbols }
$reportLines += "- MaxSymbols: " + $MaxSymbols
$reportLines += "- WindowSymbols: " + $WindowSymbols
$reportLines += ""
$reportLines += "## Key PLANT Diagnostics"
$reportLines += ""
$reportLines += "| Stream | Entropy | Top32 mass | Pattern-edge accuracy | Control hamming | Position density | Plant-like score |"
$reportLines += "|---|---:|---:|---:|---:|---:|---:|"
foreach ($r in @($controlRow,$plantedRow,$patternRow)) {
  if ($null -ne $r) {
    $reportLines += "| " + $r.label + " | " + (Fmt (ToD $r.entropy_bits)) + " | " + (Fmt (ToD $r.top32_mass)) + " | " + (Fmt (ToD $r.pattern_edge_accuracy)) + " | " + (Fmt (ToD $r.control_hamming)) + " | " + (Fmt (ToD $r.position_density)) + " | " + (Fmt (ToD $r.plant_like_score)) + " |"
  }
}
$reportLines += ""

if ($naturalRows.Count -gt 0) {
  $reportLines += "## Natural Baseline Envelope"
  $reportLines += ""
  $reportLines += "| Metric | N | Mean | Std | Min | Max | Planted | Planted z | Planted - max |"
  $reportLines += "|---|---:|---:|---:|---:|---:|---:|---:|---:|"
  foreach ($b in (Import-Csv $Baseline)) {
    $reportLines += "| " + $b.metric + " | " + $b.n + " | " + $b.mean + " | " + $b.std + " | " + $b.min + " | " + $b.max + " | " + $b.planted + " | " + $b.planted_z + " | " + $b.planted_over_max + " |"
  }
  $reportLines += ""
}

$reportLines += "## Interpretation"
$reportLines += ""
$reportLines += "KALYX-ORIGIN v0.2 does not prove natural or artificial origin. It builds a diagnostic envelope. A declared PLANT control should separate from both the unmodified control and natural baseline windows."
$reportLines += ""
$reportLines += "The core rule remains: Structure is not origin. KALYX detects transition order; ORIGIN adds genesis-relevant diagnostics."
$reportLines += ""
$reportLines += "## Artefacts"
$reportLines += ""
$reportLines += "- " + $Metrics
$reportLines += "- " + $Baseline
$reportLines += "- " + $Report

Set-Content -Encoding UTF8 $Report $reportLines

Write-Host "KALYX-ORIGIN v0.2 complete:"
Write-Host "  $Report"
Write-Host "  $Metrics"
Write-Host "  $Baseline"
