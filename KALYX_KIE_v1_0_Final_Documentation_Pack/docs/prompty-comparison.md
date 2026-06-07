# KALYX-KIE vs. Prompty

## Kurzfassung

Prompty vereinfacht das Schreiben und Ausführen von Prompt-Dateien.

KALYX-KIE vereinfacht das Schreiben von Prompts **und** erzeugt daraus Governance-Verträge, validierte Antwortformate, Dispatch-Regeln und Audit-Artefakte.

## Vergleich

| Thema | Prompty | KALYX-KIE |
|---|---|---|
| Prompt als Datei | Ja | Ja, als KPROMPT01 / Prompt-Pack |
| YAML/Metadata | Ja | Ja, plus Contract und Policy |
| LLM Runtime | Ja | Ja, über Bridge |
| Provider-Konfiguration | Ja | Ja, providerseitig über Bridge/Runtime |
| Antwortvalidierung | begrenzt | zentraler Kern |
| Allowed Actions | Tool-/Runtime-orientiert | Contract-orientiert |
| Forbidden Actions | nicht Hauptfokus | explizit |
| Strict Dispatch Gate | nein | ja |
| Workflow Policy | nein | ja |
| Audit mit Hashes | Trace-orientiert | Governance-orientiert |
| Sandbox Dispatch | nein | ja |
| Custom Governance Profiles | nein | ja |

## Positionierung

```text
Prompty = Prompt-Datei + Runtime
KALYX-KIE = Prompt + Contract + Validation + Dispatch + Audit
```

## Wann Prompty?

- schnelle Prompt-Prototypen
- VS-Code-Prompt-Authoring
- direkte Runtime-Ausführung
- einfache LLM-Anwendungen

## Wann KALYX-KIE?

- wenn LLM-Aktionen kontrolliert werden müssen
- wenn Audits erforderlich sind
- wenn Host-Anwendungen nicht direkt LLM-Ausgaben vertrauen dürfen
- wenn Workflows policy-geprüft werden sollen
- wenn Prompt-Packs als Governance-Artefakte versioniert werden sollen
