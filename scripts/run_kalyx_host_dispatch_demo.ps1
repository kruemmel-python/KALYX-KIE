param(
    [string]$Domain = "app",
    [string]$Intent = "export_document",
    [string]$State = ".\examples\app_state.json",
    [string]$Request = ".\examples\app_request.txt",
    [string]$Response = ".\examples\app_command_dispatch_valid.kresp.md",
    [string]$OutDir = ".\out\host_dispatch_demo"
)

$ErrorActionPreference = "Stop"
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$Envelope = Join-Path $OutDir "app_command.kie.md"
$ValidationAudit = Join-Path $OutDir "validation.kaudit.json"
$DispatchAudit = Join-Path $OutDir "dispatch.kdispatch.json"
$SandboxDir = Join-Path $OutDir "sandbox"

.\build_vs\Release\kalyx_make_envelope.exe --domain $Domain --intent $Intent --state $State --request $Request --out $Envelope
.\build_vs\Release\kalyx_host_dispatch_demo.exe --envelope $Envelope --response $Response --validation-audit $ValidationAudit --dispatch-audit $DispatchAudit --sandbox-dir $SandboxDir --provider offline_file --model manual_command --temperature 0 --max-tokens 2048
Get-Content $DispatchAudit -Raw
