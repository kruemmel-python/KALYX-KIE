# KALYX-KIE v0.2-v0.5 Release Notes

## Implementiert

### v0.2 — Structured Action Argument Validation
- Neuer auditierbarer Minimal-JSON-Parser ohne externe Library: `include/kalyx_json.h`, `src/kalyx_json.c`.
- Neuer Action-Vertrag: `include/kalyx_action.h`, `src/kalyx_action.c`.
- `KRESP01` wird nicht mehr per globaler Substring-Suche geprüft, sondern strukturell über Pfade wie `machine_result.command`, `machine_result.target`, `machine_result.args.mode`.
- Allowed Actions enthalten jetzt erlaubte Targets und Argumentwerte pro Domäne.

### v0.3 — Policy Rules + Confirmation Engine
- Neue Policy-Schicht: `include/kalyx_policy.h`, `src/kalyx_policy.c`.
- Maschinenlesbare Regeln: `forbidden_command`, `forbidden_target`, `forbidden_claim_contains`, `requires_confirmation_if_risk_at_least`.
- High-/Critical-Risk-Antworten werden ohne Confirmation deterministisch abgelehnt.

### v0.4 — Provider Adapter Hardening
- `python/kalyx_llm_bridge.py` wurde zu einem Provider-Adapter-System gehärtet.
- Modi: `offline`, `offline-file`, `openai-compatible`, `local-http`.
- Explizite Konfiguration über CLI/Env, Retry/Timeout, keine Provider-Logik im C-Core.

### v0.5 — Host SDK für Apps/Games/Tools
- Neuer C-Host-SDK-Kern: `include/kalyx_host.h`, `src/kalyx_host.c`.
- Neuer Python-Host-SDK-Wrapper: `sdk/python/kalyx_host_sdk.py`.
- Host-Entscheidungen: `reject`, `accept_answer`, `queue_confirmation`, `dispatch_command`.

## Tests

Neue Tests:
- `test_kalyx_json`
- `test_kalyx_action_args`
- `test_kalyx_policy_rules`
- `test_kalyx_confirmation_policy`
- `test_kalyx_host_sdk`

Zusätzlich aktualisiert:
- `test_kalyx_forbidden_actions`
- bestehende KIE-Validator-Tests

## Validierung

Linux-Validierung in dieser Umgebung:

```text
100% tests passed, 0 tests failed out of 103
```

Windows-Standardbefehl bleibt:

```powershell
$CMAKE_EXE = "C:\Program Files\CMake\bin\cmake.exe"; $CTEST_EXE = "C:\Program Files\CMake\bin\ctest.exe"; & $CMAKE_EXE -S . -B build_vs -G "Visual Studio 17 2022" -A x64; & $CMAKE_EXE --build build_vs --config Release --parallel; & $CTEST_EXE --test-dir build_vs -C Release --output-on-failure
```

## Kleine zusätzliche Portabilitätskorrekturen
- `tools/kalyx_repeat_context.c`: `_strtoui64` erhält non-Windows-Fallback auf `strtoull`.
- `tools/kdna_null_matrix.c`: non-Windows-Dateigröße nutzt portableres `ftell`.
