# KALYX-KIE v1.0 Final Demo-Runbook

## Offline-Referenzlauf

Der Offline-Lauf beweist Core, Prompt-Pack, Envelope, Validator, Dispatch und Sandbox ohne Modellabhängigkeit.

```powershell
.\scripts\run_kalyx_v1_0_final_offline_reference.ps1
```

## LM-Studio-Live-Lauf

Der Live-Lauf beweist den vollständigen LLM-Pfad.

```powershell
.\scripts\run_kalyx_v1_0_final_lmstudio_live.ps1
```

## Erwartete Artefakte

```text
out/custom_profile.kpromptpack/
out/custom_profile.kie.md
out/custom_profile_lmstudio.kresp.md
out/custom_profile_lmstudio.kaudit.json
out/custom_profile_dispatch_validation.kaudit.json
out/custom_profile.kdispatch.json
out/custom_profile_sandbox/workflow.kworkflow.json
```
