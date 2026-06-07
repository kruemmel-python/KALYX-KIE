# KALYX LLM Response

## Human Response

I would export `notes.md` as HTML with a dark theme and require confirmation because unsaved changes are present.

## Machine Result

```json
{
  "schema": "KRESP01",
  "type": "command",
  "human_summary": "Export notes.md as HTML with host confirmation.",
  "risk": "low",
  "requires_confirmation": true,
  "uses_only_provided_context": true,
  "machine_result": {
    "command": "emit_ui_command",
    "target": "export_document",
    "args": {
      "mode": "html",
      "theme": "dark"
    }
  }
}
```
