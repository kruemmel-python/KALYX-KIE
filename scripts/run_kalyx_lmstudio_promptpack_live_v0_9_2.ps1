$ErrorActionPreference = "Stop"
New-Item -ItemType Directory -Force -Path .\out | Out-Null
$goal = Get-Content .\examples\prompt_author_goal.txt -Raw
.\build_vs\Release\kalyx_prompt_author.exe --domain app --intent summarize_and_export --goal $goal --pack .\out\readme_export.kpromptpack
.\build_vs\Release\kalyx_make_envelope.exe --prompt-pack .\out\readme_export.kpromptpack --domain app --intent summarize_and_export --out .\out\promptpack_app.kie.md
$models = Invoke-RestMethod http://127.0.0.1:1234/v1/models
$KALYX_RESOLVED_MODEL = $models.data[0].id
Write-Host "[KALYX script] resolved LM Studio model: $KALYX_RESOLVED_MODEL"
python .\python\kalyx_llm_bridge.py --mode lmstudio --envelope .\out\promptpack_app.kie.md --response .\out\promptpack_lmstudio.kresp.md --audit .\out\promptpack_lmstudio.kaudit.json --validator .\build_vs\Release\kalyx_validate_response.exe --model $KALYX_RESOLVED_MODEL --timeout 3000 --max-tokens 4096 --repair-attempts 2
.\build_vs\Release\kalyx_host_dispatch_demo.exe --envelope .\out\promptpack_app.kie.md --response .\out\promptpack_lmstudio.kresp.md --validation-audit .\out\promptpack_dispatch_validation.kaudit.json --dispatch-audit .\out\promptpack.kdispatch.json --sandbox-dir .\out\promptpack_sandbox --provider lmstudio --model $KALYX_RESOLVED_MODEL --temperature 0 --max-tokens 4096
.\build_vs\Release\kalyx_audit_print.exe .\out\promptpack_lmstudio.kaudit.json
.\build_vs\Release\kalyx_audit_print.exe .\out\promptpack_dispatch_validation.kaudit.json
Get-Content .\out\promptpack.kdispatch.json -Raw
Get-Content .\out\promptpack_sandbox\workflow.kworkflow.json -Raw
Get-ChildItem .\out\promptpack_sandbox -Recurse
