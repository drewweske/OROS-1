#include "oros/ai/actor_simulation_position_snapshot_set.hpp"

#include <algorithm>
#include <cstddef>
#include <new>
#include <utility>

namespace oros::ai
{
    world::EntityId
    ActorSimulationPositionSnapshot::actor()
        const noexcept
    {
        return actor_;
    }

    const world::WorldPosition&
    ActorSimulationPositionSnapshot::position()
        const noexcept
    {
        return position_;
    }

    ActorSimulationPositionSnapshot::
        ActorSimulationPositionSnapshot(
            const world::EntityId actor,
            world::WorldPosition position)
            noexcept
        : actor_{actor},
          position_{std::move(position)}
    {
    }

    foundation::Status
    ActorSimulationPositionSnapshotSet::insert(
        const world::EntityId actor,
        world::WorldPosition position)
    {
        if (!actor.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor simulation position snapshot "
                "requires a valid persistent World "
                "entity identity.");
        }

        if (!position.is_normalized())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor simulation position snapshot "
                "requires a normalized "
                "WorldPosition.");
        }

        const std::size_t index =
            lower_bound_index(actor);

        if (
            index < snapshots_.size() &&
            snapshots_[index].actor() == actor)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Actor simulation position snapshot "
                "set already contains this actor.");
        }

        using difference_type =
            std::vector<
                ActorSimulationPositionSnapshot>::
                    difference_type;

        try
        {
            snapshots_.insert(
                snapshots_.begin() +
                    static_cast<difference_type>(
                        index),
                ActorSimulationPositionSnapshot{
                    actor,
                    std::move(position)
                });
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Actor simulation position snapshot "
                "set could not allocate storage.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "Actor simulation position snapshot "
                "set failed while inserting a "
                "snapshot.");
        }

        return {};
    }

    foundation::Result<
        world::WorldPosition>
    ActorSimulationPositionSnapshotSet::position(
        const world::EntityId actor)
        const
    {
        if (!actor.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor simulation position snapshot "
                "lookup requires a valid persistent "
                "World entity identity.");
        }

        const std::size_t index =
            lower_bound_index(actor);

        if (
            index >= snapshots_.size() ||
            snapshots_[index].actor() != actor)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    not_found,
                "Actor simulation position snapshot "
                "set does not contain this actor.");
        }

        return snapshots_[index].position();
    }

    bool
    ActorSimulationPositionSnapshotSet::contains(
        const world::EntityId actor)
        const noexcept
    {
        if (!actor.is_valid())
        {
            return false;
        }

        const std::size_t index =
            lower_bound_index(actor);

        return
            index < snapshots_.size() &&
            snapshots_[index].actor() == actor;
    }

    std::size_t
    ActorSimulationPositionSnapshotSet::size()
        const noexcept
    {
        return snapshots_.size();
    }

    bool
    ActorSimulationPositionSnapshotSet::empty()
        const noexcept
    {
        return snapshots_.empty();
    }

    std::span<
        const ActorSimulationPositionSnapshot>
    ActorSimulationPositionSnapshotSet::
        snapshots_in_entity_order()
        const noexcept
    {
        return snapshots_;
    }

    std::size_t
    ActorSimulationPositionSnapshotSet::
        lower_bound_index(
            const world::EntityId actor)
        const noexcept
    {
        const auto iterator =
            std::lower_bound(
                snapshots_.begin(),
                snapshots_.end(),
                actor,
                [](
                    const ActorSimulationPositionSnapshot&
                        snapshot,
                    const world::EntityId candidate)
                    noexcept
                {
                    return
                        snapshot.actor() <
                        candidate;
                });

        return static_cast<std::size_t>(
            iterator - snapshots_.begin());
    }
}