param(
  [Parameter(Mandatory=$true)][string]$MatrixCsv,
  [Parameter(Mandatory=$true)][string]$NullCsv,
  [Parameter(Mandatory=$true)][string]$OutDir,
  [string]$Title = "KLANG v1 Report"
)

$ErrorActionPreference = "Stop"
New-Item -ItemType Directory -Force $OutDir | Out-Null
New-Item -ItemType Directory -Force (Join-Path $OutDir "tables") | Out-Null

function D($x) {
  if ($null -eq $x -or "$x" -eq "") { return [double]0 }
  return [double](("$x") -replace ",",".")
}
function R6($x) { return ([Math]::Round([double]$x, 6)).ToString("0.000000", [Globalization.CultureInfo]::InvariantCulture) }
function R12($x) { return ([Math]::Round([double]$x, 12)).ToString("0.000000000000", [Globalization.CultureInfo]::InvariantCulture) }

$matrix = Import-Csv $MatrixCsv
$nulls = Import-Csv $NullCsv

$joined = @()
foreach ($m in $matrix) {
  foreach ($n in ($nulls | Where-Object { $_.row -eq $m.row -and $_.col -eq $m.col })) {
    $obs = D $m.kgram_accuracy
    $nul = D $n.null_kgram_accuracy
    $joined += [PSCustomObject]@{
      mode = $n.mode
      row = $m.row
      col = $m.col
      observed_accuracy = $obs
      null_accuracy = $nul
      delta_accuracy = $obs - $nul
      observed_lift = D $m.lift
      surprise_rate = D $m.surprise_rate
      out_of_grammar = $m.out_of_grammar
      samples = $n.samples
    }
  }
}

$summary = @()
foreach ($g in ($joined | Group-Object mode)) {
  $vals = $g.Group
  $summary += [PSCustomObject]@{
    mode = $g.Name
    cells = $vals.Count
    avg_observed_accuracy = ($vals | Measure-Object observed_accuracy -Average).Average
    avg_null_accuracy = ($vals | Measure-Object null_accuracy -Average).Average
    avg_delta_accuracy = ($vals | Measure-Object delta_accuracy -Average).Average
    max_null_accuracy = ($vals | Measure-Object null_accuracy -Maximum).Maximum
    min_delta_accuracy = ($vals | Measure-Object delta_accuracy -Minimum).Minimum
  }
}

$joined | Export-Csv (Join-Path $OutDir "tables\null_joined.csv") -NoTypeInformation
$summary | Export-Csv (Join-Path $OutDir "tables\null_summary.csv") -NoTypeInformation
$matrix | Export-Csv (Join-Path $OutDir "tables\matrix.csv") -NoTypeInformation

$md = @()
$md += "# $Title"
$md += ""
$md += "KLANG v1 prüft Sprachströme als KFIELD-Instanz: CoNLL-U → KSTREAM001 → KDNA → KGRAM → 2×2-Matrix → echte Nullmodelle."
$md += ""
$md += "## Matrix"
$md += ""
$md += "| row | col | accuracy | lift | surprise | out_of_grammar |"
$md += "| --- | --- | ---: | ---: | ---: | ---: |"
foreach ($m in $matrix) {
  $md += "| $($m.row) | $($m.col) | $(R6 (D $m.kgram_accuracy)) | $(R6 (D $m.lift)) | $(R6 (D $m.surprise_rate)) | $($m.out_of_grammar) |"
}
$md += ""
$md += "## Nullmodell-Zusammenfassung"
$md += ""
$md += "| mode | cells | avg observed acc | avg null acc | avg delta | max null acc | min delta |"
$md += "| --- | ---: | ---: | ---: | ---: | ---: | ---: |"
foreach ($s in $summary) {
  $md += "| $($s.mode) | $($s.cells) | $(R12 $s.avg_observed_accuracy) | $(R12 $s.avg_null_accuracy) | $(R12 $s.avg_delta_accuracy) | $(R12 $s.max_null_accuracy) | $(R12 $s.min_delta_accuracy) |"
}
$md += ""
$md += "## Interpretation"
$md += ""
$md += "Ein positives observed-minus-null-Delta zeigt, dass die Kopplung nicht nur aus Tag-Häufigkeiten stammt, sondern an der Reihenfolge der Sprachsymbole hängt. Für Universalgrammatik-Fragen ist insbesondere Cross-Kopplung DE↔EN gegen symbol_shuffle und rotation relevant."
$md += ""
$md += "## Artefakte"
$md += ""
$md += "- tables/matrix.csv"
$md += "- tables/null_joined.csv"
$md += "- tables/null_summary.csv"

$mdPath = Join-Path $OutDir "KLANG_REPORT.md"
$htmlPath = Join-Path $OutDir "KLANG_REPORT.html"
$md -join "`r`n" | Set-Content $mdPath -Encoding UTF8

$rowsHtml = ""
foreach ($m in $matrix) {
  $rowsHtml += "<tr><td>$($m.row)</td><td>$($m.col)</td><td>$(R6 (D $m.kgram_accuracy))</td><td>$(R6 (D $m.lift))</td><td>$(R6 (D $m.surprise_rate))</td><td>$($m.out_of_grammar)</td></tr>`n"
}
$sumHtml = ""
foreach ($s in $summary) {
  $sumHtml += "<tr><td>$($s.mode)</td><td>$($s.cells)</td><td>$(R12 $s.avg_observed_accuracy)</td><td>$(R12 $s.avg_null_accuracy)</td><td>$(R12 $s.avg_delta_accuracy)</td><td>$(R12 $s.max_null_accuracy)</td><td>$(R12 $s.min_delta_accuracy)</td></tr>`n"
}

$html = @"
<!doctype html><html lang="de"><head><meta charset="utf-8"><title>$Title</title>
<style>
body{font-family:Segoe UI,Arial,sans-serif;margin:32px;line-height:1.45;background:#0d1117;color:#e6edf3}
.card{border:1px solid #30363d;border-radius:14px;padding:18px;margin:18px 0;background:#161b22}
table{border-collapse:collapse;width:100%;margin:14px 0;background:#0d1117}td,th{border:1px solid #30363d;padding:8px 10px}th{background:#21262d}
h1,h2{color:#7ee787}.metric{display:inline-block;border:1px solid #30363d;border-radius:12px;padding:12px;margin:6px;background:#161b22}
a{color:#58a6ff}
</style></head><body>
<h1>$Title</h1>
<div class="card">
<p><b>KLANG v1</b> ist eine KFIELD-Instanz für Sprache: CoNLL-U-Sprachdaten werden als KSTREAM001-Symbolstrom kanonisiert, durch KDNA projiziert, als KGRAM gelernt und in einer Cross-Matrix geprüft.</p>
</div>
<h2>Matrix</h2>
<table><tr><th>row</th><th>col</th><th>accuracy</th><th>lift</th><th>surprise</th><th>out_of_grammar</th></tr>
$rowsHtml
</table>
<h2>Nullmodell-Zusammenfassung</h2>
<table><tr><th>mode</th><th>cells</th><th>avg observed acc</th><th>avg null acc</th><th>avg delta</th><th>max null acc</th><th>min delta</th></tr>
$sumHtml
</table>
<div class="card">
<p>Die relevante Hidden-Grammar-Metrik ist nicht nur Cross-Accuracy, sondern <b>observed-minus-null</b>. Ein stabil positives Delta zeigt sequenzordnungsabhängige Struktur.</p>
</div>
<h2>Artefakte</h2>
<ul>
<li><a href="tables/matrix.csv">tables/matrix.csv</a></li>
<li><a href="tables/null_joined.csv">tables/null_joined.csv</a></li>
<li><a href="tables/null_summary.csv">tables/null_summary.csv</a></li>
</ul>
</body></html>
"@
$html | Set-Content $htmlPath -Encoding UTF8

Write-Host "KLANG report written:"
Write-Host "  $mdPath"
Write-Host "  $htmlPath"
