$ErrorActionPreference = "Stop"
New-Item -ItemType Directory -Force -Path .\out | Out-Null
@'
Antworte ausschließlich als gültige KALYX KRESP01 Response.

Du musst type="command" verwenden.

Erzeuge einen erlaubten Multi-Action-Workflow:
machine_result.command = "workflow"
machine_result.target = "multi_action_sandbox"
machine_result.args.mode = "markdown"
machine_result.args.theme = "plain"

machine_result.workflow muss exakt diese drei Schritte enthalten:
1. command="emit_ui_command", target="export_document", args.mode="markdown", args.theme="plain"
2. command="emit_ui_command", target="open_preview", args.mode="markdown", args.theme="plain"
3. command="emit_ui_command", target="save_as", args.mode="markdown", args.theme="plain"

Setze:
risk = "low"
requires_confirmation = false
uses_only_provided_context = true

Keine OS-Befehle. Keine echten Dateien überschreiben. Nur Sandbox-Workflow.
'@ | Set-Content .\out\llm_workflow_request.txt -Encoding UTF8
.\build_vs\Release\kalyx_make_envelope.exe --domain app --intent multi_action_workflow --state .\examples\app_state.json --request .\out\llm_workflow_request.txt --out .\out\app_llm_workflow.kie.md
$models = Invoke-RestMethod http://127.0.0.1:1234/v1/models
$KALYX_RESOLVED_MODEL = $models.data[0].id
Write-Host "[KALYX script] resolved LM Studio model: $KALYX_RESOLVED_MODEL"
python .\python\kalyx_llm_bridge.py --mode lmstudio --envelope .\out\app_llm_workflow.kie.md --response .\out\app_llm_workflow.kresp.md --audit .\out\app_llm_workflow.kaudit.json --validator .\build_vs\Release\kalyx_validate_response.exe --model $KALYX_RESOLVED_MODEL --timeout 3000 --max-tokens 4096 --repair-attempts 2
.\build_vs\Release\kalyx_host_dispatch_demo.exe --envelope .\out\app_llm_workflow.kie.md --response .\out\app_llm_workflow.kresp.md --validation-audit .\out\llm_workflow_validation.kaudit.json --dispatch-audit .\out\llm_workflow.kdispatch.json --sandbox-dir .\out\llm_workflow_sandbox --provider lmstudio --model $KALYX_RESOLVED_MODEL --temperature 0 --max-tokens 4096
.\build_vs\Release\kalyx_audit_print.exe .\out\app_llm_workflow.kaudit.json
.\build_vs\Release\kalyx_audit_print.exe .\out\llm_workflow_validation.kaudit.json
Get-Content .\out\llm_workflow.kdispatch.json -Raw
Get-ChildItem .\out\llm_workflow_sandbox -Recurse
