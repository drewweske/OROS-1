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

        // Atomically replace memory from already-canonical,
        // strictly increasing (observer,target) facts. Validates the
        // entire input before allocating or mutating live memory.
        // This is a bulk import, not a merge: callers must stage
        // complete replacement state. Borrowed spans are invalidated
        // on success.
        [[nodiscard]]
        foundation::Status
        replace_canonical(
            std::span<const ActorVisualObservation> observations);

        [[nodiscard]]
        foundation::Status
        forget(
            world::EntityId observer,
            world::EntityId target);

        // Despawn cleanup: remove every observation mentioning this
        // identity, whether it is the observer or remembered target.
        // Idempotent (0 for missing/invalid ID). Preserves canonical
        // order and never allocates. Invalidates borrowed spans.
        [[nodiscard]]
        std::size_t forget_entity(
            world::EntityId entity) noexcept;

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