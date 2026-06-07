param(
  [Parameter(Mandatory=$true)][string]$InSymbols,
  [string]$OutDir = ".\PlantOut_v02",
  [string]$Payload = "KALYX_PLANT_POSITIVE_CONTROL_RK",
  [UInt64]$MaxSymbols = 1048576,
  [UInt64]$PatternLen = 37,
  [UInt64]$Period = 149,
  [UInt64]$Offset = 17,
  [UInt64]$Jitter = 23,
  [double]$Train = 0.70,
  [UInt64]$NullSamples = 200000,
  [UInt64]$BlockSize = 4099,
  [UInt64]$RotationOffset = 4103,
  [string]$Seed = "0x4b504c414e543032",
  [double]$MinLiftOverControl = 0.05,
  [double]$MinObservedMinusNull = 0.05,
  [switch]$Strict
)

$ErrorActionPreference = "Stop"
$Inv = [System.Globalization.CultureInfo]::InvariantCulture

function Fail($m) { throw $m }
function NeedFile($p) { if (-not (Test-Path $p)) { Fail "Fehlt: $p" } }
function D([object]$x) {
  if ($null -eq $x) { return 0.0 }
  $s = [string]$x
  if ($s.Length -eq 0) { return 0.0 }
  return [double]::Parse($s, $Inv)
}
function F9([double]$x) { return $x.ToString("0.000000000", $Inv) }
function F6([double]$x) { return $x.ToString("0.000000", $Inv) }

NeedFile $InSymbols
NeedFile ".\build_vs\Release\kalyx_plant.exe"
NeedFile ".\build_vs\Release\kdna_symbol_gram.exe"
NeedFile ".\build_vs\Release\kdna_genome_matrix.exe"
NeedFile ".\build_vs\Release\kdna_field_null_matrix.exe"

if ($PatternLen -lt 5) { Fail "PatternLen muss >= 5 sein." }
if ($Period -le $PatternLen) { Fail "Period muss > PatternLen sein." }
if ($Jitter -ge [UInt64]($Period / 2)) { Fail "Jitter muss < Period/2 sein." }

Remove-Item -Recurse -Force $OutDir -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force $OutDir | Out-Null
$OutDirAbs = (Resolve-Path $OutDir).Path

$Control = Join-Path $OutDirAbs "carrier_control.u64"
$Planted = Join-Path $OutDirAbs "carrier_planted.u64"
$Pattern = Join-Path $OutDirAbs "plant_pattern.u64"
$PlantManifest = Join-Path $OutDirAbs "plant_manifest.csv"
$Positions = Join-Path $OutDirAbs "plant_positions.csv"

Write-Host "=== KALYX-PLANT v0.2: declared seeded/jittered positive control ==="

.\build_vs\Release\kalyx_plant.exe `
  --in $InSymbols `
  --out $Planted `
  --control $Control `
  --pattern-out $Pattern `
  --manifest $PlantManifest `
  --positions $Positions `
  --payload $Payload `
  --seed $Seed `
  --max-symbols $MaxSymbols `
  --pattern-len $PatternLen `
  --period $Period `
  --offset $Offset `
  --schedule jittered `
  --jitter $Jitter `
  --mode replace

foreach ($P in @($Control,$Planted,$Pattern,$PlantManifest,$Positions)) { NeedFile $P }

$N = [UInt64]((Get-Item $Control).Length / 8)
if ($N -lt 2) { Fail "zu wenig Symbole: $N" }

Write-Host "=== KGRAM induction ==="
$ControlGram = Join-Path $OutDirAbs "carrier_control_self.kgram"
$PlantedGram = Join-Path $OutDirAbs "carrier_planted_self.kgram"
$PatternGram = Join-Path $OutDirAbs "plant_pattern_self.kgram"

.\build_vs\Release\kdna_symbol_gram.exe --symbols $Control --n $N --out $ControlGram --train $Train
if ($LASTEXITCODE -ne 0) { Fail "kdna_symbol_gram control fehlgeschlagen." }

.\build_vs\Release\kdna_symbol_gram.exe --symbols $Planted --n $N --out $PlantedGram --train $Train
if ($LASTEXITCODE -ne 0) { Fail "kdna_symbol_gram planted fehlgeschlagen." }

.\build_vs\Release\kdna_symbol_gram.exe --symbols $Pattern --n $N --out $PatternGram --train $Train
if ($LASTEXITCODE -ne 0) { Fail "kdna_symbol_gram pattern fehlgeschlagen." }

foreach ($P in @($ControlGram,$PlantedGram,$PatternGram)) { NeedFile $P }

$Manifest = Join-Path $OutDirAbs "kalyx_plant_kfield_manifest.csv"
$Rows = @(
  "name,symbols,grammar",
  "control,$Control,$ControlGram",
  "planted,$Planted,$PlantedGram",
  "pattern,$Pattern,$PatternGram"
)
$Utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllLines($Manifest, $Rows, $Utf8NoBom)

$Matrix = Join-Path $OutDirAbs "kalyx_plant.kfield"
$MatrixCsv = Join-Path $OutDirAbs "kalyx_plant.csv"
$NullCsv = Join-Path $OutDirAbs "kalyx_plant_null.csv"

Write-Host "=== KALYX-PLANT v0.2 KFIELD matrix ==="
.\scripts\kfield_matrix_from_manifest.ps1 `
  -Manifest $Manifest `
  -Out $Matrix `
  -Csv $MatrixCsv `
  -Train $Train `
  -Bins 32

Write-Host "=== KALYX-PLANT v0.2 hard null models ==="
.\build_vs\Release\kdna_field_null_matrix.exe `
  --manifest $Manifest `
  --out $NullCsv `
  --mode all `
  --sample-per-cell $NullSamples `
  --train $Train `
  --seed $Seed `
  --block-size $BlockSize `
  --rotation-offset $RotationOffset

NeedFile $MatrixCsv
NeedFile $NullCsv

$matrixRows = Import-Csv $MatrixCsv
$nullRows = Import-Csv $NullCsv

function Cell($row,$col) {
  $c = $matrixRows | Where-Object { $_.row -eq $row -and $_.col -eq $col } | Select-Object -First 1
  if ($null -eq $c) { Fail "Matrixzelle fehlt: $row -> $col" }
  return $c
}
function AvgNull($row,$col) {
  $vals = @($nullRows | Where-Object { $_.row -eq $row -and $_.col -eq $col } | ForEach-Object { D $_.null_kgram_accuracy })
  if ($vals.Count -eq 0) { return 0.0 }
  return [double](($vals | Measure-Object -Average).Average)
}
function NullByMode($row,$col,$mode) {
  $v = $nullRows | Where-Object { $_.row -eq $row -and $_.col -eq $col -and $_.mode -eq $mode } | Select-Object -First 1
  if ($null -eq $v) { return 0.0 }
  return D $v.null_kgram_accuracy
}

$controlPattern = Cell "control" "pattern"
$plantedPattern = Cell "planted" "pattern"
$patternPattern = Cell "pattern" "pattern"
$plantedPlanted = Cell "planted" "planted"
$controlControl = Cell "control" "control"

$controlAcc = D $controlPattern.kgram_accuracy
$targetAcc = D $plantedPattern.kgram_accuracy
$patternAcc = D $patternPattern.kgram_accuracy
$plantedSelfAcc = D $plantedPlanted.kgram_accuracy
$controlSelfAcc = D $controlControl.kgram_accuracy

$controlNull = AvgNull "control" "pattern"
$targetNull = AvgNull "planted" "pattern"
$patternSelfBlockNull = NullByMode "pattern" "pattern" "block_boundary"
$patternSelfSymbolNull = NullByMode "pattern" "pattern" "symbol_shuffle"
$patternSelfRotationNull = NullByMode "pattern" "pattern" "rotation"

$controlDelta = $controlAcc - $controlNull
$targetDelta = $targetAcc - $targetNull
$plantLiftOverControl = $targetAcc - $controlAcc

$PassLift = $plantLiftOverControl -ge $MinLiftOverControl
$PassDelta = $targetDelta -ge $MinObservedMinusNull
$Pass = $PassLift -and $PassDelta

$Report = Join-Path $OutDirAbs "KALYX_PLANT_REPORT.md"

$manifestLines = Get-Content $PlantManifest
$manifestTable = @()
foreach ($line in $manifestLines) {
  if ($line -match "^([^,]+),(.*)$") {
    $manifestTable += "| $($Matches[1]) | $($Matches[2]) |"
  }
}

$lines = @()
$lines += "# KALYX-PLANT v0.2 Report"
$lines += ""
$lines += "Declared seeded/jittered planted-structure positive-control benchmark. This is not a covert-channel test."
$lines += ""
$lines += "## Result"
$lines += ""
$lines += "- Pass lift over control: ``$PassLift``"
$lines += "- Pass observed-minus-null: ``$PassDelta``"
$lines += "- Overall pass: ``$Pass``"
$lines += ""
$lines += "## Inputs"
$lines += ""
$lines += "- Carrier: ``$InSymbols``"
$lines += "- Payload label: ``$Payload``"
$lines += "- N: ``$N``"
$lines += "- PatternLen: ``$PatternLen``"
$lines += "- Period: ``$Period``"
$lines += "- Offset: ``$Offset``"
$lines += "- Jitter: ``$Jitter``"
$lines += "- Train: ``$($Train.ToString($Inv))``"
$lines += "- NullSamples: ``$NullSamples``"
$lines += "- BlockSize: ``$BlockSize``"
$lines += "- RotationOffset: ``$RotationOffset``"
$lines += ""
$lines += "## Key Cells"
$lines += ""
$lines += "| Cell | Accuracy | Avg Null | Delta |"
$lines += "|---|---:|---:|---:|"
$lines += "| control → pattern | $(F9 $controlAcc) | $(F9 $controlNull) | $(F9 $controlDelta) |"
$lines += "| planted → pattern | $(F9 $targetAcc) | $(F9 $targetNull) | $(F9 $targetDelta) |"
$lines += "| pattern → pattern | $(F9 $patternAcc) | n/a | n/a |"
$lines += "| control → control | $(F9 $controlSelfAcc) | n/a | n/a |"
$lines += "| planted → planted | $(F9 $plantedSelfAcc) | n/a | n/a |"
$lines += ""
$lines += "Plant lift over control: ``$(F9 $plantLiftOverControl)``"
$lines += ""
$lines += "## Target Null Profile: planted → pattern"
$lines += ""
$lines += "| Null mode | Accuracy |"
$lines += "|---|---:|"
foreach ($m in @("symbol_shuffle","rotation","block_boundary")) {
  $lines += "| $m | $(F9 (NullByMode 'planted' 'pattern' $m)) |"
}
$lines += ""
$lines += "## Pattern Self Null Profile"
$lines += ""
$lines += "| Null mode | Accuracy |"
$lines += "|---|---:|"
$lines += "| symbol_shuffle | $(F9 $patternSelfSymbolNull) |"
$lines += "| rotation | $(F9 $patternSelfRotationNull) |"
$lines += "| block_boundary | $(F9 $patternSelfBlockNull) |"
$lines += ""
$lines += "v0.2 deliberately uses prime-ish/de-aligned pattern and null parameters so ``block_boundary`` is no longer expected to remain trivially perfect for ``pattern → pattern``."
$lines += ""
$lines += "## Plant Manifest"
$lines += ""
$lines += "| Key | Value |"
$lines += "|---|---|"
$lines += $manifestTable
$lines += ""
$lines += "## Artefacts"
$lines += ""
$lines += "- ``$Control``"
$lines += "- ``$Planted``"
$lines += "- ``$Pattern``"
$lines += "- ``$PlantManifest``"
$lines += "- ``$Positions``"
$lines += "- ``$MatrixCsv``"
$lines += "- ``$NullCsv``"
$lines += ""
$lines += "## Scientific Interpretation"
$lines += ""
$lines += "The positive-control criterion is that ``planted → pattern`` must be substantially above ``control → pattern`` and above true null models. This validates inverse KALYX sensitivity to generated transition order."
$lines += ""
$lines += "## Safety Boundary"
$lines += ""
$lines += "KALYX-PLANT creates declared positive controls for scientific validation. It does not implement stealth embedding, non-detectable communication, evasion, or covert channels."

[System.IO.File]::WriteAllLines($Report, $lines, $Utf8NoBom)

if ($Strict -and -not $Pass) {
  Get-Content $Report
  Fail "KALYX-PLANT v0.2 strict criteria failed."
}

Write-Host "KALYX-PLANT v0.2 complete:"
Write-Host "  $PlantManifest"
Write-Host "  $MatrixCsv"
Write-Host "  $NullCsv"
Write-Host "  $Report"
Write-Host ""
Write-Host ("control -> pattern acc: {0}" -f (F9 $controlAcc))
Write-Host ("planted -> pattern acc: {0}" -f (F9 $targetAcc))
Write-Host ("pattern -> pattern acc: {0}" -f (F9 $patternAcc))
Write-Host ("planted -> pattern avg null: {0}" -f (F9 $targetNull))
Write-Host ("planted -> pattern delta: {0}" -f (F9 $targetDelta))
Write-Host ("plant lift over control: {0}" -f (F9 $plantLiftOverControl))
Write-Host ("pattern self block-boundary null: {0}" -f (F9 $patternSelfBlockNull))
