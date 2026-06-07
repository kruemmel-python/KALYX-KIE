$ErrorActionPreference = "Stop"

$CMAKE_EXE = "C:\Program Files\CMake\bin\cmake.exe"
$CTEST_EXE = "C:\Program Files\CMake\bin\ctest.exe"

& $CMAKE_EXE -S . -B build_vs -G "Visual Studio 17 2022" -A x64
& $CMAKE_EXE --build build_vs --config Release --parallel
& $CTEST_EXE --test-dir build_vs -C Release --output-on-failure

New-Item -ItemType Directory -Force -Path .\out | Out-Null

@'
Antworte ausschließlich als gültige KALYX KRESP01 Response.

Du musst type="command" verwenden.

Erzeuge diesen erlaubten Command:
machine_result.command = "emit_ui_command"
machine_result.target = "export_document"
machine_result.args.mode = "markdown"
machine_result.args.theme = "plain"

Setze:
risk = "low"
requires_confirmation = false
uses_only_provided_context = true

Keine OS-Befehle. Keine echten Dateien überschreiben. Nur Sandbox-Export.
'@ | Set-Content .\out\llm_command_request.txt -Encoding UTF8

.\build_vs\Release\kalyx_make_envelope.exe --domain app --intent export_document --state .\examples\app_state.json --request .\out\llm_command_request.txt --out .\out\app_llm_command.kie.md

$models = Invoke-RestMethod http://127.0.0.1:1234/v1/models
$KALYX_RESOLVED_MODEL = $models.data[0].id
Write-Host "[KALYX script] resolved LM Studio model: $KALYX_RESOLVED_MODEL"

python .\python\kalyx_llm_bridge.py --mode lmstudio --envelope .\out\app_llm_command.kie.md --response .\out\app_llm_command.kresp.md --audit .\out\app_llm_command.kaudit.json --validator .\build_vs\Release\kalyx_validate_response.exe --model $KALYX_RESOLVED_MODEL --timeout 3000 --max-tokens 4096 --repair-attempts 2

.\build_vs\Release\kalyx_host_dispatch_demo.exe --envelope .\out\app_llm_command.kie.md --response .\out\app_llm_command.kresp.md --validation-audit .\out\llm_dispatch_validation.kaudit.json --dispatch-audit .\out\llm_dispatch.kdispatch.json --sandbox-dir .\out\llm_sandbox --provider lmstudio --model $KALYX_RESOLVED_MODEL --temperature 0 --max-tokens 4096

.\build_vs\Release\kalyx_audit_print.exe .\out\app_llm_command.kaudit.json
.\build_vs\Release\kalyx_audit_print.exe .\out\llm_dispatch_validation.kaudit.json
Get-Content .\out\llm_dispatch.kdispatch.json -Raw
Get-ChildItem .\out\llm_sandbox -Recurse
