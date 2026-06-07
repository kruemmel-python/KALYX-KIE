param(
  [string]$Dir = ".\GenomeOutFull",
  [string]$MatrixCsv = ".\GenomeOutFull\human_chr1_22_XY_full_k16.csv",
  [string]$NullCsv = ".\GenomeOutFull\ReportV3\null_models.csv",
  [string]$OutDir = ".\GenomeOutFull\ReportV3",
  [int]$SamplePerCell = 200000,
  [double]$Train = 0.70,
  [UInt64]$Seed = 0x4b444e414e554c4c,
  [UInt64]$BlockSize = 4096,
  [UInt64]$RotationOffset = 4096,
  [switch]$SkipNullRun
)

$ErrorActionPreference = "Stop"
$ci = [System.Globalization.CultureInfo]::InvariantCulture

function D([object]$x) {
  if ($null -eq $x) { return 0.0 }
  return [double]::Parse(($x.ToString().Replace(',','.')), $ci)
}

function R6([double]$x) { return [Math]::Round($x, 6) }
function R12([double]$x) { return [Math]::Round($x, 12) }

New-Item -ItemType Directory -Force $OutDir | Out-Null
New-Item -ItemType Directory -Force (Join-Path $OutDir "tables") | Out-Null
New-Item -ItemType Directory -Force (Join-Path $OutDir "controls") | Out-Null

$nullExe = ".\build_vs\Release\kdna_null_matrix.exe"
if (-not $SkipNullRun) {
  if (-not (Test-Path $nullExe)) { throw "kdna_null_matrix.exe fehlt. Erst scripts\apply_kgenome_report_v3_cmake_patch.ps1 ausführen und Release bauen." }
  & $nullExe `
    --dir $Dir `
    --human24 `
    --out $NullCsv `
    --mode all `
    --sample-per-cell $SamplePerCell `
    --train $Train `
    --seed $Seed `
    --block-size $BlockSize `
    --rotation-offset $RotationOffset
  if ($LASTEXITCODE -ne 0) { throw "kdna_null_matrix.exe fehlgeschlagen mit Code $LASTEXITCODE" }
}

if (-not (Test-Path $MatrixCsv)) { throw "MatrixCsv fehlt: $MatrixCsv" }
if (-not (Test-Path $NullCsv)) { throw "NullCsv fehlt: $NullCsv" }

$obs = Import-Csv $MatrixCsv
$nul = Import-Csv $NullCsv

# Build observed lookup by row/col
$obsMap = @{}
foreach ($r in $obs) {
  $obsMap["$($r.row)|$($r.col)"] = $r
}

$joined = foreach ($n in $nul) {
  $key = "$($n.row)|$($n.col)"
  if ($obsMap.ContainsKey($key)) {
    $o = $obsMap[$key]
    $obsAcc = D $o.kgram_accuracy
    $nullAcc = D $n.null_kgram_accuracy
    [PSCustomObject]@{
      mode = $n.mode
      row = $n.row
      col = $n.col
      observed_accuracy = $obsAcc
      observed_lift = D $o.lift
      observed_surprise = D $o.surprise_rate
      null_accuracy = $nullAcc
      null_surprise = D $n.null_surprise
      delta_accuracy = $obsAcc - $nullAcc
      samples = [UInt64]$n.samples
    }
  }
}

$joined | Export-Csv (Join-Path $OutDir "tables\null_joined.csv") -NoTypeInformation -Encoding UTF8

$modes = $joined | Group-Object mode
$summary = foreach ($g in $modes) {
  $items = @($g.Group)
  $n = [double]$items.Count
  $avgObs = ($items | ForEach-Object { $_.observed_accuracy } | Measure-Object -Average).Average
  $avgNull = ($items | ForEach-Object { $_.null_accuracy } | Measure-Object -Average).Average
  $avgDelta = ($items | ForEach-Object { $_.delta_accuracy } | Measure-Object -Average).Average
  $maxNull = ($items | ForEach-Object { $_.null_accuracy } | Measure-Object -Maximum).Maximum
  $minDelta = ($items | ForEach-Object { $_.delta_accuracy } | Measure-Object -Minimum).Minimum
  [PSCustomObject]@{
    mode = $g.Name
    cells = [int]$n
    avg_observed_accuracy = R12 $avgObs
    avg_null_accuracy = R12 $avgNull
    avg_delta_accuracy = R12 $avgDelta
    max_null_accuracy = R12 $maxNull
    min_delta_accuracy = R12 $minDelta
  }
}
$summary | Export-Csv (Join-Path $OutDir "controls\null_summary.csv") -NoTypeInformation -Encoding UTF8

$topDelta = $joined | Sort-Object delta_accuracy -Descending | Select-Object -First 50
$lowDelta = $joined | Sort-Object delta_accuracy | Select-Object -First 50
$topNull = $joined | Sort-Object null_accuracy -Descending | Select-Object -First 50

$topDelta | Export-Csv (Join-Path $OutDir "tables\top_observed_minus_null.csv") -NoTypeInformation -Encoding UTF8
$lowDelta | Export-Csv (Join-Path $OutDir "tables\lowest_observed_minus_null.csv") -NoTypeInformation -Encoding UTF8
$topNull | Export-Csv (Join-Path $OutDir "tables\top_null_accuracy.csv") -NoTypeInformation -Encoding UTF8

$manifest = [PSCustomObject]@{
  generated_at = (Get-Date).ToString("o")
  input = [PSCustomObject]@{
    dir = $Dir
    matrix_csv = $MatrixCsv
    null_csv = $NullCsv
  }
  null = [PSCustomObject]@{
    sample_per_cell = $SamplePerCell
    train = $Train
    seed = ("0x{0:x}" -f $Seed)
    block_size = $BlockSize
    rotation_offset = $RotationOffset
    modes = @($summary.mode)
  }
  summary = $summary
}
$manifest | ConvertTo-Json -Depth 6 | Set-Content (Join-Path $OutDir "report_v3_manifest.json") -Encoding UTF8

$md = @()
$md += "# KGENOME_REPORT v3 — echte Nullmodell-Berechnung"
$md += ""
$md += "Dieser Report nutzt vorhandene KDNA-Streams und vorhandene KGRAM01-Grammatiken. FASTA, KMAP, KDNA und KGRAM werden nicht neu erzeugt."
$md += ""
$md += "## Aktive Nullmodelle"
$md += ""
$md += "- **symbol_shuffle**: from/to werden unabhängig aus dem Testbereich des KDNA-Stroms gezogen. Lokale Reihenfolge wird zerstört, marginale Symbolverteilung bleibt erhalten."
$md += "- **rotation**: to wird mit festem Offset aus demselben Teststrom gelesen. Direkte Adjazenz wird zerstört, Feldverteilung bleibt erhalten."
$md += "- **block_boundary**: Übergänge werden aus künstlichen Blockgrenzen erzeugt. Lokale Blöcke bleiben als Reservoir erhalten, echte Nachbarschaftsgrenzen werden ersetzt."
$md += ""
$md += "## Nullmodell-Zusammenfassung"
$md += ""
$md += "| mode | cells | avg observed acc | avg null acc | avg delta | max null acc | min delta |"
$md += "| --- | ---: | ---: | ---: | ---: | ---: | ---: |"
foreach ($s in $summary) {
  $md += "| $($s.mode) | $($s.cells) | $($s.avg_observed_accuracy) | $($s.avg_null_accuracy) | $($s.avg_delta_accuracy) | $($s.max_null_accuracy) | $($s.min_delta_accuracy) |"
}
$md += ""
$md += "## Interpretation"
$md += ""
$md += "Wenn `avg_delta_accuracy` deutlich positiv bleibt, hängt die beobachtete KGRAM-Kopplung nicht nur an der marginalen Häufigkeit der KDNA-Symbole, sondern an der Übergangsordnung."
$md += ""
$md += "## Artefakte"
$md += ""
$md += "- controls/null_summary.csv"
$md += "- tables/null_joined.csv"
$md += "- tables/top_observed_minus_null.csv"
$md += "- tables/lowest_observed_minus_null.csv"
$md += "- tables/top_null_accuracy.csv"
$md += "- report_v3_manifest.json"

$mdPath = Join-Path $OutDir "KGENOME_REPORT_V3.md"
$md -join "`n" | Set-Content $mdPath -Encoding UTF8

$html = @"
<!doctype html><html lang="de"><head><meta charset="utf-8"><title>KGENOME_REPORT v3</title>
<style>
body{font-family:Segoe UI,Arial,sans-serif;margin:32px;line-height:1.45;color:#111}
.card{border:1px solid #ddd;border-radius:12px;padding:16px;margin:16px 0;background:#fafafa}
table{border-collapse:collapse}td,th{border:1px solid #ccc;padding:6px 10px}th{background:#eee}
code{background:#f4f4f4;padding:2px 4px;border-radius:4px}
</style></head><body>
<h1>KGENOME_REPORT v3 — echte Nullmodell-Berechnung</h1>
<p>Keine FASTA/KMAP/KGRAM-Neuberechnung. Es werden vorhandene KDNA-u64-Ströme gegen vorhandene KGRAM01-Grammatiken unter zerstörter Sequenzordnung getestet.</p>
<div class="card"><b>Sample pro Zelle:</b> $SamplePerCell<br><b>Train:</b> $Train<br><b>Block:</b> $BlockSize<br><b>Rotation:</b> $RotationOffset</div>
<h2>Nullmodell-Zusammenfassung</h2>
<table><tr><th>mode</th><th>cells</th><th>avg observed acc</th><th>avg null acc</th><th>avg delta</th><th>max null acc</th><th>min delta</th></tr>
"@
foreach ($s in $summary) {
  $html += "<tr><td>$($s.mode)</td><td>$($s.cells)</td><td>$($s.avg_observed_accuracy)</td><td>$($s.avg_null_accuracy)</td><td>$($s.avg_delta_accuracy)</td><td>$($s.max_null_accuracy)</td><td>$($s.min_delta_accuracy)</td></tr>`n"
}
$html += @"
</table>
<h2>Artefakte</h2>
<ul>
<li><a href="controls/null_summary.csv">controls/null_summary.csv</a></li>
<li><a href="tables/null_joined.csv">tables/null_joined.csv</a></li>
<li><a href="tables/top_observed_minus_null.csv">tables/top_observed_minus_null.csv</a></li>
<li><a href="tables/lowest_observed_minus_null.csv">tables/lowest_observed_minus_null.csv</a></li>
<li><a href="tables/top_null_accuracy.csv">tables/top_null_accuracy.csv</a></li>
</ul>
</body></html>
"@
$html | Set-Content (Join-Path $OutDir "KGENOME_REPORT_V3.html") -Encoding UTF8

Write-Host "KGENOME_REPORT v3 written:"
Write-Host "  $mdPath"
Write-Host "  $(Join-Path $OutDir 'KGENOME_REPORT_V3.html')"
Write-Host "  $(Join-Path $OutDir 'controls\null_summary.csv')"
