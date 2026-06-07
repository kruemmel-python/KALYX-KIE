param()

$cmake = "CMakeLists.txt"
if (-not (Test-Path $cmake)) {
  throw "CMakeLists.txt nicht gefunden. Bitte im Projekt-Hauptordner ausführen."
}

$text = Get-Content $cmake -Raw

function Add-BlockOnce {
  param([string]$Needle, [string]$Block)
  if ($script:text -notmatch [regex]::Escape($Needle)) {
    $script:text += "`r`n$Block`r`n"
  }
}

$exeBlock = @'

# KSTREAM/KFIELD v1 generic stream adapters
add_executable(kdna_stream_bytes tools/kdna_stream_bytes.c)
target_include_directories(kdna_stream_bytes PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(kdna_stream_bytes PRIVATE kdna)

add_executable(kdna_stream_tokens tools/kdna_stream_tokens.c)
target_include_directories(kdna_stream_tokens PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(kdna_stream_tokens PRIVATE kdna)

add_executable(kdna_stream_csv tools/kdna_stream_csv.c)
target_include_directories(kdna_stream_csv PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(kdna_stream_csv PRIVATE kdna)

add_executable(kdna_stream_probe tools/kdna_stream_probe.c)
target_include_directories(kdna_stream_probe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(kdna_stream_probe PRIVATE kdna)

add_executable(test_kstream tests/test_kstream.c)
target_link_libraries(test_kstream PRIVATE kdna)

# KSTREAM/KFIELD v1 tests
set(KDNA_KSTREAM_TEST_BYTES "${CMAKE_CURRENT_BINARY_DIR}/kstream_bytes_input.bin")
set(KDNA_KSTREAM_TEST_BYTES_U64 "${CMAKE_CURRENT_BINARY_DIR}/kstream_bytes.u64")
set(KDNA_KSTREAM_TEST_BYTES_SUM "${CMAKE_CURRENT_BINARY_DIR}/kstream_bytes.kstream")
file(WRITE ${KDNA_KSTREAM_TEST_BYTES} "ABC\n")
add_test(NAME kstream_bytes_file
  COMMAND $<TARGET_FILE:kdna_stream_bytes>
    --in ${KDNA_KSTREAM_TEST_BYTES}
    --out ${KDNA_KSTREAM_TEST_BYTES_U64}
    --summary ${KDNA_KSTREAM_TEST_BYTES_SUM}
    --mode byte)
add_test(NAME kstream_bytes_values
  COMMAND $<TARGET_FILE:test_kstream> b ${KDNA_KSTREAM_TEST_BYTES_U64})
set_tests_properties(kstream_bytes_values PROPERTIES DEPENDS kstream_bytes_file)
add_test(NAME kstream_bytes_probe
  COMMAND $<TARGET_FILE:kdna_stream_probe> ${KDNA_KSTREAM_TEST_BYTES_SUM})
set_tests_properties(kstream_bytes_probe PROPERTIES DEPENDS kstream_bytes_file)

set(KDNA_KSTREAM_TEST_TOKENS "${CMAKE_CURRENT_BINARY_DIR}/kstream_tokens_input.txt")
set(KDNA_KSTREAM_TEST_TOKENS_U64 "${CMAKE_CURRENT_BINARY_DIR}/kstream_tokens.u64")
set(KDNA_KSTREAM_TEST_TOKENS_SUM "${CMAKE_CURRENT_BINARY_DIR}/kstream_tokens.kstream")
file(WRITE ${KDNA_KSTREAM_TEST_TOKENS} "alpha beta alpha\n")
add_test(NAME kstream_tokens_file
  COMMAND $<TARGET_FILE:kdna_stream_tokens>
    --in ${KDNA_KSTREAM_TEST_TOKENS}
    --out ${KDNA_KSTREAM_TEST_TOKENS_U64}
    --summary ${KDNA_KSTREAM_TEST_TOKENS_SUM})
add_test(NAME kstream_tokens_values
  COMMAND $<TARGET_FILE:test_kstream> t ${KDNA_KSTREAM_TEST_TOKENS_U64})
set_tests_properties(kstream_tokens_values PROPERTIES DEPENDS kstream_tokens_file)
add_test(NAME kstream_tokens_probe
  COMMAND $<TARGET_FILE:kdna_stream_probe> ${KDNA_KSTREAM_TEST_TOKENS_SUM})
set_tests_properties(kstream_tokens_probe PROPERTIES DEPENDS kstream_tokens_file)

set(KDNA_KSTREAM_TEST_CSV "${CMAKE_CURRENT_BINARY_DIR}/kstream_csv_input.csv")
set(KDNA_KSTREAM_TEST_CSV_U64 "${CMAKE_CURRENT_BINARY_DIR}/kstream_csv.u64")
set(KDNA_KSTREAM_TEST_CSV_SUM "${CMAKE_CURRENT_BINARY_DIR}/kstream_csv.kstream")
file(WRITE ${KDNA_KSTREAM_TEST_CSV} "x\n0\n3\n7\n10\n")
add_test(NAME kstream_csv_file
  COMMAND $<TARGET_FILE:kdna_stream_csv>
    --in ${KDNA_KSTREAM_TEST_CSV}
    --out ${KDNA_KSTREAM_TEST_CSV_U64}
    --summary ${KDNA_KSTREAM_TEST_CSV_SUM}
    --column 0
    --bins 11
    --header 1
    --min 0
    --max 10)
add_test(NAME kstream_csv_values
  COMMAND $<TARGET_FILE:test_kstream> c ${KDNA_KSTREAM_TEST_CSV_U64})
set_tests_properties(kstream_csv_values PROPERTIES DEPENDS kstream_csv_file)
add_test(NAME kstream_csv_probe
  COMMAND $<TARGET_FILE:kdna_stream_probe> ${KDNA_KSTREAM_TEST_CSV_SUM})
set_tests_properties(kstream_csv_probe PROPERTIES DEPENDS kstream_csv_file)

'@

Add-BlockOnce "add_executable(kdna_stream_bytes tools/kdna_stream_bytes.c)" $exeBlock

Set-Content $cmake $text -NoNewline
Write-Host "CMakeLists.txt erweitert: KSTREAM/KFIELD v1 Adapter, Probe und Tests."
