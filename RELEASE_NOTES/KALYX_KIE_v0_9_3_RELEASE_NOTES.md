# KALYX-KIE v0.9.3 — Prompt Pack Templates + Domain Profiles

v0.9.3 erweitert den Prompt Authoring Layer um wiederverwendbare Domain-Profile.

## Neu

- `kalyx_prompt_author --list-profiles`
- `kalyx_prompt_author --profile NAME [--goal TEXT] --pack DIR`
- Vorgefertigte Profile:
  - `app.summary_export`
  - `app.code_review`
  - `game.npc_action`
  - `tool.file_transform`
  - `research.hypothesis_report`
  - `workflow.multi_action`
- Profile setzen automatisch Domain, Intent, Default-Goal, Zielmenge, Workflow-Policy, Default-Mode und Default-Theme.
- Prompt-Packs enthalten jetzt Profilinformationen in `prompt.kprompt.md`, `contract.kcontract.json`, `request.txt` und `README.md`.

## Zweck

Prompty vereinfacht Prompt-Dateien. KALYX v0.9.3 vereinfacht Prompt-Erstellung über Profile und synthetisiert daraus gleichzeitig Contract, Policy, Expected Response, Envelope, Validation, Dispatch und Audit.
