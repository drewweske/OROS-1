#include "oros/ai/actor_simulation_focus_decision_application.hpp"

namespace oros::ai
{
    foundation::Result<
        ActorSimulationFidelityTransition>
    apply_actor_simulation_focus_decision_if_fidelity_matches(
        ActorSimulationFidelityRegistry& registry,
        const ActorSimulationFocusDecision& decision)
    {
        const ActorSimulationFidelityTransition
            classified_transition =
                classify_actor_simulation_fidelity_transition(
                    decision.current_fidelity(),
                    decision.focus_fidelity());

        if (
            !decision.actor().is_valid() ||
            !is_valid_actor_simulation_fidelity(
                decision.current_fidelity()) ||
            !is_valid_actor_simulation_fidelity(
                decision.focus_fidelity()) ||
            classified_transition ==
                ActorSimulationFidelityTransition::
                    invalid ||
            classified_transition !=
                decision.transition())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Actor simulation focus decision "
                "is internally inconsistent.");
        }

        const foundation::Result<
            ActorSimulationFidelity>
            registry_fidelity =
                registry.fidelity(
                    decision.actor());

        if (!registry_fidelity.has_value())
        {
            return foundation::fail(
                registry_fidelity.error().code,
                registry_fidelity.error().message);
        }

        if (
            registry_fidelity.value() !=
                decision.current_fidelity())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Actor simulation focus decision "
                "was computed from a stale actor "
                "fidelity.");
        }

        if (
            decision.transition() ==
            ActorSimulationFidelityTransition::
                unchanged)
        {
            return
                ActorSimulationFidelityTransition::
                    unchanged;
        }

        return registry.transition(
            decision.actor(),
            decision.focus_fidelity());
    }
}