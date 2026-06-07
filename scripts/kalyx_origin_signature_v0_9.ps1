param(
  [Parameter(Mandatory=$true)][string]$Fasta,
  [Parameter(Mandatory=$true)][string]$FamiliesCsv,
  [Parameter(Mandatory=$true)][string]$OutDir,
  [string]$K = "12",
  [int]$TopFamilies = 8,
  [UInt64]$BaseStart = 0,
  [UInt64]$WindowBases = 1048576,
  [UInt64]$StepBases = 1048576,
  [int]$Windows = 64,
  [int]$PeriodMax = 512,
  [string]$Exe = ".\build_vs\Release\kalyx_signature_scan.exe",
  [switch]$Strict
)
$ErrorActionPreference = "Stop"
function Fail($m) { throw $m }

if (-not (Test-Path $Fasta)) { Fail "Fasta fehlt: $Fasta" }
if (-not (Test-Path $FamiliesCsv)) { Fail "FamiliesCsv fehlt: $FamiliesCsv" }
if (-not (Test-Path $Exe)) { Fail "Exe fehlt: $Exe" }
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

Write-Host "=== KALYX-ORIGIN v0.9: family signature scan ==="
Write-Host "fasta=$Fasta families=$FamiliesCsv out=$OutDir k=$K topFamilies=$TopFamilies start=$BaseStart window=$WindowBases step=$StepBases windows=$Windows"

$rows = Import-Csv $FamiliesCsv | Where-Object { $_.k -eq "$K" } |
  Sort-Object {[double]($_.total_count -replace ',','.')} -Descending |
  Select-Object -First $TopFamilies

if ($rows.Count -lt 1) { Fail "Keine Familien fuer k=$K in $FamiliesCsv" }

$sig = Join-Path $OutDir "kalyx_origin_v0_9_signature_k$K.csv"
$rows | ForEach-Object {
  [pscustomobject]@{
    kmer = $_.consensus
    weight = $_.total_count
    family_id = $_.family_id
    rc_balance = $_.rc_balance
    support = $_.consensus_support
  }
} | Export-Csv -NoTypeInformation -Encoding UTF8 $sig

$outCsv = Join-Path $OutDir "kalyx_origin_v0_9_signature_scan.csv"

& $Exe `
  --fasta $Fasta `
  --signature $sig `
  --out-csv $outCsv `
  --label-prefix "sig_k$K" `
  --base-start "$BaseStart" `
  --window-bases "$WindowBases" `
  --step-bases "$StepBases" `
  --windows "$Windows" `
  --period-max "$PeriodMax"

if ($LASTEXITCODE -ne 0) { Fail "kalyx_signature_scan failed: $LASTEXITCODE" }
if (-not (Test-Path $outCsv)) { Fail "Scan CSV fehlt: $outCsv" }

$scan = Import-Csv $outCsv
$rank = $scan | Sort-Object `
  @{Expression={[double]($_.signature_density -replace ',','.')};Descending=$true}, `
  @{Expression={[double]($_.best_period_match_rate -replace ',','.')};Descending=$true}

$rankCsv = Join-Path $OutDir "kalyx_origin_v0_9_signature_ranking.csv"
$rank | Export-Csv -NoTypeInformation -Encoding UTF8 $rankCsv

$report = Join-Path $OutDir "KALYX_ORIGIN_V0_9_SIGNATURE_REPORT.md"
$lines = New-Object System.Collections.Generic.List[string]
$lines.Add("# KALYX-ORIGIN v0.9 Signature Scan Report")
$lines.Add("")
$lines.Add("Family-signature scan over FASTA windows.")
$lines.Add("")
$lines.Add("## Inputs")
$lines.Add("")
$lines.Add("- Fasta: ``$Fasta``")
$lines.Add("- FamiliesCsv: ``$FamiliesCsv``")
$lines.Add("- K: ``$K``")
$lines.Add("- TopFamilies: ``$TopFamilies``")
$lines.Add("- BaseStart: ``$BaseStart``")
$lines.Add("- WindowBases: ``$WindowBases``")
$lines.Add("- StepBases: ``$StepBases``")
$lines.Add("- Windows: ``$Windows``")
$lines.Add("")
$lines.Add("## Signature Motifs")
$lines.Add("")
$lines.Add("| Rank | Family | Kmer | Weight | RC balance | Support |")
$lines.Add("|---:|---:|---|---:|---:|---:|")
$i=0
foreach($r in $rows){
  $i++
  $lines.Add("| $i | $($r.family_id) | $($r.consensus) | $($r.total_count) | $($r.rc_balance) | $($r.consensus_support) |")
}
$lines.Add("")
$lines.Add("## Top Signature Windows")
$lines.Add("")
$lines.Add("| Rank | Label | BaseStart | BaseEnd | Hits | RC Hits | Density | RC Balance | BestPeriod | PeriodRate | GC | N-rate |")
$lines.Add("|---:|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|")
$j=0
foreach($r in ($rank | Select-Object -First 20)){
  $j++
  $lines.Add("| $j | $($r.label) | $($r.base_start_0) | $($r.base_end_0) | $($r.total_hits) | $($r.total_rc_hits) | $($r.signature_density) | $($r.rc_balance) | $($r.best_period) | $($r.best_period_match_rate) | $($r.gc_rate) | $($r.n_rate) |")
}
$lines.Add("")
$lines.Add("## Interpretation Boundary")
$lines.Add("")
$lines.Add("v0.9 tests whether a motif-family signature is locally enriched or distributed across FASTA windows. It does not prove origin.")
$lines.Add("")
$lines.Add("## Artefacts")
$lines.Add("")
$lines.Add("- ``$sig``")
$lines.Add("- ``$outCsv``")
$lines.Add("- ``$rankCsv``")
$lines.Add("- ``$report``")
Set-Content -Encoding UTF8 -Path $report -Value $lines

Write-Host "KALYX-ORIGIN v0.9 complete:"
Write-Host "  $sig"
Write-Host "  $outCsv"
Write-Host "  $rankCsv"
Write-Host "  $report"

if ($Strict) {
  $top = $rank | Select-Object -First 1
  if (-not $top) { Fail "Ranking leer" }
}
