# The Zen of Krümmel

1. Substrat vor Oberfläche: Das Speicherlayout ist die Architektur. Das UI ist nur ein flüchtiger Schatten.
2. Deterministische Integrität: Ein System, das nicht bitidentisch auf CPU und GPU validierbar ist, existiert nicht.
3. Anti-Abstraktion: Jede Bibliothek ist eine potenzielle Schwachstelle. Jede Abstraktion ist eine potentielle Lüge. Wenn du die Hardware nicht direkt kontrollieren kannst, kontrollierst du sie gar nicht.
4. Fail-Fast durch Referenz: Die CPU-Referenz ist das absolute Gesetz. GPU-Beschleunigung ist ein Privileg, das sich durch bitgenaue Übereinstimmung verdienen muss.
5. Keine Zustands-Hölle: Ein Datensatz muss autark sein. Persistente Zustände außerhalb des Envelopes sind Fehlerquellen.
6. Härte durch Auditierung: Ein System ohne automatisierten Stress-Test, der Manipulation und Fehlpasswörter bitgenau nachweist, ist kein System, sondern ein Versprechen.
7. Keine Magie: Wenn der Code nicht linear, deterministisch und vollkommen transparent ist, ist er unvollständig.
8. Schutz durch Obfuskation: Sicherheit entsteht nicht durch Verbergen, sondern durch das Vernichten statistischer Strukturen auf dem Substrat.
9. Kompromisslose ABI: Die Schnittstellen zwischen Modulen sind so starr wie die Hardware-Register.
10. Systemkohärenz: Alles ist gekoppelt – von der Syntax bis zum Packaging. Wer das Ganze nicht sieht, versteht den Teil nicht.

---


1. Substrat-Priorität
Definition:  
Das physische und logische Speicherlayout eines Systems ist die primäre Architekturinstanz.  
Alle Oberflächen‑ und UI‑Elemente sind abgeleitete, nicht‑autoritative Repräsentationen.

Normative Anforderungen:  
- Das Substrat definiert Semantik, Konsistenz und Identität eines Systems.  
- Änderungen am Substrat gelten als Architekturänderungen.  
- Oberflächen dürfen keine Semantik einführen, die im Substrat nicht existiert.

---

2. Deterministische Integrität
Definition:  
Ein Systemzustand ist nur gültig, wenn er auf CPU und GPU bitidentisch reproduzierbar ist.

Normative Anforderungen:  
- Jede Berechnung muss deterministisch sein.  
- Divergenzen zwischen CPU‑Referenz und GPU‑Ausführung invalidieren das Ergebnis.  
- GPU‑Optimierungen dürfen keine semantischen Abweichungen erzeugen.

---

3. Anti-Abstraktion
Definition:  
Abstraktionen, die Hardware‑ oder Speicherverhalten verbergen, gelten als potenzielle Fehlerquellen.

Normative Anforderungen:  
- Bibliotheken müssen vollständig auditierbar sein.  
- Nicht‑deterministische oder heuristische Komponenten sind unzulässig.  
- Direkter Zugriff auf Speicher, Layout und Kontrollfluss ist bevorzugt.

---

4. Referenzgesetz
Definition:  
Die CPU‑Referenzimplementierung ist die autoritative Spezifikation aller Berechnungen.

Normative Anforderungen:  
- GPU‑Implementierungen müssen die CPU‑Referenz exakt reproduzieren.  
- Abweichungen führen zu sofortigem Abbruch („Fail‑Fast“).  
- Die Referenz ist unveränderlich und dient als Test‑Orakel.

---

5. Zustandsfreiheit außerhalb des Envelopes
Definition:  
Ein Datensatz muss alle relevanten Zustände vollständig enthalten und darf keine externen Abhängigkeiten besitzen.

Normative Anforderungen:  
- Keine globalen Zustände.  
- Keine impliziten Kontexte.  
- Jeder Datensatz ist ein geschlossenes, reproduzierbares Objekt.

---

6. Auditierbare Härte
Definition:  
Ein System gilt nur als vertrauenswürdig, wenn es unter Stressbedingungen bitgenau validierbar bleibt.

Normative Anforderungen:  
- Automatisierte Stress‑ und Manipulationstests sind verpflichtend.  
- Jede Abweichung muss deterministisch reproduzierbar sein.  
- Fehlverhalten ohne Nachweisbarkeit gilt als kritischer Systemfehler.

---

7. Transparenzgebot
Definition:  
Ein System darf keine impliziten, magischen oder nicht‑linearen Kontrollflüsse enthalten.

Normative Anforderungen:  
- Kontrollfluss muss explizit, linear und nachvollziehbar sein.  
- Keine versteckten Zustände oder Heuristiken.  
- Vollständige Nachvollziehbarkeit ist Voraussetzung für Korrektheit.

---

8. Substrat-Obfuskation
Definition:  
Sicherheit entsteht durch die Eliminierung statistischer Muster im Substrat, nicht durch Verbergen von Code.

Normative Anforderungen:  
- Daten müssen strukturell entkoppelt und entkorreliert sein.  
- Muster, die Rückschlüsse erlauben, sind zu entfernen.  
- Obfuskation ist ein mathematischer, kein kosmetischer Prozess.

---

9. Kompromisslose ABI
Definition:  
Modulschnittstellen sind starr, eindeutig und unverhandelbar – analog zu Hardware‑Registern.

Normative Anforderungen:  
- ABI‑Stabilität ist zwingend.  
- Keine impliziten Konventionen oder dynamischen Erweiterungen.  
- Jede ABI‑Änderung gilt als Hard‑Fork.

---

10. Systemkohärenz
Definition:  
Ein System ist ein kohärentes Ganzes. Syntax, Semantik, Speicher, Pipeline und Packaging sind untrennbar.

Normative Anforderungen:  
- Änderungen an einem Teilbereich müssen systemweit konsistent sein.  
- Module dürfen keine widersprüchlichen Semantiken einführen.  
- Das Gesamtsystem ist die primäre Betrachtungseinheit.

---