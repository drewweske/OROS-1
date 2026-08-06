#include "oros/physics/collision_contact.hpp"

#include <cmath>

namespace oros::physics
{
    CollisionContact::CollisionContact(
        const BroadPhasePair pair,
        const PhysicsVector3 point,
        const PhysicsUnitVector3 normal,
        const PhysicsScalar penetration_depth)
        noexcept
        : pair_{pair},
          point_{point},
          normal_{normal},
          penetration_depth_{
              penetration_depth}
    {
    }

    foundation::Result<
        CollisionContact>
    CollisionContact::create(
        const BroadPhasePair pair,
        const PhysicsVector3 point,
        const PhysicsUnitVector3 normal,
        const PhysicsScalar penetration_depth)
    {
        if (!point.is_finite())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Collision contact point must "
                "contain only finite values.");
        }

        if (!std::isfinite(
                penetration_depth) ||
            penetration_depth < 0.0)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Collision contact penetration "
                "depth must be finite and "
                "non-negative.");
        }

        return CollisionContact{
            pair,
            point,
            normal,
            penetration_depth
        };
    }

    const BroadPhasePair&
    CollisionContact::pair()
        const noexcept
    {
        return pair_;
    }

    PhysicsVector3
    CollisionContact::point()
        const noexcept
    {
        return point_;
    }

    const PhysicsUnitVector3&
    CollisionContact::normal()
        const noexcept
    {
        return normal_;
    }

    PhysicsScalar
    CollisionContact::penetration_depth()
        const noexcept
    {
        return penetration_depth_;
    }

    bool
    CollisionContact::is_touching()
        const noexcept
    {
        return
            penetration_depth_ == 0.0;
    }
}