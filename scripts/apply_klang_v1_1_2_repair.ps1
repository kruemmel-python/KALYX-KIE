param(
  [string]$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
)

$ErrorActionPreference = "Stop"

$payloadRoot = Join-Path $PSScriptRoot "..\payload"
$srcConllu = Join-Path $payloadRoot "tools\kdna_stream_conllu.c"
$srcTest   = Join-Path $payloadRoot "tests\test_kstream.c"

$dstConllu = Join-Path $ProjectRoot "tools\kdna_stream_conllu.c"
$dstTest   = Join-Path $ProjectRoot "tests\test_kstream.c"

if (-not (Test-Path $srcConllu)) { throw "Payload fehlt: $srcConllu" }
if (-not (Test-Path $srcTest))   { throw "Payload fehlt: $srcTest" }
if (-not (Test-Path (Join-Path $ProjectRoot "CMakeLists.txt"))) {
  throw "ProjectRoot scheint kein Projekt-Hauptordner zu sein: $ProjectRoot"
}

$srcText = Get-Content $srcConllu -Raw
if ($srcText -notmatch "--symbol-bits") {
  throw "Payload ist falsch: kdna_stream_conllu.c enthält kein --symbol-bits"
}

New-Item -ItemType Directory -Force (Join-Path $ProjectRoot "tools") | Out-Null
New-Item -ItemType Directory -Force (Join-Path $ProjectRoot "tests") | Out-Null

Copy-Item $srcConllu $dstConllu -Force
Copy-Item $srcTest   $dstTest   -Force

Write-Host "KLANG v1.1.2 Repair angewendet:"
Write-Host "  $dstConllu"
Write-Host "  $dstTest"
Write-Host "Erwartung nach Build: kdna_stream_conllu.exe --help zeigt --symbol-bits 32|64."
