param(
    [string]$OutDir = ".\out\strict_dispatch_gate_demo"
)

$ErrorActionPreference = "Stop"
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$Envelope = ".\examples\app_command_dispatch_envelope.kie.md"
$InvalidResponse = ".\examples\app_command_invalid_action_proposal.kresp.md"
$ValidationAudit = Join-Path $OutDir "invalid.validation.kaudit.json"
$DispatchAudit = Join-Path $OutDir "invalid.dispatch.kdispatch.json"
$SandboxDir = Join-Path $OutDir "sandbox"

& .\build_vs\Release\kalyx_host_dispatch_demo.exe --envelope $Envelope --response $InvalidResponse --validation-audit $ValidationAudit --dispatch-audit $DispatchAudit --sandbox-dir $SandboxDir --provider offline_file --model invalid_action_proposal --temperature 0 --max-tokens 2048
$ExitCode = $LASTEXITCODE
Write-Host "host dispatch demo exit code: $ExitCode"
Write-Host "Validation audit:"
Get-Content $ValidationAudit -Raw
Write-Host "Dispatch audit:"
Get-Content $DispatchAudit -Raw
Write-Host "Sandbox contents:"
Get-ChildItem $SandboxDir -Recurse -ErrorAction SilentlyContinue
exit 0
