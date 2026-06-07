param(
  [Parameter(Mandatory=$true)][string]$DeConllu,
  [Parameter(Mandatory=$true)][string]$EnConllu,
  [string]$OutRoot = ".\LangSuiteOut",
  [double]$Train = 0.70,
  [string]$Backend = "cpu",
  [UInt64]$NullSamples = 0,
  [UInt64]$MaxAutoNullSamples = 200000
)

$ErrorActionPreference = "Stop"
if (-not (Test-Path $OutRoot)) { New-Item -ItemType Directory -Force -Path $OutRoot | Out-Null }

$configs = @(
  @{ Field="upos"; Window=1; SymbolBits=32 },
  @{ Field="upos"; Window=2; SymbolBits=32 },
  @{ Field="upos"; Window=3; SymbolBits=32 },
  @{ Field="upos_deprel"; Window=2; SymbolBits=32 }
)

$runner = Join-Path $PSScriptRoot "klang_full_v1_2.ps1"
if (-not (Test-Path $runner)) { throw "Fehlt: $runner" }

foreach ($cfg in $configs) {
  $name = "{0}_w{1}" -f $cfg.Field,$cfg.Window
  $out = Join-Path $OutRoot $name
  Write-Host "=== KLANG SUITE: $name ==="
  & $runner `
    -DeConllu $DeConllu `
    -EnConllu $EnConllu `
    -OutDir $out `
    -Field $cfg.Field `
    -Window $cfg.Window `
    -SymbolBits $cfg.SymbolBits `
    -Train $Train `
    -Backend $Backend `
    -NullSamples $NullSamples `
    -MaxAutoNullSamples $MaxAutoNullSamples
}

Write-Host "KLANG suite complete: $OutRoot"
