$ErrorActionPreference = "Stop"
New-Item -ItemType Directory -Force -Path .\out | Out-Null
$goal = Get-Content .\examples\prompt_author_goal.txt -Raw
.\build_vs\Release\kalyx_prompt_author.exe --domain app --intent summarize_and_export --goal $goal --pack .\out\readme_export.kpromptpack
.\build_vs\Release\kalyx_make_envelope.exe --prompt-pack .\out\readme_export.kpromptpack --domain app --intent summarize_and_export --out .\out\promptpack_app.kie.md
Get-ChildItem .\out\readme_export.kpromptpack -Recurse
Get-Content .\out\readme_export.kpromptpack\contract.kcontract.json -Raw
