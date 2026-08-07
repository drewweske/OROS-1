#pragma once

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
    [[nodiscard]]
    foundation::Result<
        world::WorldPosition>
    move_world_capsule_along_ground(
        const WorldCellColliderRegistry& registry,
        std::uint64_t world_namespace,
        physics::ColliderId capsule_collider,
        const physics::CapsuleShape& capsule_shape,
        const world::WorldPosition&
            capsule_center,
        world::WorldDisplacement
            desired_planar_displacement,
        const WorldCapsuleTraversalSettings&
            traversal_settings,
        physics::PhysicsScalar
            maximum_substep_distance,
        std::size_t maximum_substeps,
        std::size_t
            maximum_depenetration_iterations);
}