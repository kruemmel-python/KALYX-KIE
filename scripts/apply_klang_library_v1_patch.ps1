param(
  [string]$ProjectRoot = (Get-Location).Path
)

$ErrorActionPreference = "Stop"
$patchRoot = Split-Path -Parent $PSScriptRoot
$payload = Join-Path $patchRoot "payload"

if (!(Test-Path (Join-Path $payload "tools\kdna_klang_library.c"))) {
  throw "Payload fehlt: tools\kdna_klang_library.c"
}

Copy-Item (Join-Path $payload "include\kdna_kllib.h") (Join-Path $ProjectRoot "include\kdna_kllib.h") -Force
Copy-Item (Join-Path $payload "tools\kdna_klang_library.c") (Join-Path $ProjectRoot "tools\kdna_klang_library.c") -Force
Copy-Item (Join-Path $payload "scripts\klang_library_from_suite.ps1") (Join-Path $ProjectRoot "scripts\klang_library_from_suite.ps1") -Force
Copy-Item (Join-Path $payload "docs\KLANG_LIBRARY_V1_SPEC.md") (Join-Path $ProjectRoot "docs\KLANG_LIBRARY_V1_SPEC.md") -Force

$cmake = Join-Path $ProjectRoot "CMakeLists.txt"
$text = Get-Content $cmake -Raw

if ($text -notmatch "add_executable\(kdna_klang_library") {
  $insert = @"

add_executable(kdna_klang_library tools/kdna_klang_library.c)
target_link_libraries(kdna_klang_library PRIVATE kdna)
"@
  $anchor = "add_executable(kdna_stream_conllu tools/kdna_stream_conllu.c)"
  if ($text.Contains($anchor)) {
    $text = $text.Replace($anchor, $insert + "`r`n" + $anchor)
  } else {
    $text += "`r`n" + $insert + "`r`n"
  }
}

if ($text -notmatch "klang_library_help") {
  $testBlock = @"

add_test(NAME klang_library_help
    COMMAND `$<TARGET_FILE:kdna_klang_library> --help)
"@
  $text += "`r`n" + $testBlock + "`r`n"
}

Set-Content -Encoding UTF8 $cmake $text

Write-Host "KLANG_LIBRARY v1 angewendet:"
Write-Host "  include\kdna_kllib.h"
Write-Host "  tools\kdna_klang_library.c"
Write-Host "  scripts\klang_library_from_suite.ps1"
Write-Host "  docs\KLANG_LIBRARY_V1_SPEC.md"
Write-Host "CMake erweitert: kdna_klang_library + klang_library_help."
