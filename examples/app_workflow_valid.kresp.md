# KALYX LLM Response

## Human Response

Ich schlage einen sicheren mehrstufigen Sandbox-Workflow vor: zuerst eine Markdown-Kopie exportieren, danach eine Vorschauanforderung schreiben und anschließend einen Save-As-Request als Sandbox-Artefakt erzeugen. Es werden keine echten Dateien überschrieben und keine OS-Befehle ausgeführt.

## Machine Result

```json
{
  "schema": "KRESP01",
  "type": "command",
  "human_summary": "Führe einen sicheren dreistufigen Sandbox-Workflow aus: export_document, open_preview, save_as.",
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
        "args": {"mode": "markdown", "theme": "plain"}
      },
      {
        "command": "emit_ui_command",
        "target": "open_preview",
        "args": {"mode": "markdown", "theme": "plain"}
      },
      {
        "command": "emit_ui_command",
        "target": "save_as",
        "args": {"mode": "markdown", "theme": "plain"}
      }
    ]
  }
}
```
