# KALYX-KIE v0.8 — Command Execution Sandbox + Host Dispatch Demo

v0.8 proves the complete safe command path:

```text
KIE envelope
→ KRESP command
→ response validator
→ host plan
→ sandbox dispatcher
→ dispatch audit
```

## New targets

- `kalyx_host_dispatch_demo`
- `test_kalyx_dispatch_sandbox`

## New files

- `include/kalyx_dispatch.h`
- `src/kalyx_dispatch.c`
- `tools/kalyx_host_dispatch_demo.c`
- `tests/test_kalyx_dispatch_sandbox.c`
- `examples/app_command_dispatch_valid.kresp.md`
- `examples/app_command_confirmation.kresp.md`

## Safety model

The dispatcher never executes OS commands and never overwrites source files. It only writes sandbox artifacts under the configured sandbox directory:

- `dispatch_plan.json` for actions requiring confirmation
- `answer.txt` for accepted non-command answers
- `open_preview_request.json` for safe preview requests
- `export_document_sandbox.md` for safe export requests
- `*.kdispatch.json` for dispatch audit metadata

## Example

```powershell
.\build_vs\Release\kalyx_host_dispatch_demo.exe --envelope .\out\app.kie.md --response .\examples\app_command_dispatch_valid.kresp.md --validation-audit .\out\dispatch_validation.kaudit.json --dispatch-audit .\out\dispatch.kdispatch.json --sandbox-dir .\out\sandbox --provider offline_file --model manual_command --temperature 0 --max-tokens 2048
```
