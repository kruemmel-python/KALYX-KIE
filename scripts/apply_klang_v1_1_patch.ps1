
param()

$ErrorActionPreference = "Stop"
$Root = (Get-Location).Path
$ScriptPath = $MyInvocation.MyCommand.Path
$PatchRoot = Split-Path (Split-Path $ScriptPath -Parent) -Parent

function Copy-IfDifferentPath {
  param([string]$Rel)
  $src = Join-Path $PatchRoot $Rel
  $dst = Join-Path $Root $Rel
  if (-not (Test-Path $src)) { return }
  New-Item -ItemType Directory -Force (Split-Path $dst -Parent) | Out-Null
  $srcFull = [System.IO.Path]::GetFullPath($src)
  $dstFull = [System.IO.Path]::GetFullPath($dst)
  if ($srcFull -ieq $dstFull) { return }
  Copy-Item $srcFull $dstFull -Force
}

# If the patch was unpacked into a separate folder and this script is executed
# from the project root, copy the patched files. If it was unpacked directly
# into the root, source and target are identical and copying is skipped.
Copy-IfDifferentPath "tools\kdna_stream_conllu.c"
Copy-IfDifferentPath "tools\kdna_field_null_matrix.c"
Copy-IfDifferentPath "scripts\klang_2x2_experiment.ps1"
Copy-IfDifferentPath "scripts\kfield_matrix_from_manifest.ps1"
Copy-IfDifferentPath "scripts\klang_report.ps1"
Copy-IfDifferentPath "tests\test_kstream.c"

# Header kind extension.
$Header = Join-Path $Root "include\kdna_kstream.h"
if (-not (Test-Path $Header)) { throw "Fehlt: $Header" }
$h = Get-Content $Header -Raw
if ($h -notmatch "KDNA_KSTREAM_KIND_CONLLU") {
  $h = $h -replace '#define KDNA_KSTREAM_KIND_CSV\s+3u', "#define KDNA_KSTREAM_KIND_CSV    3u`r`n#define KDNA_KSTREAM_KIND_CONLLU 4u"
  Set-Content -Path $Header -Value $h -Encoding UTF8
}

$CMake = Join-Path $Root "CMakeLists.txt"
if (-not (Test-Path $CMake)) { throw "Fehlt: $CMake" }
$cm = Get-Content $CMake -Raw

if ($cm -notmatch "add_executable\(kdna_stream_conllu") {
  Add-Content -Path $CMake -Value @'

# KLANG v1.1 / CoNLL-U language adapter
add_executable(kdna_stream_conllu tools/kdna_stream_conllu.c)
target_include_directories(kdna_stream_conllu PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(kdna_stream_conllu PRIVATE kdna)

add_executable(kdna_field_null_matrix tools/kdna_field_null_matrix.c)
target_link_libraries(kdna_field_null_matrix PRIVATE kdna)

set(KDNA_KLANG_TEST_CONLLU "${CMAKE_CURRENT_BINARY_DIR}/klang_test.conllu")
set(KDNA_KLANG_TEST_U64 "${CMAKE_CURRENT_BINARY_DIR}/klang_test.u64")
set(KDNA_KLANG_TEST_SUM "${CMAKE_CURRENT_BINARY_DIR}/klang_test.kstream")
file(WRITE ${KDNA_KLANG_TEST_CONLLU} "# sent_id = 1\n1\tThe\tthe\tDET\tDT\tDefinite=Def|PronType=Art\t2\tdet\t_\t_\n2\tdog\tdog\tNOUN\tNN\tNumber=Sing\t3\tnsubj\t_\t_\n3\truns\trun\tVERB\tVBZ\tMood=Ind|Tense=Pres\t0\troot\t_\t_\n4\t.\t.\tPUNCT\t.\t_\t3\tpunct\t_\t_\n\n# sent_id = 2\n1\tI\tI\tPRON\tPRP\tPerson=1\t2\tnsubj\t_\t_\n2\tsee\tsee\tVERB\tVBP\tMood=Ind\t0\troot\t_\t_\n")
add_test(NAME klang_conllu_file
  COMMAND $<TARGET_FILE:kdna_stream_conllu>
    --in ${KDNA_KLANG_TEST_CONLLU}
    --out ${KDNA_KLANG_TEST_U64}
    --summary ${KDNA_KLANG_TEST_SUM}
    --field upos
    --window 1
    --symbol-bits 32
    --sentence-boundary reset)
add_test(NAME klang_conllu_values
  COMMAND $<TARGET_FILE:test_kstream> l ${KDNA_KLANG_TEST_U64})
set_tests_properties(klang_conllu_values PROPERTIES DEPENDS klang_conllu_file)
add_test(NAME klang_conllu_probe
  COMMAND $<TARGET_FILE:kdna_stream_probe> ${KDNA_KLANG_TEST_SUM})
set_tests_properties(klang_conllu_probe PROPERTIES DEPENDS klang_conllu_file)
add_test(NAME klang_field_null_help
  COMMAND $<TARGET_FILE:kdna_field_null_matrix> --help)
'@
} else {
  # Ensure existing KLANG CTest invocation uses v1.1 symbol-space explicitly.
  $cm = Get-Content $CMake -Raw
  if ($cm -match "klang_conllu_file" -and $cm -notmatch "--symbol-bits 32") {
    $cm = $cm -replace '--window 1\s*\n\s*--sentence-boundary reset', "--window 1`r`n    --symbol-bits 32`r`n    --sentence-boundary reset"
    Set-Content -Path $CMake -Value $cm -Encoding UTF8
  }
}

Write-Host "KLANG v1.1 angewendet: 32-bit KLANG-Symbolraum, Degeneracy-Check, robuste Self/Cross-Kette."
