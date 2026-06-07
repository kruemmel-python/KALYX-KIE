# KALYX-KIE v1.0 Final Testmatrix

| Bereich | Erwartung |
|---|---|
| Build | Visual Studio 2022 / CMake Release Build |
| CTest | 100% Tests grün |
| Version | zentrale KIE-Tools melden `1.0-kie-v1.0-final` |
| Profile | Built-in und Custom KPROFILE01 Profile ladbar |
| Prompt-Pack | `kalyx_prompt_author` erzeugt vollständiges `.kpromptpack` |
| Envelope | `kalyx_make_envelope` erzeugt KIE01 |
| LLM | LM Studio kann KRESP01 erzeugen |
| Validator | KAUDIT01 accepted=true bei gültigem Workflow |
| Dispatch | KDISPATCH01 sandbox_executed |
| Workflow | KWORKFLOW01 status=ok, 3/3 Schritte |
