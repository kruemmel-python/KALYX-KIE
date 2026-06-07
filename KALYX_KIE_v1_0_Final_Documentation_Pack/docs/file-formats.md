# File Formats

## KPROFILE01

Ein Domain-Profil beschreibt wiederverwendbare Defaults.

```json
{
  "schema": "KPROFILE01",
  "name": "custom.summary_export",
  "domain": "app",
  "intent": "custom_summary_export",
  "mode": "workflow",
  "default_goal": "Fasse ein Dokument zusammen und exportiere es sicher.",
  "targets": ["export_document", "open_preview", "save_as"],
  "required_order": ["export_document", "open_preview", "save_as"],
  "workflow": true,
  "max_steps": 8,
  "default_mode": "markdown",
  "default_theme": "plain"
}
```

## KPROMPT01

Menschlich lesbarer Prompt-Entwurf.

```markdown
---
schema: KPROMPT01
profile: custom.summary_export
domain: app
intent: custom_summary_export
mode: workflow
---

# Goal

Fasse README.md zusammen und exportiere Markdown in die Sandbox.
```

## KCONTRACT01

Maschinenlesbarer Governance-Vertrag.

```json
{
  "schema": "KCONTRACT01",
  "domain": "app",
  "intent": "custom_summary_export",
  "response_schema": "KRESP01",
  "workflow_policy": {
    "max_steps": 8,
    "sandbox_only": true,
    "required_order": ["export_document", "open_preview", "save_as"]
  }
}
```

## KIE01

Der Interaction Envelope ist die vollständige LLM-Eingabe.

Er enthält:

- Domain
- Intent
- Request
- Allowed Actions
- Forbidden Actions
- Policy Rules
- Expected Response Shape
- State

## KRESP01

LLM-Antwortformat.

```json
{
  "schema": "KRESP01",
  "type": "command",
  "risk": "none",
  "requires_confirmation": false,
  "uses_only_provided_context": true,
  "machine_result": {
    "command": "workflow",
    "target": "multi_action_sandbox",
    "workflow": []
  }
}
```

## KAUDIT01

Validierungs-Audit.

Enthält:

- Envelope-Hash
- Response-Hash
- Provider
- Modell
- Validation-Status
- Response-Typ
- Command

## KDISPATCH01

Dispatch-Audit.

Enthält:

- Decision
- Status
- Command
- Target
- Sandbox-Dir
- Artefakt
- Workflow-Schrittanzahl

## KWORKFLOW01

Workflow-Manifest mit Step-Status.

```json
{
  "schema": "KWORKFLOW01",
  "status": "ok",
  "step_count": 3,
  "steps": [
    {
      "index": 1,
      "command": "emit_ui_command",
      "target": "export_document",
      "status": "ok"
    }
  ]
}
```
