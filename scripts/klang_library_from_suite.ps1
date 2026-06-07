param(
  [Parameter(Mandatory=$true)][string]$SuiteDir,
  [string]$Out = "",
  [string]$Json = "",
  [string]$Csv = "",
  [string]$Exe = ".\build_vs\Release\kdna_klang_library.exe",
  [switch]$Strict
)

$ErrorActionPreference = "Stop"

function Resolve-Existing([string]$Path, [string]$Label) {
  if (!(Test-Path $Path)) {
    if ($Strict) { throw "$Label fehlt: $Path" }
    return $null
  }
  return $Path
}

function Parse-Config([string]$Name) {
  if ($Name -match "^(.*)_w([0-9]+)$") {
    return [pscustomobject]@{
      Field = $Matches[1]
      Window = [int]$Matches[2]
    }
  }
  throw "Kann KLANG-Konfiguration nicht aus Ordnernamen lesen: $Name"
}

if (!(Test-Path $SuiteDir)) { throw "SuiteDir fehlt: $SuiteDir" }
if (!(Test-Path $Exe)) { throw "kdna_klang_library.exe fehlt: $Exe" }

if ($Out -eq "")  { $Out  = Join-Path $SuiteDir "klang_library.kllib" }
if ($Json -eq "") { $Json = Join-Path $SuiteDir "klang_library.json" }
if ($Csv -eq "")  { $Csv  = Join-Path $SuiteDir "klang_library_cells.csv" }

$configDirs = Get-ChildItem $SuiteDir -Directory |
  Where-Object { $_.Name -match "_w[0-9]+$" } |
  Sort-Object Name

if ($configDirs.Count -eq 0) {
  throw "Keine KLANG-Konfigurationsordner gefunden in $SuiteDir. Erwartet z.B. upos_w2 oder upos_deprel_w2."
}

$args = @()
foreach ($dir in $configDirs) {
  $cfg = Parse-Config $dir.Name
  $field = $cfg.Field
  $w = $cfg.Window
  $prefix = "${field}_w$w"

  $deSymbols = Join-Path $dir.FullName "de_${prefix}.u64"
  $deKstream = Join-Path $dir.FullName "de_${prefix}.kstream"
  $deKdna    = Join-Path $dir.FullName "de_${prefix}_kdna.u64"
  $deKgram   = Join-Path $dir.FullName "de_${prefix}_self.kgram"

  $enSymbols = Join-Path $dir.FullName "en_${prefix}.u64"
  $enKstream = Join-Path $dir.FullName "en_${prefix}.kstream"
  $enKdna    = Join-Path $dir.FullName "en_${prefix}_kdna.u64"
  $enKgram   = Join-Path $dir.FullName "en_${prefix}_self.kgram"

  $matrixCsv = Join-Path $dir.FullName "klang_${prefix}.csv"
  $nullCsv   = Join-Path $dir.FullName "klang_${prefix}_null.csv"
  $report    = Join-Path $dir.FullName "ReportV12\KLANG_REPORT_V1_2.html"
  $obs       = Join-Path $dir.FullName "ReportV12\tables\observed_vs_null.csv"

  $paths = @($deSymbols,$deKstream,$deKdna,$deKgram,$enSymbols,$enKstream,$enKdna,$enKgram,$matrixCsv,$nullCsv,$report,$obs)
  $missing = @($paths | Where-Object { !(Test-Path $_) })
  if ($missing.Count -gt 0) {
    $msg = "Konfiguration $($dir.Name) unvollständig:`n" + ($missing -join "`n")
    if ($Strict) { throw $msg }
    Write-Warning $msg
    continue
  }

  $args += "--config"
  $args += $dir.Name
  $args += $field
  $args += [string]$w
  $args += "32"
  $args += $deSymbols
  $args += $deKstream
  $args += $deKdna
  $args += $deKgram
  $args += $enSymbols
  $args += $enKstream
  $args += $enKdna
  $args += $enKgram
  $args += $matrixCsv
  $args += $nullCsv
  $args += $report
  $args += $obs
}

if ($args.Count -eq 0) { throw "Keine vollständige KLANG-Konfiguration gefunden." }

$args += "--out";  $args += $Out
$args += "--json"; $args += $Json
$args += "--csv";  $args += $Csv

Write-Host "KLANG_LIBRARY: configs=" (($args | Where-Object { $_ -eq "--config" }).Count)
& $Exe @args

if ($LASTEXITCODE -ne 0) {
  throw "kdna_klang_library.exe fehlgeschlagen mit ExitCode $LASTEXITCODE"
}

& $Exe --a $Out

Write-Host "KLANG_LIBRARY complete:"
Write-Host "  $Out"
Write-Host "  $Json"
Write-Host "  $Csv"
