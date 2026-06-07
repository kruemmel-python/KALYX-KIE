# KALYX LLM Response

## Human Response

This workflow is intentionally invalid because it repeats the export_document target.

## Machine Result

```json
{
  "schema": "KRESP01",
  "type": "command",
  "human_summary": "Invalid duplicate workflow step demo.",
  "risk": "low",
  "requires_confirmation": false,
  "uses_only_provided_context": true,
  "machine_result": {
    "command": "workflow",
    "target": "multi_action_sandbox",
    "args": {
      "mode": "markdown",
      "theme": "plain"
    },
    "workflow": [
      {
        "command": "emit_ui_command",
        "target": "export_document",
        "args": {
          "mode": "markdown",
          "theme": "plain"
        }
      },
      {
        "command": "emit_ui_command",
        "target": "export_document",
        "args": {
          "mode": "markdown",
          "theme": "plain"
        }
      }
    ]
  }
}
```
