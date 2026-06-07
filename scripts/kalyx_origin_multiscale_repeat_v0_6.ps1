
param(
  [Parameter(Mandatory=$true)][string]$Fasta,
  [Parameter(Mandatory=$true)][string]$ContextRoot,
  [Parameter(Mandatory=$true)][string]$OutRoot,
  [int[]]$KValues = @(8,12,16,20),
  [int]$Top = 10,
  [string]$Exe = ".\build_vs\Release\kalyx_repeat_context.exe",
  [switch]$Strict
)

$ErrorActionPreference = "Stop"
function Fail($m) { throw $m }

if (-not (Test-Path $Fasta)) { Fail "FASTA fehlt: $Fasta" }
if (-not (Test-Path $ContextRoot)) { Fail "ContextRoot fehlt: $ContextRoot" }

Remove-Item -Recurse -Force $OutRoot -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force $OutRoot | Out-Null

$Dirs = Get-ChildItem $ContextRoot -Directory | Where-Object { Test-Path (Join-Path $_.FullName "kalyx_origin_v0_5_fasta_context.csv") } | Sort-Object Name
if ($Dirs.Count -eq 0) { Fail "Keine v0.5 context dirs gefunden in $ContextRoot" }

$Summary = @()
foreach ($d in $Dirs) {
  $ctx = Join-Path $d.FullName "kalyx_origin_v0_5_fasta_context.csv"
  $out = Join-Path $OutRoot $d.Name
  .\scripts\kalyx_origin_repeat_v0_6.ps1 `
    -Fasta $Fasta `
    -ContextCsv $ctx `
    -OutDir $out `
    -KValues $KValues `
    -Top $Top `
    -Exe $Exe `
    -Strict:$Strict

  $sumCsv = Join-Path $out "kalyx_origin_v0_6_repeat_summary.csv"
  $topRow = Import-Csv $sumCsv | Sort-Object @{Expression={ [double](($_.max_top32).Replace(",", ".")) };Descending=$true} | Select-Object -First 1
  $Summary += [PSCustomObject]@{
    scale = $d.Name
    top_label = $topRow.label
    base_start_0 = $topRow.base_start_0
    base_end_0 = $topRow.base_end_0
    n_rate = $topRow.n_rate
    gc_rate = $topRow.gc_rate
    max_top32 = $topRow.max_top32
    min_unique_ratio = $topRow.min_unique_ratio
    k16_entropy = $topRow.k16_entropy
    report = (Join-Path $out "KALYX_ORIGIN_V0_6_REPEAT_REPORT.md")
  }
}

$SummaryCsv = Join-Path $OutRoot "kalyx_origin_v0_6_multiscale_repeat_summary.csv"
$Summary | Export-Csv -NoTypeInformation -Encoding UTF8 $SummaryCsv

$Report = Join-Path $OutRoot "KALYX_ORIGIN_V0_6_MULTISCALE_REPEAT_SUMMARY.md"
$Lines = @()
$Lines += "# KALYX-ORIGIN v0.6 Multiscale Repeat / k-mer Summary"
$Lines += ""
$Lines += "| Scale | Top label | BaseStart0 | BaseEnd0 | N-rate | GC | max top32 | min unique ratio | k16 entropy |"
$Lines += "|---|---|---:|---:|---:|---:|---:|---:|---:|"
foreach ($r in $Summary) {
  $Lines += "| $($r.scale) | $($r.top_label) | $($r.base_start_0) | $($r.base_end_0) | $($r.n_rate) | $($r.gc_rate) | $($r.max_top32) | $($r.min_unique_ratio) | $($r.k16_entropy) |"
}
$Lines += ""
$Lines += "## Artefacts"
$Lines += ""
$Lines += "- ``$SummaryCsv``"
foreach ($r in $Summary) { $Lines += "- ``$($r.report)``" }

[System.IO.File]::WriteAllLines($Report, $Lines, [System.Text.UTF8Encoding]::new($false))

Write-Host "KALYX-ORIGIN v0.6 multiscale repeat complete:"
Write-Host "  $SummaryCsv"
Write-Host "  $Report"
