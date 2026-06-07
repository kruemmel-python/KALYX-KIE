#!/usr/bin/env python3
"""Small host-side SDK for KALYX KIE/KRESP integration.

The SDK does not call a model and does not execute commands. It wraps validated
KRESP metadata into a host decision so an app/game/tool can keep execution under
its own deterministic policy boundary.
"""

from __future__ import annotations

from dataclasses import dataclass
from enum import StrEnum


class HostDecision(StrEnum):
    REJECT = "reject"
    ACCEPT_ANSWER = "accept_answer"
    QUEUE_CONFIRMATION = "queue_confirmation"
    DISPATCH_COMMAND = "dispatch_command"


@dataclass(frozen=True)
class ValidationView:
    accepted: bool
    response_type: str
    requires_confirmation: bool
    command_name: str = ""
    error: str = ""


@dataclass(frozen=True)
class HostPlan:
    decision: HostDecision
    command_name: str
    reason: str


def plan_from_validation(validation: ValidationView) -> HostPlan:
    if not validation.accepted:
        return HostPlan(HostDecision.REJECT, "", validation.error or "validation rejected response")
    if validation.requires_confirmation:
        return HostPlan(HostDecision.QUEUE_CONFIRMATION, validation.command_name, "host confirmation required before dispatch")
    if validation.response_type == "command":
        return HostPlan(HostDecision.DISPATCH_COMMAND, validation.command_name, "validated command may be dispatched by host")
    return HostPlan(HostDecision.ACCEPT_ANSWER, "", "validated non-command response")
