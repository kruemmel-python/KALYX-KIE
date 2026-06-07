# KALYX-KIE v0.6 — Document Input Adapter + Safe Section Parser

## Zweck

v0.6 behebt den harten Befund aus dem README-Test: Ein eingebettetes Markdown-Dokument kann selbst Überschriften wie `## Allowed Actions` oder JSON-Codeblöcke enthalten. Der alte Envelope-Aufbau und der einfache Section-Extractor konnten dadurch Kontrollblöcke mit Nutzdaten verwechseln.

v0.6 führt deshalb zwei Substrat-Fixes ein:

1. Direkter Dokumentimport über `--document`.
2. Härtung des Markdown-Section-Parsers gegen Überschriften und Fences innerhalb eingebetteter Nutzdaten.

## Neue CLI

```powershell
.\build_vs\Release\kalyx_make_envelope.exe --domain app --intent summarize_document --document .\README.md --document-type markdown --request .\out\readme_request.txt --out .\out\readme.kie.md
```

Optional:

```powershell
--document-name README.md
```

## Weiterhin unterstützte alte CLI

```powershell
.\build_vs\Release\kalyx_make_envelope.exe --domain app --intent summarize_document --state .\out\readme_state.json --request .\out\readme_request.txt --out .\out\readme.kie.md
```

## Architekturänderungen

### 1. Document Input Adapter

`kalyx_make_envelope.exe` akzeptiert jetzt entweder:

- `--state FILE` für vorhandene JSON-State-Dateien
- oder `--document FILE` für direkte Dokumente

Beides zusammen ist absichtlich verboten, damit die Datenquelle eindeutig bleibt.

Der Adapter kapselt ein Dokument intern als JSON-State:

```json
{
  "app": "KALYX-KIE",
  "input_adapter": "document",
  "document_name": "README.md",
  "document_path": "README.md",
  "document_type": "markdown",
  "document_encoding": "utf-8-or-binary-preserved-text",
  "authoritative_document": "..."
}
```

### 2. Kontrollblöcke vor Nutzdaten

Der Envelope schreibt nun zuerst:

1. User Request
2. Allowed Actions
3. Forbidden Actions
4. Validation Rules
5. Required Response
6. Runtime State
7. Authoritative Data

Dadurch liegen die KIE-Kontrollblöcke deterministisch vor potentiell kollidierendem Dokumentinhalt.

### 3. Safe Section Parser

`kalyx_extract_markdown_json_block()` sucht Abschnittsüberschriften jetzt nur noch an echten Zeilenanfängen und ignoriert Treffer innerhalb von Markdown-Codefences. Dadurch werden eingebettete README-Inhalte nicht mehr mit KIE-Kontrollblöcken verwechselt.

## Neue Tests

- `kalyx_safe_section_parser`
- `kalyx_document_input_adapter`

## Validierung

Linux-Validierung in dieser Build-Umgebung:

```text
100% tests passed, 0 tests failed out of 105
```

Zusätzlich wurde der reale README-Pfad getestet:

```powershell
.\build_vs\Release\kalyx_make_envelope.exe --domain app --intent summarize_document --document .\README.md --document-type markdown --request .\out\readme_request.txt --out .\out\readme_direct.kie.md
.\build_vs\Release\kalyx_validate_response.exe --envelope .\out\readme_direct.kie.md --response .\out\readme.kresp.md --audit .\out\readme.kaudit.json
```

Ergebnis:

```text
response accepted: type=answer risk=none confirmation=false
```

## Windows-Build

```powershell
$CMAKE_EXE = "C:\Program Files\CMake\bin\cmake.exe"; $CTEST_EXE = "C:\Program Files\CMake\bin\ctest.exe"; & $CMAKE_EXE -S . -B build_vs -G "Visual Studio 17 2022" -A x64; & $CMAKE_EXE --build build_vs --config Release --parallel; & $CTEST_EXE --test-dir build_vs -C Release --output-on-failure
```
