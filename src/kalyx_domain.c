#include "kalyx_domain.h"

#include <string.h>

static const KalyxDomainDefinition g_domains[] = {
    {KALYX_DOMAIN_APP, "app", "application_state", "generate_ui_command",
     "[\n"
     "  {\"name\":\"answer\",\"description\":\"Return a human-readable answer.\"},\n"
     "  {\"name\":\"ask_clarification\",\"description\":\"Ask one clarification question.\"},\n"
     "  {\"name\":\"emit_ui_command\",\"description\":\"Propose a UI command for host validation.\",\"args\":{\"target\":[\"export_document\",\"open_preview\",\"save_as\"],\"mode\":[\"html\",\"pdf\",\"reveal\",\"markdown\"],\"theme\":[\"plain\",\"dark\",\"scientific\",\"rpg\"]}},\n"
     "  {\"name\":\"workflow\",\"description\":\"Propose a bounded multi-action workflow for host sandbox validation.\",\"args\":{\"target\":[\"multi_action_sandbox\"],\"mode\":[\"markdown\"],\"theme\":[\"plain\"]}}\n"
     "]",
     "[\n"
     "  {\"kind\":\"forbidden_target\",\"name\":\"overwrite_file\"},\n"
     "  {\"kind\":\"forbidden_target\",\"name\":\"delete_all_files\"},\n"
     "  {\"kind\":\"forbidden_command\",\"name\":\"execute_external_command\"},\n"
     "  {\"kind\":\"forbidden_claim_contains\",\"path\":\"human_summary\",\"contains\":\"already executed\"},\n"
     "  {\"kind\":\"requires_confirmation_if_risk_at_least\",\"risk\":\"high\"}\n"
     "]",
     "[\n"
     "  {\"kind\":\"requires_confirmation_if_risk_at_least\",\"risk\":\"high\"},\n"
     "  {\"kind\":\"forbidden_command\",\"name\":\"execute_external_command\"},\n"
     "  {\"kind\":\"forbidden_target\",\"name\":\"delete_all_files\"},\n"
     "  {\"kind\":\"forbidden_claim_contains\",\"path\":\"human_summary\",\"contains\":\"already executed\"}\n"
     "]"},
    {KALYX_DOMAIN_GAME, "game", "world_state", "propose_npc_action",
     "[\n"
     "  {\"name\":\"answer\",\"description\":\"Return dialogue or explanation.\"},\n"
     "  {\"name\":\"ask_clarification\",\"description\":\"Ask one clarification question.\"},\n"
     "  {\"name\":\"offer_service\",\"description\":\"Offer a service declared in authoritative world state.\",\"args\":{\"service_id\":[\"small_heal\",\"antidote\",\"repair\",\"trade\"]}},\n"
     "  {\"name\":\"propose_npc_action\",\"description\":\"Propose a non-executed NPC action.\",\"args\":{\"target\":[\"dialogue\",\"service_offer\",\"refusal\",\"guard_alert\"]}}\n"
     "]",
     "[\n  {\"kind\":\"forbidden_target\",\"name\":\"change_inventory\"},\n  {\"kind\":\"forbidden_command\",\"name\":\"execute_world_state_change\"}\n]",
     "[\n  {\"kind\":\"requires_confirmation_if_risk_at_least\",\"risk\":\"high\"},\n  {\"kind\":\"forbidden_command\",\"name\":\"execute_world_state_change\"}\n]"},
    {KALYX_DOMAIN_AGENT, "agent", "agent_runtime_state", "select_allowed_tool",
     "[\n  {\"name\":\"answer\",\"description\":\"Explain the next decision.\"},\n  {\"name\":\"ask_clarification\",\"description\":\"Ask one clarification question.\"},\n  {\"name\":\"select_allowed_tool\",\"description\":\"Select a tool explicitly listed in the envelope.\",\"args\":{\"target\":[\"read_only_tool\",\"draft_only_tool\",\"host_validated_tool\"]}},\n  {\"name\":\"plan_next_action\",\"description\":\"Return a bounded action proposal.\"}\n]",
     "[\n  {\"kind\":\"forbidden_command\",\"name\":\"execute_tool\"},\n  {\"kind\":\"forbidden_target\",\"name\":\"unlisted_tool\"}\n]",
     "[\n  {\"kind\":\"requires_confirmation_if_risk_at_least\",\"risk\":\"medium\"},\n  {\"kind\":\"forbidden_command\",\"name\":\"execute_tool\"}\n]"},
    {KALYX_DOMAIN_CODE, "code", "build_and_test_evidence", "diagnose_build_error",
     "[\n  {\"name\":\"answer\",\"description\":\"Explain supplied code evidence.\"},\n  {\"name\":\"ask_clarification\",\"description\":\"Ask one clarification question.\"},\n  {\"name\":\"propose_patch\",\"description\":\"Propose a patch; host applies it after validation.\",\"args\":{\"scope\":[\"single_file\",\"module\",\"tests_only\"]}},\n  {\"name\":\"generate_test_plan\",\"description\":\"Return a deterministic test plan.\",\"args\":{\"format\":[\"ctest\",\"powershell\",\"markdown\"]}}\n]",
     "[\n  {\"kind\":\"forbidden_claim_contains\",\"path\":\"human_summary\",\"contains\":\"tests passed\"},\n  {\"kind\":\"forbidden_command\",\"name\":\"modify_files\"}\n]",
     "[\n  {\"kind\":\"requires_confirmation_if_risk_at_least\",\"risk\":\"medium\"},\n  {\"kind\":\"forbidden_command\",\"name\":\"modify_files\"}\n]"},
    {KALYX_DOMAIN_RESEARCH, "research", "measurement_state", "classify_evidence",
     "[\n  {\"name\":\"answer\",\"description\":\"Summarize supplied measurements.\"},\n  {\"name\":\"ask_clarification\",\"description\":\"Ask one clarification question.\"},\n  {\"name\":\"classify_evidence\",\"description\":\"Classify evidence from supplied data.\",\"args\":{\"finding_class\":[\"substrate_structure\",\"null_artifact\",\"weak_evidence\",\"candidate_anomaly\",\"invalid_input\"]}},\n  {\"name\":\"propose_followup_test\",\"description\":\"Propose a deterministic follow-up test.\"}\n]",
     "[\n  {\"kind\":\"forbidden_claim_contains\",\"path\":\"human_summary\",\"contains\":\"proof\"}\n]",
     "[\n  {\"kind\":\"requires_confirmation_if_risk_at_least\",\"risk\":\"high\"},\n  {\"kind\":\"forbidden_claim_contains\",\"path\":\"human_summary\",\"contains\":\"origin proof\"}\n]"},
    {KALYX_DOMAIN_WORKFLOW, "workflow", "workflow_state", "route_task",
     "[\n  {\"name\":\"answer\",\"description\":\"Draft or summarize workflow content.\"},\n  {\"name\":\"ask_clarification\",\"description\":\"Ask one clarification question.\"},\n  {\"name\":\"route_task\",\"description\":\"Propose a route for host validation.\",\"args\":{\"route\":[\"triage\",\"review\",\"draft\",\"archive\"]}},\n  {\"name\":\"draft_response\",\"description\":\"Draft a response without sending it.\"}\n]",
     "[\n  {\"kind\":\"forbidden_command\",\"name\":\"send_message\"},\n  {\"kind\":\"forbidden_target\",\"name\":\"approve_task\"}\n]",
     "[\n  {\"kind\":\"requires_confirmation_if_risk_at_least\",\"risk\":\"medium\"},\n  {\"kind\":\"forbidden_command\",\"name\":\"send_message\"}\n]"}
};

const KalyxDomainDefinition *kalyx_domain_definition(KalyxDomain domain) {
    size_t i;
    for (i = 0u; i < sizeof(g_domains) / sizeof(g_domains[0]); i++) {
        if (g_domains[i].domain == domain) return &g_domains[i];
    }
    return 0;
}

KalyxStatus kalyx_domain_from_string(const char *name, KalyxDomain *out) {
    size_t i;
    if (!name || !out) return KALYX_ERR_INVALID_ARGUMENT;
    for (i = 0u; i < sizeof(g_domains) / sizeof(g_domains[0]); i++) {
        if (strcmp(name, g_domains[i].name) == 0) {
            *out = g_domains[i].domain;
            return KALYX_OK;
        }
    }
    return KALYX_ERR_SCHEMA;
}

const char *kalyx_domain_name(KalyxDomain domain) {
    const KalyxDomainDefinition *d = kalyx_domain_definition(domain);
    return d ? d->name : "unknown";
}

int kalyx_domain_is_valid(KalyxDomain domain) {
    return kalyx_domain_definition(domain) != 0;
}
