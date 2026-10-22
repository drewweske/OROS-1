#pragma once

#include "oros/ai/actor_simulation_fidelity_state.hpp"
#include "oros/ai/actor_simulation_focus_policy.hpp"
#include "oros/ai/actor_simulation_focus_source_registry.hpp"
#include "oros/foundation/result.hpp"
#include "oros/world/world_position.hpp"

namespace oros::ai
{
    class ActorSimulationFocusDecision final
    {
    public:
        [[nodiscard]]
        static foundation::Result<
            ActorSimulationFocusDecision>
        evaluate(
            const ActorSimulationFocusPolicy& policy,
            const ActorSimulationFidelityState&
                actor_state,
            const world::WorldPosition&
                actor_position,
            const ActorSimulationFocusSourceRegistry&
                focus_sources);

        [[nodiscard]]
        world::EntityId
        actor() const noexcept;

        [[nodiscard]]
        ActorSimulationFidelity
        current_fidelity() const noexcept;

        [[nodiscard]]
        ActorSimulationFidelity
        focus_fidelity() const noexcept;

        [[nodiscard]]
        ActorSimulationFidelityTransition
        transition() const noexcept;

    private:
        ActorSimulationFocusDecision(
            world::EntityId actor,
            ActorSimulationFidelity current_fidelity,
            ActorSimulationFidelity focus_fidelity,
            ActorSimulationFidelityTransition transition)
            noexcept;

        world::EntityId actor_{};

        ActorSimulationFidelity
            current_fidelity_{
                ActorSimulationFidelity::invalid
            };

        ActorSimulationFidelity
            focus_fidelity_{
                ActorSimulationFidelity::invalid
            };

        ActorSimulationFidelityTransition
            transition_{
                ActorSimulationFidelityTransition::
                    invalid
            };
    };
}