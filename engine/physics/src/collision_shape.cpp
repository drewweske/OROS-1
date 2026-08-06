#include "oros/physics/collision_shape.hpp"

#include <cmath>
#include <type_traits>
#include <variant>

namespace oros::physics
{
    SphereShape::SphereShape(
        const PhysicsScalar radius)
        noexcept
        : radius_{radius}
    {
    }

    foundation::Result<
        SphereShape>
    SphereShape::create(
        const PhysicsScalar radius)
    {
        if (!std::isfinite(
                radius) ||
            radius <= 0.0)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Sphere radius must be finite "
                "and greater than zero.");
        }

        return SphereShape{
            radius
        };
    }

    PhysicsScalar
    SphereShape::radius()
        const noexcept
    {
        return radius_;
    }

    BoxShape::BoxShape(
        const PhysicsVector3
            half_extents)
        noexcept
        : half_extents_{
              half_extents
          }
    {
    }

    foundation::Result<
        BoxShape>
    BoxShape::create(
        const PhysicsVector3
            half_extents)
    {
        if (!half_extents.is_finite())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Box half extents must contain "
                "only finite values.");
        }

        if (half_extents.x <= 0.0 ||
            half_extents.y <= 0.0 ||
            half_extents.z <= 0.0)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Every box half extent must be "
                "greater than zero.");
        }

        return BoxShape{
            half_extents
        };
    }

    const PhysicsVector3&
    BoxShape::half_extents()
        const noexcept
    {
        return half_extents_;
    }

    CapsuleShape::CapsuleShape(
        const PhysicsScalar radius,
        const PhysicsScalar
            half_segment_length,
        PhysicsUnitVector3 axis)
        noexcept
        : radius_{radius},
          half_segment_length_{
              half_segment_length
          },
          axis_{
              axis
          }
    {
    }

    foundation::Result<
        CapsuleShape>
    CapsuleShape::create(
        const PhysicsScalar radius,
        const PhysicsScalar
            half_segment_length,
        const PhysicsUnitVector3 axis)
    {
        if (!std::isfinite(
                radius) ||
            radius <= 0.0)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Capsule radius must be finite "
                "and greater than zero.");
        }

        if (!std::isfinite(
                half_segment_length) ||
            half_segment_length < 0.0)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Capsule half-segment length must "
                "be finite and non-negative.");
        }

        if (!axis.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Capsule axis must be a valid "
                "unit direction.");
        }

        const PhysicsScalar
            total_half_height =
                radius +
                half_segment_length;

        if (!std::isfinite(
                total_half_height))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Capsule dimensions exceed the "
                "finite physics range.");
        }

        return CapsuleShape{
            radius,
            half_segment_length,
            axis
        };
    }

    PhysicsScalar
    CapsuleShape::radius()
        const noexcept
    {
        return radius_;
    }

    PhysicsScalar
    CapsuleShape::half_segment_length()
        const noexcept
    {
        return half_segment_length_;
    }

    PhysicsScalar
    CapsuleShape::total_half_height()
        const noexcept
    {
        return
            radius_ +
            half_segment_length_;
    }

    const PhysicsUnitVector3&
    CapsuleShape::axis()
        const noexcept
    {
        return axis_;
    }

    CollisionShapeKind
    collision_shape_kind(
        const CollisionShape& shape)
        noexcept
    {
        return std::visit(
            [](
                const auto& concrete_shape)
            {
                using ShapeType =
                    std::remove_cvref_t<
                        decltype(
                            concrete_shape)>;

                static_assert(
                    std::is_same_v<
                        ShapeType,
                        SphereShape> ||
                    std::is_same_v<
                        ShapeType,
                        BoxShape> ||
                    std::is_same_v<
                        ShapeType,
                        CapsuleShape>);

                return
                    concrete_shape.kind();
            },
            shape);
    }
}