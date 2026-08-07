#pragma once

#include "oros/foundation/result.hpp"
#include "oros/physical_world/world_cell_collider_registry.hpp"
#include "oros/physics/collider_id.hpp"
#include "oros/physics/collision_shape.hpp"
#include "oros/world/world_position.hpp"

#include <cstddef>
#include <cstdint>

namespace oros::physical_world
{
    [[nodiscard]]
    foundation::Result<
        world::WorldPosition>
    resolve_world_capsule_penetration(
        const WorldCellColliderRegistry& registry,
        std::uint64_t world_namespace,
        physics::ColliderId capsule_collider,
        const physics::CapsuleShape& capsule_shape,
        const world::WorldPosition&
            capsule_center,
        std::size_t maximum_iterations);
}