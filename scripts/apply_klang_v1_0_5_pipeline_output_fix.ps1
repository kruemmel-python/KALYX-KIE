# KLANG v1.0.5 pipeline-output fix
# Überschreibt nur scripts\klang_2x2_experiment.ps1.
# Keine ABI-/Runtime-/C-Änderung.
# Fix: PowerShell-Funktionen dürfen Tool-Stdout nicht als Artefaktzeilen zurückgeben.

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$src = Join-Path $PSScriptRoot "klang_2x2_experiment.ps1"
$dst = Join-Path $root "scripts\klang_2x2_experiment.ps1"

if (-not (Test-Path $src)) {
  throw "Patchquelle fehlt: $src"
}

$tmp = "$dst.tmp"
Get-Content $src -Raw | Set-Content -Encoding UTF8 $tmp
Move-Item $tmp $dst -Force

Write-Host "KLANG v1.0.5: klang_2x2_experiment.ps1 ersetzt. Tool-Stdout wird nicht mehr als Row-Objekt verarbeitet."
