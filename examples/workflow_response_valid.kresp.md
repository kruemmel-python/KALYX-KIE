# KALYX LLM Response

## Human Response

The ticket is missing an approver, so the workflow should ask for clarification before routing.

## Machine Result

```json
{
  "schema": "KRESP01",
  "type": "clarification",
  "human_summary": "Ask for the missing approver before routing WF-1024.",
  "risk": "none",
  "requires_confirmation": false,
  "uses_only_provided_context": true,
  "machine_result": {
    "question": "Who is the required approver for WF-1024?"
  }
}
```
