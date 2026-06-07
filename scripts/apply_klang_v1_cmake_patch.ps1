param()

$ErrorActionPreference = "Stop"
$Root = (Get-Location).Path

function Add-IfMissing {
  param([string]$Path, [string]$Needle, [string]$Block)
  $txt = Get-Content $Path -Raw
  if ($txt -notmatch [regex]::Escape($Needle)) {
    Add-Content -Path $Path -Value "`r`n$Block`r`n"
  }
}

# Header kind extension
$Header = Join-Path $Root "include\kdna_kstream.h"
if (-not (Test-Path $Header)) { throw "Fehlt: $Header" }
$h = Get-Content $Header -Raw
if ($h -notmatch "KDNA_KSTREAM_KIND_CONLLU") {
  $h = $h -replace '#define KDNA_KSTREAM_KIND_CSV\s+3u', "#define KDNA_KSTREAM_KIND_CSV    3u`r`n#define KDNA_KSTREAM_KIND_CONLLU 4u"
  Set-Content -Path $Header -Value $h -Encoding UTF8
}

# CMake extension
$CMake = Join-Path $Root "CMakeLists.txt"
if (-not (Test-Path $CMake)) { throw "Fehlt: $CMake" }

$Block = @'
# KLANG v1 / CoNLL-U language adapter
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
Add-IfMissing -Path $CMake -Needle "kdna_stream_conllu" -Block $Block

Write-Host "KLANG v1 CMake/Header-Patch angewendet: kdna_stream_conllu, kdna_field_null_matrix, CoNLL-U Tests."
