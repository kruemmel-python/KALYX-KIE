# KALYX-KIE v0.7 — LM Studio Runtime Hardening + OpenCL Test Guard

## Zweck

v0.7 stabilisiert die produktive KIE/LLM-Pipeline und verhindert, dass instabile OpenCL-ICDs oder Treiberpfade den normalen CPU/KIE-Testlauf durch Segfaults blockieren.

## Neu

### LM Studio Runtime Hardening

`python/kalyx_llm_bridge.py` wurde erweitert:

- neuer Modus `--mode lmstudio`
- Standard-Endpunkt für LM Studio: `http://127.0.0.1:1234/v1/chat/completions`
- API-Key-Fallback: `lm-studio`
- automatische Modell-Erkennung über `/v1/models`, wenn `--model auto`, `local-model`, `lm-studio` oder kein konkretes Modell gesetzt ist
- lokale Timeout-Defaults auf 300 Sekunden gehärtet
- `--max-tokens` und Environment-Fallback `KALYX_LLM_MAX_TOKENS`
- robuster Parser für OpenAI-kompatible Chat-/Responses-Antworten
- strenger KRESP01-Systemprompt
- optionale Reparaturschleife über `--repair-attempts`, falls der Validator die erste Modellantwort ablehnt

Beispiel:

```powershell
python .\python\kalyx_llm_bridge.py --mode lmstudio --envelope .\out\readme_direct.kie.md --response .\out\readme_lmstudio.kresp.md --audit .\out\readme_lmstudio.kaudit.json --validator .\build_vs\Release\kalyx_validate_response.exe --model auto --timeout 300 --max-tokens 4096 --repair-attempts 1
```

### OpenCL Test Guard

Die OpenCL-Runtime-Tests sind jetzt standardmäßig deaktiviert, weil ein fehlerhafter OpenCL-ICD auf manchen Windows-Systemen schon beim Treiberaufruf segfaulten kann. CPU-, KIE- und Dokument-Tests bleiben vollständig aktiv.

Standard-Build:

```powershell
$CMAKE_EXE = "C:\Program Files\CMake\bin\cmake.exe"; $CTEST_EXE = "C:\Program Files\CMake\bin\ctest.exe"; & $CMAKE_EXE -S . -B build_vs -G "Visual Studio 17 2022" -A x64; & $CMAKE_EXE --build build_vs --config Release --parallel; & $CTEST_EXE --test-dir build_vs -C Release --output-on-failure
```

OpenCL-Tests explizit aktivieren:

```powershell
$CMAKE_EXE = "C:\Program Files\CMake\bin\cmake.exe"; $CTEST_EXE = "C:\Program Files\CMake\bin\ctest.exe"; & $CMAKE_EXE -S . -B build_vs -G "Visual Studio 17 2022" -A x64 -DKALYX_ENABLE_OPENCL_TESTS=ON; & $CMAKE_EXE --build build_vs --config Release --parallel; & $CTEST_EXE --test-dir build_vs -C Release --output-on-failure
```

## Ergebnis

Der normale Testlauf prüft jetzt alle deterministischen CPU-/KIE-Pfade, ohne durch defekte OpenCL-Laufzeitpfade blockiert zu werden. OpenCL bleibt verfügbar, muss für Laufzeittests aber bewusst aktiviert werden.
