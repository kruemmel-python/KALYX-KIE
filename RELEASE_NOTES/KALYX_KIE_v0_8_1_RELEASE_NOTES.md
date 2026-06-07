# KALYX-KIE v0.8.1 — Strict Dispatch Gate

## Zweck

v0.8 bewies den Sandbox-Dispatch-Pfad, zeigte aber im realen LM-Studio-Test einen wichtigen Sicherheitsbefund: Eine formal ungültige LLM-Response wurde korrekt von der Validation abgelehnt, trotzdem konnte ein älteres/stales Dispatch-Artefakt den Eindruck eines Sandbox-Dispatchs erzeugen.

v0.8.1 schließt diese Lücke: Der Host-Dispatcher schreibt nun bei ungültiger KRESP-Validierung immer ein explizites Reject-Dispatch-Audit und beendet den Dispatch vor jeder Sandbox-Aktion.

## Änderungen

- Version auf `1.0-kie-v0.8.1` gesetzt.
- `kalyx_host_dispatch_demo` schreibt bei abgelehnter Response ein `KDISPATCH01`-Reject-Audit.
- Ungültige Responses führen zu `decision="reject"` statt zu fehlendem oder stale Dispatch-Audit.
- Kein Sandbox-Artefakt wird erzeugt, wenn die Response-Validierung fehlschlägt.
- Neuer Test: `kalyx_dispatch_strict_gate`.
- Neue Beispiel-Dateien:
  - `examples/app_command_dispatch_envelope.kie.md`
  - `examples/app_command_invalid_action_proposal.kresp.md`
- Neues Demo-Skript:
  - `scripts/run_kalyx_strict_dispatch_gate_demo.ps1`

## Validierung

Lokal geprüft:

```text
100% tests passed, 0 tests failed out of 108
```

## Produktive Regel

```text
Kein Dispatch ohne accepted=true.
Ungültige LLM-Antworten erzeugen nur ein Reject-Audit.
Die Sandbox bleibt leer.
```
