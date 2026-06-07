param(
  [string]$Dir = ".\GenomeOutFull",
  [string]$LibraryJson = ".\GenomeOutFull\human_kdna_library.json",
  [string]$LibraryCsv = ".\GenomeOutFull\human_kdna_library.csv",
  [string]$MatrixCsv = ".\GenomeOutFull\human_chr1_22_XY_full_k16.csv",
  [string]$OutDir = ".\GenomeOutFull\Report",
  [int]$Top = 50
)

$ErrorActionPreference = "Stop"

function Require-File([string]$Path) {
  if (-not (Test-Path $Path)) { throw "Fehlt: $Path" }
}

function To-Double($x) {
  if ($null -eq $x -or "$x" -eq "") { return [double]::NaN }
  return [double]::Parse("$x", [System.Globalization.CultureInfo]::InvariantCulture)
}

function Csv-Out($rows, [string]$path) {
  $rows | Export-Csv -Path $path -NoTypeInformation -Encoding UTF8
}

function HtmlEncode([string]$s) {
  return [System.Net.WebUtility]::HtmlEncode($s)
}

function New-HeatmapSvg {
  param(
    [array]$Rows,
    [string]$ValueField,
    [string]$OutPath,
    [string]$Title,
    [switch]$Invert
  )
  $chroms = @($Rows | Select-Object -ExpandProperty row -Unique)
  $cols   = @($Rows | Select-Object -ExpandProperty col -Unique)
  $cell = 24; $left = 82; $topPad = 82; $right = 40; $bottom = 90
  $w = $left + $cols.Count * $cell + $right
  $h = $topPad + $chroms.Count * $cell + $bottom
  $vals = foreach ($r in $Rows) { To-Double $r.$ValueField }
  $min = ($vals | Measure-Object -Minimum).Minimum
  $max = ($vals | Measure-Object -Maximum).Maximum
  if ($max -eq $min) { $max = $min + 1.0 }

  $sb = New-Object System.Text.StringBuilder
  [void]$sb.AppendLine("<svg xmlns='http://www.w3.org/2000/svg' width='$w' height='$h' viewBox='0 0 $w $h'>")
  [void]$sb.AppendLine("<rect width='100%' height='100%' fill='white'/>")
  [void]$sb.AppendLine("<text x='20' y='32' font-family='Arial' font-size='18' font-weight='bold'>$(HtmlEncode $Title)</text>")
  [void]$sb.AppendLine("<text x='20' y='54' font-family='Arial' font-size='12'>value=$ValueField min=$([Math]::Round($min,6)) max=$([Math]::Round($max,6))</text>")

  for ($i=0; $i -lt $cols.Count; $i++) {
    $x = $left + $i * $cell + 12; $y = $topPad - 8
    [void]$sb.AppendLine("<text x='$x' y='$y' transform='rotate(-55 $x,$y)' font-family='Arial' font-size='10'>$(HtmlEncode $cols[$i])</text>")
  }
  for ($j=0; $j -lt $chroms.Count; $j++) {
    $x = 20; $y = $topPad + $j * $cell + 16
    [void]$sb.AppendLine("<text x='$x' y='$y' font-family='Arial' font-size='11'>$(HtmlEncode $chroms[$j])</text>")
  }

  $map = @{}
  foreach ($r in $Rows) { $map["$($r.row)|$($r.col)"] = $r }

  for ($j=0; $j -lt $chroms.Count; $j++) {
    for ($i=0; $i -lt $cols.Count; $i++) {
      $key = "$($chroms[$j])|$($cols[$i])"
      if (-not $map.ContainsKey($key)) { continue }
      $v = To-Double $map[$key].$ValueField
      $t = ($v - $min) / ($max - $min)
      if ($Invert) { $t = 1.0 - $t }
      if ($t -lt 0) { $t = 0 }; if ($t -gt 1) { $t = 1 }
      $rcol = [int](255 * $t)
      $gcol = [int](255 * (1.0 - [Math]::Abs($t - 0.5) * 1.5))
      if ($gcol -lt 0) { $gcol = 0 }; if ($gcol -gt 255) { $gcol = 255 }
      $bcol = [int](255 * (1.0 - $t))
      $x = $left + $i * $cell; $y = $topPad + $j * $cell
      $stroke = if ($chroms[$j] -eq $cols[$i]) { "black" } else { "#dddddd" }
      $sw = if ($chroms[$j] -eq $cols[$i]) { "1.5" } else { "0.5" }
      $tip = "$($chroms[$j]) -> $($cols[$i]) $ValueField=$v"
      [void]$sb.AppendLine("<rect x='$x' y='$y' width='$cell' height='$cell' fill='rgb($rcol,$gcol,$bcol)' stroke='$stroke' stroke-width='$sw'><title>$(HtmlEncode $tip)</title></rect>")
    }
  }
  [void]$sb.AppendLine("</svg>")
  Set-Content -Path $OutPath -Value $sb.ToString() -Encoding UTF8
}

Require-File $MatrixCsv
Require-File $LibraryCsv
New-Item -ItemType Directory -Force $OutDir | Out-Null
New-Item -ItemType Directory -Force (Join-Path $OutDir "tables") | Out-Null
New-Item -ItemType Directory -Force (Join-Path $OutDir "figures") | Out-Null

$rows = Import-Csv $MatrixCsv
$libRows = Import-Csv $LibraryCsv
if ($rows.Count -eq 0) { throw "Matrix CSV enthält keine Zeilen: $MatrixCsv" }

$enriched = foreach ($r in $rows) {
  [PSCustomObject]@{
    row = $r.row; col = $r.col; n = $r.n; train_n = $r.train_n
    unique_variants = $r.unique_variants; grammar_edges = $r.grammar_edges
    test_transitions = $r.test_transitions; entropy_raw = (To-Double $r.entropy_raw)
    baseline_accuracy = (To-Double $r.baseline_accuracy)
    kgram_accuracy = (To-Double $r.kgram_accuracy)
    lift = (To-Double $r.lift)
    out_of_grammar = $r.out_of_grammar
    surprise_rate = (To-Double $r.surprise_rate)
    compression_ratio = (To-Double $r.compression_ratio)
    is_self = ($r.row -eq $r.col)
  }
}

$self = @($enriched | Where-Object { $_.is_self } | Sort-Object lift -Descending)
$cross = @($enriched | Where-Object { -not $_.is_self } | Sort-Object lift -Descending)
$topLift = @($enriched | Sort-Object lift -Descending | Select-Object -First $Top)
$bottomLift = @($enriched | Sort-Object lift | Select-Object -First $Top)
$topCross = @($cross | Select-Object -First $Top)
$bottomCross = @($cross | Sort-Object lift | Select-Object -First $Top)

$carrier = @(
  foreach ($g in ($cross | Group-Object col)) {
    [PSCustomObject]@{
      grammar = $g.Name
      cross_count = $g.Count
      avg_cross_lift = [Math]::Round((($g.Group | Measure-Object lift -Average).Average), 12)
      avg_cross_accuracy = [Math]::Round((($g.Group | Measure-Object kgram_accuracy -Average).Average), 12)
      avg_cross_surprise = [Math]::Round((($g.Group | Measure-Object surprise_rate -Average).Average), 12)
      min_cross_lift = [Math]::Round((($g.Group | Measure-Object lift -Minimum).Minimum), 12)
      max_cross_lift = [Math]::Round((($g.Group | Measure-Object lift -Maximum).Maximum), 12)
    }
  }
) | Sort-Object avg_cross_lift -Descending

$receiver = @(
  foreach ($g in ($cross | Group-Object row)) {
    [PSCustomObject]@{
      chromosome = $g.Name
      cross_count = $g.Count
      avg_cross_lift = [Math]::Round((($g.Group | Measure-Object lift -Average).Average), 12)
      avg_cross_accuracy = [Math]::Round((($g.Group | Measure-Object kgram_accuracy -Average).Average), 12)
      avg_cross_surprise = [Math]::Round((($g.Group | Measure-Object surprise_rate -Average).Average), 12)
      min_cross_lift = [Math]::Round((($g.Group | Measure-Object lift -Minimum).Minimum), 12)
      max_cross_lift = [Math]::Round((($g.Group | Measure-Object lift -Maximum).Maximum), 12)
    }
  }
) | Sort-Object avg_cross_lift -Descending

$chroms = @($enriched | Select-Object -ExpandProperty row -Unique)
$asym = @()
foreach ($a in $chroms) {
  foreach ($b in $chroms) {
    if ($a -ge $b) { continue }
    $ab = $enriched | Where-Object { $_.row -eq $a -and $_.col -eq $b } | Select-Object -First 1
    $ba = $enriched | Where-Object { $_.row -eq $b -and $_.col -eq $a } | Select-Object -First 1
    if ($ab -and $ba) {
      $asym += [PSCustomObject]@{
        a = $a; b = $b
        lift_a_to_b = $ab.lift; lift_b_to_a = $ba.lift
        delta_lift = [Math]::Round([Math]::Abs($ab.lift - $ba.lift), 12)
        acc_a_to_b = $ab.kgram_accuracy; acc_b_to_a = $ba.kgram_accuracy
        delta_acc = [Math]::Round([Math]::Abs($ab.kgram_accuracy - $ba.kgram_accuracy), 12)
      }
    }
  }
}
$asym = $asym | Sort-Object delta_lift -Descending

Csv-Out $self (Join-Path $OutDir "tables\self_couplings.csv")
Csv-Out $cross (Join-Path $OutDir "tables\cross_couplings.csv")
Csv-Out $topLift (Join-Path $OutDir "tables\top_lift.csv")
Csv-Out $bottomLift (Join-Path $OutDir "tables\bottom_lift.csv")
Csv-Out $topCross (Join-Path $OutDir "tables\top_cross_lift.csv")
Csv-Out $bottomCross (Join-Path $OutDir "tables\bottom_cross_lift.csv")
Csv-Out $carrier (Join-Path $OutDir "tables\grammar_carriers.csv")
Csv-Out $receiver (Join-Path $OutDir "tables\chromosome_receivers.csv")
Csv-Out $asym (Join-Path $OutDir "tables\asymmetry.csv")

New-HeatmapSvg -Rows $enriched -ValueField "lift" -OutPath (Join-Path $OutDir "figures\lift_heatmap.svg") -Title "KGENOME Full-k16 Lift Matrix"
New-HeatmapSvg -Rows $enriched -ValueField "kgram_accuracy" -OutPath (Join-Path $OutDir "figures\accuracy_heatmap.svg") -Title "KGENOME Full-k16 Accuracy Matrix"
New-HeatmapSvg -Rows $enriched -ValueField "surprise_rate" -OutPath (Join-Path $OutDir "figures\surprise_heatmap.svg") -Title "KGENOME Full-k16 Surprise Matrix" -Invert

$totalCells = $enriched.Count
$selfCount = $self.Count
$crossCount = $cross.Count
$avgSelfLift = ($self | Measure-Object lift -Average).Average
$avgCrossLift = ($cross | Measure-Object lift -Average).Average
$avgSelfAcc = ($self | Measure-Object kgram_accuracy -Average).Average
$avgCrossAcc = ($cross | Measure-Object kgram_accuracy -Average).Average
$best = $topLift[0]
$worst = $bottomLift[0]
$bestCarrier = $carrier[0]
$bestReceiver = $receiver[0]
$maxAsym = $asym[0]

$manifest = [PSCustomObject]@{
  generated_at = (Get-Date).ToString("o")
  input = [PSCustomObject]@{
    dir = $Dir; library_json = $LibraryJson; library_csv = $LibraryCsv; matrix_csv = $MatrixCsv
  }
  matrix = [PSCustomObject]@{
    cells = $totalCells; self_cells = $selfCount; cross_cells = $crossCount
    avg_self_lift = $avgSelfLift; avg_cross_lift = $avgCrossLift
    avg_self_accuracy = $avgSelfAcc; avg_cross_accuracy = $avgCrossAcc
  }
  best = [PSCustomObject]@{ row = $best.row; col = $best.col; lift = $best.lift; accuracy = $best.kgram_accuracy; surprise = $best.surprise_rate }
  weakest = [PSCustomObject]@{ row = $worst.row; col = $worst.col; lift = $worst.lift; accuracy = $worst.kgram_accuracy; surprise = $worst.surprise_rate }
  best_carrier = $bestCarrier; best_receiver = $bestReceiver; max_asymmetry = $maxAsym
  outputs = [PSCustomObject]@{ report_md = "KGENOME_REPORT.md"; report_html = "KGENOME_REPORT.html"; tables = "tables"; figures = "figures" }
}
$manifest | ConvertTo-Json -Depth 8 | Set-Content (Join-Path $OutDir "report_manifest.json") -Encoding UTF8

function MdTable($items, [string[]]$cols, [int]$limit=12) {
  $sel = @($items | Select-Object -First $limit)
  $s = "| " + ($cols -join " | ") + " |`n"
  $s += "| " + (($cols | ForEach-Object { "---" }) -join " | ") + " |`n"
  foreach ($it in $sel) {
    $vals = foreach ($c in $cols) {
      $v = $it.$c
      if ($v -is [double]) { "{0:0.##########}" -f $v } else { "$v" }
    }
    $s += "| " + ($vals -join " | ") + " |`n"
  }
  return $s
}

$md = @"
# KGENOME Enterprise Report

Quelle: vorhandene KGLIB/KGENOME-Artefakte. Keine FASTA-, KMAP-, KGRAM- oder Matrix-Neuberechnung.

## Gesamtstatus

- Matrixzellen: $totalCells
- Self-Zellen: $selfCount
- Cross-Zellen: $crossCount
- Durchschnitt Self-Lift: $([Math]::Round($avgSelfLift, 12))
- Durchschnitt Cross-Lift: $([Math]::Round($avgCrossLift, 12))
- Durchschnitt Self-Accuracy: $([Math]::Round($avgSelfAcc, 12))
- Durchschnitt Cross-Accuracy: $([Math]::Round($avgCrossAcc, 12))

## Stärkste Kopplung

- $($best.row) -> $($best.col)
- lift: $($best.lift)
- accuracy: $($best.kgram_accuracy)
- surprise: $($best.surprise_rate)

## Schwächste Kopplung

- $($worst.row) -> $($worst.col)
- lift: $($worst.lift)
- accuracy: $($worst.kgram_accuracy)
- surprise: $($worst.surprise_rate)

## Top Self-Kopplungen

$(MdTable $self @("row","col","kgram_accuracy","lift","surprise_rate","out_of_grammar","unique_variants","grammar_edges") 24)

## Top Cross-Kopplungen

$(MdTable $topCross @("row","col","kgram_accuracy","lift","surprise_rate","out_of_grammar") 30)

## Grammatik-Träger

$(MdTable $carrier @("grammar","avg_cross_lift","avg_cross_accuracy","avg_cross_surprise","min_cross_lift","max_cross_lift") 24)

## Empfängerprofile

$(MdTable $receiver @("chromosome","avg_cross_lift","avg_cross_accuracy","avg_cross_surprise","min_cross_lift","max_cross_lift") 24)

## Größte gerichtete Asymmetrien

$(MdTable $asym @("a","b","lift_a_to_b","lift_b_to_a","delta_lift","acc_a_to_b","acc_b_to_a","delta_acc") 30)

## Abbildungen

- figures/lift_heatmap.svg
- figures/accuracy_heatmap.svg
- figures/surprise_heatmap.svg

## Artefakt-Tabellen

- tables/self_couplings.csv
- tables/cross_couplings.csv
- tables/top_lift.csv
- tables/bottom_lift.csv
- tables/top_cross_lift.csv
- tables/bottom_cross_lift.csv
- tables/grammar_carriers.csv
- tables/chromosome_receivers.csv
- tables/asymmetry.csv
"@
Set-Content -Path (Join-Path $OutDir "KGENOME_REPORT.md") -Value $md -Encoding UTF8

$html = @"
<!doctype html>
<html lang="de"><head><meta charset="utf-8"><title>KGENOME Enterprise Report</title>
<style>
body{font-family:Segoe UI,Arial,sans-serif;margin:32px;line-height:1.45;color:#111}
h1,h2{letter-spacing:-0.02em}.card{border:1px solid #ddd;border-radius:12px;padding:16px;margin:16px 0;background:#fafafa}
.grid{display:grid;grid-template-columns:repeat(4,minmax(160px,1fr));gap:12px}.metric{border:1px solid #ddd;border-radius:10px;padding:12px;background:white}.metric b{display:block;font-size:22px}
a{color:#0645ad} object{max-width:100%;border:1px solid #ddd;border-radius:8px;background:white;margin-bottom:18px}
</style></head><body>
<h1>KGENOME Enterprise Report</h1>
<p>Quelle: vorhandene KGLIB/KGENOME-Artefakte. Keine FASTA-, KMAP-, KGRAM- oder Matrix-Neuberechnung.</p>
<div class="grid">
<div class="metric"><span>Matrixzellen</span><b>$totalCells</b></div>
<div class="metric"><span>Self-Lift Ø</span><b>$([Math]::Round($avgSelfLift,6))</b></div>
<div class="metric"><span>Cross-Lift Ø</span><b>$([Math]::Round($avgCrossLift,6))</b></div>
<div class="metric"><span>Cross-Zellen</span><b>$crossCount</b></div>
</div>
<div class="card"><h2>Stärkste Kopplung</h2><p><b>$($best.row) → $($best.col)</b>, lift=$($best.lift), accuracy=$($best.kgram_accuracy), surprise=$($best.surprise_rate)</p></div>
<div class="card"><h2>Schwächste Kopplung</h2><p><b>$($worst.row) → $($worst.col)</b>, lift=$($worst.lift), accuracy=$($worst.kgram_accuracy), surprise=$($worst.surprise_rate)</p></div>
<h2>Heatmaps</h2>
<object data="figures/lift_heatmap.svg" type="image/svg+xml" width="900"></object>
<object data="figures/accuracy_heatmap.svg" type="image/svg+xml" width="900"></object>
<object data="figures/surprise_heatmap.svg" type="image/svg+xml" width="900"></object>
<h2>Tabellen</h2><ul>
<li><a href="tables/self_couplings.csv">Self-Kopplungen</a></li>
<li><a href="tables/cross_couplings.csv">Cross-Kopplungen</a></li>
<li><a href="tables/grammar_carriers.csv">Grammatik-Träger</a></li>
<li><a href="tables/chromosome_receivers.csv">Empfängerprofile</a></li>
<li><a href="tables/asymmetry.csv">Asymmetrien</a></li>
</ul></body></html>
"@
Set-Content -Path (Join-Path $OutDir "KGENOME_REPORT.html") -Value $html -Encoding UTF8

Write-Host "KGENOME report written:"
Write-Host "  $OutDir\KGENOME_REPORT.md"
Write-Host "  $OutDir\KGENOME_REPORT.html"
Write-Host "  $OutDir\report_manifest.json"
Write-Host "  $OutDir\figures\lift_heatmap.svg"
Write-Host "  $OutDir\figures\accuracy_heatmap.svg"
Write-Host "  $OutDir\figures\surprise_heatmap.svg"
