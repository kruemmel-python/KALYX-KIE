# KALYX-KIE Schema-Referenz

## KPROFILE01

Beschreibt ein wiederverwendbares Profil.

Pflichtfelder:

```json
{
  "schema": "KPROFILE01",
  "name": "custom.summary_export",
  "domain": "app",
  "intent": "custom_summary_export",
  "mode": "workflow",
  "targets": ["export_document", "open_preview", "save_as"],
  "required_order": ["export_document", "open_preview", "save_as"],
  "workflow": true,
  "max_steps": 8,
  "default_mode": "markdown",
  "default_theme": "plain"
}
```

## KPROMPT01

Menschenlesbares Prompt-Asset im Markdown-Format. Es enthält Ziel, Domain, Intent, Sicherheitsregeln und gewünschte Ausgabeform.

## KCONTRACT01

Maschinenlesbarer Vertrag. Enthält erlaubte Aktionen, verbotene Aktionen, Workflow-Policy und erwartete Antwortform.

## KIE01

Envelope für das LLM. Enthält Request, State, Allowed Actions, Forbidden Actions, Validation Rules und Required Response.

## KRESP01

Antwort des LLM. Für Workflows muss gelten:

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

Validierungsledger mit Envelope-Hash, Response-Hash, Provider, Modell, Validierungsstatus und Ergebnis.

## KDISPATCH01

Dispatch-Ledger. Enthält Entscheidung, Status, Command, Target, Sandbox-Ordner, Artefaktdatei und Workflow-Zähler.

## KWORKFLOW01

Workflow-Manifest mit Status pro Schritt.

