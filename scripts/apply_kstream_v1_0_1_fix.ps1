param()

$target = "tests\test_kstream.c"
if (-not (Test-Path $target)) {
  throw "tests\test_kstream.c nicht gefunden. Bitte im Projekt-Hauptordner ausführen."
}

$src = Join-Path $PSScriptRoot "..\tests\test_kstream.c"
if (-not (Test-Path $src)) {
  throw "Patch-Datei fehlt: $src"
}

Copy-Item $src $target -Force
Write-Host "KSTREAM v1.0.1: tests\test_kstream.c ersetzt. Byte-Test ist jetzt Windows-LineEnding-robust."
