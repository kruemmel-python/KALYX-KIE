param(
  [string]$Dir = ".\GenomeOutFull",
  [string]$LibraryJson = ".\GenomeOutFull\human_kdna_library.json",
  [string]$LibraryCsv = ".\GenomeOutFull\human_kdna_library.csv",
  [string]$MatrixCsv = ".\GenomeOutFull\human_chr1_22_XY_full_k16.csv",
  [string]$OutDir = ".\GenomeOutFull\ReportV2",
  [int]$Top = 50,
  [int]$PermutationRounds = 10000,
  [int]$HistogramBins = 32
)

$ErrorActionPreference = "Stop"
$culture = [System.Globalization.CultureInfo]::InvariantCulture

function RequireFile([string]$p) {
  if (-not (Test-Path $p)) { throw "Fehlt: $p" }
}

function EnsureDir([string]$p) {
  New-Item -ItemType Directory -Force -Path $p | Out-Null
}

function ToD([object]$x) {
  if ($null -eq $x) { return [double]0.0 }
  $s = ([string]$x).Trim()
  if ([string]::IsNullOrWhiteSpace($s)) { return [double]0.0 }
  $s = $s.Replace(",", ".")
  return [double]::Parse($s, $culture)
}

function Fmt([object]$x) {
  $d = [double](ToD $x)
  return $d.ToString("0.###############", $culture)
}

function Prop([object]$row, [string]$name) {
  if ($null -eq $row) { return $null }
  $p = $row.PSObject.Properties[$name]
  if ($null -eq $p) { return $null }
  return $p.Value
}

function AvgProp($items, [string]$prop) {
  $arr = @($items)
  if ($arr.Count -eq 0) { return [double]0.0 }
  $sum = [double]0.0
  foreach ($r in $arr) { $sum += [double](ToD (Prop $r $prop)) }
  return [double]($sum / [double]$arr.Count)
}

function MinProp($items, [string]$prop) {
  $arr = @($items)
  if ($arr.Count -eq 0) { return [double]0.0 }
  $m = [double](ToD (Prop $arr[0] $prop))
  foreach ($r in $arr) {
    $v = [double](ToD (Prop $r $prop))
    if ($v -lt $m) { $m = $v }
  }
  return $m
}

function MaxProp($items, [string]$prop) {
  $arr = @($items)
  if ($arr.Count -eq 0) { return [double]0.0 }
  $m = [double](ToD (Prop $arr[0] $prop))
  foreach ($r in $arr) {
    $v = [double](ToD (Prop $r $prop))
    if ($v -gt $m) { $m = $v }
  }
  return $m
}

function AbsD([double]$x) {
  if ($x -lt 0.0) { return -$x }
  return $x
}

function CsvOut($rows, [string]$path) {
  @($rows) | Export-Csv -NoTypeInformation -Encoding UTF8 -Path $path
}

function HtmlEnc([string]$s) {
  return [System.Net.WebUtility]::HtmlEncode($s)
}

function XmlEnc([string]$s) {
  return [System.Security.SecurityElement]::Escape($s)
}

function WriteHistogramSvg($values, [string]$path, [string]$title, [int]$bins) {
  $vals = @($values | ForEach-Object { [double](ToD $_) })
  if ($vals.Count -eq 0) { $vals = @([double]0.0) }
  if ($bins -lt 4) { $bins = 4 }

  $min = $vals[0]
  $max = $vals[0]
  foreach ($v in $vals) {
    if ($v -lt $min) { $min = $v }
    if ($v -gt $max) { $max = $v }
  }
  if ($max -le $min) { $max = $min + 1.0 }

  $counts = New-Object int[] $bins
  $range = [double]($max - $min)
  foreach ($v in $vals) {
    $idx = [int]((($v - $min) / $range) * [double]$bins)
    if ($idx -lt 0) { $idx = 0 }
    if ($idx -ge $bins) { $idx = $bins - 1 }
    $counts[$idx]++
  }

  $maxC = 1
  foreach ($c in $counts) { if ($c -gt $maxC) { $maxC = $c } }

  $w = 920
  $h = 360
  $left = 60
  $bottom = 48
  $plotW = 820
  $plotH = 250
  $barW = [double]$plotW / [double]$bins

  $sb = New-Object System.Text.StringBuilder
  [void]$sb.AppendLine("<svg xmlns='http://www.w3.org/2000/svg' width='$w' height='$h' viewBox='0 0 $w $h'>")
  [void]$sb.AppendLine("<rect width='100%' height='100%' fill='white'/>")
  [void]$sb.AppendLine("<text x='20' y='28' font-size='18' font-family='Segoe UI, Arial'>$(XmlEnc $title)</text>")
  [void]$sb.AppendLine("<line x1='$left' y1='$($h-$bottom)' x2='$($left+$plotW)' y2='$($h-$bottom)' stroke='#333'/>")
  [void]$sb.AppendLine("<line x1='$left' y1='$($h-$bottom-$plotH)' x2='$left' y2='$($h-$bottom)' stroke='#333'/>")

  for ($i=0; $i -lt $bins; $i++) {
    $bh = ([double]$counts[$i] / [double]$maxC) * [double]$plotH
    $x = [double]$left + [double]$i * [double]$barW
    $y = [double]$h - [double]$bottom - $bh
    $xS = $x.ToString("0.##", $culture)
    $yS = $y.ToString("0.##", $culture)
    $bwS = ([double]($barW - 1.0)).ToString("0.##", $culture)
    $bhS = $bh.ToString("0.##", $culture)
    [void]$sb.AppendLine("<rect x='$xS' y='$yS' width='$bwS' height='$bhS' fill='#2b579a'><title>$($counts[$i])</title></rect>")
  }

  [void]$sb.AppendLine("<text x='$left' y='$($h-12)' font-size='12' font-family='Segoe UI, Arial'>$($min.ToString("0.######",$culture))</text>")
  [void]$sb.AppendLine("<text x='$($left+$plotW-90)' y='$($h-12)' font-size='12' font-family='Segoe UI, Arial'>$($max.ToString("0.######",$culture))</text>")
  [void]$sb.AppendLine("</svg>")
  Set-Content -Path $path -Value $sb.ToString() -Encoding UTF8
}

function WriteScatterSvg($rows, [string]$path) {
  $vals = @($rows)
  $w = 920
  $h = 420
  $left = 70
  $bottom = 58
  $plotW = 780
  $plotH = 300
  $sb = New-Object System.Text.StringBuilder
  [void]$sb.AppendLine("<svg xmlns='http://www.w3.org/2000/svg' width='$w' height='$h' viewBox='0 0 $w $h'>")
  [void]$sb.AppendLine("<rect width='100%' height='100%' fill='white'/>")
  [void]$sb.AppendLine("<text x='20' y='28' font-size='18' font-family='Segoe UI, Arial'>Self-Lift gegen mittleren Cross-Lift</text>")
  [void]$sb.AppendLine("<line x1='$left' y1='$($h-$bottom)' x2='$($left+$plotW)' y2='$($h-$bottom)' stroke='#333'/>")
  [void]$sb.AppendLine("<line x1='$left' y1='$($h-$bottom-$plotH)' x2='$left' y2='$($h-$bottom)' stroke='#333'/>")
  foreach ($r in $vals) {
    $xv = [double](ToD $r.avg_out_cross_lift)
    $yv = [double](ToD $r.self_lift)
    $x = [double]$left + (($xv - 0.94) / 0.06) * [double]$plotW
    $y = [double]$h - [double]$bottom - (($yv - 0.94) / 0.06) * [double]$plotH
    if ($x -lt $left) { $x = $left }
    if ($x -gt ($left+$plotW)) { $x = $left+$plotW }
    if ($y -lt ($h-$bottom-$plotH)) { $y = $h-$bottom-$plotH }
    if ($y -gt ($h-$bottom)) { $y = $h-$bottom }
    $xs = $x.ToString("0.##", $culture)
    $ys = $y.ToString("0.##", $culture)
    $label = XmlEnc "$($r.chromosome): self=$($r.self_lift), cross=$($r.avg_out_cross_lift)"
    [void]$sb.AppendLine("<circle cx='$xs' cy='$ys' r='5' fill='#d83b01'><title>$label</title></circle>")
    [void]$sb.AppendLine("<text x='$([double]($x+7))' y='$([double]($y+4))' font-size='10' font-family='Segoe UI, Arial'>$(XmlEnc $r.chromosome)</text>")
  }
  [void]$sb.AppendLine("<text x='$left' y='$($h-18)' font-size='12' font-family='Segoe UI, Arial'>avg outgoing cross lift</text>")
  [void]$sb.AppendLine("<text x='12' y='$($h-$bottom-$plotH+15)' font-size='12' font-family='Segoe UI, Arial'>self lift</text>")
  [void]$sb.AppendLine("</svg>")
  Set-Content -Path $path -Value $sb.ToString() -Encoding UTF8
}

RequireFile $LibraryCsv
RequireFile $MatrixCsv
if (-not (Test-Path $LibraryJson)) {
  Write-Warning "LibraryJson fehlt; Report nutzt LibraryCsv und MatrixCsv weiter."
}

EnsureDir $OutDir
$tablesDir = Join-Path $OutDir "tables"
$figDir = Join-Path $OutDir "figures"
$controlsDir = Join-Path $OutDir "controls"
EnsureDir $tablesDir
EnsureDir $figDir
EnsureDir $controlsDir

$libraryRows = @(Import-Csv -Path $LibraryCsv)
$rows = @(Import-Csv -Path $MatrixCsv)
if ($rows.Count -eq 0) { throw "MatrixCsv hat keine Daten: $MatrixCsv" }

$selfRows = @($rows | Where-Object { $_.row -eq $_.col })
$crossRows = @($rows | Where-Object { $_.row -ne $_.col })
$chroms = @($libraryRows | ForEach-Object { $_.name })
if ($chroms.Count -eq 0) { $chroms = @($rows | ForEach-Object { $_.row } | Sort-Object -Unique) }

$best = $rows | Sort-Object @{Expression={ToD $_.lift};Descending=$true} | Select-Object -First 1
$weakest = $rows | Sort-Object @{Expression={ToD $_.lift};Descending=$false} | Select-Object -First 1

$avgSelfLift = AvgProp $selfRows "lift"
$avgCrossLift = AvgProp $crossRows "lift"
$avgSelfAcc = AvgProp $selfRows "kgram_accuracy"
$avgCrossAcc = AvgProp $crossRows "kgram_accuracy"
$avgSelfSurp = AvgProp $selfRows "surprise_rate"
$avgCrossSurp = AvgProp $crossRows "surprise_rate"

$globalMetrics = [pscustomobject]@{
  matrix_cells = $rows.Count
  self_cells = $selfRows.Count
  cross_cells = $crossRows.Count
  avg_self_lift = Fmt $avgSelfLift
  avg_cross_lift = Fmt $avgCrossLift
  delta_self_cross_lift = Fmt ([double]($avgSelfLift - $avgCrossLift))
  avg_self_accuracy = Fmt $avgSelfAcc
  avg_cross_accuracy = Fmt $avgCrossAcc
  delta_self_cross_accuracy = Fmt ([double]($avgSelfAcc - $avgCrossAcc))
  avg_self_surprise = Fmt $avgSelfSurp
  avg_cross_surprise = Fmt $avgCrossSurp
  best_pair = "$($best.row)->$($best.col)"
  best_lift = Fmt $best.lift
  weakest_pair = "$($weakest.row)->$($weakest.col)"
  weakest_lift = Fmt $weakest.lift
  library_rows = $libraryRows.Count
}
CsvOut @($globalMetrics) (Join-Path $tablesDir "global_metrics.csv")

$carrierList = @()
foreach ($c in $chroms) {
  $r = @($crossRows | Where-Object { $_.col -eq $c })
  if ($r.Count -gt 0) {
    $carrierList += [pscustomobject]@{
      grammar = $c
      cross_count = $r.Count
      avg_cross_lift = Fmt (AvgProp $r "lift")
      avg_cross_accuracy = Fmt (AvgProp $r "kgram_accuracy")
      avg_cross_surprise = Fmt (AvgProp $r "surprise_rate")
      min_cross_lift = Fmt (MinProp $r "lift")
      max_cross_lift = Fmt (MaxProp $r "lift")
    }
  }
}
$grammarCarriers = @($carrierList) | Sort-Object @{Expression={ToD $_.avg_cross_lift};Descending=$true}
CsvOut $grammarCarriers (Join-Path $tablesDir "grammar_carriers_v2.csv")

$receiverList = @()
foreach ($c in $chroms) {
  $r = @($crossRows | Where-Object { $_.row -eq $c })
  if ($r.Count -gt 0) {
    $receiverList += [pscustomobject]@{
      chromosome = $c
      cross_count = $r.Count
      avg_cross_lift = Fmt (AvgProp $r "lift")
      avg_cross_accuracy = Fmt (AvgProp $r "kgram_accuracy")
      avg_cross_surprise = Fmt (AvgProp $r "surprise_rate")
      min_cross_lift = Fmt (MinProp $r "lift")
      max_cross_lift = Fmt (MaxProp $r "lift")
    }
  }
}
$chromosomeReceivers = @($receiverList) | Sort-Object @{Expression={ToD $_.avg_cross_lift};Descending=$true}
CsvOut $chromosomeReceivers (Join-Path $tablesDir "chromosome_receivers_v2.csv")

$selfCross = @()
foreach ($c in $chroms) {
  $self = $selfRows | Where-Object { $_.row -eq $c -and $_.col -eq $c } | Select-Object -First 1
  $outCross = @($crossRows | Where-Object { $_.row -eq $c })
  $inCross = @($crossRows | Where-Object { $_.col -eq $c })
  if ($null -ne $self) {
    $outAvg = AvgProp $outCross "lift"
    $inAvg = AvgProp $inCross "lift"
    $selfLift = ToD $self.lift
    $selfCross += [pscustomobject]@{
      chromosome = $c
      self_lift = Fmt $selfLift
      self_accuracy = Fmt $self.kgram_accuracy
      self_surprise = Fmt $self.surprise_rate
      avg_out_cross_lift = Fmt $outAvg
      avg_in_cross_lift = Fmt $inAvg
      self_minus_out_cross = Fmt ([double]($selfLift - $outAvg))
      self_minus_in_cross = Fmt ([double]($selfLift - $inAvg))
    }
  }
}
$selfCross = @($selfCross) | Sort-Object @{Expression={ToD $_.self_lift};Descending=$true}
CsvOut $selfCross (Join-Path $tablesDir "self_cross_profile.csv")

$lookup = @{}
foreach ($r in $crossRows) { $lookup["$($r.row)|$($r.col)"] = $r }
$asymList = @()
for ($i=0; $i -lt $chroms.Count; $i++) {
  for ($j=$i+1; $j -lt $chroms.Count; $j++) {
    $a = $chroms[$i]; $b = $chroms[$j]
    $ab = $lookup["$a|$b"]; $ba = $lookup["$b|$a"]
    if ($null -ne $ab -and $null -ne $ba) {
      $lab = [double](ToD $ab.lift)
      $lba = [double](ToD $ba.lift)
      $aab = [double](ToD $ab.kgram_accuracy)
      $aba = [double](ToD $ba.kgram_accuracy)
      $asymList += [pscustomobject]@{
        a = $a
        b = $b
        lift_a_to_b = Fmt $lab
        lift_b_to_a = Fmt $lba
        delta_lift = Fmt (AbsD ([double]($lab - $lba)))
        acc_a_to_b = Fmt $aab
        acc_b_to_a = Fmt $aba
        delta_acc = Fmt (AbsD ([double]($aab - $aba)))
      }
    }
  }
}
$asymmetry = @($asymList) | Sort-Object @{Expression={ToD $_.delta_lift};Descending=$true}
CsvOut $asymmetry (Join-Path $tablesDir "asymmetry_v2.csv")

$topLift = @($rows | Sort-Object @{Expression={ToD $_.lift};Descending=$true} | Select-Object -First $Top)
$bottomLift = @($rows | Sort-Object @{Expression={ToD $_.lift};Descending=$false} | Select-Object -First $Top)
$topCross = @($crossRows | Sort-Object @{Expression={ToD $_.lift};Descending=$true} | Select-Object -First $Top)
$bottomCross = @($crossRows | Sort-Object @{Expression={ToD $_.lift};Descending=$false} | Select-Object -First $Top)
CsvOut $topLift (Join-Path $tablesDir "top_lift_v2.csv")
CsvOut $bottomLift (Join-Path $tablesDir "bottom_lift_v2.csv")
CsvOut $topCross (Join-Path $tablesDir "top_cross_lift_v2.csv")
CsvOut $bottomCross (Join-Path $tablesDir "bottom_cross_lift_v2.csv")

# Diagnostic label sample: no true sequence null model.
$permLimit = $PermutationRounds
if ($permLimit -lt 0) { $permLimit = 0 }
if ($permLimit -gt 100000) { $permLimit = 100000 }
$rng = New-Object System.Random 1337
$permRows = @()
for ($i=0; $i -lt $permLimit; $i++) {
  $r = $rows[$rng.Next(0, $rows.Count)]
  $permRows += [pscustomobject]@{
    sample_index = $i
    sampled_row = $r.row
    sampled_col = $r.col
    sampled_lift = Fmt $r.lift
    sampled_accuracy = Fmt $r.kgram_accuracy
    diagnostic = "label_sample_only_not_sequence_null_model"
  }
}
CsvOut $permRows (Join-Path $controlsDir "label_permutation_sample.csv")

$controlSummary = @(
  [pscustomobject]@{control="label_permutation"; status="diagnostic_only"; meaning="Samples existing matrix values; not a sequence null model."}
  [pscustomobject]@{control="symbol_shuffle"; status="not_computed"; meaning="Requires shuffled KDNA streams and KGRAM rebuild."}
  [pscustomobject]@{control="block_shuffle"; status="not_computed"; meaning="Requires block-level shuffled KDNA streams and KGRAM rebuild."}
  [pscustomobject]@{control="train_test_rotation"; status="not_computed"; meaning="Requires alternate KGRAM train window."}
  [pscustomobject]@{control="k_sweep"; status="not_computed"; meaning="Requires k=12/k=16/k=20 FASTA-symbol pipeline."}
)
CsvOut $controlSummary (Join-Path $controlsDir "control_summary.csv")

$protocol = @"
# KGENOME_REPORT v2 Kontrollprotokoll

Dieser Report verwendet nur vorhandene KGLIB/KGENOME-Artefakte und berechnet keine Sequenz neu.

## Aktive Diagnose

- Matrixweite Self-vs-Cross-Profile.
- Lift-/Accuracy-/Surprise-Verteilungen.
- Gerichtete Asymmetrieanalyse.
- Label-Sampling über vorhandene Matrixwerte.

## Wichtige Grenze

Label-Sampling ist kein echtes Sequenz-Nullmodell. Es prüft Report- und Matrixstruktur, aber nicht, ob die KDNA/KGRAM-Struktur durch Symbolreihenfolge entsteht.

## Noch zu berechnende echte Nullmodelle

- Symbol-Shuffle der KDNA-Ströme.
- Block-Shuffle.
- Train/Test-Rotation.
- k-Sweep, z. B. k=12/k=16/k=20.
"@
Set-Content -Path (Join-Path $controlsDir "CONTROL_PROTOCOL.md") -Value $protocol -Encoding UTF8

WriteHistogramSvg (@($crossRows | ForEach-Object { $_.lift })) (Join-Path $figDir "cross_lift_distribution.svg") "Cross-Lift-Verteilung" $HistogramBins
WriteHistogramSvg (@($selfRows | ForEach-Object { $_.lift })) (Join-Path $figDir "self_lift_distribution.svg") "Self-Lift-Verteilung" $HistogramBins
WriteHistogramSvg (@($asymmetry | ForEach-Object { $_.delta_lift })) (Join-Path $figDir "asymmetry_distribution.svg") "Asymmetrie-Verteilung" $HistogramBins
WriteScatterSvg $selfCross (Join-Path $figDir "self_vs_cross_scatter.svg")

$bestCarrier = $grammarCarriers | Select-Object -First 1
$bestReceiver = $chromosomeReceivers | Select-Object -First 1
$maxAsym = $asymmetry | Select-Object -First 1

$manifest = [ordered]@{
  generated_at = (Get-Date).ToString("o")
  version = "KGENOME_REPORT_V2_0_4"
  input = [ordered]@{
    dir = $Dir
    library_json = $LibraryJson
    library_csv = $LibraryCsv
    matrix_csv = $MatrixCsv
  }
  matrix = [ordered]@{
    cells = $rows.Count
    self_cells = $selfRows.Count
    cross_cells = $crossRows.Count
    avg_self_lift = Fmt $avgSelfLift
    avg_cross_lift = Fmt $avgCrossLift
    avg_self_accuracy = Fmt $avgSelfAcc
    avg_cross_accuracy = Fmt $avgCrossAcc
  }
  best = [ordered]@{
    row = $best.row
    col = $best.col
    lift = Fmt $best.lift
    accuracy = Fmt $best.kgram_accuracy
    surprise = Fmt $best.surprise_rate
  }
  weakest = [ordered]@{
    row = $weakest.row
    col = $weakest.col
    lift = Fmt $weakest.lift
    accuracy = Fmt $weakest.kgram_accuracy
    surprise = Fmt $weakest.surprise_rate
  }
  best_carrier = $bestCarrier
  best_receiver = $bestReceiver
  max_asymmetry = $maxAsym
  controls = "diagnostic_only_no_sequence_null_model"
}
($manifest | ConvertTo-Json -Depth 8) | Set-Content -Path (Join-Path $OutDir "report_v2_manifest.json") -Encoding UTF8

$md = @()
$md += "# KGENOME Enterprise Report v2"
$md += ""
$md += "Quelle: vorhandene KGLIB/KGENOME-Artefakte. Keine FASTA-, KMAP-, KGRAM- oder Matrix-Neuberechnung."
$md += ""
$md += "## Gesamtstatus"
$md += ""
$md += "- Matrixzellen: $($rows.Count)"
$md += "- Self-Zellen: $($selfRows.Count)"
$md += "- Cross-Zellen: $($crossRows.Count)"
$md += "- Durchschnitt Self-Lift: $(Fmt $avgSelfLift)"
$md += "- Durchschnitt Cross-Lift: $(Fmt $avgCrossLift)"
$md += "- Durchschnitt Self-Accuracy: $(Fmt $avgSelfAcc)"
$md += "- Durchschnitt Cross-Accuracy: $(Fmt $avgCrossAcc)"
$md += ""
$md += "## Wissenschaftliche Grenze"
$md += ""
$md += "Dieser v2-Report ist eine Diagnose der vorhandenen Matrix. Die Label-Permutation ist kein echtes Sequenz-Nullmodell. Echte Nullmodelle erfordern Symbol-Shuffle, Block-Shuffle, Train/Test-Rotation oder k-Sweep mit neuen abgeleiteten Artefakten."
$md += ""
$md += "## Stärkste Kopplung"
$md += ""
$md += "- $($best.row) -> $($best.col)"
$md += "- lift: $(Fmt $best.lift)"
$md += "- accuracy: $(Fmt $best.kgram_accuracy)"
$md += "- surprise: $(Fmt $best.surprise_rate)"
$md += ""
$md += "## Schwächste Kopplung"
$md += ""
$md += "- $($weakest.row) -> $($weakest.col)"
$md += "- lift: $(Fmt $weakest.lift)"
$md += "- accuracy: $(Fmt $weakest.kgram_accuracy)"
$md += "- surprise: $(Fmt $weakest.surprise_rate)"
$md += ""
$md += "## Wichtigste Tabellen"
$md += ""
$md += "- tables/global_metrics.csv"
$md += "- tables/self_cross_profile.csv"
$md += "- tables/grammar_carriers_v2.csv"
$md += "- tables/chromosome_receivers_v2.csv"
$md += "- tables/asymmetry_v2.csv"
$md += "- controls/control_summary.csv"
$md += "- controls/CONTROL_PROTOCOL.md"
$md += ""
$md += "## Abbildungen"
$md += ""
$md += "- figures/cross_lift_distribution.svg"
$md += "- figures/self_lift_distribution.svg"
$md += "- figures/asymmetry_distribution.svg"
$md += "- figures/self_vs_cross_scatter.svg"
Set-Content -Path (Join-Path $OutDir "KGENOME_REPORT_V2.md") -Value ($md -join "`n") -Encoding UTF8

$html = @"
<!doctype html>
<html lang="de"><head><meta charset="utf-8"><title>KGENOME Enterprise Report v2</title>
<style>
body{font-family:Segoe UI,Arial,sans-serif;margin:32px;line-height:1.45;color:#111}
h1,h2{letter-spacing:-0.02em}.grid{display:grid;grid-template-columns:repeat(4,minmax(160px,1fr));gap:12px}.metric{border:1px solid #ddd;border-radius:10px;padding:12px;background:#fff}.metric b{display:block;font-size:22px}
.card{border:1px solid #ddd;border-radius:12px;padding:16px;margin:16px 0;background:#fafafa}
object{max-width:100%;border:1px solid #ddd;border-radius:8px;background:white;margin-bottom:18px}
a{color:#0645ad}
</style></head><body>
<h1>KGENOME Enterprise Report v2</h1>
<p>Quelle: vorhandene KGLIB/KGENOME-Artefakte. Keine FASTA-, KMAP-, KGRAM- oder Matrix-Neuberechnung.</p>
<div class="grid">
<div class="metric"><span>Matrixzellen</span><b>$($rows.Count)</b></div>
<div class="metric"><span>Self-Lift Ø</span><b>$(HtmlEnc (Fmt $avgSelfLift))</b></div>
<div class="metric"><span>Cross-Lift Ø</span><b>$(HtmlEnc (Fmt $avgCrossLift))</b></div>
<div class="metric"><span>Cross-Zellen</span><b>$($crossRows.Count)</b></div>
</div>
<div class="card"><h2>Stärkste Kopplung</h2><p><b>$(HtmlEnc "$($best.row) → $($best.col)")</b>, lift=$(HtmlEnc (Fmt $best.lift)), accuracy=$(HtmlEnc (Fmt $best.kgram_accuracy)), surprise=$(HtmlEnc (Fmt $best.surprise_rate))</p></div>
<div class="card"><h2>Schwächste Kopplung</h2><p><b>$(HtmlEnc "$($weakest.row) → $($weakest.col)")</b>, lift=$(HtmlEnc (Fmt $weakest.lift)), accuracy=$(HtmlEnc (Fmt $weakest.kgram_accuracy)), surprise=$(HtmlEnc (Fmt $weakest.surprise_rate))</p></div>
<div class="card"><h2>Kontrollstatus</h2><p>Dieser v2-Report enthält nur Matrixdiagnostik. Label-Sampling ist kein echtes Sequenz-Nullmodell. Siehe <a href="controls/CONTROL_PROTOCOL.md">CONTROL_PROTOCOL.md</a>.</p></div>
<h2>Diagnostik-Abbildungen</h2>
<object data="figures/cross_lift_distribution.svg" type="image/svg+xml" width="940"></object>
<object data="figures/self_lift_distribution.svg" type="image/svg+xml" width="940"></object>
<object data="figures/asymmetry_distribution.svg" type="image/svg+xml" width="940"></object>
<object data="figures/self_vs_cross_scatter.svg" type="image/svg+xml" width="940"></object>
<h2>Tabellen</h2><ul>
<li><a href="tables/global_metrics.csv">Global Metrics</a></li>
<li><a href="tables/self_cross_profile.csv">Self-vs-Cross Profile</a></li>
<li><a href="tables/grammar_carriers_v2.csv">Grammatik-Träger v2</a></li>
<li><a href="tables/chromosome_receivers_v2.csv">Empfängerprofile v2</a></li>
<li><a href="tables/asymmetry_v2.csv">Asymmetrien v2</a></li>
<li><a href="controls/control_summary.csv">Kontrollübersicht</a></li>
</ul>
</body></html>
"@
Set-Content -Path (Join-Path $OutDir "KGENOME_REPORT_V2.html") -Value $html -Encoding UTF8

Write-Host "KGENOME report v2.0.4 written:"
Write-Host "  $(Join-Path $OutDir 'KGENOME_REPORT_V2.md')"
Write-Host "  $(Join-Path $OutDir 'KGENOME_REPORT_V2.html')"
Write-Host "  $(Join-Path $OutDir 'report_v2_manifest.json')"
Write-Host "  $(Join-Path $figDir 'cross_lift_distribution.svg')"
Write-Host "  $(Join-Path $figDir 'self_lift_distribution.svg')"
Write-Host "  $(Join-Path $figDir 'asymmetry_distribution.svg')"
Write-Host "  $(Join-Path $figDir 'self_vs_cross_scatter.svg')"
