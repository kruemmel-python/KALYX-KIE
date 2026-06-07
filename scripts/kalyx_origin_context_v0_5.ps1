
param(
  [Parameter(Mandatory=$true)][string]$Fasta,
  [Parameter(Mandatory=$true)][string]$RankingCsv,
  [Parameter(Mandatory=$true)][string]$OutDir,
  [UInt32]$K = 16,
  [UInt64]$WindowSymbols = 262144,
  [Int32]$Top = 20,
  [string]$Exe = ".\build_vs\Release\kalyx_fasta_context.exe",
  [switch]$Strict
)
$ErrorActionPreference = "Stop"
function Fail($m) { throw $m }

if (-not (Test-Path $Fasta)) { Fail "FASTA fehlt: $Fasta" }
if (-not (Test-Path $RankingCsv)) { Fail "RankingCsv fehlt: $RankingCsv" }
if (-not (Test-Path $Exe)) { Fail "kalyx_fasta_context fehlt: $Exe" }

New-Item -ItemType Directory -Force $OutDir | Out-Null
$OutCsv = Join-Path $OutDir "kalyx_origin_v0_5_fasta_context.csv"
$Report = Join-Path $OutDir "KALYX_ORIGIN_V0_5_CONTEXT_REPORT.md"
Remove-Item -Force $OutCsv -ErrorAction SilentlyContinue

$rows = Import-Csv $RankingCsv
if ($rows.Count -eq 0) { Fail "RankingCsv leer: $RankingCsv" }

$sel = $rows | Select-Object -First $Top
Write-Host "=== KALYX-ORIGIN v0.5: Coordinate + FASTA Context ==="
Write-Host "fasta=$Fasta ranking=$RankingCsv top=$Top window_symbols=$WindowSymbols k=$K"

$first = $true
foreach ($r in $sel) {
  $label = [string]$r.label
  $skipRaw = [string]$r.skip_symbols
  if ([string]::IsNullOrWhiteSpace($skipRaw)) { Fail "Ranking-Zeile ohne skip_symbols: $label" }
  $skip = [UInt64]$skipRaw

  $args = @(
    "--fasta", $Fasta,
    "--label", $label,
    "--symbol-skip", "$skip",
    "--symbol-count", "$WindowSymbols",
    "--k", "$K",
    "--out-csv", $OutCsv
  )
  if (-not $first) { $args += "--append" }
  & $Exe @args
  if ($LASTEXITCODE -ne 0) {
    if ($Strict) { Fail "kalyx_fasta_context fehlgeschlagen: $label skip=$skip" }
  }
  $first = $false
}

$ctx = Import-Csv $OutCsv
$rankByN = $ctx | Sort-Object {[double]($_.n_rate -replace ",",".")} -Descending
$rankByGc = $ctx | Sort-Object {[double]($_.gc_rate -replace ",",".")} -Descending

$lines = @()
$lines += "# KALYX-ORIGIN v0.5 FASTA Context Report"
$lines += ""
$lines += "Coordinate + FASTA context for ORIGIN anomaly windows."
$lines += ""
$lines += "## Inputs"
$lines += ""
$lines += "- Fasta: ``$Fasta``"
$lines += "- RankingCsv: ``$RankingCsv``"
$lines += "- K: ``$K``"
$lines += "- WindowSymbols: ``$WindowSymbols``"
$lines += "- Top: ``$Top``"
$lines += ""
$lines += "## Top FASTA Context Rows"
$lines += ""
$lines += "| Label | SymbolSkip | BaseStart0 | BaseEnd0 | BaseLen | GC | N-rate | ValidBaseRate | Status |"
$lines += "|---|---:|---:|---:|---:|---:|---:|---:|---|"
foreach ($r in $ctx | Select-Object -First $Top) {
  $lines += "| $($r.label) | $($r.symbol_skip) | $($r.base_start_0) | $($r.base_end_0) | $($r.base_len) | $($r.gc_rate) | $($r.n_rate) | $($r.valid_base_rate) | $($r.status) |"
}
$lines += ""
$lines += "## Highest N-rate among selected windows"
$lines += ""
$lines += "| Label | N-rate | BaseStart0 | BaseEnd0 | BaseLen |"
$lines += "|---|---:|---:|---:|---:|"
foreach ($r in $rankByN | Select-Object -First ([Math]::Min(10,$ctx.Count))) {
  $lines += "| $($r.label) | $($r.n_rate) | $($r.base_start_0) | $($r.base_end_0) | $($r.base_len) |"
}
$lines += ""
$lines += "## Interpretation Boundary"
$lines += ""
$lines += "This maps KDNA symbol anomaly windows back to approximate FASTA base coordinates by counting valid A/C/G/T k-mers. It does not prove biological or artificial origin. It provides context needed to test N-content, GC shifts, low-complexity/repeat hypotheses and coordinate reproducibility."
$lines += ""
$lines += "## Artefacts"
$lines += ""
$lines += "- ``$OutCsv``"
$lines += "- ``$Report``"

[System.IO.File]::WriteAllLines($Report, $lines, [System.Text.UTF8Encoding]::new($false))

Write-Host "KALYX-ORIGIN v0.5 context complete:"
Write-Host "  $OutCsv"
Write-Host "  $Report"
