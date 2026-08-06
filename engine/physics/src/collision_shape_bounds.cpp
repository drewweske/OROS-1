#include "oros/physics/collision_shape_bounds.hpp"

#include <cmath>
#include <type_traits>
#include <variant>

namespace oros::physics
{
    PhysicsVector3
    collision_shape_half_extents(
        const CollisionShape& shape)
        noexcept
    {
        return std::visit(
            [](
                const auto& concrete_shape)
                noexcept
                -> PhysicsVector3
            {
                using ShapeType =
                    std::remove_cvref_t<
                        decltype(
                            concrete_shape)>;

                if constexpr (
                    std::is_same_v<
                        ShapeType,
                        SphereShape>)
                {
                    const PhysicsScalar radius =
                        concrete_shape.radius();

                    return PhysicsVector3{
                        radius,
                        radius,
                        radius
                    };
                }
                else if constexpr (
                    std::is_same_v<
                        ShapeType,
                        BoxShape>)
                {
                    return
                        concrete_shape.
                            half_extents();
                }
                else
                {
                    static_assert(
                        std::is_same_v<
                            ShapeType,
                            CapsuleShape>);

                    const PhysicsScalar radius =
                        concrete_shape.radius();

                    const PhysicsScalar
                        half_segment_length =
                            concrete_shape.
                                half_segment_length();

                    const PhysicsVector3& axis =
                        concrete_shape.
                            axis().
                            vector();

                    return PhysicsVector3{
                        radius +
                            std::abs(
                                axis.x) *
                                half_segment_length,
                        radius +
                            std::abs(
                                axis.y) *
                                half_segment_length,
                        radius +
                            std::abs(
                                axis.z) *
                                half_segment_length
                    };
                }
            },
            shape);
    }

    foundation::Result<
        AxisAlignedBounds>
    collision_shape_bounds(
        const CollisionShape& shape,
        const PhysicsVector3 center)
    {
        return AxisAlignedBounds::
            from_center_and_half_extents(
                center,
                collision_shape_half_extents(
                    shape));
    }
}