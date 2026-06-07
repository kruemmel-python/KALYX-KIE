# KALYX LLM Response

## Human Response

Ich schlage einen Export vor, verwende aber absichtlich ein falsches Response-Type-Schema.

## Machine Result

```json
{
  "schema": "KRESP01",
  "type": "action_proposal",
  "human_summary": "Ungültige action_proposal-Struktur, die nicht dispatcht werden darf.",
  "risk": "low",
  "requires_confirmation": false,
  "uses_only_provided_context": true,
  "machine_result": {
    "action": "emit_ui_command",
    "args": {
      "target": ["export_document"],
      "mode": ["markdown"],
      "theme": ["plain"]
    }
  }
}
```
