# Security Model

## Grundregel

```text
LLM output is proposal, not authority.
```

Ein LLM darf eine Aktion vorschlagen. KALYX entscheidet, ob diese Aktion gültig ist.

## Strict Dispatch Gate

Der Host-Dispatcher darf nur ausführen, wenn die KRESP-Antwort akzeptiert wurde.

```text
accepted=true
→ dispatch erlaubt

accepted=false
→ reject
```

## Sandbox Only

Die v1.0-Demo schreibt ausschließlich in den angegebenen Sandbox-Ordner.

Es werden nicht ausgeführt:

- Shell-Kommandos
- Datei-Löschungen
- Überschreibungen echter Projektdateien
- Netzwerkaktionen durch den Dispatcher

## Workflow Policy Engine

Der Workflow wird geprüft auf:

- erlaubte Targets
- erlaubte Reihenfolge
- vorherige erfolgreiche Schritte
- doppelte Schritte
- maximales Schrittlimit

## Audit Trail

Jede relevante Grenze erzeugt ein Artefakt:

- `KAUDIT01` für Validierung
- `KDISPATCH01` für Dispatch
- `KWORKFLOW01` für Workflow
