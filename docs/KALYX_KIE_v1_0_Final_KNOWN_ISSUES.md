# KALYX-KIE v1.0 Final Known Issues

## Windows Parallelbuild / Pagefile

Bei parallelem Build kann Windows in Commit-/Pagefile-Grenzen laufen. Der offizielle Final-Pfad nutzt daher seriellen Build:

```powershell
--parallel 1 -- /m:1 /p:CL_MPCount=1
```

## LM Studio

LM Studio muss laufen, ein Modell muss geladen sein, und `http://127.0.0.1:1234/v1/models` muss erreichbar sein.

## Demo-Dispatcher

Der Demo-Dispatcher schreibt ausschließlich in den Sandbox-Ordner. Echte OS-Kommandos, Löschen oder Überschreiben realer Dateien sind nicht Teil von v1.0 Final.
