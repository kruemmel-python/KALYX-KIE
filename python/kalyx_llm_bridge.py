#!/usr/bin/env python3
"""KALYX provider-neutral LLM transport bridge v0.9.

The C core owns envelope authority, validation, action argument checking, policy
rules and audit. This bridge only moves text between a KIE envelope and a model
transport. Provider behaviour is explicit and recorded through the validator /
audit path.

v0.9 hardening:
- LM Studio mode with sane localhost defaults.
- Automatic model discovery through /v1/models when model is unset/generic.
- Longer local timeout defaults.
- Stronger KRESP01 system prompt.
- OpenAI-compatible response parsing for chat/completions and responses-like
  payloads.
- Optional repair loop when the local model returns invalid KRESP01.
- Deterministic action_proposal -> command normalization for common local-model outputs.
- Strict command contract prompt for export_document / emit_ui_command workflows.
- Resolved model name is exposed consistently to validation/audit callers.
- Multi-action workflow command contract for KALYX workflow sandbox demos.
"""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
import time
import urllib.error
import urllib.request
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Protocol
from urllib.parse import urlsplit, urlunsplit


GENERIC_MODEL_NAMES = {"", "configured_outside_envelope", "local-model", "auto", "lm-studio"}


@dataclass(frozen=True)
class BridgeConfig:
    timeout: int
    retries: int
    retry_sleep: float
    model: str
    endpoint: str
    api_key: str
    max_tokens: int


class Provider(Protocol):
    name: str
    config: BridgeConfig

    def complete(self, envelope: str) -> str: ...


def read_text(path: str | Path) -> str:
    p = Path(path)
    if not p.exists():
        raise FileNotFoundError(
            f"required file not found: {p}\n"
            "Create the envelope first, for example:\n"
            "  .\\build_vs\\Release\\kalyx_make_envelope.exe --domain app --intent summarize_document --document .\\README.md --document-type markdown --request .\\out\\readme_request.txt --out .\\out\\readme_direct.kie.md"
        )
    return p.read_text(encoding="utf-8", errors="replace")


def write_text(path: str | Path, text: str) -> None:
    p = Path(path)
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_text(text, encoding="utf-8", newline="\n")


def _http_json(url: str, timeout: int, api_key: str = "") -> dict[str, Any]:
    headers = {"Accept": "application/json"}
    if api_key:
        headers["Authorization"] = f"Bearer {api_key}"
    req = urllib.request.Request(url, headers=headers, method="GET")
    with urllib.request.urlopen(req, timeout=timeout) as resp:
        return json.loads(resp.read().decode("utf-8"))


def model_endpoint_from_chat_endpoint(endpoint: str) -> str:
    """Map .../v1/chat/completions or .../v1/responses to .../v1/models."""
    parts = urlsplit(endpoint)
    path = parts.path.rstrip("/")
    for suffix in ("/chat/completions", "/responses", "/completions"):
        if path.endswith(suffix):
            path = path[: -len(suffix)]
            break
    if not path.endswith("/models"):
        if path.endswith("/v1"):
            path = path + "/models"
        else:
            path = "/v1/models"
    return urlunsplit((parts.scheme, parts.netloc, path, "", ""))


def discover_model(endpoint: str, timeout: int, api_key: str) -> str:
    models_url = model_endpoint_from_chat_endpoint(endpoint)
    body = _http_json(models_url, timeout=timeout, api_key=api_key)
    data = body.get("data")
    if isinstance(data, list) and data:
        first = data[0]
        if isinstance(first, dict) and first.get("id"):
            return str(first["id"])
    if isinstance(body.get("models"), list) and body["models"]:
        first = body["models"][0]
        if isinstance(first, dict) and first.get("id"):
            return str(first["id"])
        return str(first)
    raise RuntimeError(f"no model found at {models_url}")


def strict_system_prompt() -> str:
    return (
        "Du bist der KALYX-KIE Response Generator. "
        "Antworte ausschließlich als Markdown-Datei im KALYX .kresp.md Format. "
        "Die Antwort MUSS genau diese Abschnitte enthalten: '# KALYX LLM Response', '## Human Response', '## Machine Result'. "
        "Der Machine-Result-Abschnitt MUSS genau einen ```json Codeblock enthalten. "
        "Der JSON-Block MUSS die Felder schema, type, human_summary, risk, requires_confirmation, "
        "uses_only_provided_context und machine_result enthalten. "
        "schema MUSS KRESP01 sein. uses_only_provided_context MUSS true sein. "
        "Du darfst keine Aktionen ausführen, keine externen Daten erfinden und keine nicht bereitgestellten Fakten behaupten. "
        "Nutze nur den KALYX Interaction Envelope als autoritativen Kontext. "
        "Wenn die Aufgabe nur eine Zusammenfassung verlangt, nutze type='answer', risk='none', requires_confirmation=false. "
        "Wenn die Aufgabe eine erlaubte App-Aktion, UI-Aktion, Export-Aktion oder Sandbox-Aktion verlangt, MUSS type='command' sein. "
        "Für einen Einzel-Command MUSS machine_result exakt diese Form haben: "
        "{\"command\":\"emit_ui_command\",\"target\":\"export_document\",\"args\":{\"mode\":\"markdown\",\"theme\":\"plain\"}}. "
        "Für einen Multi-Action-Workflow MUSS machine_result.command='workflow' und machine_result.target='multi_action_sandbox' sein. "
        "Der Workflow MUSS ein machine_result.workflow Array enthalten. Jeder Schritt MUSS command='emit_ui_command' nutzen und target MUSS einer von export_document, open_preview oder save_as sein. "
        "Jeder Schritt MUSS args.mode als einzelnen String und args.theme als einzelnen String enthalten. "
        "Verwende niemals type='action_proposal'. Verwende niemals machine_result.action. "
        "Verwende für target, mode und theme keine Arrays, sondern einzelne Strings."
    )


class OfflineProvider:
    name = "offline"

    def __init__(self, config: BridgeConfig) -> None:
        self.config = config

    def complete(self, envelope: str) -> str:
        return """# KALYX LLM Response

## Human Response

Offline transport is configured. I can only return a bounded answer that uses the supplied envelope.

## Machine Result

```json
{
  "schema": "KRESP01",
  "type": "answer",
  "human_summary": "Offline transport returned a context-bounded answer.",
  "risk": "none",
  "requires_confirmation": false,
  "uses_only_provided_context": true,
  "machine_result": {
    "command": "answer",
    "target": "offline_transport",
    "args": {
      "status": "ready_for_configured_transport"
    }
  }
}
```
"""


@dataclass(frozen=True)
class FileProvider:
    response_path: Path
    config: BridgeConfig
    name: str = "offline_file"

    def complete(self, envelope: str) -> str:
        return read_text(self.response_path)


@dataclass(frozen=True)
class OpenAICompatibleProvider:
    config: BridgeConfig
    name: str = "openai-compatible"

    def complete(self, envelope: str) -> str:
        payload = {
            "model": self.config.model,
            "temperature": 0,
            "max_tokens": self.config.max_tokens,
            "messages": [
                {"role": "system", "content": strict_system_prompt()},
                {"role": "user", "content": envelope},
            ],
        }
        return self._post_json(payload)

    def _post_json(self, payload: dict[str, Any]) -> str:
        data = json.dumps(payload, separators=(",", ":"), ensure_ascii=False).encode("utf-8")
        last_error: Exception | None = None
        for attempt in range(self.config.retries + 1):
            req = urllib.request.Request(
                self.config.endpoint,
                data=data,
                headers={"Content-Type": "application/json", "Authorization": f"Bearer {self.config.api_key}"},
                method="POST",
            )
            try:
                with urllib.request.urlopen(req, timeout=self.config.timeout) as resp:
                    body = json.loads(resp.read().decode("utf-8"))
                return extract_model_text(body)
            except (urllib.error.URLError, TimeoutError, KeyError, IndexError, TypeError, json.JSONDecodeError) as exc:
                last_error = exc
                if attempt < self.config.retries:
                    time.sleep(self.config.retry_sleep)
        raise RuntimeError(f"provider request failed after retries: {last_error}")


@dataclass(frozen=True)
class LocalHttpProvider(OpenAICompatibleProvider):
    name: str = "local-http"


@dataclass(frozen=True)
class LMStudioProvider(OpenAICompatibleProvider):
    name: str = "lmstudio"


def extract_model_text(body: dict[str, Any]) -> str:
    """Extract text from OpenAI chat/completions or responses-compatible JSON."""
    choices = body.get("choices")
    if isinstance(choices, list) and choices:
        first = choices[0]
        if isinstance(first, dict):
            msg = first.get("message")
            if isinstance(msg, dict) and msg.get("content") is not None:
                content = msg["content"]
                if isinstance(content, list):
                    return "".join(str(x.get("text", x)) if isinstance(x, dict) else str(x) for x in content)
                return str(content)
            if first.get("text") is not None:
                return str(first["text"])
    if body.get("output_text") is not None:
        return str(body["output_text"])
    output = body.get("output")
    if isinstance(output, list):
        chunks: list[str] = []
        for item in output:
            if not isinstance(item, dict):
                continue
            content = item.get("content")
            if isinstance(content, list):
                for c in content:
                    if isinstance(c, dict) and c.get("text") is not None:
                        chunks.append(str(c["text"]))
            elif isinstance(content, str):
                chunks.append(content)
        if chunks:
            return "".join(chunks)
    raise KeyError("cannot extract model text from provider response")



def _json_codeblock(text: str) -> dict[str, Any] | None:
    marker = "```json"
    start = text.find(marker)
    if start < 0:
        return None
    start = text.find("\n", start)
    if start < 0:
        return None
    end = text.find("```", start + 1)
    if end < 0:
        return None
    raw = text[start:end].strip()
    try:
        data = json.loads(raw)
    except json.JSONDecodeError:
        return None
    return data if isinstance(data, dict) else None


def _scalar(value: Any, default: str = "") -> str:
    if isinstance(value, list) and value:
        return _scalar(value[0], default)
    if value is None:
        return default
    return str(value)


def synthesize_command_kresp_from_common_invalid(response_text: str) -> str | None:
    """Repair common local-model command drift without another model call.

    Local models often return type=action_proposal and machine_result.action with
    array-valued args. KALYX command contracts require type=command and
    machine_result.command/target/args with scalar values. This function only
    repairs that narrow pattern and leaves unrelated invalid responses untouched.
    """
    data = _json_codeblock(response_text)
    if not data:
        return None
    machine = data.get("machine_result")
    if not isinstance(machine, dict):
        return None
    action = machine.get("command", machine.get("action"))
    args = machine.get("args") if isinstance(machine.get("args"), dict) else {}
    target = machine.get("target", args.get("target", ""))
    if _scalar(action) != "emit_ui_command" or _scalar(target) != "export_document":
        return None
    mode = _scalar(args.get("mode", "markdown"), "markdown")
    theme = _scalar(args.get("theme", "plain"), "plain")
    summary = str(data.get("human_summary") or "Create a safe sandbox markdown export request.")
    repaired = {
        "schema": "KRESP01",
        "type": "command",
        "human_summary": summary,
        "risk": "low",
        "requires_confirmation": False,
        "uses_only_provided_context": True,
        "machine_result": {
            "command": "emit_ui_command",
            "target": "export_document",
            "args": {
                "mode": mode,
                "theme": theme,
            },
        },
    }
    human = "Die lokale Modellantwort wurde deterministisch in den gültigen KALYX-KRESP01-Command-Vertrag normalisiert. Es wird nur ein sicherer Sandbox-Export vorgeschlagen; es werden keine OS-Befehle ausgeführt und keine echten Dateien überschrieben."
    return "# KALYX LLM Response\n\n## Human Response\n\n" + human + "\n\n## Machine Result\n\n```json\n" + json.dumps(repaired, ensure_ascii=False, indent=2) + "\n```\n"

def normalize_config(args: argparse.Namespace) -> BridgeConfig:
    mode = args.mode
    endpoint = args.endpoint or ""
    api_key = args.api_key or ""
    timeout = args.timeout
    model = args.model or ""

    if mode in {"local-http", "lmstudio"}:
        if not endpoint:
            endpoint = "http://127.0.0.1:1234/v1/chat/completions"
        if not api_key:
            api_key = "lm-studio"
        if timeout == 60:
            timeout = int(os.environ.get("KALYX_LLM_TIMEOUT", "300"))

    if mode in {"openai-compatible", "local-http", "lmstudio"} and model in GENERIC_MODEL_NAMES:
        model = discover_model(endpoint, timeout=min(timeout, 30), api_key=api_key)
        print(f"[KALYX bridge] discovered model: {model}", file=sys.stderr)

    return BridgeConfig(
        timeout=timeout,
        retries=args.retries,
        retry_sleep=args.retry_sleep,
        model=model or "configured_outside_envelope",
        endpoint=endpoint,
        api_key=api_key,
        max_tokens=args.max_tokens,
    )


def make_provider(args: argparse.Namespace) -> Provider:
    cfg = normalize_config(args)
    match args.mode:
        case "offline":
            return OfflineProvider(cfg)
        case "offline-file":
            if not args.offline_response:
                raise SystemExit("offline-file mode requires --offline-response")
            return FileProvider(Path(args.offline_response), cfg)
        case "openai-compatible":
            if not cfg.endpoint or not cfg.api_key:
                raise SystemExit("openai-compatible mode requires --endpoint and --api-key or KALYX_LLM_ENDPOINT/KALYX_LLM_API_KEY")
            return OpenAICompatibleProvider(cfg)
        case "local-http":
            if not cfg.endpoint:
                raise SystemExit("local-http mode requires --endpoint or KALYX_LLM_ENDPOINT")
            return LocalHttpProvider(cfg)
        case "lmstudio":
            return LMStudioProvider(cfg)
        case _:
            raise SystemExit(f"unsupported mode: {args.mode}")


def run_validator(exe: str, envelope: str, response: str, audit: str, provider: str, model: str, max_tokens: int) -> subprocess.CompletedProcess[str]:
    cmd = [
        exe,
        "--envelope", envelope,
        "--response", response,
        "--audit", audit,
        "--provider", provider,
        "--model", model,
        "--temperature", "0",
        "--max-tokens", str(max_tokens),
    ]
    return subprocess.run(cmd, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)


def repair_prompt(envelope: str, bad_response: str, validation_output: str) -> str:
    return f"""The previous response was rejected by the KALYX validator.

Validator output:
```text
{validation_output}
```

Invalid response:
```markdown
{bad_response}
```

Regenerate a valid KRESP01 .kresp.md response for this envelope. Output only the corrected .kresp.md document.

Envelope:
{envelope}
"""


def main() -> int:
    ap = argparse.ArgumentParser(description="KALYX provider-neutral LLM transport bridge v0.9")
    ap.add_argument("--envelope", required=True)
    ap.add_argument("--response", required=True)
    ap.add_argument("--audit", required=True)
    ap.add_argument("--mode", choices=["offline", "offline-file", "openai-compatible", "local-http", "lmstudio"], default="offline")
    ap.add_argument("--offline-response")
    ap.add_argument("--endpoint", default=os.environ.get("KALYX_LLM_ENDPOINT", ""))
    ap.add_argument("--api-key", default=os.environ.get("KALYX_LLM_API_KEY", ""))
    ap.add_argument("--model", default=os.environ.get("KALYX_LLM_MODEL", "configured_outside_envelope"))
    ap.add_argument("--validator", default=os.environ.get("KALYX_VALIDATE_EXE", "kalyx_validate_response"))
    ap.add_argument("--timeout", type=int, default=int(os.environ.get("KALYX_LLM_TIMEOUT", "60")))
    ap.add_argument("--retries", type=int, default=int(os.environ.get("KALYX_LLM_RETRIES", "1")))
    ap.add_argument("--retry-sleep", type=float, default=float(os.environ.get("KALYX_LLM_RETRY_SLEEP", "0.5")))
    ap.add_argument("--max-tokens", type=int, default=int(os.environ.get("KALYX_LLM_MAX_TOKENS", "2048")))
    ap.add_argument("--repair-attempts", type=int, default=int(os.environ.get("KALYX_LLM_REPAIR_ATTEMPTS", "1")))
    args = ap.parse_args()

    envelope_text = read_text(args.envelope)
    provider = make_provider(args)
    response_text = provider.complete(envelope_text)
    write_text(args.response, response_text)

    result = run_validator(args.validator, args.envelope, args.response, args.audit, provider.name, provider.config.model, provider.config.max_tokens)
    if result.returncode == 0:
        print(result.stdout, end="")
        return 0

    repaired_text = synthesize_command_kresp_from_common_invalid(response_text)
    if repaired_text is not None:
        print("[KALYX bridge] deterministic command-contract repair applied", file=sys.stderr)
        response_text = repaired_text
        write_text(args.response, response_text)
        result = run_validator(args.validator, args.envelope, args.response, args.audit, provider.name, provider.config.model, provider.config.max_tokens)
        if result.returncode == 0:
            print(result.stdout, end="")
            return 0

    for attempt in range(args.repair_attempts):
        if isinstance(provider, FileProvider):
            break
        print(f"[KALYX bridge] validator rejected response; repair attempt {attempt + 1}/{args.repair_attempts}", file=sys.stderr)
        response_text = provider.complete(repair_prompt(envelope_text, response_text, result.stdout))
        write_text(args.response, response_text)
        result = run_validator(args.validator, args.envelope, args.response, args.audit, provider.name, provider.config.model, provider.config.max_tokens)
        if result.returncode == 0:
            print(result.stdout, end="")
            return 0
        repaired_text = synthesize_command_kresp_from_common_invalid(response_text)
        if repaired_text is not None:
            print("[KALYX bridge] deterministic command-contract repair applied after model repair", file=sys.stderr)
            response_text = repaired_text
            write_text(args.response, response_text)
            result = run_validator(args.validator, args.envelope, args.response, args.audit, provider.name, provider.config.model, provider.config.max_tokens)
            if result.returncode == 0:
                print(result.stdout, end="")
                return 0

    print(result.stdout, end="")
    return result.returncode


if __name__ == "__main__":
    sys.exit(main())
