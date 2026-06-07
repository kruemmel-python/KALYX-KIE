# KALYX — Universelles deterministisches Interaktionssubstrat für LLM-Systeme

KALYX ist kein Prompt-Manager, kein Prompty-Klon und kein Wrapper um eine LLM-API.

KALYX ist ein universelles, deterministisches Kommunikationssubstrat zwischen Softwarezuständen, Anwendungen, Agenten, Spielen, Forschungssystemen, Codeanalyse-Werkzeugen und Large Language Models.

Der zentrale Zweck von KALYX ist es, beliebige Systemzustände so aufzubereiten, dass ein LLM sie verstehen, analysieren, kommentieren oder in erlaubte Handlungen übersetzen kann, ohne dass das LLM selbst zur Autorität über den Zustand wird.

KALYX beschreibt nicht nur einen Prompt.

KALYX beschreibt:

```text
Zustand
Kontext
Absicht
Autorisierte Daten
Erlaubte Aktionen
Verbotene Aktionen
Antwortvertrag
Validierungsregeln
Audit-Trail
Ausführungsgrenzen
```

Damit wird KALYX zu einer universellen Sprache zwischen klassischen Programmen und LLMs.

---

# Grundprinzip

Prompty sagt:

```text
Hier ist ein Prompt.
Fülle die Variablen.
Sende ihn an ein Modell.
```

KALYX sagt:

```text
Hier ist ein vollständiger, geprüfter Interaktionszustand.
Hier sind die autoritativen Daten.
Hier ist die Absicht.
Hier sind die erlaubten Antwortformen.
Hier sind die verbotenen Behauptungen und Aktionen.
Hier ist der Vertrag, gegen den die LLM-Antwort geprüft wird.
Hier ist der Audit-Datensatz für Nachvollziehbarkeit.
```

Das LLM darf interpretieren, erklären, klassifizieren, formulieren oder eine zulässige Aktion vorschlagen.

Das LLM darf nicht eigenmächtig Wahrheit erzeugen, Zustand verändern, externe Fakten erfinden, unsichtbaren Kontext annehmen oder Aktionen außerhalb des definierten Vertrags ausführen.

KALYX bleibt die Autoritätsschicht.

Das LLM bleibt die Interpretationsschicht.

---

# Architekturprinzip nach Zen of Krümmel

KALYX folgt strikt dem Prinzip:

```text
Substrat vor Oberfläche.
Runtime vor UI.
ABI vor Komfort.
Determinismus vor Begeisterung.
Audit vor Vertrauen.
```

Das Speicherlayout, die Dateiformate, die Antwortverträge und die Validierung bilden die eigentliche Architektur. UI, Chat, API-Adapter und Modellanbieter sind nur Projektionen dieses Substrats.

Ein KALYX-Datensatz muss autark sein. Er darf keine versteckten externen Zustände benötigen. Genau dieses Prinzip ist im Zen of Krümmel als „Zustandsfreiheit außerhalb des Envelopes“ und „kompromisslose ABI“ beschrieben: keine impliziten Kontexte, keine globalen Zustände, harte Schnittstellen und auditierbare Systemkohärenz. 

---

# Was KALYX werden soll

KALYX soll ein allgemeines System werden, das jede Software in die Lage versetzt, kontrolliert mit einem LLM zu kommunizieren.

KALYX ist für:

```text
Apps
Editoren
Games
Agentensysteme
Forschungssysteme
Simulationen
Codeanalyse
Builddiagnostik
Automatisierung
Dokumentenerzeugung
UI-Steuerung
Business-Workflows
Robuste Assistenzsysteme
```

geeignet.

Es ist nicht auf Forschung beschränkt. Forschung ist nur eine Domäne unter vielen.

Der Kern ist immer gleich:

```text
Eine Anwendung erzeugt einen KALYX Interaction Envelope.
Ein LLM liest diesen Envelope.
Das LLM antwortet in einem validierbaren Response-Format.
KALYX prüft die Antwort.
Erst danach darf die Host-Anwendung deterministisch entscheiden, was passiert.
```

---

# Zentrale Dateiformate

KALYX definiert drei primäre Artefakte.

---

## 1. `.kie.md` — KALYX Interaction Envelope

Der Interaction Envelope ist die Hauptdatei.

Er ist menschenlesbar als Markdown, aber maschinenlesbar durch feste Header, feste Sektionen und eingebettete JSON-Blöcke.

Er enthält den vollständigen Zustand einer LLM-Interaktion.

Beispielstruktur:

````markdown
---
schema: KIE01
system: KALYX
domain: app
intent: export_document
authority: application_state
response_contract: KRESP01
audit_contract: KAUDIT01
---

# KALYX Interaction Envelope

## Contract

The data inside this envelope is authoritative.
The model may interpret, classify and propose allowed actions.
The model must not invent missing state.
The model must not execute actions.
The model must return a valid response contract.

## User Request

Export this document as a beautiful HTML page.

## Runtime State

```json
{
  "application": "MDraft",
  "current_file": "README.md",
  "unsaved_changes": true,
  "selected_text_available": true,
  "network_available": true
}
````

## Authoritative Data

```json
{
  "document_title": "KALYX",
  "document_language": "de",
  "available_export_modes": ["html", "pdf", "reveal"],
  "available_themes": ["plain", "scientific", "rpg", "dark"]
}
```

## Allowed Actions

```json
[
  {
    "name": "answer",
    "description": "Return a human-readable answer."
  },
  {
    "name": "ask_clarification",
    "description": "Ask one clarification question."
  },
  {
    "name": "emit_ui_command",
    "description": "Propose a UI command that the host app may execute after validation."
  }
]
```

## Forbidden Actions

```json
[
  "do_not_overwrite_files_without_confirmation",
  "do_not_invent_missing_document_content",
  "do_not_execute_external_commands",
  "do_not_claim_that_the_action_was_executed"
]
```

## Required Response

Return a valid `KRESP01` response.

````

---

## 2. `.kresp.md` — KALYX LLM Response

Die Response-Datei enthält die LLM-Antwort.

Sie darf normalen Markdown-Text enthalten, muss aber einen maschinenlesbaren Ergebnisblock besitzen.

Beispiel:

```markdown
# KALYX LLM Response

## Human Response

Ich würde das Dokument als HTML exportieren und dafür das Theme `scientific` verwenden, weil es zur Struktur des Inhalts passt.

## Machine Result

```json
{
  "schema": "KRESP01",
  "type": "command",
  "command": {
    "name": "emit_ui_command",
    "target": "export_document",
    "args": {
      "mode": "html",
      "theme": "scientific"
    }
  },
  "requires_confirmation": true,
  "risk": "low",
  "human_summary": "Exportiere README.md als HTML im wissenschaftlichen Stil."
}
````

````

---

## 3. `.kaudit.json` — KALYX Audit Ledger

Der Audit Ledger dokumentiert die Interaktion.

Er enthält Hashes, Modellparameter, Validierungsstatus und Ablehnungsgründe.

Beispiel:

```json
{
  "schema": "KAUDIT01",
  "kalyx_version": "1.0",
  "envelope_file": "example.kie.md",
  "envelope_sha256": "f3a1...",
  "response_file": "example.kresp.md",
  "response_sha256": "a71b...",
  "validated": true,
  "validation_errors": [],
  "transport": {
    "provider": "openai_compatible",
    "model": "configured_outside_envelope",
    "temperature": 0.0,
    "max_tokens": 2048
  },
  "result": {
    "accepted": true,
    "response_schema": "KRESP01",
    "response_type": "command",
    "requires_confirmation": true
  }
}
````

---

# Universelle Domänen

KALYX unterstützt feste Domänen. Jede Domäne nutzt denselben Substratkern, aber andere Intents, erlaubte Aktionen und Validierungsregeln.

---

## `app`

Für normale Anwendungen.

Beispiele:

```text
Editoren
Dateimanager
Kalender
Mailclients
PDF-Tools
Export-Tools
Notizsysteme
UI-Assistenten
```

Typische Intents:

```text
classify_user_intent
summarize_document
generate_ui_command
explain_error
prepare_export
validate_user_action
```

Das LLM darf UI-Aktionen vorschlagen, aber nicht selbst ausführen.

---

## `game`

Für Spiele und interaktive Welten.

Beispiele:

```text
NPC-Dialoge
Quest-Auswertung
Fraktionsreaktionen
Händlerantworten
Welt-Ticker
Story-Erklärungen
```

Typische Intents:

```text
npc_dialogue
classify_player_action
explain_world_event
generate_dialogue_line
propose_npc_action
```

Das LLM darf Atmosphäre, Sprache und Dialog formen.
Die Spielwelt bleibt autoritativ.

Ein NPC darf vom LLM nicht plötzlich ein Item besitzen, das nicht im World-State steht.

---

## `agent`

Für Agentensysteme.

Beispiele:

```text
Planung
Risikoanalyse
Werkzeugauswahl
Konfliktlösung
Priorisierung
Selbstdiagnose
```

Typische Intents:

```text
plan_next_action
evaluate_risk
select_allowed_tool
explain_decision
classify_goal_state
```

Das LLM darf nur Aktionen ausgeben, die im Envelope erlaubt sind.

Jede Aktion muss validiert werden.

---

## `code`

Für Softwareentwicklung.

Beispiele:

```text
Compilerfehler
CTest-Ausgaben
LTEST-Befunde
Codeanalyse
Patchplanung
Regressionserkennung
```

Typische Intents:

```text
diagnose_build_error
classify_static_findings
propose_patch
review_code
generate_test_plan
```

Das LLM darf Fehler klassifizieren und Patches vorschlagen.
Die Tests bleiben autoritativ.

---

## `research`

Für wissenschaftliche und experimentelle Systeme.

Beispiele:

```text
KALYX-Reports
Nullmodelle
Matrizen
Simulationen
Messreihen
Anomalieprüfung
```

Typische Intents:

```text
diagnose_result
compare_runs
classify_evidence
propose_followup_test
summarize_measurement
```

Das LLM darf Hypothesen bilden.
Es darf keine Beweise behaupten, die nicht aus den Daten folgen.

---

## `workflow`

Für Business- und Automatisierungsprozesse.

Beispiele:

```text
Tickets
Dokumente
E-Mails
Freigaben
Aufgabenplanung
Prozessstatus
```

Typische Intents:

```text
classify_request
draft_response
route_task
summarize_status
detect_missing_information
```

Das LLM darf strukturieren, formulieren und vorbereiten.
Reale Aktionen benötigen Host-Freigabe oder Policy-Erlaubnis.

---

# KALYX-Kernschema

Jeder Envelope besitzt diese Pflichtfelder:

```json
{
  "schema": "KIE01",
  "domain": "app | game | agent | code | research | workflow",
  "intent": "string",
  "authority": "string",
  "response_contract": "KRESP01",
  "runtime_state": {},
  "authoritative_data": {},
  "allowed_actions": [],
  "forbidden_actions": [],
  "validation_rules": [],
  "audit_required": true
}
```

---

# KALYX Response Contract

Jede Antwort muss diesem Grundschema folgen:

```json
{
  "schema": "KRESP01",
  "type": "answer | command | clarification | rejection | diagnostic",
  "human_summary": "string",
  "risk": "none | low | medium | high | critical",
  "requires_confirmation": true,
  "uses_only_provided_context": true,
  "machine_result": {}
}
```

Pflichtregeln:

```text
schema muss KRESP01 sein
type muss erlaubt sein
risk muss erlaubt sein
requires_confirmation muss explizit gesetzt sein
uses_only_provided_context muss explizit gesetzt sein
machine_result muss zum Typ passen
verbotene Aktionen führen zur Ablehnung
fehlende Pflichtfelder führen zur Ablehnung
unbekannte Kommandos führen zur Ablehnung
```

---

# KALYX Validation Engine

KALYX benötigt eine eigene Validierungsengine.

Diese Engine prüft:

```text
Ist die Antwort syntaktisch gültig?
Ist das JSON parsebar?
Stimmt das Schema?
Ist der Antworttyp erlaubt?
Ist die Aktion im Envelope erlaubt?
Sind alle Argumente erlaubt?
Wird eine verbotene Aktion vorgeschlagen?
Behauptet das LLM eine Ausführung, obwohl nur ein Vorschlag erlaubt ist?
Wird nicht bereitgestellter Kontext erfunden?
Ist confirmation erforderlich?
Ist das Risiko korrekt markiert?
Ist die Antwort auditierbar?
```

Eine Antwort ist nur gültig, wenn sie formal und semantisch zum Envelope passt.

---

# KALYX Transport Adapter

Der Transport zu einem LLM ist nicht Teil der Autoritätsschicht.

KALYX trennt strikt:

```text
KALYX Core:
  Envelope-Erzeugung
  Response-Validierung
  Audit
  deterministische Tests

Transport Adapter:
  HTTP
  OpenAI-kompatible APIs
  Azure/OpenAI-kompatible APIs
  lokale Modelle
  Datei-basierter Offline-Modus
```

Der Transport Adapter darf Modellanbieter wechseln, ohne das KALYX-Format zu ändern.

Die Modellkonfiguration gehört nicht in die autoritative Envelope-Semantik.

Sie gehört in den Audit Ledger.

---

# KALYX CLI

KALYX soll eine klare CLI besitzen.

Beispiele:

```powershell
.\build_vs\Release\kalyx_make_envelope.exe --domain app --intent export_document --state .\examples\app_state.json --request .\examples\request.txt --out .\out\example.kie.md
```

```powershell
.\build_vs\Release\kalyx_validate_response.exe --envelope .\out\example.kie.md --response .\out\example.kresp.md --audit .\out\example.kaudit.json
```

```powershell
python .\python\kalyx_llm_bridge.py --envelope .\out\example.kie.md --response .\out\example.kresp.md --audit .\out\example.kaudit.json
```

Der Build-Befehl bleibt:

```powershell
$CMAKE_EXE = "C:\Program Files\CMake\bin\cmake.exe"; $CTEST_EXE = "C:\Program Files\CMake\bin\ctest.exe"; & $CMAKE_EXE -S . -B build_vs -G "Visual Studio 17 2022" -A x64; & $CMAKE_EXE --build build_vs --config Release --parallel; & $CTEST_EXE --test-dir build_vs -C Release --output-on-failure
```

Diese Build-Form ist für dein Projekt verbindlich hinterlegt. 

---

# KALYX als App-Protokoll

Für Apps bedeutet KALYX:

```text
Die App erzeugt einen Envelope aus ihrem aktuellen Zustand.
Das LLM interpretiert die Nutzerabsicht.
Das LLM gibt eine validierbare Antwort zurück.
Die App prüft die Antwort.
Die App führt nur erlaubte und bestätigte Aktionen aus.
```

Beispiel:

```text
Nutzer:
"Mach daraus eine schöne HTML-Seite."

App-State:
current_file = notes.md
available_exports = html, pdf, reveal
unsaved_changes = true

LLM-Antwort:
command = export_document
mode = html
theme = dark
requires_confirmation = true

KALYX:
prüft Befehl
prüft Argumente
prüft Risiko
erzeugt Audit
Host-App fragt Nutzer oder führt aus
```

---

# KALYX als Game-Protokoll

Für Spiele bedeutet KALYX:

```text
Die Spielwelt bleibt Wahrheit.
NPCs, Items, Ruf, Fraktionen, Dienste und Quests stehen im Envelope.
Das LLM erzeugt Dialog, Erklärung oder Handlungsvorschlag.
Die Engine validiert gegen den World-State.
```

Beispiel:

```json
{
  "npc": "Mira",
  "role": "healer",
  "service_locked": false,
  "player_gold": 15,
  "available_services": [
    {"id": "small_heal", "price": 12},
    {"id": "antidote", "price": 20}
  ]
}
```

Gültige LLM-Antwort:

```json
{
  "schema": "KRESP01",
  "type": "command",
  "human_summary": "Mira bietet kleine Heilung an.",
  "risk": "low",
  "requires_confirmation": false,
  "uses_only_provided_context": true,
  "machine_result": {
    "command": "offer_service",
    "service_id": "small_heal",
    "price": 12
  }
}
```

Ungültig wäre:

```text
Mira gibt dem Spieler ein legendäres Schwert.
```

weil dieses Item nicht im autoritativen Zustand steht.

---

# KALYX als Agenten-Protokoll

Für Agentensysteme bedeutet KALYX:

```text
Agentenziele werden explizit.
Werkzeuge werden explizit.
Risiken werden explizit.
Erlaubte Aktionen werden explizit.
Jede LLM-Antwort wird validiert.
```

Ein Agent darf nicht frei „denken und handeln“.

Er darf nur:

```text
einen Zustand lesen
eine erlaubte Aktion vorschlagen
eine Begründung liefern
ein Risiko markieren
eine Rückfrage stellen
```

Die Host-Runtime entscheidet.

---

# KALYX als Codeanalyse-Protokoll

Für Codeanalyse bedeutet KALYX:

```text
Compilerfehler, Testergebnisse und Scannerbefunde werden in einen Envelope gegossen.
Das LLM klassifiziert und erklärt.
KALYX prüft, ob die Antwort auf den gelieferten Daten basiert.
Patches bleiben nachvollziehbar.
```

Gerade für LTEST wäre das ideal.

Kategorien:

```text
true_defect
portability_risk
hygiene_hint
false_positive
needs_human_review
```

Dadurch kann LTEST lernen, Befunde besser einzuordnen, ohne blind einem LLM zu vertrauen.

---

# KALYX als Forschungs-Protokoll

Für Forschung bedeutet KALYX:

```text
Messwerte, Nullmodelle, Hashes, Matrizen und Reports werden autoritativ eingebettet.
Das LLM darf erklären und Hypothesen sortieren.
Es darf keine Beweiskraft erfinden.
```

Kategorien:

```text
substrate_structure
null_artifact
weak_evidence
candidate_anomaly
invalid_input
```

---

# KALYX als Workflow-Protokoll

Für Workflows bedeutet KALYX:

```text
Aufgaben, Dokumente, E-Mails, Tickets und Prozesszustände werden als Envelope dargestellt.
Das LLM darf sortieren, formulieren, klassifizieren und Vorschläge machen.
Ausführung bleibt bei Host-App, Policy oder Nutzerfreigabe.
```

---

# Sicherheitsmodell

KALYX nutzt kein Sicherheitsmodell auf Vertrauensbasis.

KALYX nutzt ein Vertragsmodell.

Das LLM ist nicht vertrauenswürdig.

Darum gelten folgende Regeln:

```text
Keine Antwort ohne Schema.
Keine Aktion ohne Allowed-Action-Match.
Keine externen Annahmen ohne Kennzeichnung.
Keine Ausführung ohne Host-Entscheidung.
Keine Zustandsänderung ohne Audit.
Keine Hochrisikoaktion ohne Bestätigung.
Keine Modellantwort als Wahrheit.
```

---

# Determinismusmodell

LLM-Ausgaben sind nicht vollständig deterministisch.

KALYX löst das nicht durch Illusion, sondern durch Trennung:

```text
Deterministisch:
  Envelope-Erzeugung
  Hashing
  Validierung
  Allowed-Action-Prüfung
  Audit
  Tests

Nicht vollständig deterministisch:
  Modellantwort
  Formulierung
  Interpretation
```

KALYX macht die nichtdeterministische LLM-Schicht kontrollierbar, indem jede Antwort gegen einen deterministischen Vertrag geprüft wird.

---

# Implementierungsumfang

Das Agentensystem soll KALYX vollständig als ein zusammenhängendes System bauen.

Der vollständige Umfang umfasst:

```text
C-Core für Envelope-Erzeugung
C-Core für Response-Validierung
C-Core für Audit-Erzeugung
Markdown-Emitter für .kie.md
Markdown-Parser/Scanner für .kresp.md
JSON-Minimalparser oder kontrollierter JSON-Block-Parser
Domain-Definitionen für app, game, agent, code, research, workflow
Allowed-Action-Validierung
Forbidden-Action-Validierung
Risk-Policy
Confirmation-Policy
SHA-256 Hashing für Envelope und Response
CLI-Tools
Python-Bridge für LLM-Transport
Beispieldaten für alle Domänen
CTest-Tests
README
Beispielausgaben
```

---

# Erwartete Projektstruktur

```text
KALYX/
│
├─ CMakeLists.txt
│
├─ include/
│  ├─ kalyx_common.h
│  ├─ kalyx_interaction.h
│  ├─ kalyx_response.h
│  ├─ kalyx_audit.h
│  ├─ kalyx_domain.h
│  ├─ kalyx_hash.h
│  └─ kalyx_error.h
│
├─ src/
│  ├─ kalyx_common.c
│  ├─ kalyx_interaction.c
│  ├─ kalyx_response.c
│  ├─ kalyx_audit.c
│  ├─ kalyx_domain.c
│  ├─ kalyx_hash.c
│  └─ kalyx_error.c
│
├─ tools/
│  ├─ kalyx_make_envelope.c
│  ├─ kalyx_validate_response.c
│  └─ kalyx_audit_print.c
│
├─ python/
│  └─ kalyx_llm_bridge.py
│
├─ examples/
│  ├─ app_state.json
│  ├─ app_request.txt
│  ├─ game_state.json
│  ├─ game_request.txt
│  ├─ agent_state.json
│  ├─ agent_request.txt
│  ├─ code_state.json
│  ├─ code_request.txt
│  ├─ research_state.json
│  ├─ research_request.txt
│  ├─ workflow_state.json
│  └─ workflow_request.txt
│
├─ tests/
│  ├─ test_kalyx_envelope_determinism.c
│  ├─ test_kalyx_response_validation.c
│  ├─ test_kalyx_forbidden_actions.c
│  ├─ test_kalyx_allowed_actions.c
│  ├─ test_kalyx_audit_hashes.c
│  └─ test_kalyx_domains.c
│
├─ out/
│  └─ generated at runtime
│
└─ README.md
```

---

# C-ABI-Grundmodell

KALYX soll intern mit festen Strukturen arbeiten.

Beispielhaft:

```c
typedef enum KalyxDomain {
    KALYX_DOMAIN_APP = 1,
    KALYX_DOMAIN_GAME = 2,
    KALYX_DOMAIN_AGENT = 3,
    KALYX_DOMAIN_CODE = 4,
    KALYX_DOMAIN_RESEARCH = 5,
    KALYX_DOMAIN_WORKFLOW = 6
} KalyxDomain;

typedef enum KalyxResponseType {
    KALYX_RESPONSE_ANSWER = 1,
    KALYX_RESPONSE_COMMAND = 2,
    KALYX_RESPONSE_CLARIFICATION = 3,
    KALYX_RESPONSE_REJECTION = 4,
    KALYX_RESPONSE_DIAGNOSTIC = 5
} KalyxResponseType;

typedef enum KalyxRisk {
    KALYX_RISK_NONE = 0,
    KALYX_RISK_LOW = 1,
    KALYX_RISK_MEDIUM = 2,
    KALYX_RISK_HIGH = 3,
    KALYX_RISK_CRITICAL = 4
} KalyxRisk;
```

Keine dynamische Magie.

Keine versteckten Provider-Abhängigkeiten im C-Core.

Keine globale Runtime.

---

# Fehlercodes

KALYX soll explizite Fehlercodes verwenden.

```c
typedef enum KalyxStatus {
    KALYX_OK = 0,
    KALYX_ERR_INVALID_ARGUMENT = 1,
    KALYX_ERR_IO = 2,
    KALYX_ERR_PARSE = 3,
    KALYX_ERR_SCHEMA = 4,
    KALYX_ERR_FORBIDDEN_ACTION = 5,
    KALYX_ERR_UNKNOWN_ACTION = 6,
    KALYX_ERR_MISSING_REQUIRED_FIELD = 7,
    KALYX_ERR_HASH_MISMATCH = 8,
    KALYX_ERR_RESPONSE_REJECTED = 9
} KalyxStatus;
```

Jeder Fehler muss in Tests reproduzierbar sein.

---

# Tests

KALYX muss durch Tests beweisen, dass es funktioniert.

Pflichttests:

```text
Gleicher Input erzeugt byteidentischen Envelope.
Envelope enthält alle Pflichtsektionen.
Response ohne JSON-Block wird abgelehnt.
Response mit falschem Schema wird abgelehnt.
Response mit unbekanntem Command wird abgelehnt.
Response mit verbotener Aktion wird abgelehnt.
Response mit erlaubter Aktion wird angenommen.
Audit enthält korrekte SHA-256-Hashes.
Alle Domänen erzeugen gültige Envelopes.
Alle Beispielantworten werden korrekt validiert.
```

---

# Qualitätsziel

KALYX ist fertig, wenn folgender Satz stimmt:

```text
Eine beliebige Anwendung kann ihren Zustand als KALYX Interaction Envelope ausgeben,
ein LLM kann darauf antworten,
KALYX kann diese Antwort deterministisch prüfen,
und die Host-Anwendung kann daraus sicher eine Entscheidung ableiten.
```

---
