$ErrorActionPreference = "Stop"
New-Item -ItemType Directory -Force -Path .\out | Out-Null
$goal = "Fasse README.md für neue Benutzer zusammen und erzeuge danach einen sicheren Markdown-Export in der Sandbox. Verwende dafür einen validierbaren KALYX-Workflow mit export_document, open_preview und save_as."
.\build_vs\Release\kalyx_prompt_author.exe --profile-dir .\profiles --profile custom.summary_export --goal $goal --pack .\out\custom_profile.kpromptpack
.\build_vs\Release\kalyx_make_envelope.exe --prompt-pack .\out\custom_profile.kpromptpack --domain app --intent custom_summary_export --out .\out\custom_profile.kie.md
$models = Invoke-RestMethod http://127.0.0.1:1234/v1/models
$KALYX_RESOLVED_MODEL = $models.data[0].id
Write-Host "[KALYX script] resolved LM Studio model: $KALYX_RESOLVED_MODEL"
python .\python\kalyx_llm_bridge.py --mode lmstudio --envelope .\out\custom_profile.kie.md --response .\out\custom_profile_lmstudio.kresp.md --audit .\out\custom_profile_lmstudio.kaudit.json --validator .\build_vs\Release\kalyx_validate_response.exe --model $KALYX_RESOLVED_MODEL --timeout 3000 --max-tokens 4096 --repair-attempts 2
.\build_vs\Release\kalyx_host_dispatch_demo.exe --envelope .\out\custom_profile.kie.md --response .\out\custom_profile_lmstudio.kresp.md --validation-audit .\out\custom_profile_dispatch_validation.kaudit.json --dispatch-audit .\out\custom_profile.kdispatch.json --sandbox-dir .\out\custom_profile_sandbox --provider lmstudio --model $KALYX_RESOLVED_MODEL --temperature 0 --max-tokens 4096
.\build_vs\Release\kalyx_audit_print.exe .\out\custom_profile_lmstudio.kaudit.json
.\build_vs\Release\kalyx_audit_print.exe .\out\custom_profile_dispatch_validation.kaudit.json
Get-Content .\out\custom_profile.kdispatch.json -Raw
Get-Content .\out\custom_profile_sandbox\workflow.kworkflow.json -Raw
Get-ChildItem .\out\custom_profile.kpromptpack -Recurse
Get-ChildItem .\out\custom_profile_sandbox -Recurse
