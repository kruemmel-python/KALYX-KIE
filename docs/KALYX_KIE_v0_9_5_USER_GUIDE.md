# KALYX-KIE v1.0 Final Benutzeranleitung

## Ziel

KALYX-KIE erzeugt aus einem Ziel einen versionierbaren Prompt-Pack, baut daraus einen KIE01-Envelope, lässt ein LLM eine KRESP01-Antwort erzeugen, validiert diese Antwort, dispatcht nur erlaubte Sandbox-Workflows und schreibt Audit-Artefakte.

Die Grundkette lautet:

```text
KPROFILE01 / Profil
→ KPROMPT01 Prompt
→ KCONTRACT01 Contract
→ KIE01 Envelope
→ LM Studio / LLM
→ KRESP01 Response
→ KAUDIT01 Validation Ledger
→ KDISPATCH01 Dispatch Ledger
→ KWORKFLOW01 Workflow Manifest
```

## Schnellstart

1. ZIP entpacken.
2. Im entpackten Ordner PowerShell öffnen.
3. LM Studio starten, wenn der Live-Lauf genutzt werden soll.
4. Seriell bauen, um Windows-Pagefile-/MSBuild-Out-of-Memory-Probleme zu vermeiden.
5. Prompt-Pack erzeugen.
6. Envelope erzeugen.
7. LLM-Bridge oder Offline-Response ausführen.
8. Dispatch-Audit und Sandbox-Artefakte prüfen.

## Wichtige Begriffe

| Begriff | Bedeutung |
|---|---|
| `KPROFILE01` | Wiederverwendbares Domain-Profil als JSON-Datei. |
| `KPROMPT01` | Menschenlesbarer Prompt-Entwurf. |
| `KCONTRACT01` | Maschinenlesbarer Vertrag mit erlaubten Aktionen und Workflow-Policy. |
| `KIE01` | KALYX Interaction Envelope für das LLM. |
| `KRESP01` | Strukturierte Antwort des LLM. |
| `KAUDIT01` | Validierungs-Audit mit Hashes und Ergebnis. |
| `KDISPATCH01` | Dispatch-Audit der Host-Sandbox. |
| `KWORKFLOW01` | Workflow-Manifest mit Status pro Schritt. |

## Wichtige Tools

| Tool | Aufgabe |
|---|---|
| `kalyx_prompt_author.exe` | Erzeugt Prompt-Pack, Contract und Request aus Profil und Ziel. |
| `kalyx_make_envelope.exe` | Erzeugt `.kie.md` aus Prompt-Pack, State oder Dokument. |
| `kalyx_validate_response.exe` | Prüft `.kresp.md` gegen `.kie.md` und schreibt `.kaudit.json`. |
| `kalyx_host_dispatch_demo.exe` | Führt nur validierte Sandbox-Aktionen aus. |
| `kalyx_audit_print.exe` | Zeigt Audit-Dateien lesbar an. |
| `python/kalyx_llm_bridge.py` | Sendet Envelope an LM Studio oder anderen OpenAI-kompatiblen Provider. |

## Profile anzeigen

```powershell
.\build_vs\Release\kalyx_prompt_author.exe --list-profiles --profile-dir .\profiles
```

## Prompt-Pack aus Custom-Profil erzeugen

```powershell
$goal = "Fasse README.md für neue Benutzer zusammen und exportiere Markdown sicher in die Sandbox."
.\build_vs\Release\kalyx_prompt_author.exe --profile-dir .\profiles --profile custom.summary_export --goal $goal --pack .\out\custom_profile.kpromptpack
```

## Envelope aus Prompt-Pack erzeugen

```powershell
.\build_vs\Release\kalyx_make_envelope.exe --prompt-pack .\out\custom_profile.kpromptpack --domain app --intent custom_summary_export --out .\out\custom_profile.kie.md
```

## LM-Studio-Live-Lauf

```powershell
$models = Invoke-RestMethod http://127.0.0.1:1234/v1/models
$KALYX_RESOLVED_MODEL = $models.data[0].id
python .\python\kalyx_llm_bridge.py --mode lmstudio --envelope .\out\custom_profile.kie.md --response .\out\custom_profile_lmstudio.kresp.md --audit .\out\custom_profile_lmstudio.kaudit.json --validator .\build_vs\Release\kalyx_validate_response.exe --model $KALYX_RESOLVED_MODEL --timeout 3000 --max-tokens 4096 --repair-attempts 2
```

## Dispatch

```powershell
.\build_vs\Release\kalyx_host_dispatch_demo.exe --envelope .\out\custom_profile.kie.md --response .\out\custom_profile_lmstudio.kresp.md --validation-audit .\out\custom_profile_dispatch_validation.kaudit.json --dispatch-audit .\out\custom_profile.kdispatch.json --sandbox-dir .\out\custom_profile_sandbox --provider lmstudio --model $KALYX_RESOLVED_MODEL --temperature 0 --max-tokens 4096
```

## Sicherheitsregeln

KALYX-KIE v1.0 Final führt keine echten OS-Befehle aus. Der Demo-Dispatcher schreibt ausschließlich in den angegebenen Sandbox-Ordner. Ungültige LLM-Antworten werden durch das Strict Dispatch Gate abgelehnt.

