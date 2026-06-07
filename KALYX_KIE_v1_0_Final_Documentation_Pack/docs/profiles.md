# Profiles

## Built-in Profiles

```text
app.summary_export
app.code_review
game.npc_action
tool.file_transform
research.hypothesis_report
workflow.multi_action
```

## Custom Profile Registry

Custom Profiles liegen in:

```text
profiles/*.kprofile.json
```

Ein Profil kann direkt geladen werden:

```powershell
.\build_vs\Release\kalyx_prompt_author.exe --profile-file .\profiles\custom.summary_export.kprofile.json --pack .\out\custom_profile.kpromptpack
```

Oder über Registry-Name:

```powershell
.\build_vs\Release\kalyx_prompt_author.exe --profile-dir .\profiles --profile custom.summary_export --pack .\out\custom_profile.kpromptpack
```

## Profil-Design-Regeln

Ein gutes Profil sollte enthalten:

- klare Domain
- klaren Intent
- kleines Aktionsset
- explizite Forbidden Actions
- Workflow-Reihenfolge
- maximales Schrittlimit
- Sandbox-only-Regel
- Defaults für Mode und Theme
