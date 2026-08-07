#include "oros/physical_world/world_cell_collider_set.hpp"

#include "oros/world/world_position.hpp"

#include <algorithm>
#include <cstddef>
#include <new>
#include <span>
#include <utility>
#include <vector>

namespace oros::physical_world
{
    namespace
    {
        [[nodiscard]]
        bool is_canonical_cell_local_center(
            const physics::PhysicsVector3 center)
            noexcept
        {
            return
                center.is_finite() &&
                center.x >=
                    -world::
                        world_cell_half_extent_meters &&
                center.x <
                    world::
                        world_cell_half_extent_meters &&
                center.y >=
                    -world::
                        world_cell_half_extent_meters &&
                center.y <
                    world::
                        world_cell_half_extent_meters &&
                center.z >=
                    -world::
                        world_cell_half_extent_meters &&
                center.z <
                    world::
                        world_cell_half_extent_meters;
        }

        [[nodiscard]]
        bool collider_identity_less(
            const physics::ColliderGeometry& left,
            const physics::ColliderGeometry& right)
            noexcept
        {
            return
                left.collider() <
                right.collider();
        }
    }

    foundation::Result<
        WorldCellColliderSet>
    WorldCellColliderSet::create(
        const streaming::WorldCellKey cell_key,
        const std::span<
            const physics::ColliderGeometry>
            colliders)
    {
        if (!cell_key.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell collider set requires "
                "a valid cell key.");
        }

        for (const physics::ColliderGeometry&
                 geometry : colliders)
        {
            const physics::ColliderId collider =
                geometry.collider();

            if (!collider.is_valid())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "World cell collider set contains "
                    "an invalid collider identity.");
            }

            if (collider.owner.world_namespace !=
                cell_key.world_namespace)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "World cell collider owner belongs "
                    "to a different world namespace.");
            }

            if (!is_canonical_cell_local_center(
                    geometry.center()))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "World cell collider center must "
                    "be finite and inside the "
                    "canonical cell-local range.");
            }
        }

        try
        {
            std::vector<
                physics::ColliderGeometry>
                owned_colliders{
                    colliders.begin(),
                    colliders.end()
                };

            std::sort(
                owned_colliders.begin(),
                owned_colliders.end(),
                collider_identity_less);

            const auto duplicate_iterator =
                std::adjacent_find(
                    owned_colliders.begin(),
                    owned_colliders.end(),
                    [](
                        const physics::
                            ColliderGeometry& left,
                        const physics::
                            ColliderGeometry& right)
                    {
                        return
                            left.collider() ==
                            right.collider();
                    });

            if (duplicate_iterator !=
                owned_colliders.end())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "World cell collider set contains "
                    "a duplicate persistent collider "
                    "identity.");
            }

            return WorldCellColliderSet{
                cell_key,
                std::move(
                    owned_colliders)
            };
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Unable to allocate world cell "
                "collider storage.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "An unexpected failure occurred "
                "while creating a world cell "
                "collider set.");
        }
    }

    WorldCellColliderSet::
    WorldCellColliderSet(
        const streaming::WorldCellKey cell_key,
        std::vector<
            physics::ColliderGeometry>
            colliders) noexcept
        : cell_key_{
              cell_key
          },
          colliders_{
              std::move(
                  colliders)
          }
    {
    }

    bool
    WorldCellColliderSet::is_valid()
        const noexcept
    {
        if (!cell_key_.is_valid())
        {
            return false;
        }

        physics::ColliderId
            previous_collider{};

        bool has_previous_collider{};

        for (const physics::ColliderGeometry&
                 geometry : colliders_)
        {
            const physics::ColliderId collider =
                geometry.collider();

            if (!collider.is_valid() ||
                collider.owner.world_namespace !=
                    cell_key_.world_namespace ||
                !is_canonical_cell_local_center(
                    geometry.center()))
            {
                return false;
            }

            if (has_previous_collider &&
                !(previous_collider <
                    collider))
            {
                return false;
            }

            previous_collider =
                collider;

            has_previous_collider =
                true;
        }

        return true;
    }

    const streaming::WorldCellKey&
    WorldCellColliderSet::cell_key()
        const noexcept
    {
        return cell_key_;
    }

    std::span<
        const physics::ColliderGeometry>
    WorldCellColliderSet::colliders()
        const noexcept
    {
        return std::span<
            const physics::ColliderGeometry>{
                colliders_
            };
    }

    const physics::ColliderGeometry*
    WorldCellColliderSet::find(
        const physics::ColliderId collider)
        const noexcept
    {
        if (!collider.is_valid() ||
            collider.owner.world_namespace !=
                cell_key_.world_namespace)
        {
            return nullptr;
        }

        const auto iterator =
            std::lower_bound(
                colliders_.begin(),
                colliders_.end(),
                collider,
                [](
                    const physics::
                        ColliderGeometry& geometry,
                    const physics::
                        ColliderId candidate)
                {
                    return
                        geometry.collider() <
                        candidate;
                });

        if (iterator == colliders_.end() ||
            iterator->collider() !=
                collider)
        {
            return nullptr;
        }

        return &(*iterator);
    }

    bool
    WorldCellColliderSet::contains(
        const physics::ColliderId collider)
        const noexcept
    {
        return find(collider) != nullptr;
    }

    std::size_t
    WorldCellColliderSet::size()
        const noexcept
    {
        return colliders_.size();
    }

    bool
    WorldCellColliderSet::empty()
        const noexcept
    {
        return colliders_.empty();
    }
}