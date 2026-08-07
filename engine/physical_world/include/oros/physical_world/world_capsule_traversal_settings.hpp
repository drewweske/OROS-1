#pragma once

#include "oros/foundation/result.hpp"
#include "oros/physics/physics_vector.hpp"

namespace oros::physical_world
{
    class WorldCapsuleTraversalSettings final
    {
    public:
        [[nodiscard]]
        static foundation::Result<
            WorldCapsuleTraversalSettings>
        create(
            physics::PhysicsUnitVector3
                up_direction,
            physics::PhysicsScalar
                minimum_walkable_up_dot,
            physics::PhysicsScalar
                maximum_step_height,
            physics::PhysicsScalar
                maximum_ground_snap_distance);

        [[nodiscard]]
        const physics::PhysicsUnitVector3&
        up_direction() const noexcept;

        [[nodiscard]]
        physics::PhysicsScalar
        minimum_walkable_up_dot()
            const noexcept;

        [[nodiscard]]
        physics::PhysicsScalar
        maximum_step_height()
            const noexcept;

        [[nodiscard]]
        physics::PhysicsScalar
        maximum_ground_snap_distance()
            const noexcept;

    private:
        WorldCapsuleTraversalSettings(
            physics::PhysicsUnitVector3
                up_direction,
            physics::PhysicsScalar
                minimum_walkable_up_dot,
            physics::PhysicsScalar
                maximum_step_height,
            physics::PhysicsScalar
                maximum_ground_snap_distance)
            noexcept;

        physics::PhysicsUnitVector3
            up_direction_;

        physics::PhysicsScalar
            minimum_walkable_up_dot_{};

        physics::PhysicsScalar
            maximum_step_height_{};

        physics::PhysicsScalar
            maximum_ground_snap_distance_{};
    };
}