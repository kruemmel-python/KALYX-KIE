# Quick Start

Diese Anleitung führt durch den minimalen KALYX-KIE-v1.0-Final-Lauf.

## Voraussetzungen

- Windows
- Visual Studio 2022 Build Tools
- CMake
- PowerShell
- Python 3.12
- optional: LM Studio mit lokalem Modell

## Offline-Referenzlauf

Der Offline-Lauf nutzt eine mitgelieferte gültige KRESP-Datei. Er beweist Build, Tests, Prompt-Pack, Envelope, Validation, Dispatch und Sandbox ohne LLM-Abhängigkeit.

```powershell
.\scripts\run_kalyx_v1_0_final_offline_reference.ps1
```

Erwartung:

```text
100% tests passed, 0 tests failed out of 130
dispatch decision=sandbox_executed
KWORKFLOW01 status=ok
```

## LM-Studio-Live-Lauf

```powershell
.\scripts\run_kalyx_v1_0_final_lmstudio_live.ps1
```

Erwartung:

```text
resolved LM Studio model: <model>
response accepted
dispatch decision=sandbox_executed
```

## Version prüfen

```powershell
.\scripts\run_kalyx_v1_0_final_version_check.ps1
```

Erwartung:

```text
1.0-kie-v1.0-final
```
