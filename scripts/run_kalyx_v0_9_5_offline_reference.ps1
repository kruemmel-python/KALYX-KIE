$ErrorActionPreference = "Stop"
$CMAKE_EXE = "C:\Program Files\CMake\bin\cmake.exe"
$CTEST_EXE = "C:\Program Files\CMake\bin\ctest.exe"
Remove-Item .\build_vs -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item .\out -Recurse -Force -ErrorAction SilentlyContinue
& $CMAKE_EXE -S . -B build_vs -G "Visual Studio 17 2022" -A x64
& $CMAKE_EXE --build build_vs --config Release --parallel 1 -- /m:1 /p:CL_MPCount=1
& $CTEST_EXE --test-dir build_vs -C Release --output-on-failure
New-Item -ItemType Directory -Force -Path .\out | Out-Null
$goal = "Fasse README.md für neue Benutzer zusammen und exportiere Markdown sicher in die Sandbox."
.\build_vs\Release\kalyx_prompt_author.exe --list-profiles --profile-dir .\profiles
.\build_vs\Release\kalyx_prompt_author.exe --profile-dir .\profiles --profile custom.summary_export --goal $goal --pack .\out\custom_profile.kpromptpack
.\build_vs\Release\kalyx_make_envelope.exe --prompt-pack .\out\custom_profile.kpromptpack --domain app --intent custom_summary_export --out .\out\custom_profile.kie.md
.\build_vs\Release\kalyx_host_dispatch_demo.exe --envelope .\out\custom_profile.kie.md --response .\examples\app_workflow_valid.kresp.md --validation-audit .\out\custom_profile_validation.kaudit.json --dispatch-audit .\out\custom_profile.kdispatch.json --sandbox-dir .\out\custom_profile_sandbox --provider offline_file --model manual_custom_profile --temperature 0 --max-tokens 4096
.\build_vs\Release\kalyx_audit_print.exe .\out\custom_profile_validation.kaudit.json
Get-Content .\out\custom_profile.kdispatch.json -Raw
Get-Content .\out\custom_profile_sandbox\workflow.kworkflow.json -Raw
Get-ChildItem .\out\custom_profile.kpromptpack -Recurse
Get-ChildItem .\out\custom_profile_sandbox -Recurse
