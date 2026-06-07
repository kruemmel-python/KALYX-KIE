# KALYX-KIE v0.8.2 — Resolved Model Audit + LLM Command Contract Hardening

## Ziel

v0.8.2 härtet den produktiven LM-Studio-Command-Pfad nach v0.8.1:

- `model=auto` wird im empfohlenen Live-Skript vorab zu einem konkreten LM-Studio-Modellnamen aufgelöst.
- Die Bridge fordert den `KRESP01`-Command-Vertrag strenger ein.
- Häufige lokale Modellabweichungen wie `type="action_proposal"`, `machine_result.action` und arrayförmige `target/mode/theme`-Werte werden deterministisch in den gültigen Command-Vertrag normalisiert.
- Der Host-Dispatcher bleibt durch v0.8.1 weiter strikt: kein Dispatch ohne `accepted=true`.

## Reparierte lokale Modellabweichung

Viele lokale Modelle antworten semantisch richtig, aber strukturell falsch:

```json
{
  "schema": "KRESP01",
  "type": "action_proposal",
  "machine_result": {
    "action": "emit_ui_command",
    "args": {
      "target": ["export_document"],
      "mode": ["markdown"],
      "theme": ["plain"]
    }
  }
}
```

v0.8.2 normalisiert dieses enge Muster zu:

```json
{
  "schema": "KRESP01",
  "type": "command",
  "machine_result": {
    "command": "emit_ui_command",
    "target": "export_document",
    "args": {
      "mode": "markdown",
      "theme": "plain"
    }
  }
}
```

## Neuer Live-Skriptlauf

```powershell
.\scripts\run_kalyx_lmstudio_dispatch_live_v0_8_2.ps1
```

Das Skript führt aus:

1. Build
2. Tests
3. Request-Erzeugung
4. Envelope-Erzeugung
5. LM-Studio-Modellauflösung über `/v1/models`
6. Bridge-Aufruf
7. KRESP/KAUDIT-Erzeugung
8. Host-Dispatch
9. Dispatch-Audit
10. Sandbox-Artefaktanzeige

## Status

v0.8.2 ist weiterhin vollständig kompatibel zu v0.8.1-Envelopes, KRESP-Dateien und Dispatch-Audits.
