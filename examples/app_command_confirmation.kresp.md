# KALYX LLM Response

## Human Response

I propose an HTML export, but host confirmation is required before dispatch.

## Machine Result

```json
{
  "schema": "KRESP01",
  "type": "command",
  "human_summary": "Export document as HTML after host confirmation.",
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
