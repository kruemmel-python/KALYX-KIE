$ErrorActionPreference = "Stop"
New-Item -ItemType Directory -Force -Path .\out | Out-Null
.\build_vs\Release\kalyx_make_envelope.exe --domain app --intent multi_action_workflow --state .\examples\app_state.json --request .\examples\app_workflow_request.txt --out .\out\app_workflow.kie.md
.\build_vs\Release\kalyx_host_dispatch_demo.exe --envelope .\out\app_workflow.kie.md --response .\examples\app_workflow_valid.kresp.md --validation-audit .\out\workflow_validation.kaudit.json --dispatch-audit .\out\workflow.kdispatch.json --sandbox-dir .\out\workflow_sandbox --provider offline_file --model manual_workflow --temperature 0 --max-tokens 4096
.\build_vs\Release\kalyx_audit_print.exe .\out\workflow_validation.kaudit.json
Get-Content .\out\workflow.kdispatch.json -Raw
Get-ChildItem .\out\workflow_sandbox -Recurse
