#include "oros/ai/actor_simulation_focus_decision_set_application.hpp"

#include "oros/ai/actor_simulation_focus_decision_application.hpp"

#include <cstddef>
#include <span>

namespace oros::ai
{
    namespace
    {
        [[nodiscard]]
        bool
        is_internally_consistent_focus_decision(
            const ActorSimulationFocusDecision& decision)
            noexcept
        {
            const ActorSimulationFidelityTransition
                classified_transition =
                    classify_actor_simulation_fidelity_transition(
                        decision.current_fidelity(),
                        decision.focus_fidelity());

            return
                decision.actor().is_valid() &&
                is_valid_actor_simulation_fidelity(
                    decision.current_fidelity()) &&
                is_valid_actor_simulation_fidelity(
                    decision.focus_fidelity()) &&
                classified_transition !=
                    ActorSimulationFidelityTransition::
                        invalid &&
                classified_transition ==
                    decision.transition();
        }
    }

    foundation::Status
    apply_actor_simulation_focus_decision_set_if_fidelities_match(
        ActorSimulationFidelityRegistry& registry,
        const ActorSimulationFocusDecisionSet& decision_set)
    {
        const std::span<
            const ActorSimulationFidelityState>
            states =
                registry.states_in_entity_order();

        const std::span<
            const ActorSimulationFocusDecision>
            decisions =
                decision_set.decisions_in_entity_order();

        if (states.size() != decisions.size())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Actor simulation focus decision "
                "set does not match the current "
                "fidelity actor universe.");
        }

        for (
            std::size_t index = 0U;
            index < decisions.size();
            ++index)
        {
            const ActorSimulationFidelityState&
                state =
                    states[index];

            const ActorSimulationFocusDecision&
                decision =
                    decisions[index];

            if (!state.is_valid())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "Actor simulation fidelity "
                    "registry contains an invalid "
                    "authoritative state.");
            }

            if (
                !is_internally_consistent_focus_decision(
                    decision))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_state,
                    "Actor simulation focus decision "
                    "set contains an internally "
                    "inconsistent decision.");
            }

            if (state.actor() != decision.actor())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_state,
                    "Actor simulation focus decision "
                    "set no longer matches the "
                    "fidelity registry actor order.");
            }

            if (
                state.fidelity() !=
                    decision.current_fidelity())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_state,
                    "Actor simulation focus decision "
                    "set contains a stale actor "
                    "fidelity.");
            }
        }

        for (
            const ActorSimulationFocusDecision&
                decision :
            decisions)
        {
            foundation::Result<
                ActorSimulationFidelityTransition>
                application =
                    apply_actor_simulation_focus_decision_if_fidelity_matches(
                        registry,
                        decision);

            if (!application.has_value())
            {
                return foundation::fail(
                    application.error().code,
                    application.error().message);
            }
        }

        return {};
    }
}