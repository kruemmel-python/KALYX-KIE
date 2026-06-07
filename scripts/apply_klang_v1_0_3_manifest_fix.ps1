$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$patchRoot = $PSScriptRoot

# Wenn dieser Scriptordner bereits der Projektordner scripts ist, liegen die
# Fix-Dateien nach ZIP-Entpacken direkt neben diesem Apply-Script. Deshalb wird
# nicht Copy-Item von sich selbst genutzt, sondern der Inhalt hart geschrieben.

$klangSrc = Join-Path $patchRoot "klang_2x2_experiment.ps1"
$kfieldSrc = Join-Path $patchRoot "kfield_matrix_from_manifest.ps1"

if (-not (Test-Path $klangSrc)) { throw "Patchdatei fehlt: $klangSrc" }
if (-not (Test-Path $kfieldSrc)) { throw "Patchdatei fehlt: $kfieldSrc" }

$klangDst = Join-Path $root "scripts\klang_2x2_experiment.ps1"
$kfieldDst = Join-Path $root "scripts\kfield_matrix_from_manifest.ps1"

Get-Content $klangSrc -Raw | Set-Content -Encoding UTF8 $klangDst
Get-Content $kfieldSrc -Raw | Set-Content -Encoding UTF8 $kfieldDst

Write-Host "KLANG v1.0.3: Manifest-Erzeugung und KFIELD-Manifest-Parser aktualisiert."
Write-Host "Fix:"
Write-Host "  - klang_manifest.csv wird kulturunabhängig mit Komma geschrieben"
Write-Host "  - kfield_matrix_from_manifest.ps1 akzeptiert Komma und Semikolon defensiv"
