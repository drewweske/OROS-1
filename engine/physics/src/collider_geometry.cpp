#include "oros/physics/collider_geometry.hpp"

#include "oros/physics/collision_shape_bounds.hpp"

#include <utility>

namespace oros::physics
{
    ColliderGeometry::ColliderGeometry(
        const ColliderId collider,
        CollisionShape shape,
        const PhysicsVector3 center)
        noexcept
        : collider_{collider},
          shape_{std::move(shape)},
          center_{center}
    {
    }

    foundation::Result<
        ColliderGeometry>
    ColliderGeometry::create(
        const ColliderId collider,
        CollisionShape shape,
        const PhysicsVector3 center)
    {
        if (!collider.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Collider geometry requires a "
                "valid persistent collider "
                "identity.");
        }

        if (!center.is_finite())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Collider geometry center must "
                "contain only finite values.");
        }

        const auto bounds_result =
            collision_shape_bounds(
                shape,
                center);

        if (!bounds_result.has_value())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Collider geometry bounds cannot "
                "be represented at the requested "
                "center.");
        }

        return ColliderGeometry{
            collider,
            std::move(shape),
            center
        };
    }

    ColliderId
    ColliderGeometry::collider()
        const noexcept
    {
        return collider_;
    }

    const CollisionShape&
    ColliderGeometry::shape()
        const noexcept
    {
        return shape_;
    }

    PhysicsVector3
    ColliderGeometry::center()
        const noexcept
    {
        return center_;
    }

    foundation::Result<
        AxisAlignedBounds>
    ColliderGeometry::bounds()
        const
    {
        return collision_shape_bounds(
            shape_,
            center_);
    }

    foundation::Result<
        ColliderGeometry>
    ColliderGeometry::with_center(
        const PhysicsVector3 center)
        const
    {
        return create(
            collider_,
            shape_,
            center);
    }
}