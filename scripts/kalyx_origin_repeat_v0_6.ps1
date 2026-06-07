
param(
  [Parameter(Mandatory=$true)][string]$Fasta,
  [Parameter(Mandatory=$true)][string]$ContextCsv,
  [Parameter(Mandatory=$true)][string]$OutDir,
  [int[]]$KValues = @(8,12,16,20),
  [int]$Top = 20,
  [string]$Exe = ".\build_vs\Release\kalyx_repeat_context.exe",
  [switch]$Strict
)

$ErrorActionPreference = "Stop"

function Fail($m) { throw $m }

if (-not (Test-Path $Fasta)) { Fail "FASTA fehlt: $Fasta" }
if (-not (Test-Path $ContextCsv)) { Fail "ContextCsv fehlt: $ContextCsv" }
if (-not (Test-Path $Exe)) { Fail "kalyx_repeat_context.exe fehlt: $Exe" }

Remove-Item -Recurse -Force $OutDir -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force $OutDir | Out-Null

$Rows = Import-Csv $ContextCsv | Select-Object -First $Top
if ($Rows.Count -eq 0) { Fail "ContextCsv enthält keine Zeilen: $ContextCsv" }

$AllCsv = Join-Path $OutDir "kalyx_origin_v0_6_repeat_context.csv"
Remove-Item -Force $AllCsv -ErrorAction SilentlyContinue

Write-Host "=== KALYX-ORIGIN v0.6: raw FASTA k-mer/repeat context ==="
Write-Host "fasta=$Fasta context=$ContextCsv top=$Top k=$($KValues -join ',')"

$first = $true
foreach ($k in $KValues) {
  foreach ($r in $Rows) {
    $label = "$($r.label)_k$k"
    $baseStart = [UInt64]$r.base_start_0
    $baseEnd = [UInt64]$r.base_end_0
    $args = @(
      "--fasta", $Fasta,
      "--label", $label,
      "--base-start", "$baseStart",
      "--base-end", "$baseEnd",
      "--k", "$k",
      "--out-csv", $AllCsv
    )
    if (-not $first) { $args += "--append" }
    & $Exe @args
    if ($LASTEXITCODE -ne 0) { Fail "kalyx_repeat_context fehlgeschlagen: $label" }
    $first = $false
  }
}

$Data = Import-Csv $AllCsv

function Num($x) {
  if ($null -eq $x -or "$x" -eq "") { return 0.0 }
  return [double](("$x").Replace(",", "."))
}

$SummaryRows = foreach ($group in ($Data | Group-Object label)) {
  $rows = $group.Group
  $k16 = $rows | Where-Object { $_.k -eq "16" } | Select-Object -First 1
  if ($null -eq $k16) { $k16 = $rows | Select-Object -First 1 }
  [PSCustomObject]@{
    label = ($group.Name -replace "_k\d+$","")
    base_start_0 = $k16.base_start_0
    base_end_0 = $k16.base_end_0
    n_rate = Num $k16.n_rate
    gc_rate = Num $k16.gc_rate
    k16_unique_ratio = Num $k16.unique_kmer_ratio
    k16_entropy = Num $k16.kmer_entropy_bits
    k16_top32 = Num $k16.kmer_top32_mass
    longest_homopolymer = [UInt64]$k16.longest_homopolymer
    longest_dinuc_tandem_bases = [UInt64]$k16.longest_dinuc_tandem_bases
    min_unique_ratio = (($rows | ForEach-Object { Num $_.unique_kmer_ratio }) | Measure-Object -Minimum).Minimum
    max_top32 = (($rows | ForEach-Object { Num $_.kmer_top32_mass }) | Measure-Object -Maximum).Maximum
    min_entropy = (($rows | ForEach-Object { Num $_.kmer_entropy_bits }) | Measure-Object -Minimum).Minimum
  }
}

$SummaryCsv = Join-Path $OutDir "kalyx_origin_v0_6_repeat_summary.csv"
$SummaryRows | Export-Csv -NoTypeInformation -Encoding UTF8 $SummaryCsv

$Ranked = $SummaryRows | Sort-Object @{Expression="max_top32";Descending=$true}, @{Expression="min_unique_ratio";Descending=$false}
$Report = Join-Path $OutDir "KALYX_ORIGIN_V0_6_REPEAT_REPORT.md"

$Lines = @()
$Lines += "# KALYX-ORIGIN v0.6 Repeat / Raw k-mer Context Report"
$Lines += ""
$Lines += "Raw FASTA-level k-mer and repeat diagnostics for ORIGIN candidate windows."
$Lines += ""
$Lines += "## Inputs"
$Lines += ""
$Lines += "- Fasta: ``$Fasta``"
$Lines += "- ContextCsv: ``$ContextCsv``"
$Lines += "- Top: ``$Top``"
$Lines += "- KValues: ``$($KValues -join ',')``"
$Lines += ""
$Lines += "## Top repeat/k-mer concentration rows"
$Lines += ""
$Lines += "| Rank | Label | BaseStart0 | BaseEnd0 | N-rate | GC | k16 unique ratio | k16 entropy | k16 top32 | max top32 | min unique ratio | homopolymer | dinuc tandem |"
$Lines += "|---:|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|"
$i = 1
foreach ($r in ($Ranked | Select-Object -First 20)) {
  $Lines += "| $i | $($r.label) | $($r.base_start_0) | $($r.base_end_0) | $("{0:F12}" -f $r.n_rate) | $("{0:F12}" -f $r.gc_rate) | $("{0:F12}" -f $r.k16_unique_ratio) | $("{0:F9}" -f $r.k16_entropy) | $("{0:F12}" -f $r.k16_top32) | $("{0:F12}" -f $r.max_top32) | $("{0:F12}" -f $r.min_unique_ratio) | $($r.longest_homopolymer) | $($r.longest_dinuc_tandem_bases) |"
  $i++
}
$Lines += ""
$Lines += "## Interpretation Boundary"
$Lines += ""
$Lines += "v0.6 tests whether ORIGIN anomalies are visible already at raw FASTA k-mer/repeat level. It does not prove natural or artificial origin."
$Lines += ""
$Lines += "## Artefacts"
$Lines += ""
$Lines += "- ``$AllCsv``"
$Lines += "- ``$SummaryCsv``"
$Lines += "- ``$Report``"

[System.IO.File]::WriteAllLines($Report, $Lines, [System.Text.UTF8Encoding]::new($false))

Write-Host "KALYX-ORIGIN v0.6 repeat context complete:"
Write-Host "  $AllCsv"
Write-Host "  $SummaryCsv"
Write-Host "  $Report"
