# Developer Guide

## Repository-Integration

KALYX-KIE lässt sich in Host-Anwendungen über drei Ebenen integrieren:

1. Prompt-Pack-Erzeugung
2. Response-Validierung
3. Sandbox-/Host-Dispatch

## Host-Integration

Eine Host-App sollte niemals direkt LLM-Ausgabe ausführen.

Empfohlen:

```text
LLM Response
→ kalyx_validate_response
→ accepted=true?
→ Host Policy
→ Sandbox / real adapter
→ Audit
```

## Eigene Profile

1. `profiles/my.domain.kprofile.json` anlegen
2. `schema=KPROFILE01` setzen
3. erlaubte Targets definieren
4. Workflow-Reihenfolge definieren
5. Prompt-Pack erzeugen
6. Envelope erzeugen
7. LLM/Offline-Test ausführen

## Produktionsadapter

Die Demo schreibt nur Sandbox-Artefakte.

Produktionsadapter sollten getrennt implementiert werden und weiterhin:

- Validation-Audit prüfen
- Dispatch-Audit schreiben
- explizite Confirmation respektieren
- keine impliziten globalen Zustände verwenden
