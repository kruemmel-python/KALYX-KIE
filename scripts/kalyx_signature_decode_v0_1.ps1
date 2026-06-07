param(
  [string]$Root = ".",
  [string]$Log = "Powershell.log",
  [string]$Fasta = "",
  [string]$OutDir = "Decode_chr17_v01",
  [string]$Periods = "171,342,512,1024,1048576"
)

$ErrorActionPreference = "Stop"

$Script = Join-Path $Root "python\kalyx_signature_decoder.py"
if (-not (Test-Path $Script)) {
  throw "Decoder fehlt: $Script"
}

$args = @(
  $Script,
  "--root", $Root,
  "--log", $Log,
  "--outdir", $OutDir,
  "--periods", $Periods
)

if ($Fasta -ne "") {
  $args += "--fasta"
  $args += $Fasta
}

python @args
