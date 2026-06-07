
param(
  [Parameter(Mandatory=$true)][string]$Fasta,
  [Parameter(Mandatory=$true)][string]$ContextCsv,
  [Parameter(Mandatory=$true)][string]$OutDir,
  [string]$KValues = "16,20",
  [int]$Top = 10,
  [int]$TopKmers = 64,
  [int]$PeriodMax = 512,
  [string]$Exe = ".\build_vs\Release\kalyx_motif_context.exe",
  [switch]$Strict
)

$ErrorActionPreference = "Stop"
function Fail($m) { throw $m }
function Read-CsvSmart($Path) {
  $raw = Get-Content $Path -Raw
  if ($raw -match ';') { return Import-Csv $Path -Delimiter ';' }
  return Import-Csv $Path
}
function Num($x) {
  if ($null -eq $x) { return 0.0 }
  return [double](("$x").Replace(",", "."))
}

if (-not (Test-Path $Fasta)) { Fail "FASTA fehlt: $Fasta" }
if (-not (Test-Path $ContextCsv)) { Fail "Context CSV fehlt: $ContextCsv" }
if (-not (Test-Path $Exe)) { Fail "kalyx_motif_context.exe fehlt: $Exe" }
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$Rows = @(Read-CsvSmart $ContextCsv | Select-Object -First $Top)
if ($Rows.Count -eq 0) { Fail "Context CSV leer: $ContextCsv" }
$Ks = @($KValues.Split(",") | ForEach-Object { [int]($_.Trim()) } | Where-Object { $_ -gt 0 })

Write-Host "=== KALYX-ORIGIN v0.7: motif / period decomposition ==="
Write-Host "fasta=$Fasta context=$ContextCsv out=$OutDir top=$Top k=$KValues topKmers=$TopKmers periodMax=$PeriodMax"

$AllSummary = Join-Path $OutDir "kalyx_origin_v0_7_motif_summary_all.csv"
$AllTop = Join-Path $OutDir "kalyx_origin_v0_7_top_kmers_all.csv"
$AllPeriods = Join-Path $OutDir "kalyx_origin_v0_7_periods_all.csv"
Remove-Item $AllSummary,$AllTop,$AllPeriods -ErrorAction SilentlyContinue

$firstSum = $true
$firstTop = $true
$firstPeriod = $true

foreach ($r in $Rows) {
  $label = [string]$r.label
  $baseStart = [UInt64](Num $r.base_start_0)
  $baseEnd   = [UInt64](Num $r.base_end_0)
  if ($baseEnd -le $baseStart) { continue }

  foreach ($k in $Ks) {
    $prefix = "${label}_k${k}"
    $local = Join-Path $OutDir $prefix
    New-Item -ItemType Directory -Force -Path $local | Out-Null

    & $Exe `
      --fasta $Fasta `
      --base-start "$baseStart" `
      --base-end "$baseEnd" `
      --label $label `
      --k "$k" `
      --top "$TopKmers" `
      --period-max "$PeriodMax" `
      --out-dir $local `
      --out-prefix $prefix

    if ($LASTEXITCODE -ne 0) { Fail "kalyx_motif_context fehlgeschlagen: $label k=$k" }

    $sum = Join-Path $local "${prefix}_summary.csv"
    $topCsv = Join-Path $local "${prefix}_top_kmers.csv"
    $perCsv = Join-Path $local "${prefix}_periods.csv"

    if ($firstSum) { Get-Content $sum | Set-Content -Encoding UTF8 $AllSummary; $firstSum=$false }
    else { Get-Content $sum | Select-Object -Skip 1 | Add-Content -Encoding UTF8 $AllSummary }

    if ($firstTop) { Get-Content $topCsv | Set-Content -Encoding UTF8 $AllTop; $firstTop=$false }
    else { Get-Content $topCsv | Select-Object -Skip 1 | Add-Content -Encoding UTF8 $AllTop }

    $periodRows = Import-Csv $perCsv | Sort-Object {[double]$_.match_rate} -Descending | Select-Object -First 16
    $tmp = Join-Path $local "${prefix}_top_periods.csv"
    $periodRows | Export-Csv -NoTypeInformation -Encoding UTF8 $tmp

    if ($firstPeriod) {
      '"label","k","period","valid_pairs","match_rate"' | Set-Content -Encoding UTF8 $AllPeriods
      $firstPeriod=$false
    }
    foreach ($pr in $periodRows) {
      ('"{0}","{1}","{2}","{3}","{4}"' -f $label,$k,$pr.period,$pr.valid_pairs,$pr.match_rate) | Add-Content -Encoding UTF8 $AllPeriods
    }
  }
}

# Build concise report
$SummaryRows = @(Import-Csv $AllSummary)
$TopRows = @(Import-Csv $AllTop | Select-Object -First 30)
$PeriodRowsAll = @(Import-Csv $AllPeriods | Select-Object -First 30)

$Report = Join-Path $OutDir "KALYX_ORIGIN_V0_7_MOTIF_REPORT.md"
$Lines = @()
$Lines += "# KALYX-ORIGIN v0.7 Motif / Period Report"
$Lines += ""
$Lines += "Motif decomposition for ORIGIN candidate windows."
$Lines += ""
$Lines += "## Inputs"
$Lines += ""
$Lines += "- Fasta: ``$Fasta``"
$Lines += "- ContextCsv: ``$ContextCsv``"
$Lines += "- Top context rows: ``$Top``"
$Lines += "- KValues: ``$KValues``"
$Lines += "- TopKmers: ``$TopKmers``"
$Lines += "- PeriodMax: ``$PeriodMax``"
$Lines += ""
$Lines += "## Summary rows"
$Lines += ""
$Lines += "| Label | k | BaseStart | BaseEnd | UniqueRatio | Entropy | TopN mass | GC | N-rate |"
$Lines += "|---|---:|---:|---:|---:|---:|---:|---:|---:|"
foreach ($s in ($SummaryRows | Select-Object -First 40)) {
  $Lines += ("| {0} | {1} | {2} | {3} | {4} | {5} | {6} | {7} | {8} |" -f `
    $s.label,$s.k,$s.base_start_0,$s.base_end_0,$s.unique_ratio,$s.entropy_bits,$s.topN_mass,$s.gc_rate,$s.n_rate)
}
$Lines += ""
$Lines += "## Top exported k-mers"
$Lines += ""
$Lines += "| Label | k | Rank | kmer | Count | Freq | RevComp | RC Count | GapMean | GapStd |"
$Lines += "|---|---:|---:|---|---:|---:|---|---:|---:|---:|"
foreach ($t in $TopRows) {
  $Lines += ("| {0} | {1} | {2} | {3} | {4} | {5} | {6} | {7} | {8} | {9} |" -f `
    $t.label,$t.k,$t.rank,$t.kmer,$t.count,$t.frequency,$t.revcomp,$t.revcomp_count,$t.gap_mean,$t.gap_std)
}
$Lines += ""
$Lines += "## Top period candidates"
$Lines += ""
$Lines += "| Label | k | Period | ValidPairs | MatchRate |"
$Lines += "|---|---:|---:|---:|---:|"
foreach ($p in $PeriodRowsAll) {
  $Lines += ("| {0} | {1} | {2} | {3} | {4} |" -f $p.label,$p.k,$p.period,$p.valid_pairs,$p.match_rate)
}
$Lines += ""
$Lines += "## Interpretation Boundary"
$Lines += ""
$Lines += "v0.7 decomposes ORIGIN candidates into explicit motif, reverse-complement and period diagnostics. It does not prove natural or artificial origin."
$Lines += ""
$Lines += "## Artefacts"
$Lines += ""
$Lines += "- ``$AllSummary``"
$Lines += "- ``$AllTop``"
$Lines += "- ``$AllPeriods``"
$Lines += "- ``$Report``"

$Lines | Set-Content -Encoding UTF8 $Report
Write-Host "KALYX-ORIGIN v0.7 motif context complete:"
Write-Host "  $AllSummary"
Write-Host "  $AllTop"
Write-Host "  $AllPeriods"
Write-Host "  $Report"
