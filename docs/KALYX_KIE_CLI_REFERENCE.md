# KALYX-KIE CLI-Referenz v1.0 Final

## kalyx_prompt_author

```powershell
kalyx_prompt_author --list-profiles [--profile-dir DIR]
kalyx_prompt_author --profile NAME [--profile-dir DIR] [--goal TEXT] [--pack DIR]
kalyx_prompt_author --profile-file FILE [--goal TEXT] [--pack DIR]
kalyx_prompt_author --domain app --intent NAME --goal TEXT --pack DIR
kalyx_prompt_author --version
```

## kalyx_make_envelope

```powershell
kalyx_make_envelope --prompt-pack DIR --domain app --intent NAME --out FILE [--state FILE]
kalyx_make_envelope --domain app --intent summarize_document --document FILE --document-type markdown --request FILE --out FILE
kalyx_make_envelope --version
```

## kalyx_validate_response

```powershell
kalyx_validate_response --envelope FILE --response FILE --audit FILE --provider NAME --model NAME --temperature 0 --max-tokens 4096
kalyx_validate_response --version
```

## kalyx_host_dispatch_demo

```powershell
kalyx_host_dispatch_demo --envelope FILE --response FILE --validation-audit FILE --dispatch-audit FILE --sandbox-dir DIR --provider NAME --model NAME --temperature 0 --max-tokens 4096
kalyx_host_dispatch_demo --version
```

## kalyx_audit_print

```powershell
kalyx_audit_print FILE
```


## v1.0 Final official scripts

```powershell
.\scripts\run_kalyx_v1_0_final_offline_reference.ps1
.\scripts\run_kalyx_v1_0_final_lmstudio_live.ps1
.\scripts\run_kalyx_v1_0_final_version_check.ps1
```
