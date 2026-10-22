#include "oros/ai/actor_simulation_fidelity_state.hpp"

namespace oros::ai
{
    foundation::Result<ActorSimulationFidelityState>
    ActorSimulationFidelityState::create(
        const world::EntityId actor,
        const ActorSimulationFidelity fidelity)
    {
        if (!actor.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor simulation state requires a "
                "valid persistent World entity "
                "identity.");
        }

        if (!is_valid_actor_simulation_fidelity(
                fidelity))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor simulation state requires a "
                "valid simulation fidelity.");
        }

        return ActorSimulationFidelityState{
            actor,
            fidelity
        };
    }

    bool
    ActorSimulationFidelityState::is_valid()
        const noexcept
    {
        return
            actor_.is_valid() &&
            is_valid_actor_simulation_fidelity(
                fidelity_);
    }

    world::EntityId
    ActorSimulationFidelityState::actor()
        const noexcept
    {
        return actor_;
    }

    ActorSimulationFidelity
    ActorSimulationFidelityState::fidelity()
        const noexcept
    {
        return fidelity_;
    }

    foundation::Result<
        ActorSimulationFidelityTransition>
    ActorSimulationFidelityState::transition_to(
        const ActorSimulationFidelity fidelity)
    {
        if (!is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Actor simulation state is invalid.");
        }

        if (!is_valid_actor_simulation_fidelity(
                fidelity))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor simulation fidelity transition "
                "requires a valid target fidelity.");
        }

        const ActorSimulationFidelityTransition
            transition =
                classify_actor_simulation_fidelity_transition(
                    fidelity_,
                    fidelity);

        if (transition ==
            ActorSimulationFidelityTransition::
                invalid)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "Actor simulation fidelity transition "
                "classification failed.");
        }

        fidelity_ = fidelity;

        return transition;
    }

    ActorSimulationFidelityState::
        ActorSimulationFidelityState(
            const world::EntityId actor,
            const ActorSimulationFidelity fidelity)
            noexcept
        : actor_{actor},
          fidelity_{fidelity}
    {
    }
}