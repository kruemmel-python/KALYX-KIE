param(
  [Parameter(Mandatory=$true)][string]$Fasta,
  [Parameter(Mandatory=$true)][string]$FamilyRoot,
  [Parameter(Mandatory=$true)][string]$OutRoot,
  [string]$K = "12",
  [int]$TopFamilies = 8,
  [UInt64]$WindowBases = 1048576,
  [UInt64]$StepBases = 1048576,
  [int]$Windows = 80,
  [int]$PeriodMax = 512,
  [string]$Exe = ".\build_vs\Release\kalyx_signature_scan.exe",
  [switch]$Strict
)
$ErrorActionPreference = "Stop"
function Fail($m) { throw $m }

if (-not (Test-Path $Fasta)) { Fail "Fasta fehlt: $Fasta" }
if (-not (Test-Path $FamilyRoot)) { Fail "FamilyRoot fehlt: $FamilyRoot" }
New-Item -ItemType Directory -Force -Path $OutRoot | Out-Null

$families = Get-ChildItem $FamilyRoot -Recurse -Filter "kalyx_origin_v0_8_families_all.csv" |
  Select-Object -First 1
if (-not $families) { Fail "Keine kalyx_origin_v0_8_families_all.csv unter $FamilyRoot gefunden" }

$cmd = Join-Path (Split-Path -Parent $PSCommandPath) "kalyx_origin_signature_v0_9.ps1"
$out = Join-Path $OutRoot "genome_scan"
& $cmd `
  -Fasta $Fasta `
  -FamiliesCsv $families.FullName `
  -OutDir $out `
  -K $K `
  -TopFamilies $TopFamilies `
  -BaseStart 0 `
  -WindowBases $WindowBases `
  -StepBases $StepBases `
  -Windows $Windows `
  -PeriodMax $PeriodMax `
  -Exe $Exe `
  -Strict:$Strict

$summary = Join-Path $OutRoot "KALYX_ORIGIN_V0_9_MULTISCALE_SIGNATURE_SUMMARY.md"
$lines = New-Object System.Collections.Generic.List[string]
$lines.Add("# KALYX-ORIGIN v0.9 Multiscale/Genome Signature Summary")
$lines.Add("")
$lines.Add("- Fasta: ``$Fasta``")
$lines.Add("- FamilyRoot: ``$FamilyRoot``")
$lines.Add("- FamiliesCsv: ``$($families.FullName)``")
$lines.Add("- K: ``$K``")
$lines.Add("- TopFamilies: ``$TopFamilies``")
$lines.Add("- WindowBases: ``$WindowBases``")
$lines.Add("- StepBases: ``$StepBases``")
$lines.Add("- Windows: ``$Windows``")
$rank = Join-Path $out "kalyx_origin_v0_9_signature_ranking.csv"
if (Test-Path $rank) {
  $top = Import-Csv $rank | Select-Object -First 10
  $lines.Add("")
  $lines.Add("## Top Windows")
  $lines.Add("")
  $lines.Add("| Rank | Label | BaseStart | BaseEnd | Density | RC Balance | Period | PeriodRate | N-rate |")
  $lines.Add("|---:|---|---:|---:|---:|---:|---:|---:|---:|")
  $i=0
  foreach($r in $top) {
    $i++
    $lines.Add("| $i | $($r.label) | $($r.base_start_0) | $($r.base_end_0) | $($r.signature_density) | $($r.rc_balance) | $($r.best_period) | $($r.best_period_match_rate) | $($r.n_rate) |")
  }
}
$lines.Add("")
$lines.Add("## Interpretation Boundary")
$lines.Add("")
$lines.Add("This scan checks distribution of a learned motif-family signature. It does not prove origin.")
Set-Content -Encoding UTF8 -Path $summary -Value $lines

Write-Host "KALYX-ORIGIN v0.9 multiscale/signature complete:"
Write-Host "  $summary"
