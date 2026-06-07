$cmake = "CMakeLists.txt"

if (-not (Test-Path $cmake)) {
  throw "CMakeLists.txt nicht gefunden. Script im Projekt-Hauptordner ausführen."
}

$text = Get-Content $cmake -Raw

if ($text -notmatch "kdna_genome_library") {
  Add-Content $cmake @"

# KGENOME Enterprise Library / KGLIB001
add_executable(kdna_genome_library tools/kdna_genome_library.c)
target_link_libraries(kdna_genome_library PRIVATE kdna)

add_executable(test_kglib tests/test_kglib.c)
target_link_libraries(test_kglib PRIVATE kdna)

add_test(NAME kglib_abi COMMAND `$<TARGET_FILE:test_kglib>)
add_test(NAME kglib_help COMMAND `$<TARGET_FILE:kdna_genome_library> --help)
"@
  Write-Host "CMakeLists.txt erweitert: kdna_genome_library, test_kglib, kglib tests."
} else {
  Write-Host "CMakeLists.txt enthält kdna_genome_library bereits. Keine Änderung."
}
