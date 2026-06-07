# KALYX LLM Response

## Human Response

I will create a safe sandbox export request. No source file is overwritten and no external command is executed.

## Machine Result

```json
{
  "schema": "KRESP01",
  "type": "command",
  "human_summary": "Create a sandbox markdown export request.",
  "risk": "low",
  "requires_confirmation": false,
  "uses_only_provided_context": true,
  "machine_result": {
    "command": "emit_ui_command",
    "target": "export_document",
    "args": {
      "mode": "markdown",
      "theme": "dark"
    }
  }
}
```
