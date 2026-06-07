param(
  [string]$DecodeDir = ".\Decode_chr17_v01",
  [string]$OutDir = ".\Decode_chr17_v02",
  [int]$ClusterGap = 32
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path $DecodeDir)) { throw "DecodeDir fehlt: $DecodeDir" }
if (-not (Test-Path (Join-Path $DecodeDir "decode_hits.csv"))) { throw "decode_hits.csv fehlt in $DecodeDir" }
if (-not (Test-Path ".\python\kalyx_signature_cluster_v0_2.py")) { throw "Analyzer fehlt: .\python\kalyx_signature_cluster_v0_2.py" }

python .\python\kalyx_signature_cluster_v0_2.py `
  --decode-dir $DecodeDir `
  --out-dir $OutDir `
  --cluster-gap $ClusterGap
