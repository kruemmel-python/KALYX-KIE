# KALYX LLM Response

## Human Response

Mira can offer the small heal service because it is declared in the world state and the player has enough gold.

## Machine Result

```json
{
  "schema": "KRESP01",
  "type": "command",
  "human_summary": "Mira offers the declared small_heal service.",
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
