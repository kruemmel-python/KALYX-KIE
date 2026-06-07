# KALYX-KIE

[![Version](https://img.shields.io/badge/version-1.0--kie--v1.0--final-00d4ff)](#)
[![Status](https://img.shields.io/badge/status-v1.0%20final-72ffb8)](#)
[![Runtime](https://img.shields.io/badge/runtime-C%20%2B%20Python-9cf)](#)
[![LLM](https://img.shields.io/badge/LLM-LM%20Studio%20%2F%20OpenAI--compatible-blueviolet)](#)
[![Governance](https://img.shields.io/badge/governance-validation%20%2B%20audit%20%2B%20sandbox-ffd36a)](#)

**KALYX-KIE** ist ein Dateiformat- und Runtime-Substrat für kontrollierbare LLM-Interaktionen.

Prompty macht Prompt-Dateien einfach.  
**KALYX macht LLM-Interaktionen kontrollierbar.**

KALYX-KIE verbindet:

```text
Prompt Authoring
→ Contract Synthesis
→ Interaction Envelope
→ LLM Response Validation
→ Strict Dispatch Gate
→ Workflow Policy Engine
→ Sandbox Execution
→ Audit Trail
```

Das Ziel ist nicht nur, Prompts auszulagern, sondern aus einem Prompt ein prüfbares, versionierbares und auditierbares Interaktionspaket zu machen.

---

## Warum KALYX-KIE?

Klassische Prompt-Dateien beantworten vor allem diese Frage:

```text
Was soll das LLM schreiben?
```

KALYX-KIE beantwortet zusätzlich:

```text
Was darf das LLM vorschlagen?
Welche Aktionen sind erlaubt?
Welche Aktionen sind verboten?
Welche Antwortstruktur ist gültig?
Wann braucht eine Aktion Bestätigung?
Welche Workflow-Reihenfolge ist erlaubt?
Was wurde wirklich validiert?
Was wurde wirklich dispatcht?
Welche Artefakte wurden erzeugt?
```

Damit ist KALYX-KIE nicht nur ein Prompt-Format, sondern ein Governance-Layer für LLM-Aktionen.

---

## Quick Start

### 1. Projekt bauen

```powershell
$CMAKE_EXE = "C:\Program Files\CMake\bin\cmake.exe"; $CTEST_EXE = "C:\Program Files\CMake\bin\ctest.exe"; Remove-Item .\build_vs -Recurse -Force -ErrorAction SilentlyContinue; Remove-Item .\out -Recurse -Force -ErrorAction SilentlyContinue; & $CMAKE_EXE -S . -B build_vs -G "Visual Studio 17 2022" -A x64; & $CMAKE_EXE --build build_vs --config Release --parallel 1 -- /m:1 /p:CL_MPCount=1; & $CTEST_EXE --test-dir build_vs -C Release --output-on-failure
```

Erwartung:

```text
100% tests passed, 0 tests failed out of 130
```

---

### 2. Prompt-Pack aus einem Profil erzeugen

```powershell
New-Item -ItemType Directory -Force -Path .\out; $goal = "Fasse README.md für neue Benutzer zusammen und exportiere Markdown sicher in die Sandbox."; .\build_vs\Release\kalyx_prompt_author.exe --profile-dir .\profiles --profile custom.summary_export --goal $goal --pack .\out\custom_profile.kpromptpack
```

Dadurch entsteht:

```text
out/custom_profile.kpromptpack/
├─ allowed_actions.json
├─ contract.kcontract.json
├─ example_state.json
├─ expected_response_shape.md
├─ policy_rules.json
├─ prompt.kprompt.md
├─ README.md
└─ request.txt
```

---

### 3. KIE-Envelope erzeugen

```powershell
.\build_vs\Release\kalyx_make_envelope.exe --prompt-pack .\out\custom_profile.kpromptpack --domain app --intent custom_summary_export --out .\out\custom_profile.kie.md
```

---

### 4. LLM über LM Studio ausführen

LM Studio muss laufen und ein Modell geladen haben.

```powershell
$models = Invoke-RestMethod http://127.0.0.1:1234/v1/models; $KALYX_RESOLVED_MODEL = $models.data[0].id; python .\python\kalyx_llm_bridge.py --mode lmstudio --envelope .\out\custom_profile.kie.md --response .\out\custom_profile_lmstudio.kresp.md --audit .\out\custom_profile_lmstudio.kaudit.json --validator .\build_vs\Release\kalyx_validate_response.exe --model $KALYX_RESOLVED_MODEL --timeout 3000 --max-tokens 4096 --repair-attempts 2
```

---

### 5. Validierte Antwort dispatchen

```powershell
.\build_vs\Release\kalyx_host_dispatch_demo.exe --envelope .\out\custom_profile.kie.md --response .\out\custom_profile_lmstudio.kresp.md --validation-audit .\out\custom_profile_dispatch_validation.kaudit.json --dispatch-audit .\out\custom_profile.kdispatch.json --sandbox-dir .\out\custom_profile_sandbox --provider lmstudio --model $KALYX_RESOLVED_MODEL --temperature 0 --max-tokens 4096
```

---

### 6. Audits und Sandbox-Artefakte prüfen

```powershell
.\build_vs\Release\kalyx_audit_print.exe .\out\custom_profile_lmstudio.kaudit.json; .\build_vs\Release\kalyx_audit_print.exe .\out\custom_profile_dispatch_validation.kaudit.json; Get-Content .\out\custom_profile.kdispatch.json -Raw; Get-Content .\out\custom_profile_sandbox\workflow.kworkflow.json -Raw; Get-ChildItem .\out\custom_profile_sandbox -Recurse
```

---

## Der KALYX-KIE-Flow

```text
KPROFILE01
→ KPROMPT01
→ KCONTRACT01
→ KPROMPTPACK
→ KIE01
→ LLM
→ KRESP01
→ KAUDIT01
→ KDISPATCH01
→ KWORKFLOW01
→ Sandbox-Artefakte
```

---

## Kernkonzepte

| Konzept | Bedeutung |
|---|---|
| `KPROFILE01` | Wiederverwendbares Domain-Profil |
| `KPROMPT01` | Menschlich lesbarer Prompt-Entwurf |
| `KCONTRACT01` | Maschinenlesbarer Governance-Vertrag |
| `.kpromptpack/` | Versionierbares Prompt- und Contract-Paket |
| `KIE01` | KALYX Interaction Envelope für das LLM |
| `KRESP01` | Gültiges LLM-Antwortformat |
| `KAUDIT01` | Validierungs-Audit mit Hashes |
| `KDISPATCH01` | Dispatch-Audit des Host-Gates |
| `KWORKFLOW01` | Workflow-Manifest mit Step-Status |

---

## Eingebaute Profile

```text
app.summary_export
app.code_review
game.npc_action
tool.file_transform
research.hypothesis_report
workflow.multi_action
```

Custom Profiles liegen in:

```text
profiles/*.kprofile.json
```

Profile anzeigen:

```powershell
.\build_vs\Release\kalyx_prompt_author.exe --list-profiles --profile-dir .\profiles
```

---

## Sicherheit

KALYX-KIE folgt einem einfachen Prinzip:

```text
Ein LLM darf vorschlagen.
KALYX muss validieren.
Der Host darf nur nach accepted=true dispatchen.
Die Demo schreibt ausschließlich Sandbox-Artefakte.
```

Die Demo führt keine Betriebssystembefehle aus und überschreibt keine echten Dateien.

---

## Offizielle Runbooks

```powershell
.\scripts\run_kalyx_v1_0_final_offline_reference.ps1
.\scripts\run_kalyx_v1_0_final_lmstudio_live.ps1
.\scripts\run_kalyx_v1_0_final_version_check.ps1
```

---

## Dokumentation

- [Quick Start](docs/quickstart.md)
- [Concepts](docs/concepts.md)
- [File Formats](docs/file-formats.md)
- [CLI Reference](docs/cli-reference.md)
- [Runbooks](docs/runbooks.md)
- [Profiles](docs/profiles.md)
- [Security Model](docs/security-model.md)
- [Troubleshooting](docs/troubleshooting.md)
- [Prompty Comparison](docs/prompty-comparison.md)

---

## Lizenz

Projektabhängig. Für Distribution bitte die Projektlizenz ergänzen.
