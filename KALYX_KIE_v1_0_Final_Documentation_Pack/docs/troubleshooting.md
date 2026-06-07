# Troubleshooting

## PowerShell-Skript nicht gefunden

Wenn ein Skript nicht gefunden wird, ist meistens der falsche Projektordner aktiv.

Beispiel:

```powershell
PS D:\KALYX_KIE_v1_0_RC2>
```

In diesem Ordner existieren keine Final-Skripte.

Lösung:

```powershell
cd D:\KALYX_KIE_v1_0_Final
```

## Nicht genügend Arbeitsspeicher beim Build

Nutze seriellen MSBuild:

```powershell
& $CMAKE_EXE --build build_vs --config Release --parallel 1 -- /m:1 /p:CL_MPCount=1
```

## LM Studio nicht erreichbar

Prüfe:

```powershell
Invoke-RestMethod http://127.0.0.1:1234/v1/models
```

## Response rejected

Dann ist die LLM-Antwort nicht KRESP01-konform.

Prüfe:

```powershell
Get-Content .\out\custom_profile_lmstudio.kresp.md -Raw
Get-Content .\out\custom_profile_lmstudio.kaudit.json -Raw
```

## Kein Dispatch

Wenn `accepted=false`, ist das korrekt. KALYX blockiert dann.
