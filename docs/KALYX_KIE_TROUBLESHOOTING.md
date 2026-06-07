# KALYX-KIE Troubleshooting

## MSBuild OutOfMemory / Auslagerungsdatei zu klein

Symptom:

```text
System.OutOfMemoryException
Die Auslagerungsdatei ist zu klein
CL.exe konnte nicht ausgeführt werden
cmd.exe konnte nicht ausgeführt werden
```

Lösung:

```powershell
& $CMAKE_EXE --build build_vs --config Release --parallel 1 -- /m:1 /p:CL_MPCount=1
```

## LM Studio nicht erreichbar

Prüfen:

```powershell
Invoke-RestMethod http://127.0.0.1:1234/v1/models
```

## LLM erzeugt ungültige Response

Nutze höhere Reparaturversuche:

```powershell
--repair-attempts 2
```

## Dispatch wird abgelehnt

Prüfe zuerst die Validation-Audit-Datei. Dispatch ist nur erlaubt, wenn `accepted=true` ist.
