# KLANG v1.0.4 blank-manifest-line fix
# Überschreibt nur scripts\kfield_matrix_from_manifest.ps1.
# Keine ABI-/Runtime-/C-Änderung.

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$src = Join-Path $PSScriptRoot "kfield_matrix_from_manifest.ps1"
$dst = Join-Path $root "scripts\kfield_matrix_from_manifest.ps1"

if (-not (Test-Path $src)) {
  throw "Patchquelle fehlt: $src"
}

# Write via temp file to avoid partial writes.
$tmp = "$dst.tmp"
Get-Content $src -Raw | Set-Content -Encoding UTF8 $tmp
Move-Item $tmp $dst -Force

Write-Host "KLANG v1.0.4: kfield_matrix_from_manifest.ps1 ersetzt. Leere Manifest-Zeilen werden ignoriert."
