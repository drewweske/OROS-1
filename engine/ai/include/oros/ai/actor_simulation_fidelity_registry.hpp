#pragma once

#include "oros/ai/actor_simulation_fidelity_state.hpp"
#include "oros/foundation/result.hpp"
#include "oros/world/entity_id.hpp"

#include <cstddef>
#include <span>
#include <type_traits>
#include <vector>

namespace oros::ai
{
    static_assert(
        std::is_nothrow_move_constructible_v<
            ActorSimulationFidelityState>);

    static_assert(
        std::is_nothrow_move_assignable_v<
            ActorSimulationFidelityState>);

    class ActorSimulationFidelityRegistry final
    {
    public:
        ActorSimulationFidelityRegistry() = default;

        ActorSimulationFidelityRegistry(
            const ActorSimulationFidelityRegistry&) =
                default;

        ActorSimulationFidelityRegistry&
        operator=(
            const ActorSimulationFidelityRegistry&) =
                default;

        ActorSimulationFidelityRegistry(
            ActorSimulationFidelityRegistry&&)
            noexcept = default;

        ActorSimulationFidelityRegistry&
        operator=(
            ActorSimulationFidelityRegistry&&)
            noexcept = default;

        [[nodiscard]]
        foundation::Status
        insert(
            world::EntityId actor,
            ActorSimulationFidelity fidelity);

        [[nodiscard]]
        foundation::Status
        remove(
            world::EntityId actor);

        [[nodiscard]]
        foundation::Result<
            ActorSimulationFidelityTransition>
        transition(
            world::EntityId actor,
            ActorSimulationFidelity fidelity);

        [[nodiscard]]
        foundation::Result<
            ActorSimulationFidelity>
        fidelity(
            world::EntityId actor) const;

        [[nodiscard]]
        bool contains(
            world::EntityId actor) const noexcept;

        [[nodiscard]]
        std::size_t size() const noexcept;

        [[nodiscard]]
        bool empty() const noexcept;

        [[nodiscard]]
        std::span<
            const ActorSimulationFidelityState>
        states_in_entity_order() const noexcept;

    private:
        [[nodiscard]]
        std::size_t lower_bound_index(
            world::EntityId actor) const noexcept;

        std::vector<
            ActorSimulationFidelityState>
            states_{};
    };
}