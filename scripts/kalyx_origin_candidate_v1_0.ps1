param(
  [Parameter(Mandatory=$true)][string]$CandidateName,
  [Parameter(Mandatory=$true)][UInt64]$CandidateStart,
  [Parameter(Mandatory=$true)][UInt64]$CandidateEnd,
  [Parameter(Mandatory=$true)][UInt64]$ExtendedStart,
  [Parameter(Mandatory=$true)][UInt64]$ExtendedEnd,
  [Parameter(Mandatory=$true)][string]$Fasta,
  [Parameter(Mandatory=$true)][string]$ProjectLog,
  [Parameter(Mandatory=$true)][string]$OutDir,
  [switch]$Strict
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version 2.0

function Fail([string]$Message) {
  if ($Strict) { throw $Message }
  Write-Warning $Message
}

function Need-File([string]$Path, [string]$Name) {
  if (-not (Test-Path $Path)) {
    Fail "$Name fehlt: $Path"
    return $false
  }
  return $true
}

function Read-CsvSafe([string]$Path) {
  if (-not (Test-Path $Path)) { return @() }
  return @(Import-Csv $Path)
}

function First-OrNull($Rows) {
  if ($null -eq $Rows) { return $null }
  $a = @($Rows)
  if ($a.Count -eq 0) { return $null }
  return $a[0]
}

function Get-Field($Obj, [string]$Name, [string]$Default = "") {
  if ($null -eq $Obj) { return $Default }
  $p = $Obj.PSObject.Properties[$Name]
  if ($null -eq $p) { return $Default }
  if ($null -eq $p.Value) { return $Default }
  return [string]$p.Value
}

function Add-Evidence([string]$Stage, [string]$Metric, [string]$Value, [string]$Source) {
  $script:Evidence += [pscustomobject]@{
    stage  = $Stage
    metric = $Metric
    value  = $Value
    source = $Source
  }
}

function Add-Line([string]$Text) {
  $script:Lines += $Text
}

function Short-Hash([string]$Path) {
  if (-not (Test-Path $Path)) { return "" }
  try {
    return (Get-FileHash $Path -Algorithm SHA256).Hash
  } catch {
    return ""
  }
}

$ProjectRoot = (Get-Location).Path
New-Item -ItemType Directory -Force $OutDir | Out-Null
$TablesDir = Join-Path $OutDir "tables"
New-Item -ItemType Directory -Force $TablesDir | Out-Null

# Canonical input artefacts from the ORIGIN chain.
$V03Ranking = ".\Origin_chr17_v03\kalyx_origin_v0_3_ranking.csv"
$V04ZoomRanking = ".\Origin_chr17_v04_zoom_20_27M_w262k\kalyx_origin_v0_4_ranking.csv"
$V04MultiSummary = ".\Origin_chr17_v04_multiscale\KALYX_ORIGIN_V0_4_MULTISCALE_SUMMARY.md"
$V05Context = ".\Origin_chr17_v05_context_zoom\kalyx_origin_v0_5_fasta_context.csv"
$V06Repeat = ".\Origin_chr17_v06_repeat_zoom\kalyx_origin_v0_6_repeat_summary.csv"
$V07MotifSummary = ".\Origin_chr17_v07_motif_zoom\kalyx_origin_v0_7_motif_summary_all.csv"
$V07TopKmers = ".\Origin_chr17_v07_motif_zoom\kalyx_origin_v0_7_top_kmers_all.csv"
$V07Periods = ".\Origin_chr17_v07_motif_zoom\kalyx_origin_v0_7_periods_all.csv"
$V08Families = ".\Origin_chr17_v08_family_zoom\kalyx_origin_v0_8_families_all.csv"
$V08Members = ".\Origin_chr17_v08_family_zoom\kalyx_origin_v0_8_members_all.csv"
$V09Signature = ".\Origin_chr17_v09_signature_scan\kalyx_origin_v0_9_signature_k12.csv"
$V09Ranking = ".\Origin_chr17_v09_signature_scan\kalyx_origin_v0_9_signature_ranking.csv"

[void](Need-File $Fasta "FASTA")
[void](Need-File $ProjectLog "ProjectLog")
[void](Need-File $V09Ranking "v0.9 ranking")
[void](Need-File $V09Signature "v0.9 signature")
[void](Need-File $V08Families "v0.8 families")
[void](Need-File $V07TopKmers "v0.7 top kmers")
[void](Need-File $V06Repeat "v0.6 repeat summary")
[void](Need-File $V05Context "v0.5 context")
[void](Need-File $V04ZoomRanking "v0.4 zoom ranking")
[void](Need-File $V03Ranking "v0.3 ranking")

$Evidence = @()

# Read data.
$v03 = Read-CsvSafe $V03Ranking
$v04 = Read-CsvSafe $V04ZoomRanking
$v05 = Read-CsvSafe $V05Context
$v06 = Read-CsvSafe $V06Repeat
$v07s = Read-CsvSafe $V07MotifSummary
$v07k = Read-CsvSafe $V07TopKmers
$v07p = Read-CsvSafe $V07Periods
$v08 = Read-CsvSafe $V08Families
$v09s = Read-CsvSafe $V09Signature
$v09r = Read-CsvSafe $V09Ranking

$v03Top = First-OrNull $v03
$v04Top = First-OrNull $v04
$v05Top = First-OrNull $v05
$v06Top = First-OrNull $v06
$v07Top = First-OrNull $v07s
$v07KTop = First-OrNull $v07k
$v07PTop = First-OrNull $v07p
$v08Top = First-OrNull $v08
$v09SigTop = First-OrNull $v09s
$v09Top = First-OrNull $v09r

# Evidence table.
Add-Evidence "v0.3" "patternless_top_window" (("{0} score={1}" -f (Get-Field $v03Top "label"), (Get-Field $v03Top "origin_anomaly_score"))) $V03Ranking
Add-Evidence "v0.4" "zoom_top_window" (("{0} skip={1} score={2}" -f (Get-Field $v04Top "label"), (Get-Field $v04Top "skip_symbols"), (Get-Field $v04Top "origin_anomaly_score"))) $V04ZoomRanking
Add-Evidence "v0.5" "fasta_context_top" (("base={0}..{1} n_rate={2} gc={3}" -f (Get-Field $v05Top "base_start_0"), (Get-Field $v05Top "base_end_0"), (Get-Field $v05Top "n_rate"), (Get-Field $v05Top "gc_rate"))) $V05Context
Add-Evidence "v0.6" "raw_kmer_top" (("base={0}..{1} k16_unique_ratio={2} k16_entropy={3} k16_top32={4}" -f (Get-Field $v06Top "base_start_0"), (Get-Field $v06Top "base_end_0"), (Get-Field $v06Top "k16_unique_ratio"), (Get-Field $v06Top "k16_entropy"), (Get-Field $v06Top "k16_top32"))) $V06Repeat
Add-Evidence "v0.7" "top_motif" (("label={0} k={1} kmer={2} count={3} rc_count={4} gap_mean={5}" -f (Get-Field $v07KTop "label"), (Get-Field $v07KTop "k"), (Get-Field $v07KTop "kmer"), (Get-Field $v07KTop "count"), (Get-Field $v07KTop "revcomp_count"), (Get-Field $v07KTop "gap_mean"))) $V07TopKmers
Add-Evidence "v0.7" "top_period" (("label={0} k={1} period={2} match_rate={3}" -f (Get-Field $v07PTop "label"), (Get-Field $v07PTop "k"), (Get-Field $v07PTop "period"), (Get-Field $v07PTop "match_rate"))) $V07Periods
Add-Evidence "v0.8" "top_family" (("consensus={0} total_count={1} rc_balance={2} labels={3}" -f (Get-Field $v08Top "consensus"), (Get-Field $v08Top "total_count"), (Get-Field $v08Top "rc_balance"), (Get-Field $v08Top "labels_count"))) $V08Families
Add-Evidence "v0.9" "signature_top_motif" (("kmer={0} weight={1} rc_balance={2} support={3}" -f (Get-Field $v09SigTop "kmer"), (Get-Field $v09SigTop "weight"), (Get-Field $v09SigTop "rc_balance"), (Get-Field $v09SigTop "support"))) $V09Signature
Add-Evidence "v0.9" "signature_top_window" (("base={0}..{1} hits={2} density={3} rc_hits={4} n_rate={5}" -f (Get-Field $v09Top "base_start_0"), (Get-Field $v09Top "base_end_0"), (Get-Field $v09Top "total_hits"), (Get-Field $v09Top "signature_density"), (Get-Field $v09Top "total_rc_hits"), (Get-Field $v09Top "n_rate"))) $V09Ranking

$EvidenceCsv = Join-Path $TablesDir "kalyx_origin_v1_0_evidence.csv"
$Evidence | Export-Csv -NoTypeInformation -Encoding UTF8 $EvidenceCsv

# Copy compact source tables for immutability.
$SourceList = @(
  $V03Ranking,$V04ZoomRanking,$V05Context,$V06Repeat,$V07MotifSummary,
  $V07TopKmers,$V07Periods,$V08Families,$V08Members,$V09Signature,$V09Ranking
)
foreach ($p in $SourceList) {
  if (Test-Path $p) {
    Copy-Item -Force $p (Join-Path $TablesDir (Split-Path -Leaf $p))
  }
}

# Project log digest and small build/test extraction.
$ProjectLogHash = Short-Hash $ProjectLog
$FastaHash = Short-Hash $Fasta
$LogText = ""
if (Test-Path $ProjectLog) {
  $LogText = Get-Content $ProjectLog -Raw
}

$BuildEvidence = @()
if ($LogText -match "100% tests passed, 0 tests failed out of 67") {
  $BuildEvidence += "KGENOME base stack: 67/67 tests passed."
}
if ($LogText -match "kdna_project: source=.*chr17_k16\.u64.*backend=opencl") {
  $BuildEvidence += "chr17 KDNA projection executed through OpenCL path."
}
if ($LogText -match "chr17 full symbols=82919499") {
  $BuildEvidence += "chr17 full FASTA symbol stream contains 82,919,499 k16 symbols."
}
if ($LogText -match "kdna_fasta_symbols: in=.*chr17\.fa.*symbols=82919499") {
  $BuildEvidence += "chr17 FASTA-to-k16 symbolization recorded."
}

$CandidateJson = [pscustomobject]@{
  version = "KALYX_ORIGIN_CANDIDATE_V1_0_1"
  candidate_name = $CandidateName
  coordinate_system = "0-based FASTA base coordinates"
  candidate = [pscustomobject]@{
    start_0 = [UInt64]$CandidateStart
    end_0 = [UInt64]$CandidateEnd
    length = [UInt64]($CandidateEnd - $CandidateStart)
  }
  extended = [pscustomobject]@{
    start_0 = [UInt64]$ExtendedStart
    end_0 = [UInt64]$ExtendedEnd
    length = [UInt64]($ExtendedEnd - $ExtendedStart)
  }
  fasta = [pscustomobject]@{
    path = $Fasta
    sha256 = $FastaHash
  }
  project_log = [pscustomobject]@{
    path = $ProjectLog
    sha256 = $ProjectLogHash
    extracted_evidence = $BuildEvidence
  }
  top_signature_window = $v09Top
  top_family = $v08Top
  top_motif = $v07KTop
  top_period = $v07PTop
  evidence_csv = $EvidenceCsv
  interpretation_boundary = "KALYX-ORIGIN v1.0.2 consolidates diagnostics. It does not prove natural or artificial origin."
  generated_utc = (Get-Date).ToUniversalTime().ToString("o")
}

$JsonPath = Join-Path $OutDir "kalyx_origin_v1_0_candidate.json"
$CandidateJson | ConvertTo-Json -Depth 8 | Set-Content -Encoding UTF8 $JsonPath

# Command trail.
$CommandTrailPath = Join-Path $OutDir "KALYX_ORIGIN_V1_0_COMMAND_TRAIL.md"
$Trail = @()
$Trail += "# KALYX-ORIGIN v1.0 Command Trail"
$Trail += ""
$Trail += "This file records the reproducible candidate consolidation layer."
$Trail += ""
$Trail += '```powershell'
$Trail += ".\scripts\kalyx_origin_candidate_v1_0.ps1 ``"
$Trail += "  -CandidateName $CandidateName ``"
$Trail += "  -CandidateStart $CandidateStart ``"
$Trail += "  -CandidateEnd $CandidateEnd ``"
$Trail += "  -ExtendedStart $ExtendedStart ``"
$Trail += "  -ExtendedEnd $ExtendedEnd ``"
$Trail += "  -Fasta $Fasta ``"
$Trail += "  -ProjectLog $ProjectLog ``"
$Trail += "  -OutDir $OutDir ``"
$Trail += "  -Strict"
$Trail += '```'
$Trail += ""
$Trail += "Upstream artefacts:"
foreach ($p in $SourceList) { $Trail += ("- " + $p) }
$Trail | Set-Content -Encoding UTF8 $CommandTrailPath

# Markdown report.
$ReportPath = Join-Path $OutDir "KALYX_ORIGIN_V1_0_CANDIDATE_REPORT.md"
$Lines = @()

Add-Line "# KALYX-ORIGIN v1.0 Candidate Report"
Add-Line ""
Add-Line "Candidate consolidation report for the chr17 ORIGIN chain."
Add-Line ""
Add-Line "## Candidate Coordinates"
Add-Line ""
Add-Line "| Field | Value |"
Add-Line "|---|---:|"
Add-Line "| Candidate | $CandidateName |"
Add-Line "| Primary start 0-based | $CandidateStart |"
Add-Line "| Primary end 0-based | $CandidateEnd |"
Add-Line ("| Primary length | {0} |" -f ($CandidateEnd - $CandidateStart))
Add-Line "| Extended start 0-based | $ExtendedStart |"
Add-Line "| Extended end 0-based | $ExtendedEnd |"
Add-Line ("| Extended length | {0} |" -f ($ExtendedEnd - $ExtendedStart))
Add-Line ""
Add-Line "## Reproducibility Anchors"
Add-Line ""
Add-Line "| Artefact | SHA256 |"
Add-Line "|---|---|"
Add-Line "| FASTA | $FastaHash |"
Add-Line "| Project log | $ProjectLogHash |"
Add-Line ""
Add-Line "## Extracted Build / Runtime Evidence"
Add-Line ""
if ($BuildEvidence.Count -gt 0) {
  foreach ($b in $BuildEvidence) { Add-Line "- $b" }
} else {
  Add-Line "- No build evidence pattern was extracted from the project log."
}
Add-Line ""
Add-Line "## Evidence Table"
Add-Line ""
Add-Line "| Stage | Metric | Value |"
Add-Line "|---|---|---|"
foreach ($e in $Evidence) {
  Add-Line ("| {0} | {1} | {2} |" -f $e.stage, $e.metric, (($e.value -replace "\|","/")))
}
Add-Line ""
Add-Line "## Signature Motifs"
Add-Line ""
Add-Line "| kmer | weight | family | rc_balance | support |"
Add-Line "|---|---:|---:|---:|---:|"
foreach ($m in @($v09s | Select-Object -First 12)) {
  Add-Line ("| {0} | {1} | {2} | {3} | {4} |" -f (Get-Field $m "kmer"), (Get-Field $m "weight"), (Get-Field $m "family_id"), (Get-Field $m "rc_balance"), (Get-Field $m "support"))
}
Add-Line ""
Add-Line "## Top Signature Windows"
Add-Line ""
Add-Line "| Rank | Label | BaseStart | BaseEnd | Hits | Density | RC Hits | RC Balance | GC | N-rate |"
Add-Line "|---:|---|---:|---:|---:|---:|---:|---:|---:|---:|"
$rank = 0
foreach ($r in @($v09r | Select-Object -First 12)) {
  $rank++
  Add-Line ("| {0} | {1} | {2} | {3} | {4} | {5} | {6} | {7} | {8} | {9} |" -f $rank, (Get-Field $r "label"), (Get-Field $r "base_start_0"), (Get-Field $r "base_end_0"), (Get-Field $r "total_hits"), (Get-Field $r "signature_density"), (Get-Field $r "total_rc_hits"), (Get-Field $r "rc_balance"), (Get-Field $r "gc_rate"), (Get-Field $r "n_rate"))
}
Add-Line ""
Add-Line "## Consolidated Interpretation"
Add-Line ""
Add-Line "KALYX-ORIGIN v1.0 consolidates the chr17 candidate as a localized, directionally biased motif-family signature. The primary coordinate range is the high-density signature core. The extended range includes the transition flanks."
Add-Line ""
Add-Line "This is an origin candidate, not an origin proof. The report distinguishes structure from genesis. Natural repeat architecture, assembly context, projection effects and non-natural generation remain separate hypotheses until externally tested."
Add-Line ""
Add-Line "## Artefacts"
Add-Line ""
Add-Line '- kalyx_origin_v1_0_candidate.json'
Add-Line '- tables/kalyx_origin_v1_0_evidence.csv'
Add-Line '- KALYX_ORIGIN_V1_0_COMMAND_TRAIL.md'
Add-Line '- copied upstream CSV tables in tables/'
$Lines | Set-Content -Encoding UTF8 $ReportPath

Write-Host "KALYX-ORIGIN v1.0 candidate report complete:"
Write-Host "  $ReportPath"
Write-Host "  $JsonPath"
Write-Host "  $EvidenceCsv"
Write-Host "  $CommandTrailPath"
