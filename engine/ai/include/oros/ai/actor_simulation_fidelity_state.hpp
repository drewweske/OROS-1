#pragma once

#include "oros/foundation/result.hpp"
#include "oros/world/entity_id.hpp"

#include <cstdint>

namespace oros::ai
{
    enum class ActorSimulationFidelity :
        std::uint8_t
    {
        invalid = 0,
        statistical_distant = 1,
        deep_local = 2
    };

    [[nodiscard]]
    constexpr bool
    is_valid_actor_simulation_fidelity(
        const ActorSimulationFidelity fidelity)
        noexcept
    {
        return
            fidelity ==
                ActorSimulationFidelity::
                    statistical_distant ||
            fidelity ==
                ActorSimulationFidelity::
                    deep_local;
    }

    enum class ActorSimulationFidelityTransition :
        std::uint8_t
    {
        invalid = 0,
        unchanged = 1,
        promotion_to_deep_local = 2,
        demotion_to_statistical_distant = 3
    };

    [[nodiscard]]
    constexpr ActorSimulationFidelityTransition
    classify_actor_simulation_fidelity_transition(
        const ActorSimulationFidelity from,
        const ActorSimulationFidelity to)
        noexcept
    {
        if (!is_valid_actor_simulation_fidelity(from) ||
            !is_valid_actor_simulation_fidelity(to))
        {
            return
                ActorSimulationFidelityTransition::
                    invalid;
        }

        if (from == to)
        {
            return
                ActorSimulationFidelityTransition::
                    unchanged;
        }

        if (
            from ==
                ActorSimulationFidelity::
                    statistical_distant &&
            to ==
                ActorSimulationFidelity::
                    deep_local)
        {
            return
                ActorSimulationFidelityTransition::
                    promotion_to_deep_local;
        }

        if (
            from ==
                ActorSimulationFidelity::
                    deep_local &&
            to ==
                ActorSimulationFidelity::
                    statistical_distant)
        {
            return
                ActorSimulationFidelityTransition::
                    demotion_to_statistical_distant;
        }

        return
            ActorSimulationFidelityTransition::
                invalid;
    }

    class ActorSimulationFidelityState final
    {
    public:
        [[nodiscard]]
        static foundation::Result<
            ActorSimulationFidelityState>
        create(
            world::EntityId actor,
            ActorSimulationFidelity fidelity);

        [[nodiscard]]
        bool is_valid() const noexcept;

        [[nodiscard]]
        world::EntityId
        actor() const noexcept;

        [[nodiscard]]
        ActorSimulationFidelity
        fidelity() const noexcept;

        [[nodiscard]]
        foundation::Result<
            ActorSimulationFidelityTransition>
        transition_to(
            ActorSimulationFidelity fidelity);

    private:
        ActorSimulationFidelityState(
            world::EntityId actor,
            ActorSimulationFidelity fidelity)
            noexcept;

        world::EntityId actor_{};

        ActorSimulationFidelity
            fidelity_{
                ActorSimulationFidelity::invalid
            };
    };
}