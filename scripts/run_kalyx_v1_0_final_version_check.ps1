# KALYX-KIE v1.0 Final version check
$ErrorActionPreference = "Stop"
.\build_vs\Release\kalyx_prompt_author.exe --version
.\build_vs\Release\kalyx_make_envelope.exe --version
.\build_vs\Release\kalyx_validate_response.exe --version
.\build_vs\Release\kalyx_host_dispatch_demo.exe --version
Get-Content .\KALYX_KIE_v1_0_Final_MANIFEST.json -Raw
