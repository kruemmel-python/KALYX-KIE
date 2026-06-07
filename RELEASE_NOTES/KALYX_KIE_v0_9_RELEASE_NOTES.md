# KALYX-KIE v0.9 — Multi-Action Workflow Sandbox

v0.9 erweitert KALYX-KIE vom einzelnen validierten Host-Command zu einem kontrollierten, auditierbaren Multi-Action-Workflow.

## Neu

- `machine_result.command = "workflow"`
- `machine_result.target = "multi_action_sandbox"`
- `machine_result.workflow[]` als sequenzielle, validierte Schrittliste
- Workflow-Sandbox-Artefakte:
  - `workflow_step_01_export_document_sandbox.md`
  - `workflow_step_02_open_preview_request.json`
  - `workflow_step_03_save_as_request.json`
  - `workflow.kworkflow.json`
- Dispatch-Audit erweitert um:
  - `workflow_step_count`
  - `workflow_executed_count`
- Neuer Test: `kalyx_workflow_sandbox`
- Neue Beispiele:
  - `examples/app_workflow_request.txt`
  - `examples/app_workflow_valid.kresp.md`
  - `examples/app_workflow_forbidden.kresp.md`
- Neues Skript:
  - `scripts/run_kalyx_workflow_sandbox_demo_v0_9.ps1`
  - `scripts/run_kalyx_lmstudio_workflow_live_v0_9.ps1`

## Sicherheitsmodell

Der Workflow-Dispatcher führt keine OS-Befehle aus, überschreibt keine Quelldateien und schreibt ausschließlich in den angegebenen Sandbox-Ordner. Jeder Workflow-Schritt muss dem internen Sandbox-Allowlist-Vertrag entsprechen.

## Teststatus

Lokaler Referenzlauf:

```text
100% tests passed, 0 tests failed out of 109
```
