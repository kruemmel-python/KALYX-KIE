
param(
  [Parameter(Mandatory=$true)][string]$Fasta,
  [Parameter(Mandatory=$true)][string]$ContextRoot,
  [Parameter(Mandatory=$true)][string]$OutRoot,
  [string]$KValues = "16,20",
  [int]$Top = 5,
  [int]$TopKmers = 64,
  [int]$PeriodMax = 512,
  [switch]$Strict
)

$ErrorActionPreference = "Stop"
function Fail($m) { throw $m }

if (-not (Test-Path $Fasta)) { Fail "FASTA fehlt: $Fasta" }
if (-not (Test-Path $ContextRoot)) { Fail "ContextRoot fehlt: $ContextRoot" }
New-Item -ItemType Directory -Force -Path $OutRoot | Out-Null

$dirs = Get-ChildItem $ContextRoot -Directory | Where-Object { Test-Path (Join-Path $_.FullName "kalyx_origin_v0_5_fasta_context.csv") } | Sort-Object Name
if (-not $dirs) { Fail "Keine v0.5 context directories gefunden in $ContextRoot" }

$Summary = Join-Path $OutRoot "KALYX_ORIGIN_V0_7_MULTISCALE_MOTIF_SUMMARY.md"
$L = @("# KALYX-ORIGIN v0.7 Multiscale Motif Summary","")
foreach ($d in $dirs) {
  $ctx = Join-Path $d.FullName "kalyx_origin_v0_5_fasta_context.csv"
  $out = Join-Path $OutRoot $d.Name
  & .\scripts\kalyx_origin_motif_v0_7.ps1 `
    -Fasta $Fasta `
    -ContextCsv $ctx `
    -OutDir $out `
    -KValues $KValues `
    -Top $Top `
    -TopKmers $TopKmers `
    -PeriodMax $PeriodMax `
    -Strict:$Strict

  $sumCsv = Join-Path $out "kalyx_origin_v0_7_motif_summary_all.csv"
  $topCsv = Join-Path $out "kalyx_origin_v0_7_top_kmers_all.csv"
  $rows = @(Import-Csv $sumCsv | Select-Object -First 4)
  $tops = @(Import-Csv $topCsv | Select-Object -First 4)
  $L += "## $($d.Name)"
  $L += ""
  $L += "- Context: ``$ctx``"
  $L += "- Output: ``$out``"
  foreach ($r in $rows) {
    $L += ("- {0} k={1}: unique_ratio={2}, entropy={3}, topN_mass={4}, GC={5}, N={6}" -f $r.label,$r.k,$r.unique_ratio,$r.entropy_bits,$r.topN_mass,$r.gc_rate,$r.n_rate)
  }
  if ($tops.Count -gt 0) {
    $t=$tops[0]
    $L += ("- top motif example: label={0}, k={1}, kmer={2}, freq={3}, revcomp={4}, rc_count={5}" -f $t.label,$t.k,$t.kmer,$t.frequency,$t.revcomp,$t.revcomp_count)
  }
  $L += ""
}
$L | Set-Content -Encoding UTF8 $Summary
Write-Host "KALYX-ORIGIN v0.7 multiscale motif complete:"
Write-Host "  $Summary"
