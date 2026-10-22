#include "oros/ai/actor_simulation_focus_decision.hpp"

#include <span>

namespace oros::ai
{
    foundation::Result<
        ActorSimulationFocusDecision>
    ActorSimulationFocusDecision::evaluate(
        const ActorSimulationFocusPolicy& policy,
        const ActorSimulationFidelityState&
            actor_state,
        const world::WorldPosition&
            actor_position,
        const ActorSimulationFocusSourceRegistry&
            focus_sources)
    {
        if (!actor_state.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Actor simulation focus decision "
                "requires a valid actor fidelity "
                "state.");
        }

        ActorSimulationFidelity
            focus_fidelity =
                ActorSimulationFidelity::
                    statistical_distant;

        const std::span<
            const ActorSimulationFocusSource>
            sources =
                focus_sources.
                    sources_in_entity_order();

        if (sources.empty())
        {
            const foundation::Result<
                ActorSimulationFidelity>
                evaluation =
                    policy.evaluate(
                        actor_state.fidelity(),
                        actor_position,
                        std::span<
                            const world::WorldPosition>{});

            if (!evaluation.has_value())
            {
                return foundation::fail(
                    evaluation.error().code,
                    evaluation.error().message);
            }

            focus_fidelity =
                evaluation.value();
        }
        else
        {
            for (
                const ActorSimulationFocusSource&
                    source :
                sources)
            {
                const std::span<
                    const world::WorldPosition>
                    single_focus{
                        &source.position(),
                        1U
                    };

                const foundation::Result<
                    ActorSimulationFidelity>
                    evaluation =
                        policy.evaluate(
                            actor_state.fidelity(),
                            actor_position,
                            single_focus);

                if (!evaluation.has_value())
                {
                    return foundation::fail(
                        evaluation.error().code,
                        evaluation.error().message);
                }

                if (
                    evaluation.value() ==
                    ActorSimulationFidelity::
                        deep_local)
                {
                    focus_fidelity =
                        ActorSimulationFidelity::
                            deep_local;

                    break;
                }
            }
        }

        const ActorSimulationFidelityTransition
            transition =
                classify_actor_simulation_fidelity_transition(
                    actor_state.fidelity(),
                    focus_fidelity);

        if (
            transition ==
            ActorSimulationFidelityTransition::
                invalid)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "Actor simulation focus decision "
                "produced an invalid fidelity "
                "transition.");
        }

        return ActorSimulationFocusDecision{
            actor_state.actor(),
            actor_state.fidelity(),
            focus_fidelity,
            transition
        };
    }

    world::EntityId
    ActorSimulationFocusDecision::actor()
        const noexcept
    {
        return actor_;
    }

    ActorSimulationFidelity
    ActorSimulationFocusDecision::
        current_fidelity()
        const noexcept
    {
        return current_fidelity_;
    }

    ActorSimulationFidelity
    ActorSimulationFocusDecision::
        focus_fidelity()
        const noexcept
    {
        return focus_fidelity_;
    }

    ActorSimulationFidelityTransition
    ActorSimulationFocusDecision::transition()
        const noexcept
    {
        return transition_;
    }

    ActorSimulationFocusDecision::
        ActorSimulationFocusDecision(
            const world::EntityId actor,
            const ActorSimulationFidelity
                current_fidelity,
            const ActorSimulationFidelity
                focus_fidelity,
            const ActorSimulationFidelityTransition
                transition)
            noexcept
        : actor_{actor},
          current_fidelity_{current_fidelity},
          focus_fidelity_{focus_fidelity},
          transition_{transition}
    {
    }
}