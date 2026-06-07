# CLI Reference

## kalyx_prompt_author.exe

Erzeugt Prompt-Packs aus Ziel, Profil oder Profil-Datei.

```powershell
.\build_vs\Release\kalyx_prompt_author.exe --list-profiles --profile-dir .\profiles
```

```powershell
.\build_vs\Release\kalyx_prompt_author.exe --profile-dir .\profiles --profile custom.summary_export --goal "Fasse README.md zusammen" --pack .\out\custom_profile.kpromptpack
```

```powershell
.\build_vs\Release\kalyx_prompt_author.exe --profile-file .\profiles\custom.summary_export.kprofile.json --pack .\out\custom_profile.kpromptpack
```

## kalyx_make_envelope.exe

Erzeugt `.kie.md` aus Prompt-Pack oder State/Request.

```powershell
.\build_vs\Release\kalyx_make_envelope.exe --prompt-pack .\out\custom_profile.kpromptpack --domain app --intent custom_summary_export --out .\out\custom_profile.kie.md
```

## kalyx_validate_response.exe

Validiert eine `.kresp.md` gegen einen Envelope.

```powershell
.\build_vs\Release\kalyx_validate_response.exe --envelope .\out\custom_profile.kie.md --response .\examples\app_workflow_valid.kresp.md --audit .\out\custom_profile_validation.kaudit.json
```

## kalyx_host_dispatch_demo.exe

Dispatcht nur validierte Aktionen in eine Sandbox.

```powershell
.\build_vs\Release\kalyx_host_dispatch_demo.exe --envelope .\out\custom_profile.kie.md --response .\examples\app_workflow_valid.kresp.md --validation-audit .\out\custom_profile_validation.kaudit.json --dispatch-audit .\out\custom_profile.kdispatch.json --sandbox-dir .\out\custom_profile_sandbox
```

## kalyx_audit_print.exe

Gibt Audit-Dateien lesbar aus.

```powershell
.\build_vs\Release\kalyx_audit_print.exe .\out\custom_profile_validation.kaudit.json
```

## kalyx_llm_bridge.py

Spricht LM Studio oder OpenAI-kompatible Provider an.

```powershell
python .\python\kalyx_llm_bridge.py --mode lmstudio --envelope .\out\custom_profile.kie.md --response .\out\custom_profile_lmstudio.kresp.md --audit .\out\custom_profile_lmstudio.kaudit.json --validator .\build_vs\Release\kalyx_validate_response.exe --model $KALYX_RESOLVED_MODEL --timeout 3000 --max-tokens 4096 --repair-attempts 2
```
