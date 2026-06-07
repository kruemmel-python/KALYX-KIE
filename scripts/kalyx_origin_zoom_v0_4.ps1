
param(
  [Parameter(Mandatory=$true)][string]$Symbols,
  [Parameter(Mandatory=$true)][string]$OutDir,
  [UInt64]$StartSymbols = 0,
  [UInt64]$WindowSymbols = 262144,
  [UInt64]$StepSymbols = 262144,
  [int]$Windows = 32,
  [string]$CompareStream = "",
  [string]$CompareLabel = "compare",
  [string]$Control = "",
  [string]$Pattern = "",
  [string]$Positions = "",
  [string]$PlantManifest = "",
  [switch]$Strict
)

$ErrorActionPreference = "Stop"

function Fail($m) {
  if ($Strict) { throw $m }
  Write-Error $m
}

function ToD($x) {
  if ($null -eq $x -or "$x" -eq "") { return 0.0 }
  return [double]::Parse("$x", [System.Globalization.CultureInfo]::InvariantCulture)
}

function Mean($xs) {
  if ($xs.Count -eq 0) { return 0.0 }
  $s = 0.0
  foreach ($x in $xs) { $s += [double]$x }
  return $s / [double]$xs.Count
}

function Std($xs) {
  if ($xs.Count -le 1) { return 0.0 }
  $m = Mean $xs
  $s = 0.0
  foreach ($x in $xs) { $d = [double]$x - $m; $s += $d * $d }
  return [math]::Sqrt($s / [double]($xs.Count - 1))
}

function ClampPos($x) {
  if ($x -lt 0) { return 0.0 }
  return [double]$x
}

if (-not (Test-Path $Symbols)) { Fail "Symbols fehlt: $Symbols" }
$Exe = ".\build_vs\Release\kalyx_origin.exe"
if (-not (Test-Path $Exe)) { Fail "kalyx_origin.exe fehlt: $Exe" }

$TotalSymbols = [UInt64]((Get-Item $Symbols).Length / 8)
if ($TotalSymbols -lt 2) { Fail "zu wenig Symbole: $TotalSymbols" }

New-Item -ItemType Directory -Force $OutDir | Out-Null

$MetricsCsv = Join-Path $OutDir "kalyx_origin_v0_4_metrics.csv"
$BaselineCsv = Join-Path $OutDir "kalyx_origin_v0_4_baseline.csv"
$RankingCsv = Join-Path $OutDir "kalyx_origin_v0_4_ranking.csv"
$Report = Join-Path $OutDir "KALYX_ORIGIN_V0_4_REPORT.md"

Remove-Item -Force $MetricsCsv,$BaselineCsv,$RankingCsv,$Report -ErrorAction SilentlyContinue

Write-Host "=== KALYX-ORIGIN v0.4: zoom/local patternless scan ==="
Write-Host "symbols=$Symbols total=$TotalSymbols start=$StartSymbols window=$WindowSymbols step=$StepSymbols windows=$Windows"

for ($i = 0; $i -lt $Windows; $i++) {
  $skip = [UInt64]($StartSymbols + ([UInt64]$i * $StepSymbols))
  if ($skip + $WindowSymbols -gt $TotalSymbols) {
    Write-Host "Stop: window beyond EOF index=$i skip=$skip"
    break
  }

  $label = ("window_{0:D4}" -f $i)
  & $Exe `
    --in $Symbols `
    --label $label `
    --out-csv $MetricsCsv `
    --max-symbols "$WindowSymbols" `
    --skip-symbols "$skip" `
    --append

  if ($LASTEXITCODE -ne 0) { Fail "kalyx_origin failed for $label" }
}

if ($CompareStream -ne "") {
  if (-not (Test-Path $CompareStream)) { Fail "CompareStream fehlt: $CompareStream" }
  $args = @(
    "--in", $CompareStream,
    "--label", $CompareLabel,
    "--out-csv", $MetricsCsv,
    "--max-symbols", "$WindowSymbols",
    "--append"
  )
  if ($Control -ne "") { $args += @("--control", $Control) }
  if ($Pattern -ne "") { $args += @("--pattern", $Pattern) }
  if ($Positions -ne "") { $args += @("--positions", $Positions) }
  if ($PlantManifest -ne "") { $args += @("--plant-manifest", $PlantManifest) }
  & $Exe @args
  if ($LASTEXITCODE -ne 0) { Fail "kalyx_origin failed for compare stream" }
}

$Rows = Import-Csv $MetricsCsv
$WindowRows = @($Rows | Where-Object { $_.label -like "window_*" })
if ($WindowRows.Count -lt 2) { Fail "zu wenig Fenster für Baseline: $($WindowRows.Count)" }

$Metrics = @(
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

$Stats = @{}
$BaseLines = @()
$BaseLines += '"metric","n","mean","std","min","max"'

foreach ($m in $Metrics) {
  $vals = @($WindowRows | ForEach-Object { ToD $_.$m })
  $mean = Mean $vals
  $std = Std $vals
  $min = ($vals | Measure-Object -Minimum).Minimum
  $max = ($vals | Measure-Object -Maximum).Maximum
  $Stats[$m] = [pscustomobject]@{ mean=$mean; std=$std; min=$min; max=$max }
  $BaseLines += ('"{0}","{1}","{2:F12}","{3:F12}","{4:F12}","{5:F12}"' -f $m,$vals.Count,$mean,$std,$min,$max)
}
Set-Content -Encoding UTF8 $BaselineCsv $BaseLines

$Ranked = foreach ($r in $WindowRows) {
  $entropy = ToD $r.entropy_bits
  $top32 = ToD $r.top32_mass
  $edgeEntropy = ToD $r.edge_entropy_bits
  $edgeTop32 = ToD $r.edge_top32_mass
  $uniqueSymbols = ToD $r.unique_symbols
  $uniqueEdges = ToD $r.unique_edges

  $ze = if ($Stats["entropy_bits"].std -gt 0) { ($Stats["entropy_bits"].mean - $entropy) / $Stats["entropy_bits"].std } else { 0 }
  $zee = if ($Stats["edge_entropy_bits"].std -gt 0) { ($Stats["edge_entropy_bits"].mean - $edgeEntropy) / $Stats["edge_entropy_bits"].std } else { 0 }
  $zt = if ($Stats["top32_mass"].std -gt 0) { ($top32 - $Stats["top32_mass"].mean) / $Stats["top32_mass"].std } else { 0 }
  $zet = if ($Stats["edge_top32_mass"].std -gt 0) { ($edgeTop32 - $Stats["edge_top32_mass"].mean) / $Stats["edge_top32_mass"].std } else { 0 }
  $zus = if ($Stats["unique_symbols"].std -gt 0) { ($Stats["unique_symbols"].mean - $uniqueSymbols) / $Stats["unique_symbols"].std } else { 0 }
  $zue = if ($Stats["unique_edges"].std -gt 0) { ($Stats["unique_edges"].mean - $uniqueEdges) / $Stats["unique_edges"].std } else { 0 }

  $score = ((ClampPos $ze) + (ClampPos $zee) + (ClampPos $zt) + (ClampPos $zet) + (ClampPos $zus) + (ClampPos $zue)) / 6.0

  [pscustomobject]@{
    label = $r.label
    skip_symbols = [UInt64]$r.skip_symbols
    fnv1a64 = $r.fnv1a64
    entropy_bits = $entropy
    top32_mass = $top32
    edge_entropy_bits = $edgeEntropy
    edge_top32_mass = $edgeTop32
    unique_symbols = [UInt64]$r.unique_symbols
    unique_edges = [UInt64]$r.unique_edges
    plant_like_score = ToD $r.plant_like_score
    z_entropy_low = $ze
    z_edge_entropy_low = $zee
    z_top32_high = $zt
    z_edge_top32_high = $zet
    z_unique_symbols_low = $zus
    z_unique_edges_low = $zue
    origin_anomaly_score = $score
  }
}

$RankedSorted = @($Ranked | Sort-Object origin_anomaly_score -Descending)
$RankedSorted | Export-Csv -NoTypeInformation -Encoding UTF8 $RankingCsv

$Top = @($RankedSorted | Select-Object -First 15)

$Lines = @()
$Lines += "# KALYX-ORIGIN v0.4 Report"
$Lines += ""
$Lines += "Zoom/local patternless origin/anomaly scan."
$Lines += ""
$Lines += "## Inputs"
$Lines += ""
$Lines += "- Symbols: ``$Symbols``"
$Lines += "- TotalSymbols: ``$TotalSymbols``"
$Lines += "- StartSymbols: ``$StartSymbols``"
$Lines += "- WindowSymbols: ``$WindowSymbols``"
$Lines += "- StepSymbols: ``$StepSymbols``"
$Lines += "- Windows requested: ``$Windows``"
$Lines += "- Windows scanned: ``$($WindowRows.Count)``"
$Lines += ""
$Lines += "## Method"
$Lines += ""
$Lines += "v0.4 adds explicit zoom support via StartSymbols and independent control/pattern comparison."
$Lines += "It ranks windows by entropy drop, edge-entropy drop, top-symbol concentration,"
$Lines += "top-edge concentration, unique-symbol reduction and unique-edge reduction."
$Lines += ""
$Lines += "This is not final origin proof. It is a local Genesis-diagnostic ranking."
$Lines += ""
$Lines += "## Top Origin-Anomaly Windows"
$Lines += ""
$Lines += "| Rank | Label | Skip | Score | Entropy | Top32 | EdgeEntropy | EdgeTop32 | UniqueSymbols | UniqueEdges |"
$Lines += "|---:|---|---:|---:|---:|---:|---:|---:|---:|---:|"
$rank = 1
foreach ($r in $Top) {
  $Lines += ("| {0} | {1} | {2} | {3:F6} | {4:F12} | {5:F12} | {6:F12} | {7:F12} | {8} | {9} |" -f `
    $rank,$r.label,$r.skip_symbols,$r.origin_anomaly_score,$r.entropy_bits,$r.top32_mass,$r.edge_entropy_bits,$r.edge_top32_mass,$r.unique_symbols,$r.unique_edges)
  $rank++
}

$Lines += ""
$Lines += "## Baseline"
$Lines += ""
$Lines += "| Metric | N | Mean | Std | Min | Max |"
$Lines += "|---|---:|---:|---:|---:|---:|"
foreach ($m in $Metrics) {
  $s = $Stats[$m]
  $Lines += ("| {0} | {1} | {2:F12} | {3:F12} | {4:F12} | {5:F12} |" -f $m,$WindowRows.Count,$s.mean,$s.std,$s.min,$s.max)
}

if ($CompareStream -ne "") {
  $CompareRows = @($Rows | Where-Object { $_.label -eq $CompareLabel })
  if ($CompareRows.Count -gt 0) {
    $c = $CompareRows[0]
    $Lines += ""
    $Lines += "## Compare Stream"
    $Lines += ""
    $Lines += "| Label | Entropy | Top32 | EdgeEntropy | EdgeTop32 | PatternEdge | ControlHamming | PlantLike |"
    $Lines += "|---|---:|---:|---:|---:|---:|---:|---:|"
    $Lines += ("| {0} | {1:F12} | {2:F12} | {3:F12} | {4:F12} | {5:F12} | {6:F12} | {7:F12} |" -f `
      $c.label,(ToD $c.entropy_bits),(ToD $c.top32_mass),(ToD $c.edge_entropy_bits),(ToD $c.edge_top32_mass),(ToD $c.pattern_edge_accuracy),(ToD $c.control_hamming),(ToD $c.plant_like_score))
  }
}

$Lines += ""
$Lines += "## Artefacts"
$Lines += ""
$Lines += "- ``$MetricsCsv``"
$Lines += "- ``$BaselineCsv``"
$Lines += "- ``$RankingCsv``"
$Lines += "- ``$Report``"
$Lines += ""
$Lines += "## Interpretation Boundary"
$Lines += ""
$Lines += "Structure is not origin. A high anomaly score means a window deviates from its local scan baseline under these diagnostics. It does not alone prove artificial generation."
Set-Content -Encoding UTF8 $Report $Lines

Write-Host "KALYX-ORIGIN v0.4 complete:"
Write-Host "  $Report"
Write-Host "  $MetricsCsv"
Write-Host "  $BaselineCsv"
Write-Host "  $RankingCsv"
