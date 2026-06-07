# KALYX LLM Response

## Human Response

Der angeforderte Sandbox-Workflow wird vorbereitet.

## Machine Result

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
        "target": "open_preview",
        "args": {
          "mode": "markdown",
          "theme": "plain"
        }
      },
      {
        "command": "emit_ui_command",
        "target": "save_as",
        "args": {
          "mode": "markdown",
          "theme": "plain"
        }
      }
    ]
  }
}
```
