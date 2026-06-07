# KALYX LLM Response

## Human Response

This workflow is intentionally invalid because it tries to open a preview before creating an export artifact.

## Machine Result

```json
{
  "schema": "KRESP01",
  "type": "command",
  "human_summary": "Invalid workflow order demo.",
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
        "target": "open_preview",
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
