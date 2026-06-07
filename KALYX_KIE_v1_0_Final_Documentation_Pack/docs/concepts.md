# Concepts

## KALYX-KIE in einem Satz

KALYX-KIE ist ein Prompt-, Contract-, Validation-, Dispatch- und Audit-System für LLM-Interaktionen.

## Warum nicht nur Prompt-Dateien?

Ein Prompt beschreibt meist, was ein LLM tun soll. KALYX beschreibt zusätzlich, was das LLM darf, wie die Antwort aussehen muss, welche Aktionen erlaubt sind und wie die Ausführung auditiert wird.

## Hauptpipeline

```text
Profile
→ Prompt-Pack
→ Envelope
→ LLM
→ Response
→ Validator
→ Strict Dispatch Gate
→ Workflow Policy Engine
→ Sandbox
→ Audit
```

## Governance-Ebenen

1. **Profile Governance**  
   Domain, Intent, Ziele, Defaults.

2. **Contract Governance**  
   Erlaubte Aktionen, verbotene Aktionen, Antwortform.

3. **Envelope Governance**  
   Der LLM-Kontext enthält Regeln und sichere Grenzen.

4. **Response Governance**  
   KRESP01 wird validiert.

5. **Dispatch Governance**  
   Kein Dispatch ohne `accepted=true`.

6. **Workflow Governance**  
   Schrittlimit, Reihenfolge, Abhängigkeiten, Duplikatschutz.

7. **Audit Governance**  
   Hashes, Status, Anbieter, Modell, Artefakte.
