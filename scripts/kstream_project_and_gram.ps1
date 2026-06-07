param(
  [Parameter(Mandatory=$true)][string]$Symbols,
  [Parameter(Mandatory=$true)][string]$OutSymbols,
  [Parameter(Mandatory=$true)][string]$OutGrammar,
  [string]$Backend = "opencl",
  [string]$Kernel = "kernels\kdna_eval.cl",
  [double]$XMin = -8.0,
  [double]$XMax = 8.0,
  [double]$Train = 0.70,
  [UInt64]$Chunk = 262144
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path $Symbols)) { throw "Symbols fehlt: $Symbols" }

$N = [UInt64]((Get-Item $Symbols).Length / 8)
if ($N -lt 2) { throw "zu wenig Symbole: $N" }

Write-Host "KSTREAM -> KDNA: $Symbols n=$N"

$args = @(
  "--symbols", $Symbols,
  "--n", "$N",
  "--out-symbols", $OutSymbols,
  "--mode", "affine",
  "--k", "16",
  "--xmin", "$XMin",
  "--xmax", "$XMax",
  "--backend", $Backend,
  "--chunk", "$Chunk"
)

if ($Backend -eq "opencl") {
  $args += "--kernel"
  $args += $Kernel
}

& .\build_vs\Release\kdna_project.exe @args

$KN = [UInt64]((Get-Item $OutSymbols).Length / 8)
if ($KN -ne $N) { throw "KDNA symbol count mismatch raw=$N kdna=$KN" }

Write-Host "KDNA -> KGRAM: $OutSymbols"
& .\build_vs\Release\kdna_symbol_gram.exe `
  --symbols $OutSymbols `
  --n $KN `
  --out $OutGrammar `
  --train $Train

& .\build_vs\Release\kdna_probe.exe $OutGrammar
