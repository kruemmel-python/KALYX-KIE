# Runbooks

## Offline Reference Run

```powershell
.\scripts\run_kalyx_v1_0_final_offline_reference.ps1
```

Dieser Lauf beweist:

```text
Build
CTest
Profile Listing
Prompt-Pack
Envelope
Offline-KRESP
Validator
Dispatch
KDISPATCH01
KWORKFLOW01
Sandbox-Artefakte
```

## LM Studio Live Run

```powershell
.\scripts\run_kalyx_v1_0_final_lmstudio_live.ps1
```

Dieser Lauf beweist zusätzlich:

```text
LM Studio Model Discovery
LLM Response Generation
KRESP Validation
Live Dispatch
```

## Version Check

```powershell
.\scripts\run_kalyx_v1_0_final_version_check.ps1
```
