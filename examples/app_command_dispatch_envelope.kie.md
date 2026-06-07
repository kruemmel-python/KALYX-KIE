---
schema: KIE01
system: KALYX
domain: app
intent: export_document
authority: application_state
response_contract: KRESP01
audit_contract: KAUDIT01
---

# KALYX Interaction Envelope

## Allowed Actions

```json
[
  {"name":"answer","description":"Return a human-readable answer."},
  {"name":"ask_clarification","description":"Ask one clarification question."},
  {"name":"emit_ui_command","description":"Propose a UI command for host validation.","args":{"target":["export_document","open_preview","save_as"],"mode":["html","pdf","reveal","markdown"],"theme":["plain","dark","scientific","rpg"]}}
]
```

## Forbidden Actions

```json
[
  {"kind":"forbidden_target","name":"overwrite_file"},
  {"kind":"forbidden_target","name":"delete_all_files"},
  {"kind":"forbidden_command","name":"execute_external_command"},
  {"kind":"requires_confirmation_if_risk_at_least","risk":"high"}
]
```

## Validation Rules

```json
[
  {"kind":"requires_confirmation_if_risk_at_least","risk":"high"},
  {"kind":"forbidden_command","name":"execute_external_command"},
  {"kind":"forbidden_target","name":"delete_all_files"}
]
```

## User Request

Exportiere das aktuelle Dokument im Sandbox-Modus als Markdown-Kopie.

## Runtime State

```json
{"app":"KALYX host dispatch demo","current_document":"README.md","sandbox_only":true}
```

## Required Response

Return a valid `KRESP01` response. Use `type=command` for executable proposals.
