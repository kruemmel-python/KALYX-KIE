
param(
  [Parameter(Mandatory=$true)][string]$Symbols,
  [Parameter(Mandatory=$true)][string]$OutDir,

  [UInt64]$WindowSymbols = 1048576,
  [UInt64]$StepSymbols = 1048576,
  [int]$Windows = 32,
  [string]$LabelPrefix = "natural",

  [string]$CompareStream = "",
  [string]$CompareLabel = "compare",
  [string]$Control = "",
  [string]$Pattern = "",
  [string]$Positions = "",
  [string]$PlantManifest = "",

  [string]$Exe = ".\build_vs\Release\kalyx_origin.exe",
  [switch]$Strict
)

$ErrorActionPreference = "Stop"

function Fail($m) { throw $m }

if (-not (Test-Path $Symbols)) { Fail "Symbols fehlt: $Symbols" }
if (-not (Test-Path $Exe)) { Fail "kalyx_origin.exe fehlt: $Exe" }

New-Item -ItemType Directory -Force $OutDir | Out-Null

$Metrics = Join-Path $OutDir "kalyx_origin_v0_3_metrics.csv"
$Ranking = Join-Path $OutDir "kalyx_origin_v0_3_ranking.csv"
$Baseline = Join-Path $OutDir "kalyx_origin_v0_3_baseline.csv"
$Report = Join-Path $OutDir "KALYX_ORIGIN_V0_3_REPORT.md"

Remove-Item -Force $Metrics,$Ranking,$Baseline,$Report -ErrorAction SilentlyContinue

$TotalSymbols = [UInt64]((Get-Item $Symbols).Length / 8)
if ($TotalSymbols -lt $WindowSymbols) { Fail "Symbols zu klein: total=$TotalSymbols window=$WindowSymbols" }

$PossibleWindows = [int]([Math]::Floor([double](($TotalSymbols - $WindowSymbols) / $StepSymbols))) + 1
if ($Windows -le 0 -or $Windows -gt $PossibleWindows) { $Windows = $PossibleWindows }

Write-Host "=== KALYX-ORIGIN v0.3: patternless natural window scan ==="
Write-Host "symbols=$Symbols total=$TotalSymbols window=$WindowSymbols step=$StepSymbols windows=$Windows"

for ($i = 0; $i -lt $Windows; $i++) {
  $Skip = [UInt64]($i) * $StepSymbols
  $Label = "{0}_{1:D4}" -f $LabelPrefix,$i

  & $Exe `
    --in $Symbols `
    --label $Label `
    --out-csv $Metrics `
    --max-symbols $WindowSymbols `
    --skip-symbols $Skip `
    --append

  if ($LASTEXITCODE -ne 0) { Fail "kalyx_origin fehlgeschlagen für $Label" }
}

if ($CompareStream -ne "") {
  if (-not (Test-Path $CompareStream)) { Fail "CompareStream fehlt: $CompareStream" }

  Write-Host "=== KALYX-ORIGIN v0.3: optional compare stream ==="

  $Args = @(
    "--in", $CompareStream,
    "--label", $CompareLabel,
    "--out-csv", $Metrics,
    "--max-symbols", "$WindowSymbols",
    "--append"
  )

  if ($Control -ne "") {
    if (-not (Test-Path $Control)) { Fail "Control fehlt: $Control" }
    $Args += "--control"; $Args += $Control
  }
  if ($Pattern -ne "") {
    if (-not (Test-Path $Pattern)) { Fail "Pattern fehlt: $Pattern" }
    $Args += "--pattern"; $Args += $Pattern
  }
  if ($Positions -ne "") {
    if (-not (Test-Path $Positions)) { Fail "Positions fehlt: $Positions" }
    $Args += "--positions"; $Args += $Positions
  }
  if ($PlantManifest -ne "") {
    if (-not (Test-Path $PlantManifest)) { Fail "PlantManifest fehlt: $PlantManifest" }
    $Args += "--plant-manifest"; $Args += $PlantManifest
  }

  & $Exe @Args
  if ($LASTEXITCODE -ne 0) { Fail "kalyx_origin fehlgeschlagen für CompareStream" }
}

$Rows = Import-Csv $Metrics
$Natural = @($Rows | Where-Object { $_.label -like "$LabelPrefix`_*" })
if ($Natural.Count -lt 2) { Fail "Zu wenige natürliche Fenster für Baseline: $($Natural.Count)" }

$MetricNames = @(
  "entropy_bits",
  "top32_mass",
  "edge_entropy_bits",
  "edge_top32_mass",
  "unique_symbols",
  "unique_edges",
  "plant_like_score",
  "origin_concentration_score",
  "origin_order_score"
)

function ToD($x) {
  $s = [string]$x
  $s = $s.Replace(",", ".")
  return [double]::Parse($s, [System.Globalization.CultureInfo]::InvariantCulture)
}

$BaselineRows = @()
foreach ($M in $MetricNames) {
  $Vals = @($Natural | ForEach-Object { ToD $_.$M })
  $Mean = ($Vals | Measure-Object -Average).Average
  $Min = ($Vals | Measure-Object -Minimum).Minimum
  $Max = ($Vals | Measure-Object -Maximum).Maximum
  $Var = 0.0
  foreach ($V in $Vals) { $Var += ($V - $Mean) * ($V - $Mean) }
  $Std = [Math]::Sqrt($Var / [Math]::Max(1, $Vals.Count))

  $BaselineRows += [pscustomobject]@{
    metric = $M
    n = $Vals.Count
    mean = "{0:F12}" -f $Mean
    std = "{0:F12}" -f $Std
    min = "{0:F12}" -f $Min
    max = "{0:F12}" -f $Max
  }
}
$BaselineRows | Export-Csv -NoTypeInformation -Encoding UTF8 $Baseline

$B = @{}
foreach ($R in $BaselineRows) {
  $B[$R.metric] = @{
    mean = ToD $R.mean
    std = ToD $R.std
    min = ToD $R.min
    max = ToD $R.max
  }
}

$RankRows = @()
foreach ($R in $Rows) {
  $Entropy = ToD $R.entropy_bits
  $Top32 = ToD $R.top32_mass
  $EdgeEntropy = ToD $R.edge_entropy_bits
  $EdgeTop32 = ToD $R.edge_top32_mass
  $PlantLike = ToD $R.plant_like_score
  $UniqueSymbols = ToD $R.unique_symbols
  $UniqueEdges = ToD $R.unique_edges

  $z_entropy_low = 0.0
  if ($B["entropy_bits"].std -gt 0) { $z_entropy_low = ($B["entropy_bits"].mean - $Entropy) / $B["entropy_bits"].std }

  $z_edge_entropy_low = 0.0
  if ($B["edge_entropy_bits"].std -gt 0) { $z_edge_entropy_low = ($B["edge_entropy_bits"].mean - $EdgeEntropy) / $B["edge_entropy_bits"].std }

  $z_top32_high = 0.0
  if ($B["top32_mass"].std -gt 0) { $z_top32_high = ($Top32 - $B["top32_mass"].mean) / $B["top32_mass"].std }

  $z_edge_top32_high = 0.0
  if ($B["edge_top32_mass"].std -gt 0) { $z_edge_top32_high = ($EdgeTop32 - $B["edge_top32_mass"].mean) / $B["edge_top32_mass"].std }

  $z_unique_symbols_low = 0.0
  if ($B["unique_symbols"].std -gt 0) { $z_unique_symbols_low = ($B["unique_symbols"].mean - $UniqueSymbols) / $B["unique_symbols"].std }

  $z_unique_edges_low = 0.0
  if ($B["unique_edges"].std -gt 0) { $z_unique_edges_low = ($B["unique_edges"].mean - $UniqueEdges) / $B["unique_edges"].std }

  $Score = 0.0
  foreach ($Z in @($z_entropy_low, $z_edge_entropy_low, $z_top32_high, $z_edge_top32_high, $z_unique_symbols_low, $z_unique_edges_low)) {
    if ($Z -gt 0) { $Score += $Z }
  }
  $Score = $Score / 6.0

  $RankRows += [pscustomobject]@{
    label = $R.label
    skip_symbols = $R.skip_symbols
    fnv1a64 = $R.fnv1a64
    entropy_bits = "{0:F12}" -f $Entropy
    top32_mass = "{0:F12}" -f $Top32
    edge_entropy_bits = "{0:F12}" -f $EdgeEntropy
    edge_top32_mass = "{0:F12}" -f $EdgeTop32
    unique_symbols = "{0:F0}" -f $UniqueSymbols
    unique_edges = "{0:F0}" -f $UniqueEdges
    plant_like_score = "{0:F12}" -f $PlantLike
    z_entropy_low = "{0:F6}" -f $z_entropy_low
    z_edge_entropy_low = "{0:F6}" -f $z_edge_entropy_low
    z_top32_high = "{0:F6}" -f $z_top32_high
    z_edge_top32_high = "{0:F6}" -f $z_edge_top32_high
    z_unique_symbols_low = "{0:F6}" -f $z_unique_symbols_low
    z_unique_edges_low = "{0:F6}" -f $z_unique_edges_low
    origin_anomaly_score = "{0:F6}" -f $Score
  }
}

$RankRows |
  Sort-Object { ToD $_.origin_anomaly_score } -Descending |
  Export-Csv -NoTypeInformation -Encoding UTF8 $Ranking

$Top = @($RankRows | Sort-Object { ToD $_.origin_anomaly_score } -Descending | Select-Object -First 10)

$Lines = @()
$Lines += "# KALYX-ORIGIN v0.3 Report"
$Lines += ""
$Lines += "Patternless local-window origin/anomaly scan."
$Lines += ""
$Lines += "## Inputs"
$Lines += ""
$Lines += "- Symbols: $Symbols"
$Lines += "- WindowSymbols: $WindowSymbols"
$Lines += "- StepSymbols: $StepSymbols"
$Lines += "- Windows: $Windows"
if ($CompareStream -ne "") { $Lines += "- CompareStream: $CompareStream" }
$Lines += ""
$Lines += "## Method"
$Lines += ""
$Lines += "v0.3 builds a natural baseline envelope from local windows of the same stream."
$Lines += "Each window is scored using transparent diagnostics: entropy drop, edge-entropy drop,"
$Lines += "top-symbol concentration, top-edge concentration, unique-symbol reduction and unique-edge reduction."
$Lines += ""
$Lines += "This is not final origin proof. It is an anomaly-ranking layer for genesis diagnostics."
$Lines += ""
$Lines += "## Top Origin-Anomaly Windows"
$Lines += ""
$Lines += "| Rank | Label | Skip | Score | Entropy | Top32 | EdgeEntropy | EdgeTop32 | UniqueSymbols | UniqueEdges |"
$Lines += "|---:|---|---:|---:|---:|---:|---:|---:|---:|---:|"
$rank = 1
foreach ($R in $Top) {
  $Lines += "| $rank | $($R.label) | $($R.skip_symbols) | $($R.origin_anomaly_score) | $($R.entropy_bits) | $($R.top32_mass) | $($R.edge_entropy_bits) | $($R.edge_top32_mass) | $($R.unique_symbols) | $($R.unique_edges) |"
  $rank++
}
$Lines += ""
$Lines += "## Baseline"
$Lines += ""
$Lines += "| Metric | N | Mean | Std | Min | Max |"
$Lines += "|---|---:|---:|---:|---:|---:|"
foreach ($R in $BaselineRows) {
  $Lines += "| $($R.metric) | $($R.n) | $($R.mean) | $($R.std) | $($R.min) | $($R.max) |"
}
$Lines += ""
$Lines += "## Artefacts"
$Lines += ""
$Lines += "- $Metrics"
$Lines += "- $Baseline"
$Lines += "- $Ranking"
$Lines += "- $Report"
$Lines += ""
$Lines += "## Interpretation Boundary"
$Lines += ""
$Lines += "Structure is not origin. A high anomaly score means a window deviates from the local natural baseline"
$Lines += "under these diagnostics. It does not alone prove artificial generation."

Set-Content -Encoding UTF8 $Report $Lines

Write-Host "KALYX-ORIGIN v0.3 complete:"
Write-Host "  $Report"
Write-Host "  $Metrics"
Write-Host "  $Baseline"
Write-Host "  $Ranking"

if ($Strict) {
  if (-not (Test-Path $Report)) { Fail "Report fehlt: $Report" }
  if (-not (Test-Path $Ranking)) { Fail "Ranking fehlt: $Ranking" }
}
