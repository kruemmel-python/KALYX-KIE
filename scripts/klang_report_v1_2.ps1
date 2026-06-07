param(
  [string]$OutDir = ".\LangOut",
  [string]$MatrixCsv = "",
  [string]$NullCsv = "",
  [string]$ReportDir = "",
  [string]$Title = "KLANG_REPORT v1.2"
)

$ErrorActionPreference = "Stop"

function To-D([object]$v) {
  if ($null -eq $v) { return [double]::NaN }
  $s = ([string]$v).Trim()
  if ($s.Length -eq 0) { return [double]::NaN }
  $s = $s.Replace(",", ".")
  return [double]::Parse($s, [System.Globalization.CultureInfo]::InvariantCulture)
}

function To-U64([object]$v) {
  if ($null -eq $v) { return [UInt64]0 }
  $s = ([string]$v).Trim()
  if ($s.Length -eq 0) { return [UInt64]0 }
  return [UInt64]::Parse($s, [System.Globalization.CultureInfo]::InvariantCulture)
}

function R6([double]$v) {
  if ([double]::IsNaN($v)) { return "nan" }
  return $v.ToString("0.000000", [System.Globalization.CultureInfo]::InvariantCulture)
}

function R12([double]$v) {
  if ([double]::IsNaN($v)) { return "nan" }
  return $v.ToString("0.000000000000", [System.Globalization.CultureInfo]::InvariantCulture)
}

function MeanD($values) {
  $arr = @($values | Where-Object { -not [double]::IsNaN([double]$_) })
  if ($arr.Count -eq 0) { return [double]::NaN }
  $sum = 0.0
  foreach ($x in $arr) { $sum += [double]$x }
  return $sum / [double]$arr.Count
}

function Ensure-Dir([string]$p) {
  if (-not (Test-Path $p)) { New-Item -ItemType Directory -Force -Path $p | Out-Null }
}

if (-not (Test-Path $OutDir)) { throw "OutDir fehlt: $OutDir" }

if ($MatrixCsv.Trim().Length -eq 0) {
  $candidates = Get-ChildItem $OutDir -Filter "klang_*_w*.csv" -File |
    Where-Object { $_.Name -notlike "*_null.csv" } |
    Sort-Object LastWriteTime -Descending
  if ($candidates.Count -eq 0) { throw "Keine KLANG-Matrix-CSV in $OutDir gefunden." }
  $MatrixCsv = $candidates[0].FullName
}

if ($NullCsv.Trim().Length -eq 0) {
  $baseName = [System.IO.Path]::GetFileNameWithoutExtension($MatrixCsv)
  $candidate = Join-Path $OutDir ($baseName + "_null.csv")
  if (Test-Path $candidate) { $NullCsv = $candidate }
  else {
    $candidates = Get-ChildItem $OutDir -Filter "klang_*_w*_null.csv" -File | Sort-Object LastWriteTime -Descending
    if ($candidates.Count -gt 0) { $NullCsv = $candidates[0].FullName }
  }
}

if ($ReportDir.Trim().Length -eq 0) { $ReportDir = Join-Path $OutDir "ReportV12" }
Ensure-Dir $ReportDir
Ensure-Dir (Join-Path $ReportDir "tables")
Ensure-Dir (Join-Path $ReportDir "data")

if (-not (Test-Path $MatrixCsv)) { throw "MatrixCsv fehlt: $MatrixCsv" }
$matrix = Import-Csv $MatrixCsv
if ($matrix.Count -eq 0) { throw "MatrixCsv enthält keine Daten: $MatrixCsv" }

$required = @("row","col","kgram_accuracy","lift","surprise_rate","out_of_grammar")
foreach ($r in $required) {
  if (-not ($matrix[0].PSObject.Properties.Name -contains $r)) {
    throw "MatrixCsv braucht Spalte '$r'. Gefunden: $($matrix[0].PSObject.Properties.Name -join ',')"
  }
}

$matrixRows = foreach ($m in $matrix) {
  [pscustomobject]@{
    row = [string]$m.row
    col = [string]$m.col
    n = if ($m.PSObject.Properties.Name -contains "n") { To-U64 $m.n } else { [UInt64]0 }
    train_n = if ($m.PSObject.Properties.Name -contains "train_n") { To-U64 $m.train_n } else { [UInt64]0 }
    grammar_edges = if ($m.PSObject.Properties.Name -contains "grammar_edges") { To-U64 $m.grammar_edges } else { [UInt64]0 }
    test_transitions = if ($m.PSObject.Properties.Name -contains "test_transitions") { To-U64 $m.test_transitions } else { [UInt64]0 }
    entropy_raw = if ($m.PSObject.Properties.Name -contains "entropy_raw") { To-D $m.entropy_raw } else { [double]::NaN }
    baseline_accuracy = if ($m.PSObject.Properties.Name -contains "baseline_accuracy") { To-D $m.baseline_accuracy } else { [double]::NaN }
    kgram_accuracy = To-D $m.kgram_accuracy
    lift = To-D $m.lift
    out_of_grammar = To-U64 $m.out_of_grammar
    surprise_rate = To-D $m.surprise_rate
    compression_ratio = if ($m.PSObject.Properties.Name -contains "compression_ratio") { To-D $m.compression_ratio } else { [double]::NaN }
  }
}

$nullRows = @()
if ($NullCsv.Trim().Length -gt 0 -and (Test-Path $NullCsv)) {
  $nullRaw = Import-Csv $NullCsv
  foreach ($n in $nullRaw) {
    if (-not ($n.PSObject.Properties.Name -contains "mode")) { continue }
    $nullRows += [pscustomobject]@{
      mode = [string]$n.mode
      row = [string]$n.row
      col = [string]$n.col
      samples = if ($n.PSObject.Properties.Name -contains "samples") { To-U64 $n.samples } else { [UInt64]0 }
      null_kgram_accuracy = if ($n.PSObject.Properties.Name -contains "null_kgram_accuracy") { To-D $n.null_kgram_accuracy } else { [double]::NaN }
      null_surprise = if ($n.PSObject.Properties.Name -contains "null_surprise") { To-D $n.null_surprise } else { [double]::NaN }
      seed = if ($n.PSObject.Properties.Name -contains "seed") { [string]$n.seed } else { "" }
      train = if ($n.PSObject.Properties.Name -contains "train") { To-D $n.train } else { [double]::NaN }
      block_size = if ($n.PSObject.Properties.Name -contains "block_size") { To-U64 $n.block_size } else { [UInt64]0 }
      rotation_offset = if ($n.PSObject.Properties.Name -contains "rotation_offset") { To-U64 $n.rotation_offset } else { [UInt64]0 }
    }
  }
}

$joined = foreach ($m in $matrixRows) {
  $ns = @($nullRows | Where-Object { $_.row -eq $m.row -and $_.col -eq $m.col })
  $avgNull = MeanD ($ns | ForEach-Object { $_.null_kgram_accuracy })
  [pscustomobject]@{
    row = $m.row
    col = $m.col
    class = if ($m.row -eq $m.col) { "self" } else { "cross" }
    observed_accuracy = $m.kgram_accuracy
    baseline_accuracy = $m.baseline_accuracy
    lift = $m.lift
    surprise_rate = $m.surprise_rate
    out_of_grammar = $m.out_of_grammar
    grammar_edges = $m.grammar_edges
    avg_null_accuracy = $avgNull
    observed_minus_null = if ([double]::IsNaN($avgNull)) { [double]::NaN } else { $m.kgram_accuracy - $avgNull }
    null_modes = ($ns | ForEach-Object { $_.mode }) -join "|"
  }
}

$nullSummary = foreach ($mode in ($nullRows | Select-Object -ExpandProperty mode -Unique)) {
  $r = @($nullRows | Where-Object { $_.mode -eq $mode })
  [pscustomobject]@{
    mode = $mode
    cells = $r.Count
    avg_null_accuracy = MeanD ($r | ForEach-Object { $_.null_kgram_accuracy })
    max_null_accuracy = if ($r.Count -gt 0) { ($r | Sort-Object null_kgram_accuracy -Descending | Select-Object -First 1).null_kgram_accuracy } else { [double]::NaN }
    avg_null_surprise = MeanD ($r | ForEach-Object { $_.null_surprise })
    samples_per_cell = if ($r.Count -gt 0) { ($r | Select-Object -First 1).samples } else { [UInt64]0 }
  }
}

$self = @($joined | Where-Object { $_.class -eq "self" })
$cross = @($joined | Where-Object { $_.class -eq "cross" })

$asym = @()
foreach ($a in $cross) {
  $b = $cross | Where-Object { $_.row -eq $a.col -and $_.col -eq $a.row } | Select-Object -First 1
  if ($null -ne $b -and (($a.row + "->" + $a.col) -lt ($b.row + "->" + $b.col))) {
    $asym += [pscustomobject]@{
      pair = "$($a.row)<->$($a.col)"
      forward = "$($a.row)->$($a.col)"
      reverse = "$($b.row)->$($b.col)"
      forward_accuracy = $a.observed_accuracy
      reverse_accuracy = $b.observed_accuracy
      delta_accuracy = [math]::Abs($a.observed_accuracy - $b.observed_accuracy)
      forward_lift = $a.lift
      reverse_lift = $b.lift
      delta_lift = [math]::Abs($a.lift - $b.lift)
      forward_surprise = $a.surprise_rate
      reverse_surprise = $b.surprise_rate
    }
  }
}

$tables = Join-Path $ReportDir "tables"
$matrixRows | Export-Csv (Join-Path $tables "observed_matrix.csv") -NoTypeInformation -Encoding UTF8
$joined | Export-Csv (Join-Path $tables "observed_vs_null.csv") -NoTypeInformation -Encoding UTF8
$self | Export-Csv (Join-Path $tables "self_couplings.csv") -NoTypeInformation -Encoding UTF8
$cross | Export-Csv (Join-Path $tables "cross_couplings.csv") -NoTypeInformation -Encoding UTF8
$nullSummary | Export-Csv (Join-Path $tables "null_summary.csv") -NoTypeInformation -Encoding UTF8
$asym | Export-Csv (Join-Path $tables "asymmetry.csv") -NoTypeInformation -Encoding UTF8

$avgSelfAcc = MeanD ($self | ForEach-Object { $_.observed_accuracy })
$avgCrossAcc = MeanD ($cross | ForEach-Object { $_.observed_accuracy })
$avgSelfLift = MeanD ($self | ForEach-Object { $_.lift })
$avgCrossLift = MeanD ($cross | ForEach-Object { $_.lift })
$avgCrossDelta = MeanD ($cross | ForEach-Object { $_.observed_minus_null })
$minCrossDelta = if ($cross.Count -gt 0) { ($cross | Sort-Object observed_minus_null | Select-Object -First 1).observed_minus_null } else { [double]::NaN }
$maxCrossSurprise = if ($cross.Count -gt 0) { ($cross | Sort-Object surprise_rate -Descending | Select-Object -First 1).surprise_rate } else { [double]::NaN }

$manifest = [pscustomobject]@{
  generated_at = (Get-Date).ToString("o")
  matrix_csv = (Resolve-Path $MatrixCsv).Path
  null_csv = if ($NullCsv -and (Test-Path $NullCsv)) { (Resolve-Path $NullCsv).Path } else { "" }
  cells = $matrixRows.Count
  self_cells = $self.Count
  cross_cells = $cross.Count
  avg_self_accuracy = $avgSelfAcc
  avg_cross_accuracy = $avgCrossAcc
  avg_self_lift = $avgSelfLift
  avg_cross_lift = $avgCrossLift
  avg_cross_observed_minus_null = $avgCrossDelta
}
$manifest | ConvertTo-Json -Depth 6 | Set-Content -Encoding UTF8 (Join-Path $ReportDir "report_v1_2_manifest.json")

function MdTable($rows, [string[]]$cols, [int]$limit=20) {
  $out = @()
  $out += "| " + ($cols -join " | ") + " |"
  $out += "| " + (($cols | ForEach-Object { "---" }) -join " | ") + " |"
  foreach ($r in (@($rows) | Select-Object -First $limit)) {
    $cells = foreach ($c in $cols) {
      $v = $r.$c
      if ($v -is [double]) { R12 $v } else { [string]$v }
    }
    $out += "| " + ($cells -join " | ") + " |"
  }
  return ($out -join "`n")
}

$md = @"
# $Title

KLANG v1.2 wertet die vorhandenen KLANG-Artefakte aus. Es werden keine CoNLL-U-Dateien neu gelesen, keine KDNA-Symbole neu projiziert und keine KGRAM-Grammatiken neu induziert.

## Kernmetriken

| Metrik | Wert |
| --- | ---: |
| Matrixzellen | $($matrixRows.Count) |
| Self-Zellen | $($self.Count) |
| Cross-Zellen | $($cross.Count) |
| Durchschnitt Self-Accuracy | $(R12 $avgSelfAcc) |
| Durchschnitt Cross-Accuracy | $(R12 $avgCrossAcc) |
| Durchschnitt Self-Lift | $(R12 $avgSelfLift) |
| Durchschnitt Cross-Lift | $(R12 $avgCrossLift) |
| Durchschnitt Cross observed-null | $(R12 $avgCrossDelta) |
| Minimum Cross observed-null | $(R12 $minCrossDelta) |
| Maximale Cross-Surprise | $(R12 $maxCrossSurprise) |

## Beobachtete Matrix

$(MdTable $joined @("row","col","class","observed_accuracy","lift","surprise_rate","out_of_grammar","avg_null_accuracy","observed_minus_null") 16)

## Nullmodell-Zusammenfassung

$(MdTable $nullSummary @("mode","cells","avg_null_accuracy","max_null_accuracy","avg_null_surprise","samples_per_cell") 16)

## Asymmetrie

$(MdTable $asym @("pair","forward","reverse","forward_accuracy","reverse_accuracy","delta_accuracy","forward_lift","reverse_lift","delta_lift") 16)

## Interpretation

Ein belastbarer KLANG-Hidden-Grammar-Befund liegt dann vor, wenn Self- und Cross-Kopplungen deutlich oberhalb der Nullmodelle bleiben. Besonders relevant ist **observed_minus_null**: diese Metrik prüft, ob die beobachtete Kopplung über das hinausgeht, was durch zerstörte Reihenfolge, Rotation oder Blockgrenzen erklärbar wäre.

Bei POS-basierten Symbolströmen ist Accuracy allein nicht ausreichend, weil das Alphabet klein sein kann. Deshalb werden Lift, Surprise, Out-of-Grammar und die Nullmodell-Differenz gemeinsam betrachtet.

## Artefakte

- `tables/observed_matrix.csv`
- `tables/observed_vs_null.csv`
- `tables/self_couplings.csv`
- `tables/cross_couplings.csv`
- `tables/null_summary.csv`
- `tables/asymmetry.csv`
- `report_v1_2_manifest.json`
"@

$mdPath = Join-Path $ReportDir "KLANG_REPORT_V1_2.md"
$md | Set-Content -Encoding UTF8 $mdPath

$css = @"
body{font-family:Segoe UI,Arial,sans-serif;margin:36px;background:#0c0f14;color:#e8eef7;line-height:1.55}
h1,h2{color:#7df9ff}
.card{border:1px solid #26384d;border-radius:14px;padding:16px;margin:16px 0;background:#121925;box-shadow:0 0 20px rgba(125,249,255,.08)}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(220px,1fr));gap:12px}
.metric{border:1px solid #2d4057;border-radius:12px;padding:12px;background:#0f1722}
.metric b{display:block;color:#ff4fd8;font-size:12px;text-transform:uppercase;letter-spacing:.08em}
.metric span{font-size:22px;color:#fff}
table{border-collapse:collapse;width:100%;font-size:13px;margin:12px 0}
th,td{border:1px solid #2b3b50;padding:6px 8px;text-align:right}
th{background:#172233;color:#7df9ff}
td:first-child,th:first-child{text-align:left}
code{background:#172233;padding:2px 5px;border-radius:4px}
a{color:#7df9ff}
"@

function HtmlTable($rows, [string[]]$cols, [int]$limit=20) {
  $s = "<table><tr>" + (($cols | ForEach-Object { "<th>$([System.Web.HttpUtility]::HtmlEncode($_))</th>" }) -join "") + "</tr>"
  foreach ($r in (@($rows) | Select-Object -First $limit)) {
    $s += "<tr>"
    foreach ($c in $cols) {
      $v = $r.$c
      if ($v -is [double]) { $txt = R6 $v } else { $txt = [string]$v }
      $s += "<td>$([System.Web.HttpUtility]::HtmlEncode($txt))</td>"
    }
    $s += "</tr>"
  }
  $s += "</table>"
  return $s
}

Add-Type -AssemblyName System.Web

$html = @"
<!doctype html>
<html lang="de"><head><meta charset="utf-8"><title>$Title</title><style>$css</style></head>
<body>
<h1>$Title</h1>
<div class="card"><p>KLANG v1.2 wertet vorhandene Self/Cross- und Nullmodell-Artefakte aus. Die KDNA/KGRAM-Pipeline wird nicht neu berechnet.</p></div>
<div class="grid">
<div class="metric"><b>Self Accuracy</b><span>$(R6 $avgSelfAcc)</span></div>
<div class="metric"><b>Cross Accuracy</b><span>$(R6 $avgCrossAcc)</span></div>
<div class="metric"><b>Self Lift</b><span>$(R6 $avgSelfLift)</span></div>
<div class="metric"><b>Cross Lift</b><span>$(R6 $avgCrossLift)</span></div>
<div class="metric"><b>Cross Δ Null</b><span>$(R6 $avgCrossDelta)</span></div>
<div class="metric"><b>Max Cross Surprise</b><span>$(R6 $maxCrossSurprise)</span></div>
</div>
<h2>Beobachtete Matrix vs. Null</h2>
$(HtmlTable $joined @("row","col","class","observed_accuracy","lift","surprise_rate","out_of_grammar","avg_null_accuracy","observed_minus_null") 16)
<h2>Nullmodell-Zusammenfassung</h2>
$(HtmlTable $nullSummary @("mode","cells","avg_null_accuracy","max_null_accuracy","avg_null_surprise","samples_per_cell") 16)
<h2>Asymmetrie</h2>
$(HtmlTable $asym @("pair","forward","reverse","forward_accuracy","reverse_accuracy","delta_accuracy","forward_lift","reverse_lift","delta_lift") 16)
<h2>Interpretation</h2>
<div class="card"><p>Ein KLANG-Hidden-Grammar-Befund ist dann stark, wenn Cross-Kopplungen deutlich oberhalb der Nullmodelle bleiben. Accuracy allein ist bei POS-Strömen nicht ausreichend; entscheidend sind Lift, Surprise, Out-of-Grammar und <code>observed_minus_null</code>.</p></div>
<h2>Artefakte</h2>
<ul>
<li><a href="tables/observed_vs_null.csv">tables/observed_vs_null.csv</a></li>
<li><a href="tables/null_summary.csv">tables/null_summary.csv</a></li>
<li><a href="tables/asymmetry.csv">tables/asymmetry.csv</a></li>
<li><a href="report_v1_2_manifest.json">report_v1_2_manifest.json</a></li>
</ul>
</body></html>
"@

$htmlPath = Join-Path $ReportDir "KLANG_REPORT_V1_2.html"
$html | Set-Content -Encoding UTF8 $htmlPath

Write-Host "KLANG v1.2 report written:"
Write-Host "  $mdPath"
Write-Host "  $htmlPath"
Write-Host "  $(Join-Path $tables 'observed_vs_null.csv')"
