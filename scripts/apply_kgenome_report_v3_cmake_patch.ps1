# Adds kdna_null_matrix.exe to the existing CMake project without rebuilding data artifacts.
$cmake = "CMakeLists.txt"
if (-not (Test-Path $cmake)) { throw "CMakeLists.txt nicht gefunden. Bitte im Projekt-Hauptordner ausführen." }
$text = Get-Content $cmake -Raw

if ($text -notmatch "kdna_null_matrix") {
  $insert = @"

add_executable(kdna_null_matrix tools/kdna_null_matrix.c)
target_link_libraries(kdna_null_matrix PRIVATE kdna)
"@
  # append safely; this tool is standalone C and only links kdna for consistent project build layout
  Add-Content -Path $cmake -Value $insert
  Write-Host "CMakeLists.txt erweitert: kdna_null_matrix."
} else {
  Write-Host "CMakeLists.txt enthält kdna_null_matrix bereits."
}
