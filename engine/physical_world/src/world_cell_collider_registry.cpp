#include "oros/physical_world/world_cell_collider_registry.hpp"

#include "oros/physical_world/world_cell_collider_payload.hpp"

#include <cstddef>
#include <new>
#include <utility>

namespace oros::physical_world
{
    namespace
    {
        [[nodiscard]]
        bool should_have_active_colliders(
            const streaming::
                WorldCellResidencyState state)
            noexcept
        {
            return
                state ==
                    streaming::
                        WorldCellResidencyState::
                            resident ||
                state ==
                    streaming::
                        WorldCellResidencyState::
                            unload_queued;
        }
    }

    bool
    WorldCellColliderRegistry::is_valid()
        const noexcept
    {
        for (std::size_t index = 0U;
             index < active_cells_.size();
             ++index)
        {
            const ActiveCell&
                active_cell =
                    active_cells_[index];

            if (!active_cell.
                    revision.
                    is_valid() ||
                !active_cell.
                    collider_set.
                    is_valid() ||
                active_cell.
                    revision.
                    cell_key !=
                active_cell.
                    collider_set.
                    cell_key())
            {
                return false;
            }

            if (index > 0U)
            {
                const streaming::WorldCellKey&
                    previous_key =
                        active_cells_[
                            index - 1U].
                            revision.
                            cell_key;

                if (!(previous_key <
                      active_cell.
                          revision.
                          cell_key))
                {
                    return false;
                }
            }

            for (const physics::
                     ColliderGeometry&
                     geometry :
                 active_cell.
                     collider_set.
                     colliders())
            {
                const physics::ColliderId
                    collider =
                        geometry.collider();

                for (std::size_t
                         previous_index = 0U;
                     previous_index < index;
                     ++previous_index)
                {
                    if (active_cells_[
                            previous_index].
                            collider_set.
                            contains(
                                collider))
                    {
                        return false;
                    }
                }
            }
        }

        return true;
    }

    foundation::Status
    WorldCellColliderRegistry::synchronize(
        const streaming::WorldCellResidency&
            residency) noexcept
    {
        if (!residency.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Cannot synchronize physical world "
                "colliders from invalid world cell "
                "residency state.");
        }

        const streaming::WorldCellKey
            cell_key =
                residency.cell_key();

        const std::size_t index =
            lower_bound_index(
                cell_key);

        const bool cell_is_active =
            index <
                active_cells_.size() &&
            active_cells_[index].
                revision.
                cell_key ==
                cell_key;

        if (!should_have_active_colliders(
                residency.state()))
        {
            if (cell_is_active)
            {
                active_cells_.erase(
                    active_cells_.begin() +
                    static_cast<
                        std::ptrdiff_t>(
                            index));
            }

            return foundation::Status{};
        }

        const streaming::WorldCellSnapshot*
            snapshot =
                residency.snapshot();

        const streaming::WorldCellRevisionId&
            resident_revision =
                residency.resident_revision();

        if (snapshot == nullptr ||
            !snapshot->is_valid() ||
            !resident_revision.is_valid() ||
            resident_revision.cell_key !=
                cell_key ||
            snapshot->key() !=
                cell_key ||
            snapshot->revision() !=
                resident_revision.revision)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Active physical world residency "
                "requires a valid matching resident "
                "snapshot and revision identity.");
        }

        if (cell_is_active)
        {
            if (active_cells_[index].
                    revision ==
                resident_revision)
            {
                return foundation::Status{};
            }

            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "An active physical world cell "
                "cannot change snapshot revision "
                "without first becoming inactive.");
        }

        foundation::Result<
            WorldCellColliderSet>
            collider_set_result =
                deserialize_world_cell_collider_payload(
                    cell_key,
                    snapshot->payload());

        if (!collider_set_result.has_value())
        {
            return foundation::fail(
                collider_set_result.
                    error().
                    code,
                collider_set_result.
                    error().
                    message);
        }

        WorldCellColliderSet collider_set =
            std::move(
                collider_set_result.value());

        for (const physics::ColliderGeometry&
                 geometry :
             collider_set.colliders())
        {
            const physics::ColliderId collider =
                geometry.collider();

            for (const ActiveCell&
                     active_cell :
                 active_cells_)
            {
                if (active_cell.
                        collider_set.
                        contains(
                            collider))
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_state,
                        "A persistent collider identity "
                        "cannot be active in more than "
                        "one world cell.");
                }
            }
        }

        try
        {
            active_cells_.insert(
                active_cells_.begin() +
                    static_cast<
                        std::ptrdiff_t>(
                            index),
                ActiveCell{
                    resident_revision,
                    std::move(
                        collider_set)
                });
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Unable to allocate active physical "
                "world collider registry storage.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "An unexpected failure occurred "
                "while activating physical world "
                "colliders.");
        }

        return foundation::Status{};
    }

    const WorldCellColliderSet*
    WorldCellColliderRegistry::find_cell(
        const streaming::WorldCellKey
            cell_key) const noexcept
    {
        if (!cell_key.is_valid())
        {
            return nullptr;
        }

        const std::size_t index =
            lower_bound_index(
                cell_key);

        if (index >=
                active_cells_.size() ||
            active_cells_[index].
                revision.
                cell_key !=
                cell_key)
        {
            return nullptr;
        }

        return
            &active_cells_[index].
                collider_set;
    }

    const physics::ColliderGeometry*
    WorldCellColliderRegistry::find(
        const physics::ColliderId
            collider) const noexcept
    {
        if (!collider.is_valid())
        {
            return nullptr;
        }

        for (const ActiveCell&
                 active_cell :
             active_cells_)
        {
            const physics::ColliderGeometry*
                geometry =
                    active_cell.
                        collider_set.
                        find(
                            collider);

            if (geometry != nullptr)
            {
                return geometry;
            }
        }

        return nullptr;
    }

    bool
    WorldCellColliderRegistry::contains_cell(
        const streaming::WorldCellKey
            cell_key) const noexcept
    {
        return
            find_cell(
                cell_key) !=
            nullptr;
    }

    bool
    WorldCellColliderRegistry::contains(
        const physics::ColliderId
            collider) const noexcept
    {
        return
            find(
                collider) !=
            nullptr;
    }

    std::size_t
    WorldCellColliderRegistry::
    active_cell_count() const noexcept
    {
        return active_cells_.size();
    }

    std::size_t
    WorldCellColliderRegistry::
    collider_count() const noexcept
    {
        std::size_t count{};

        for (const ActiveCell&
                 active_cell :
             active_cells_)
        {
            count +=
                active_cell.
                    collider_set.
                    size();
        }

        return count;
    }

    bool
    WorldCellColliderRegistry::empty()
        const noexcept
    {
        return active_cells_.empty();
    }

    std::size_t
    WorldCellColliderRegistry::
    lower_bound_index(
        const streaming::WorldCellKey
            cell_key) const noexcept
    {
        std::size_t first{};
        std::size_t count =
            active_cells_.size();

        while (count > 0U)
        {
            const std::size_t step =
                count / 2U;

            const std::size_t middle =
                first + step;

            if (active_cells_[middle].
                    revision.
                    cell_key <
                cell_key)
            {
                first =
                    middle + 1U;

                count -=
                    step + 1U;
            }
            else
            {
                count =
                    step;
            }
        }

        return first;
    }
}