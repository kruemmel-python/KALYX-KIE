$ErrorActionPreference = "Stop"
$CMAKE_EXE = "C:\Program Files\CMake\bin\cmake.exe"
$CTEST_EXE = "C:\Program Files\CMake\bin\ctest.exe"
& $CMAKE_EXE -S . -B build_vs -G "Visual Studio 17 2022" -A x64
& $CMAKE_EXE --build build_vs --config Release --parallel
& $CTEST_EXE --test-dir build_vs -C Release --output-on-failure
New-Item -ItemType Directory -Force -Path .\out | Out-Null
.\build_vs\Release\kalyx_make_envelope.exe --domain app --intent multi_action_workflow --state .\examples\app_state.json --request .\examples\app_workflow_request.txt --out .\out\app_workflow.kie.md
.\build_vs\Release\kalyx_host_dispatch_demo.exe --envelope .\out\app_workflow.kie.md --response .\examples\app_workflow_valid.kresp.md --validation-audit .\out\workflow_validation.kaudit.json --dispatch-audit .\out\workflow.kdispatch.json --sandbox-dir .\out\workflow_sandbox --provider offline_file --model manual_workflow --temperature 0 --max-tokens 4096
.\build_vs\Release\kalyx_audit_print.exe .\out\workflow_validation.kaudit.json
Get-Content .\out\workflow.kdispatch.json -Raw
Get-Content .\out\workflow_sandbox\workflow.kworkflow.json -Raw
Get-ChildItem .\out\workflow_sandbox -Recurse
.\build_vs\Release\kalyx_host_dispatch_demo.exe --envelope .\out\app_workflow.kie.md --response .\examples\app_workflow_order_violation.kresp.md --validation-audit .\out\workflow_order_validation.kaudit.json --dispatch-audit .\out\workflow_order.kdispatch.json --sandbox-dir .\out\workflow_order_sandbox --provider offline_file --model order_violation --temperature 0 --max-tokens 4096
Get-Content .\out\workflow_order.kdispatch.json -Raw
Get-Content .\out\workflow_order_sandbox\workflow.kworkflow.json -Raw
