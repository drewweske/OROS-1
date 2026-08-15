#pragma once

#include "oros/ai/actor_visual_observation.hpp"
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
            ActorVisualObservation>);

    static_assert(
        std::is_nothrow_move_assignable_v<
            ActorVisualObservation>);

    class ActorVisualObservationMemory final
    {
    public:
        ActorVisualObservationMemory() =
            default;

        ActorVisualObservationMemory(
            const ActorVisualObservationMemory&) =
                default;

        ActorVisualObservationMemory&
        operator=(
            const ActorVisualObservationMemory&) =
                default;

        ActorVisualObservationMemory(
            ActorVisualObservationMemory&&)
            noexcept = default;

        ActorVisualObservationMemory&
        operator=(
            ActorVisualObservationMemory&&)
            noexcept = default;

        [[nodiscard]]
        foundation::Status
        remember(
            const ActorVisualObservation&
                observation);

        [[nodiscard]]
        foundation::Status
        forget(
            world::EntityId observer,
            world::EntityId target);

        [[nodiscard]]
        bool contains(
            world::EntityId observer,
            world::EntityId target)
            const noexcept;

        [[nodiscard]]
        foundation::Result<
            ActorVisualObservation>
        find(
            world::EntityId observer,
            world::EntityId target) const;

        [[nodiscard]]
        foundation::Result<
            std::span<
                const ActorVisualObservation>>
        observations_for_observer(
            world::EntityId observer) const;

        [[nodiscard]]
        std::size_t
        size() const noexcept;

        [[nodiscard]]
        bool
        empty() const noexcept;

        [[nodiscard]]
        std::span<
            const ActorVisualObservation>
        observations_in_canonical_order()
            const noexcept;

    private:
        [[nodiscard]]
        std::size_t
        lower_bound_index(
            world::EntityId observer,
            world::EntityId target)
            const noexcept;

        [[nodiscard]]
        std::size_t
        lower_bound_observer_index(
            world::EntityId observer)
            const noexcept;

        [[nodiscard]]
        std::size_t
        upper_bound_observer_index(
            world::EntityId observer)
            const noexcept;

        std::vector<
            ActorVisualObservation>
            observations_{};
    };
}