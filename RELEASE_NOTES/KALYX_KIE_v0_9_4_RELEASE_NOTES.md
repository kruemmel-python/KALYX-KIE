# KALYX-KIE v0.9.4 — Profile Registry + Custom Profile Loader

v0.9.4 erweitert den Prompt-Authoring-Layer um eine flexible Profile Registry.

## Neu

- `--profile-file FILE` lädt ein externes `KPROFILE01` JSON-Profil.
- `--profile-dir DIR --profile NAME` sucht zusätzlich nach `DIR/NAME.kprofile.json`.
- `--list-profiles --profile-dir DIR` zeigt eingebaute Profile und gefundene Custom-Profile.
- Prompt-Packs übernehmen Domain, Intent, Zielbeschreibung, Workflow-Policy, Targets und Defaults aus Custom-Profilen.
- Beispielprofile liegen unter `profiles/`.

## KPROFILE01 Minimalform

```json
{
  "schema": "KPROFILE01",
  "name": "custom.summary_export",
  "domain": "app",
  "intent": "custom_summary_export",
  "mode": "workflow",
  "default_goal": "Fasse ein Dokument zusammen und exportiere es in die Sandbox.",
  "description": "Custom Profil.",
  "targets": ["export_document", "open_preview", "save_as"],
  "required_order": ["export_document", "open_preview", "save_as"],
  "workflow": true,
  "max_steps": 8,
  "default_mode": "markdown",
  "default_theme": "plain"
}
```

## Ziel

KALYX-Profile sind jetzt nicht mehr nur fest eingebaut. Teams können eigene wiederverwendbare Governance-Presets versionieren und daraus Prompt-Packs synthetisieren.
