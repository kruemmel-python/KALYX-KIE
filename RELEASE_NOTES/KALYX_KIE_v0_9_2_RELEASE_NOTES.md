# KALYX-KIE v0.9.2 — Prompt Authoring + Contract Synthesis

v0.9.2 ergänzt den bestehenden Governance-, Validation-, Workflow- und Audit-Kern um eine Prompt-Authoring-Schicht.

## Neue Konzepte

- `KPROMPT01`: menschenlesbares Prompt-Authoring-Asset.
- `KCONTRACT01`: maschinenlesbarer Governance-Vertrag.
- `.kpromptpack/`: Paket aus Prompt, Contract, Request, Allowed Actions, Policy Rules, Example State und Response Shape.

## Neues Tool

```powershell
.\build_vs\Release\kalyx_prompt_author.exe --domain app --intent summarize_and_export --goal "Fasse README.md zusammen und exportiere Markdown in die Sandbox" --pack .\out\readme_export.kpromptpack
```

## Integration

`kalyx_make_envelope` kann nun direkt aus einem Prompt-Pack einen Envelope erzeugen:

```powershell
.\build_vs\Release\kalyx_make_envelope.exe --prompt-pack .\out\readme_export.kpromptpack --domain app --intent summarize_and_export --out .\out\promptpack_app.kie.md
```

Damit wird KALYX zu:

```text
Prompt Authoring + Contract Synthesis + Validation + Dispatch + Audit
```
