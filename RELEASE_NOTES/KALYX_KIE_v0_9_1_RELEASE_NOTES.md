# KALYX-KIE v0.9.1 — Workflow Policy Engine + Step Dependencies

## Kernziel

v0.9.1 härtet den in v0.9 eingeführten Multi-Action-Workflow. Workflows werden nicht mehr nur gegen eine Sandbox-Allowlist geprüft, sondern zusätzlich gegen eine explizite Workflow-Policy.

## Neue Policy-Regeln

- Maximal 8 Workflow-Schritte.
- Nur Sandbox-Ziele sind erlaubt: `export_document`, `open_preview`, `save_as`.
- Keine doppelten Targets innerhalb eines Workflows.
- `open_preview` darf nur nach erfolgreichem `export_document` laufen.
- `save_as` darf nur nach erfolgreichem `open_preview` laufen.
- Die Schritt-Reihenfolge muss monoton sein: `export_document → open_preview → save_as`.
- Bei Policy-Verstoß wird kein Schritt ausgeführt.
- Bei Abbruch wird trotzdem ein `KWORKFLOW01`-Manifest mit `status="aborted"` geschrieben.

## KWORKFLOW01-Erweiterung

Jeder Schritt erhält nun:

```json
{
  "index": 1,
  "command": "emit_ui_command",
  "target": "export_document",
  "status": "ok|pending|rejected|failed",
  "reason": "...",
  "artifact": "..."
}
```

## Neue Testabdeckung

- `kalyx_workflow_policy_engine`
- gültiger Workflow mit 3 Schritten
- ungültige Reihenfolge
- doppelter Schritt
- Abbruchmanifest mit `status="aborted"`
- keine teilweise Ausführung bei Policy-Verstoß

## Neue Beispiele

- `examples/app_workflow_order_violation.kresp.md`
- `examples/app_workflow_duplicate_step.kresp.md`

## Neue Skripte

- `scripts/run_kalyx_workflow_policy_demo_v0_9_1.ps1`
- `scripts/run_kalyx_lmstudio_workflow_live_v0_9_1.ps1`
