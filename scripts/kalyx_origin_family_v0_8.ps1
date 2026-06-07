
param(
  [Parameter(Mandatory=$true)][string]$TopKmersCsv,
  [Parameter(Mandatory=$true)][string]$OutDir,
  [string]$KValues = "0",
  [int]$Hamming = 2,
  [UInt64]$MinCount = 1,
  [UInt64]$MaxRows = 20000,
  [int]$TopFamilies = 25,
  [switch]$RcLink,
  [string]$Exe = ".\build_vs\Release\kalyx_motif_family.exe",
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

if (-not (Test-Path $TopKmersCsv)) { Fail "Top-kmers CSV fehlt: $TopKmersCsv" }
if (-not (Test-Path $Exe)) { Fail "kalyx_motif_family.exe fehlt: $Exe" }
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$Ks = @($KValues.Split(",") | ForEach-Object { [int]($_.Trim()) } | Where-Object { $_ -ge 0 })
if ($Ks.Count -eq 0) { $Ks = @(0) }

Write-Host "=== KALYX-ORIGIN v0.8: motif-family / consensus clustering ==="
Write-Host "top=$TopKmersCsv out=$OutDir k=$KValues hamming=$Hamming minCount=$MinCount rcLink=$($RcLink.IsPresent)"

$RunDirs = @()
foreach ($k in $Ks) {
  $label = if ($k -eq 0) { "all_k" } else { "k$k" }
  $RunDir = Join-Path $OutDir $label
  New-Item -ItemType Directory -Force -Path $RunDir | Out-Null
  $args = @(
    "--top-kmers", $TopKmersCsv,
    "--out-dir", $RunDir,
    "--label", $label,
    "--hamming", "$Hamming",
    "--min-count", "$MinCount",
    "--max-rows", "$MaxRows"
  )
  if ($k -gt 0) { $args += @("--k", "$k") }
  if ($RcLink) { $args += @("--rc-link", "1") } else { $args += @("--rc-link", "0") }
  & $Exe @args
  if ($LASTEXITCODE -ne 0) { Fail "kalyx_motif_family failed for $label rc=$LASTEXITCODE" }
  $RunDirs += $RunDir
}

$AllFamilies = Join-Path $OutDir "kalyx_origin_v0_8_families_all.csv"
$AllMembers  = Join-Path $OutDir "kalyx_origin_v0_8_members_all.csv"
$AllEdges    = Join-Path $OutDir "kalyx_origin_v0_8_edges_all.csv"
Remove-Item $AllFamilies,$AllMembers,$AllEdges -ErrorAction SilentlyContinue

$firstF = $true; $firstM = $true; $firstE = $true
foreach ($d in $RunDirs) {
  $f = Join-Path $d "kalyx_origin_v0_8_families.csv"
  $m = Join-Path $d "kalyx_origin_v0_8_members.csv"
  $e = Join-Path $d "kalyx_origin_v0_8_edges.csv"
  if (Test-Path $f) {
    $lines = Get-Content $f
    if ($firstF) { $lines | Set-Content -Encoding UTF8 $AllFamilies; $firstF=$false }
    else { $lines | Select-Object -Skip 1 | Add-Content -Encoding UTF8 $AllFamilies }
  }
  if (Test-Path $m) {
    $lines = Get-Content $m
    if ($firstM) { $lines | Set-Content -Encoding UTF8 $AllMembers; $firstM=$false }
    else { $lines | Select-Object -Skip 1 | Add-Content -Encoding UTF8 $AllMembers }
  }
  if (Test-Path $e) {
    $lines = Get-Content $e
    if ($firstE) { $lines | Set-Content -Encoding UTF8 $AllEdges; $firstE=$false }
    else { $lines | Select-Object -Skip 1 | Add-Content -Encoding UTF8 $AllEdges }
  }
}

$Families = @()
if (Test-Path $AllFamilies) {
  $Families = @(Read-CsvSmart $AllFamilies | Sort-Object {[UInt64]($_.total_count)} -Descending)
}

$Report = Join-Path $OutDir "KALYX_ORIGIN_V0_8_FAMILY_REPORT.md"
$Lines = New-Object System.Collections.Generic.List[string]
$Lines.Add("# KALYX-ORIGIN v0.8 Motif-Family Report")
$Lines.Add("")
$Lines.Add("Consensus and motif-family clustering for KALYX-ORIGIN v0.7 top-kmer outputs.")
$Lines.Add("")
$Lines.Add("## Inputs")
$Lines.Add("")
$Lines.Add("- TopKmersCsv: ``$TopKmersCsv``")
$Lines.Add("- KValues: ``$KValues``")
$Lines.Add("- Hamming: ``$Hamming``")
$Lines.Add("- MinCount: ``$MinCount``")
$Lines.Add("- RcLink: ``$($RcLink.IsPresent)``")
$Lines.Add("")
$Lines.Add("## Top Motif Families")
$Lines.Add("")
$Lines.Add("| Rank | k | Family | Size | TotalCount | Consensus | Support | RC balance | Top member | Labels |")
$Lines.Add("|---:|---:|---:|---:|---:|---|---:|---:|---|---:|")
$rank = 1
foreach ($f in ($Families | Select-Object -First $TopFamilies)) {
  $Lines.Add(("| {0} | {1} | {2} | {3} | {4} | {5} | {6:N6} | {7:N6} | {8} | {9} |" -f `
    $rank,$f.k,$f.family_id,$f.size,$f.total_count,$f.consensus,(Num $f.consensus_support),(Num $f.rc_balance),$f.top_member,$f.labels_count))
  $rank++
}
$Lines.Add("")
$Lines.Add("## Interpretation Boundary")
$Lines.Add("")
$Lines.Add("v0.8 clusters motif carriers into Hamming-neighborhood families and reports consensus, family mass and reverse-complement balance. It does not prove natural or artificial origin.")
$Lines.Add("")
$Lines.Add("## Artefacts")
$Lines.Add("")
$Lines.Add("- ``$AllFamilies``")
$Lines.Add("- ``$AllMembers``")
$Lines.Add("- ``$AllEdges``")
$Lines | Set-Content -Encoding UTF8 $Report

Write-Host "KALYX-ORIGIN v0.8 complete:"
Write-Host "  $Report"
Write-Host "  $AllFamilies"
Write-Host "  $AllMembers"
Write-Host "  $AllEdges"
