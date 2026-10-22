#pragma once

#include "oros/foundation/result.hpp"
#include "oros/world/entity_id.hpp"
#include "oros/world/world_position.hpp"

#include <cstddef>
#include <span>
#include <type_traits>
#include <vector>

namespace oros::ai
{
    class ActorSimulationPositionSnapshotSet;

    class ActorSimulationPositionSnapshot final
    {
    public:
        ActorSimulationPositionSnapshot(
            const ActorSimulationPositionSnapshot&) =
                default;

        ActorSimulationPositionSnapshot&
        operator=(
            const ActorSimulationPositionSnapshot&) =
                default;

        ActorSimulationPositionSnapshot(
            ActorSimulationPositionSnapshot&&)
            noexcept = default;

        ActorSimulationPositionSnapshot&
        operator=(
            ActorSimulationPositionSnapshot&&)
            noexcept = default;

        [[nodiscard]]
        world::EntityId
        actor() const noexcept;

        [[nodiscard]]
        const world::WorldPosition&
        position() const noexcept;

    private:
        friend class
            ActorSimulationPositionSnapshotSet;

        ActorSimulationPositionSnapshot(
            world::EntityId actor,
            world::WorldPosition position)
            noexcept;

        world::EntityId actor_{};
        world::WorldPosition position_;
    };

    static_assert(
        std::is_nothrow_move_constructible_v<
            ActorSimulationPositionSnapshot>);

    static_assert(
        std::is_nothrow_move_assignable_v<
            ActorSimulationPositionSnapshot>);

    class ActorSimulationPositionSnapshotSet final
    {
    public:
        ActorSimulationPositionSnapshotSet() =
            default;

        ActorSimulationPositionSnapshotSet(
            const ActorSimulationPositionSnapshotSet&) =
                default;

        ActorSimulationPositionSnapshotSet&
        operator=(
            const ActorSimulationPositionSnapshotSet&) =
                default;

        ActorSimulationPositionSnapshotSet(
            ActorSimulationPositionSnapshotSet&&)
            noexcept = default;

        ActorSimulationPositionSnapshotSet&
        operator=(
            ActorSimulationPositionSnapshotSet&&)
            noexcept = default;

        [[nodiscard]]
        foundation::Status
        insert(
            world::EntityId actor,
            world::WorldPosition position);

        [[nodiscard]]
        foundation::Result<
            world::WorldPosition>
        position(
            world::EntityId actor) const;

        [[nodiscard]]
        bool contains(
            world::EntityId actor)
            const noexcept;

        [[nodiscard]]
        std::size_t size() const noexcept;

        [[nodiscard]]
        bool empty() const noexcept;

        [[nodiscard]]
        std::span<
            const ActorSimulationPositionSnapshot>
        snapshots_in_entity_order()
            const noexcept;

    private:
        [[nodiscard]]
        std::size_t lower_bound_index(
            world::EntityId actor)
            const noexcept;

        std::vector<
            ActorSimulationPositionSnapshot>
            snapshots_{};
    };
}