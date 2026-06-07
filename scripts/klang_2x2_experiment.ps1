param(
  [Parameter(Mandatory=$true)][string]$DeConllu,
  [Parameter(Mandatory=$true)][string]$EnConllu,
  [string]$OutDir = ".\LangOut",
  [string]$Field = "upos",
  [int]$Window = 2,
  [ValidateSet(32,64)][int]$SymbolBits = 32,
  [double]$Train = 0.70,
  [string]$Backend = "cpu",
  [string]$Kernel = "kernels\kdna_eval.cl",

  # KLANG v1.0.1:
  #   0 = automatisch aus Testbereichsgröße bestimmen.
  #   >0 = explizit verwenden.
  [UInt64]$NullSamples = 0,

  [UInt64]$MinAutoNullSamples = 10000,
  [UInt64]$MaxAutoNullSamples = 1000000,

  [UInt64]$Seed = 0x4b4c414e475631
)

$ErrorActionPreference = "Stop"

# KLANG v1.0.2:
# PowerShell besitzt die automatische Variable $Input. Deshalb darf die
# interne Convert-One-Funktion keinen Parameter "Input" verwenden.
# Zusätzlich wird "gpu" als Nutzeralias für den vorhandenen OpenCL-Backendpfad
# normalisiert.
if ($Backend -eq "gpu") {
  $Backend = "opencl"
}

if (-not (Test-Path $DeConllu)) { throw "Deutsch-CoNLL-U fehlt: $DeConllu" }
if (-not (Test-Path $EnConllu)) { throw "Englisch-CoNLL-U fehlt: $EnConllu" }
if (-not (Test-Path ".\build_vs\Release\kdna_stream_conllu.exe")) { throw "kdna_stream_conllu.exe fehlt. Erst KLANG kompilieren." }
if (-not (Test-Path ".\build_vs\Release\kdna_field_null_matrix.exe")) { throw "kdna_field_null_matrix.exe fehlt. Erst KLANG kompilieren." }

New-Item -ItemType Directory -Force $OutDir | Out-Null

function Get-U64Count {
  param([Parameter(Mandatory=$true)][string]$Path)
  if (-not (Test-Path $Path)) {
    throw "Symbolstrom nicht gefunden: $Path"
  }
  $len = (Get-Item $Path).Length
  if (($len % 8) -ne 0) {
    throw "Symbolstrom ist kein uint64[n]-Payload: $Path length=$len"
  }
  return [UInt64]($len / 8)
}


function Get-KgramRuleCount {
  param([Parameter(Mandatory=$true)][string]$Path)
  if (-not (Test-Path $Path)) { throw "KGRAM nicht gefunden: $Path" }
  $len = (Get-Item $Path).Length
  if ($len -lt 128) { throw "KGRAM zu klein: $Path length=$len" }
  $payload = $len - 128
  if (($payload % 256) -ne 0) {
    throw "KGRAM payload ist kein Vielfaches von 256: $Path payload=$payload"
  }
  return [UInt64]($payload / 256)
}

function Get-TestEdges {
  param(
    [Parameter(Mandatory=$true)][UInt64]$N,
    [Parameter(Mandatory=$true)][double]$Train
  )
  if ($N -lt 3) { return [UInt64]0 }
  $trainN = [UInt64][Math]::Floor(([double]$N) * $Train)
  if ($trainN -ge ($N - 1)) { return [UInt64]0 }
  $testN = $N - $trainN
  if ($testN -lt 2) { return [UInt64]0 }
  return [UInt64]($testN - 1)
}

function Clamp-U64 {
  param([UInt64]$Value, [UInt64]$Min, [UInt64]$Max)
  if ($Value -lt $Min) { return $Min }
  if ($Value -gt $Max) { return $Max }
  return $Value
}

function Select-AutoNullSamples {
  param(
    [Parameter(Mandatory=$true)]$Rows,
    [Parameter(Mandatory=$true)][double]$Train,
    [UInt64]$MinSamples,
    [UInt64]$MaxSamples
  )

  $minEdges = [UInt64]::MaxValue
  $totalEdges = [UInt64]0

  foreach ($r in $Rows) {
    $n = Get-U64Count -Path $r.symbols
    $edges = Get-TestEdges -N $n -Train $Train
    if ($edges -lt 1) {
      throw "Zu wenige Testübergänge für $($r.name): n=$n train=$Train"
    }
    if ($edges -lt $minEdges) { $minEdges = $edges }
    $totalEdges += $edges
  }

  # Heuristik:
  # - kleine Korpora: mehr Resampling pro realem Test-Edge, aber gedeckelt
  # - mittlere Korpora: stabile Standardgröße
  # - große Korpora: mehr Samples, aber nicht unkontrolliert
  [UInt64]$candidate = 0

  if ($minEdges -lt 5000) {
    $candidate = [UInt64]($minEdges * 20)
    $candidate = Clamp-U64 -Value $candidate -Min 1000 -Max 50000
  } elseif ($minEdges -lt 50000) {
    $candidate = [UInt64]($minEdges * 10)
    $candidate = Clamp-U64 -Value $candidate -Min $MinSamples -Max 200000
  } elseif ($minEdges -lt 500000) {
    $candidate = 200000
  } elseif ($minEdges -lt 5000000) {
    $candidate = 500000
  } else {
    $candidate = 1000000
  }

  $candidate = Clamp-U64 -Value $candidate -Min $MinSamples -Max $MaxSamples

  [PSCustomObject]@{
    samples = $candidate
    min_test_edges = $minEdges
    total_test_edges = $totalEdges
  }
}

function Convert-One {
  param([string]$Name, [string]$ConlluPath)

  $raw = Join-Path $OutDir "$Name`_$Field`_w$Window.u64"
  $sum = Join-Path $OutDir "$Name`_$Field`_w$Window.kstream"
  $kdna = Join-Path $OutDir "$Name`_$Field`_w$Window`_kdna.u64"
  $gram = Join-Path $OutDir "$Name`_$Field`_w$Window`_self.kgram"

  Write-Host "=== KLANG CoNLL-U -> KSTREAM: $Name field=$Field window=$Window ==="
  & .\build_vs\Release\kdna_stream_conllu.exe `
    --in $ConlluPath `
    --out $raw `
    --summary $sum `
    --field $Field `
    --window $Window `
    --symbol-bits $SymbolBits `
    --sentence-boundary reset 2>&1 | ForEach-Object { Write-Host $_ }

  if ($LASTEXITCODE -ne 0) {
    throw "kdna_stream_conllu fehlgeschlagen für $Name ($ConlluPath), exit=$LASTEXITCODE"
  }
  if (-not (Test-Path $raw)) { throw "KLANG raw u64 wurde nicht erzeugt: $raw" }
  if (-not (Test-Path $sum)) { throw "KLANG KSTREAM summary wurde nicht erzeugt: $sum" }

  & .\build_vs\Release\kdna_stream_probe.exe $sum 2>&1 | ForEach-Object { Write-Host $_ }

  Write-Host "=== KSTREAM -> KDNA -> KGRAM: $Name ==="
  & .\scripts\kstream_project_and_gram.ps1 `
    -Symbols $raw `
    -OutSymbols $kdna `
    -OutGrammar $gram `
    -Backend $Backend `
    -Kernel $Kernel `
    -Train $Train 2>&1 | ForEach-Object { Write-Host $_ }

  if (-not (Test-Path $kdna)) { throw "KDNA u64 wurde nicht erzeugt: $kdna" }
  if (-not (Test-Path $gram)) { throw "KGRAM wurde nicht erzeugt: $gram" }

  $rules = Get-KgramRuleCount -Path $gram
  if ($rules -le 1) {
    throw "KLANG_GRAMMAR_DEGENERATE: $Name erzeugte nur $rules KGRAM-Regel(n). Prüfe Symbolraum/Feld/Window. Für KLANG v1.1 sollte --symbol-bits 32 aktiv sein."
  }

  [PSCustomObject]@{
    name = $Name
    raw = $raw
    symbols = $kdna
    grammar = $gram
    summary = $sum
    rules = $rules
  }
}

$rows = @()
$rows += Convert-One -Name "de" -ConlluPath $DeConllu
$rows += Convert-One -Name "en" -ConlluPath $EnConllu

# KLANG v1.0.5:
# PowerShell-Funktionen geben alles zurück, was im Success-Stream landet.
# Externe Tools und Hilfsskripte können Text in diesen Stream schreiben.
# Deshalb wird die Artefaktliste hier hart auf echte Row-Objekte gefiltert.
$rows = @(
  $rows | Where-Object {
    $_ -is [pscustomobject] -and
    $null -ne $_.PSObject.Properties["name"] -and
    $null -ne $_.PSObject.Properties["symbols"] -and
    $null -ne $_.PSObject.Properties["grammar"] -and
    ([string]$_.name).Length -gt 0 -and
    ([string]$_.symbols).Length -gt 0 -and
    ([string]$_.grammar).Length -gt 0
  }
)

if ($rows.Count -ne 2) {
  throw "KLANG erwartete genau 2 Artefaktzeilen (de,en), gefunden: $($rows.Count). Prüfe Tool-Ausgaben und Script-Pipeline."
}

$manifest = Join-Path $OutDir "klang_manifest.csv"

# KLANG v1.0.3:
# Export-Csv kann je nach PowerShell/Locale ein anderes Trennzeichen erzeugen.
# KFIELD erwartet kanonisch name,symbols,grammar mit Komma. Wir schreiben das
# Manifest deshalb hart und kulturunabhängig.
$manifestLines = New-Object System.Collections.Generic.List[string]
$manifestLines.Add("name,symbols,grammar")
foreach ($r in $rows) {
  $manifestLines.Add(("{0},{1},{2}" -f $r.name, $r.symbols, $r.grammar))
}
$manifestLines | Set-Content -Encoding ASCII $manifest

$matrix = Join-Path $OutDir "klang_$Field`_w$Window.kfield"
$csv = Join-Path $OutDir "klang_$Field`_w$Window.csv"

Write-Host "=== KLANG 2x2 KFIELD Matrix ==="
.\scripts\kfield_matrix_from_manifest.ps1 `
  -Manifest $manifest `
  -Out $matrix `
  -Csv $csv `
  -Train $Train `
  -Bins 32

$chosenNullSamples = $NullSamples
$autoInfo = $null
if ($chosenNullSamples -eq 0) {
  $autoInfo = Select-AutoNullSamples `
    -Rows $rows `
    -Train $Train `
    -MinSamples $MinAutoNullSamples `
    -MaxSamples $MaxAutoNullSamples
  $chosenNullSamples = [UInt64]$autoInfo.samples
  Write-Host "=== KLANG Auto NullSamples ==="
  Write-Host "  min_test_edges=$($autoInfo.min_test_edges)"
  Write-Host "  total_test_edges=$($autoInfo.total_test_edges)"
  Write-Host "  selected_null_samples_per_cell=$chosenNullSamples"
} else {
  Write-Host "=== KLANG explicit NullSamples ==="
  Write-Host "  selected_null_samples_per_cell=$chosenNullSamples"
}

$runManifest = Join-Path $OutDir "klang_run_manifest.json"
$runObj = [PSCustomObject]@{
  version = "KLANG v1.1"
  de_conllu = $DeConllu
  en_conllu = $EnConllu
  field = $Field
  window = $Window
  symbol_bits = $SymbolBits
  train = $Train
  backend = $Backend
  null_samples = $chosenNullSamples
  null_samples_mode = $(if ($NullSamples -eq 0) { "auto" } else { "explicit" })
  min_auto_null_samples = $MinAutoNullSamples
  max_auto_null_samples = $MaxAutoNullSamples
  min_test_edges = $(if ($autoInfo -ne $null) { $autoInfo.min_test_edges } else { $null })
  total_test_edges = $(if ($autoInfo -ne $null) { $autoInfo.total_test_edges } else { $null })
  manifest = $manifest
  matrix_csv = $csv
  de_rules = ($rows | Where-Object { $_.name -eq "de" } | Select-Object -ExpandProperty rules)
  en_rules = ($rows | Where-Object { $_.name -eq "en" } | Select-Object -ExpandProperty rules)
}
$runObj | ConvertTo-Json -Depth 4 | Set-Content -Encoding UTF8 $runManifest

$nullCsv = Join-Path $OutDir "klang_$Field`_w$Window`_null.csv"
Write-Host "=== KLANG true null models ==="
.\build_vs\Release\kdna_field_null_matrix.exe `
  --manifest $manifest `
  --out $nullCsv `
  --sample-per-cell $chosenNullSamples `
  --train $Train `
  --seed $Seed `
  --block-size 256 `
  --rotation-offset 64

Write-Host "=== KLANG Report ==="
.\scripts\klang_report.ps1 `
  -MatrixCsv $csv `
  -NullCsv $nullCsv `
  -OutDir (Join-Path $OutDir "Report") `
  -Title "KLANG v1.1 $Field window=$Window symbol_bits=$SymbolBits"

Write-Host "KLANG v1.1 experiment complete:"
Write-Host "  $manifest"
Write-Host "  $csv"
Write-Host "  $nullCsv"
Write-Host "  $runManifest"
Write-Host "  $(Join-Path $OutDir 'Report\KLANG_REPORT.html')"
