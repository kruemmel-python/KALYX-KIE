# KALYX-KIE v0.6.1 — Audit Writer Diagnostics

Diese Patch-Version behebt den irreführenden Fehler `cannot write audit`, der auch dann ausgegeben wurde, wenn nicht der Audit-Pfad, sondern Envelope oder Response nicht lesbar waren.

## Änderungen

- `kalyx_validate_response` prüft jetzt vor der Validierung explizit, ob Envelope und Response lesbar sind.
- Fehlende Response-Dateien melden jetzt `cannot read response: ...` statt `cannot write audit`.
- Fehlende Envelope-Dateien melden jetzt `cannot read envelope: ...`.
- Audit-Parent-Verzeichnisse werden automatisch angelegt.
- `kalyx_write_audit_file` erzeugt ebenfalls fehlende Parent-Verzeichnisse für den Audit-Pfad.
- Audit-Schreibfehler zeigen jetzt zusätzlich den KALYX-Status an.

## Wichtig

Wenn nach dem Umstieg auf einen neuen Projektordner `out/readme.kresp.md` fehlt, muss die Response-Datei neu erzeugt oder aus dem alten Ordner kopiert werden.
