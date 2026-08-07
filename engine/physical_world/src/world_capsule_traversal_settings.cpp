#include "oros/physical_world/world_capsule_traversal_settings.hpp"

#include <cmath>

namespace oros::physical_world
{
    foundation::Result<
        WorldCapsuleTraversalSettings>
    WorldCapsuleTraversalSettings::create(
        const physics::PhysicsUnitVector3
            up_direction,
        const physics::PhysicsScalar
            minimum_walkable_up_dot,
        const physics::PhysicsScalar
            maximum_step_height,
        const physics::PhysicsScalar
            maximum_ground_snap_distance)
    {
        if (!up_direction.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Capsule traversal requires a "
                "valid unit up direction.");
        }

        if (!std::isfinite(
                minimum_walkable_up_dot) ||
            minimum_walkable_up_dot <= 0.0 ||
            minimum_walkable_up_dot > 1.0)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Capsule traversal minimum "
                "walkable up dot must be finite "
                "and in the range (0, 1].");
        }

        if (!std::isfinite(
                maximum_step_height) ||
            maximum_step_height < 0.0)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Capsule traversal maximum step "
                "height must be finite and "
                "non-negative.");
        }

        if (!std::isfinite(
                maximum_ground_snap_distance) ||
            maximum_ground_snap_distance < 0.0)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Capsule traversal maximum ground "
                "snap distance must be finite and "
                "non-negative.");
        }

        return WorldCapsuleTraversalSettings{
            up_direction,
            minimum_walkable_up_dot,
            maximum_step_height,
            maximum_ground_snap_distance
        };
    }

    const physics::PhysicsUnitVector3&
    WorldCapsuleTraversalSettings::
        up_direction() const noexcept
    {
        return up_direction_;
    }

    physics::PhysicsScalar
    WorldCapsuleTraversalSettings::
        minimum_walkable_up_dot()
        const noexcept
    {
        return minimum_walkable_up_dot_;
    }

    physics::PhysicsScalar
    WorldCapsuleTraversalSettings::
        maximum_step_height()
        const noexcept
    {
        return maximum_step_height_;
    }

    physics::PhysicsScalar
    WorldCapsuleTraversalSettings::
        maximum_ground_snap_distance()
        const noexcept
    {
        return maximum_ground_snap_distance_;
    }

    WorldCapsuleTraversalSettings::
        WorldCapsuleTraversalSettings(
            const physics::PhysicsUnitVector3
                up_direction,
            const physics::PhysicsScalar
                minimum_walkable_up_dot,
            const physics::PhysicsScalar
                maximum_step_height,
            const physics::PhysicsScalar
                maximum_ground_snap_distance)
            noexcept
        : up_direction_{
              up_direction
          },
          minimum_walkable_up_dot_{
              minimum_walkable_up_dot
          },
          maximum_step_height_{
              maximum_step_height
          },
          maximum_ground_snap_distance_{
              maximum_ground_snap_distance
          }
    {
    }
}