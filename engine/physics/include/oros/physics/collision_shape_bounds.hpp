#pragma once

#include "oros/foundation/result.hpp"
#include "oros/physics/axis_aligned_bounds.hpp"
#include "oros/physics/collision_shape.hpp"
#include "oros/physics/physics_vector.hpp"

namespace oros::physics
{
    [[nodiscard]]
    PhysicsVector3
    collision_shape_half_extents(
        const CollisionShape& shape)
        noexcept;

    [[nodiscard]]
    foundation::Result<
        AxisAlignedBounds>
    collision_shape_bounds(
        const CollisionShape& shape,
        PhysicsVector3 center);
}