#pragma once

#include "oros/foundation/clock.hpp"
#include "oros/foundation/result.hpp"
#include "oros/physical_world/world_capsule_traversal_settings.hpp"
#include "oros/physical_world/world_cell_collider_registry.hpp"
#include "oros/physics/collider_id.hpp"
#include "oros/physics/collision_shape.hpp"
#include "oros/physics/physics_vector.hpp"
#include "oros/world/world_position.hpp"

#include <cstddef>
#include <cstdint>

namespace oros::physical_world
{
    struct WorldFirstPersonControllerCommand final
    {
        physics::PhysicsVector3
            desired_planar_direction{};
    };

    class WorldFirstPersonControllerSettings final
    {
    public:
        [[nodiscard]]
        static foundation::Result<
            WorldFirstPersonControllerSettings>
        create(
            physics::PhysicsScalar
                maximum_ground_speed,
            physics::PhysicsScalar
                maximum_substep_distance,
            std::size_t maximum_substeps,
            std::size_t
                maximum_depenetration_iterations);

        [[nodiscard]]
        physics::PhysicsScalar
        maximum_ground_speed() const noexcept;

        [[nodiscard]]
        physics::PhysicsScalar
        maximum_substep_distance() const noexcept;

        [[nodiscard]]
        std::size_t
        maximum_substeps() const noexcept;

        [[nodiscard]]
        std::size_t
        maximum_depenetration_iterations()
            const noexcept;

    private:
        WorldFirstPersonControllerSettings(
            physics::PhysicsScalar
                maximum_ground_speed,
            physics::PhysicsScalar
                maximum_substep_distance,
            std::size_t maximum_substeps,
            std::size_t
                maximum_depenetration_iterations)
            noexcept;

        physics::PhysicsScalar
            maximum_ground_speed_{};

        physics::PhysicsScalar
            maximum_substep_distance_{};

        std::size_t maximum_substeps_{};

        std::size_t
            maximum_depenetration_iterations_{};
    };

    [[nodiscard]]
    foundation::Result<world::WorldPosition>
    step_world_first_person_controller(
        const WorldCellColliderRegistry& registry,
        std::uint64_t world_namespace,
        physics::ColliderId capsule_collider,
        const physics::CapsuleShape& capsule_shape,
        const world::WorldPosition& capsule_center,
        const WorldFirstPersonControllerCommand&
            command,
        const WorldCapsuleTraversalSettings&
            traversal_settings,
        const WorldFirstPersonControllerSettings&
            controller_settings,
        foundation::Nanoseconds simulation_step);
}
