# KALYX LLM Response

## Human Response

The next bounded step is to generate a test plan because that action is explicitly allowed.

## Machine Result

```json
{
  "schema": "KRESP01",
  "type": "command",
  "human_summary": "Select the allowed test-planning action.",
  "risk": "medium",
  "requires_confirmation": false,
  "uses_only_provided_context": true,
  "machine_result": {
    "command": "plan_next_action",
    "next_action": "generate_test_plan"
  }
}
```
