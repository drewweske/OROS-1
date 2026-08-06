#pragma once

#include "oros/foundation/result.hpp"
#include "oros/physics/axis_aligned_bounds.hpp"
#include "oros/physics/collider_id.hpp"
#include "oros/physics/collision_shape.hpp"
#include "oros/physics/physics_vector.hpp"

namespace oros::physics
{
    class ColliderGeometry final
    {
    public:
        [[nodiscard]]
        static foundation::Result<
            ColliderGeometry>
        create(
            ColliderId collider,
            CollisionShape shape,
            PhysicsVector3 center);

        [[nodiscard]]
        ColliderId
        collider() const noexcept;

        [[nodiscard]]
        const CollisionShape&
        shape() const noexcept;

        [[nodiscard]]
        PhysicsVector3
        center() const noexcept;

        [[nodiscard]]
        foundation::Result<
            AxisAlignedBounds>
        bounds() const;

        [[nodiscard]]
        foundation::Result<
            ColliderGeometry>
        with_center(
            PhysicsVector3 center)
            const;

        bool operator==(
            const ColliderGeometry&)
            const noexcept = default;

    private:
        ColliderGeometry(
            ColliderId collider,
            CollisionShape shape,
            PhysicsVector3 center)
            noexcept;

        ColliderId collider_;
        CollisionShape shape_;
        PhysicsVector3 center_;
    };
}