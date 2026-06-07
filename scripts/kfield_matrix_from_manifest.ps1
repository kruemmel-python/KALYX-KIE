param(
  [Parameter(Mandatory=$true)][string]$Manifest,
  [Parameter(Mandatory=$true)][string]$Out,
  [Parameter(Mandatory=$true)][string]$Csv,
  [double]$Train = 0.70,
  [int]$Bins = 32,
  [string]$Exe = ".\build_vs\Release\kdna_genome_matrix.exe"
)

$ErrorActionPreference = "Stop"

function Fail($m) { throw $m }

if (-not (Test-Path $Manifest)) { Fail "Manifest fehlt: $Manifest" }
if (-not (Test-Path $Exe)) { Fail "kdna_genome_matrix.exe fehlt: $Exe" }

# Robust manifest parser:
# - accepts comma or semicolon separator
# - skips empty/whitespace/comment lines
# - trims UTF-8 BOM from header
# - requires name,symbols,grammar columns
$rawLines = Get-Content $Manifest
$lines = @()
foreach ($l in $rawLines) {
  if ($null -eq $l) { continue }
  $t = [string]$l
  $t = $t.Trim()
  if ($t.Length -eq 0) { continue }
  if ($t.StartsWith("#")) { continue }
  $lines += $t
}

if ($lines.Count -lt 2) {
  Fail "Manifest enthält zu wenige Datenzeilen: $Manifest"
}

$headerLine = $lines[0].TrimStart([char]0xFEFF)
$delimiter = ","
if (($headerLine -notmatch ",") -and ($headerLine -match ";")) {
  $delimiter = ";"
}

$headers = $headerLine.Split($delimiter) | ForEach-Object { $_.Trim().Trim('"').ToLowerInvariant() }
$idxName = [Array]::IndexOf($headers, "name")
$idxSymbols = [Array]::IndexOf($headers, "symbols")
$idxGrammar = [Array]::IndexOf($headers, "grammar")

if ($idxName -lt 0 -or $idxSymbols -lt 0 -or $idxGrammar -lt 0) {
  Fail "Manifest braucht Spalten: name,symbols,grammar. Header='$headerLine'"
}

$entries = @()
for ($i = 1; $i -lt $lines.Count; $i++) {
  $line = $lines[$i]
  if ($line.Length -eq 0) { continue }
  $cols = $line.Split($delimiter)
  if ($cols.Count -le [Math]::Max($idxGrammar, [Math]::Max($idxName, $idxSymbols))) {
    Fail "Manifest-Zeile hat zu wenige Spalten: '$line'"
  }

  $name = $cols[$idxName].Trim().Trim('"')
  $symbols = $cols[$idxSymbols].Trim().Trim('"')
  $grammar = $cols[$idxGrammar].Trim().Trim('"')

  if ($name.Length -eq 0 -and $symbols.Length -eq 0 -and $grammar.Length -eq 0) {
    continue
  }
  if ($name.Length -eq 0 -or $symbols.Length -eq 0 -or $grammar.Length -eq 0) {
    Fail "Manifest-Zeile unvollständig: name='$name' symbols='$symbols' grammar='$grammar' line='$line'"
  }
  if (-not (Test-Path $symbols)) { Fail "Symbols fehlt für '$name': $symbols" }
  if (-not (Test-Path $grammar)) { Fail "Grammar fehlt für '$name': $grammar" }

  $entries += [PSCustomObject]@{
    name = $name
    symbols = $symbols
    grammar = $grammar
  }
}

if ($entries.Count -lt 2) {
  Fail "Manifest braucht mindestens 2 Einträge, gefunden: $($entries.Count)"
}

$args = @()
foreach ($e in $entries) {
  $args += "--entry"
  $args += $e.name
  $args += $e.symbols
  $args += $e.grammar
}
$args += "--out"
$args += $Out
$args += "--csv"
$args += $Csv
$args += "--train"
$args += ([System.Globalization.CultureInfo]::InvariantCulture.TextInfo.ToLower($Train.ToString([System.Globalization.CultureInfo]::InvariantCulture)))
$args += "--bins"
$args += "$Bins"

Write-Host "KFIELD matrix from manifest: entries=$($entries.Count) out=$Out csv=$Csv"
& $Exe @args
if ($LASTEXITCODE -ne 0) {
  Fail "kdna_genome_matrix failed with exit code $LASTEXITCODE"
}
