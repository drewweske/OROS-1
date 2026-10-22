#include "oros/physics/collider_segment_query.hpp"

#include "oros/foundation/result.hpp"
#include "oros/physics/collider_geometry.hpp"
#include "oros/physics/collision_shape.hpp"
#include "oros/physics/physics_vector.hpp"
#include "oros/world/entity_id.hpp"

#include <iostream>
#include <limits>
#include <optional>
#include <string_view>
#include <type_traits>

namespace
{
    struct TestState final
    {
        int checks{};
        int failures{};
    };

    void check(
        TestState& state,
        const bool condition,
        const std::string_view name)
    {
        ++state.checks;

        if (condition)
        {
            std::cout
                << "[pass] "
                << name
                << '\n';

            return;
        }

        ++state.failures;

        std::cerr
            << "[fail] "
            << name
            << '\n';
    }

    void check_hit(
        TestState& state,
        const oros::foundation::Result<
            std::optional<
                oros::physics::
                    ColliderSegmentHit>>& result,
        const oros::physics::ColliderId
            expected_collider,
        const oros::physics::PhysicsScalar
            expected_fraction,
        const std::string_view name)
    {
        check(
            state,
            result.has_value() &&
                result.value().
                    has_value() &&
                result.value()->
                        collider() ==
                    expected_collider &&
                oros::physics::nearly_equal(
                    result.value()->
                        segment_fraction(),
                    expected_fraction),
            name);
    }

    void check_miss(
        TestState& state,
        const oros::foundation::Result<
            std::optional<
                oros::physics::
                    ColliderSegmentHit>>& result,
        const std::string_view name)
    {
        check(
            state,
            result.has_value() &&
                !result.value().
                    has_value(),
            name);
    }
}

int main()
{
    using namespace oros::foundation;
    using namespace oros::physics;
    using namespace oros::world;

    static_assert(
        !std::is_default_constructible_v<
            ColliderSegmentHit>);

    TestState state{};

    const ColliderId sphere_collider{
        EntityId{
            0x4F524F53ULL,
            1ULL
        },
        1U
    };

    const ColliderId box_collider{
        EntityId{
            0x4F524F53ULL,
            2ULL
        },
        1U
    };

    const ColliderId capsule_collider{
        EntityId{
            0x4F524F53ULL,
            3ULL
        },
        1U
    };

    const ColliderId
        arbitrary_axis_capsule_collider{
            EntityId{
                0x4F524F53ULL,
                4ULL
            },
            1U
        };

    const auto invalid_id_result =
        ColliderSegmentHit::create(
            invalid_collider_id,
            0.5);

    check(
        state,
        !invalid_id_result.has_value() &&
            invalid_id_result.error().code ==
                ErrorCode::invalid_argument,
        "ColliderSegmentHit rejects invalid ColliderId");

    const auto nan_fraction_result =
        ColliderSegmentHit::create(
            sphere_collider,
            std::numeric_limits<
                PhysicsScalar>::
                    quiet_NaN());

    check(
        state,
        !nan_fraction_result.has_value() &&
            nan_fraction_result.error().code ==
                ErrorCode::invalid_argument,
        "ColliderSegmentHit rejects NaN fraction");

    const auto positive_infinity_result =
        ColliderSegmentHit::create(
            sphere_collider,
            std::numeric_limits<
                PhysicsScalar>::
                    infinity());

    check(
        state,
        !positive_infinity_result.
                has_value() &&
            positive_infinity_result.
                    error().code ==
                ErrorCode::invalid_argument,
        "ColliderSegmentHit rejects positive infinity fraction");

    const auto negative_infinity_result =
        ColliderSegmentHit::create(
            sphere_collider,
            -std::numeric_limits<
                PhysicsScalar>::
                    infinity());

    check(
        state,
        !negative_infinity_result.
                has_value() &&
            negative_infinity_result.
                    error().code ==
                ErrorCode::invalid_argument,
        "ColliderSegmentHit rejects negative infinity fraction");

    const auto negative_fraction_result =
        ColliderSegmentHit::create(
            sphere_collider,
            -0.01);

    check(
        state,
        !negative_fraction_result.
                has_value() &&
            negative_fraction_result.
                    error().code ==
                ErrorCode::invalid_argument,
        "ColliderSegmentHit rejects fraction below zero");

    const auto oversized_fraction_result =
        ColliderSegmentHit::create(
            sphere_collider,
            1.01);

    check(
        state,
        !oversized_fraction_result.
                has_value() &&
            oversized_fraction_result.
                    error().code ==
                ErrorCode::invalid_argument,
        "ColliderSegmentHit rejects fraction above one");

    const auto zero_fraction_result =
        ColliderSegmentHit::create(
            sphere_collider,
            0.0);

    check(
        state,
        zero_fraction_result.has_value() &&
            zero_fraction_result.
                    value().
                    segment_fraction() ==
                0.0,
        "ColliderSegmentHit accepts exact zero");

    const auto one_fraction_result =
        ColliderSegmentHit::create(
            sphere_collider,
            1.0);

    check(
        state,
        one_fraction_result.has_value() &&
            one_fraction_result.
                    value().
                    segment_fraction() ==
                1.0,
        "ColliderSegmentHit accepts exact one");

    const auto sphere_shape_result =
        SphereShape::create(
            1.0);

    if (!sphere_shape_result.has_value())
    {
        return 1;
    }

    const auto sphere_geometry_result =
        ColliderGeometry::create(
            sphere_collider,
            CollisionShape{
                sphere_shape_result.value()
            },
            physics_zero_vector);

    if (!sphere_geometry_result.has_value())
    {
        return 1;
    }

    const ColliderGeometry sphere_geometry =
        sphere_geometry_result.value();

    const auto invalid_start_result =
        query_collider_segment_hit(
            PhysicsVector3{
                std::numeric_limits<
                    PhysicsScalar>::
                        quiet_NaN(),
                0.0,
                0.0
            },
            physics_zero_vector,
            sphere_geometry);

    check(
        state,
        !invalid_start_result.has_value() &&
            invalid_start_result.error().code ==
                ErrorCode::invalid_argument,
        "Query rejects non-finite segment start");

    const auto invalid_end_result =
        query_collider_segment_hit(
            physics_zero_vector,
            PhysicsVector3{
                0.0,
                std::numeric_limits<
                    PhysicsScalar>::
                        infinity(),
                0.0
            },
            sphere_geometry);

    check(
        state,
        !invalid_end_result.has_value() &&
            invalid_end_result.error().code ==
                ErrorCode::invalid_argument,
        "Query rejects non-finite segment end");

    check_hit(
        state,
        query_collider_segment_hit(
            PhysicsVector3{
                -2.0,
                0.0,
                0.0
            },
            PhysicsVector3{
                2.0,
                0.0,
                0.0
            },
            sphere_geometry),
        sphere_collider,
        0.25,
        "Sphere through-center query returns earliest hit");

    check_hit(
        state,
        query_collider_segment_hit(
            PhysicsVector3{
                -2.0,
                1.0,
                0.0
            },
            PhysicsVector3{
                2.0,
                1.0,
                0.0
            },
            sphere_geometry),
        sphere_collider,
        0.5,
        "Sphere tangent counts as a hit");

    check_miss(
        state,
        query_collider_segment_hit(
            PhysicsVector3{
                -2.0,
                2.0,
                0.0
            },
            PhysicsVector3{
                2.0,
                2.0,
                0.0
            },
            sphere_geometry),
        "Sphere miss returns empty optional");

    check_hit(
        state,
        query_collider_segment_hit(
            physics_zero_vector,
            PhysicsVector3{
                2.0,
                0.0,
                0.0
            },
            sphere_geometry),
        sphere_collider,
        0.0,
        "Sphere start inside returns fraction zero");

    check_hit(
        state,
        query_collider_segment_hit(
            PhysicsVector3{
                -2.0,
                0.0,
                0.0
            },
            PhysicsVector3{
                -1.0,
                0.0,
                0.0
            },
            sphere_geometry),
        sphere_collider,
        1.0,
        "Sphere endpoint-only touch returns fraction one");

    check_hit(
        state,
        query_collider_segment_hit(
            physics_zero_vector,
            physics_zero_vector,
            sphere_geometry),
        sphere_collider,
        0.0,
        "Degenerate segment inside sphere returns zero");

    check_miss(
        state,
        query_collider_segment_hit(
            PhysicsVector3{
                2.0,
                0.0,
                0.0
            },
            PhysicsVector3{
                2.0,
                0.0,
                0.0
            },
            sphere_geometry),
        "Degenerate segment outside sphere misses");

    const auto box_shape_result =
        BoxShape::create(
            PhysicsVector3{
                1.0,
                1.0,
                1.0
            });

    if (!box_shape_result.has_value())
    {
        return 1;
    }

    const auto box_geometry_result =
        ColliderGeometry::create(
            box_collider,
            CollisionShape{
                box_shape_result.value()
            },
            physics_zero_vector);

    if (!box_geometry_result.has_value())
    {
        return 1;
    }

    const ColliderGeometry box_geometry =
        box_geometry_result.value();

    check_hit(
        state,
        query_collider_segment_hit(
            PhysicsVector3{
                -2.0,
                0.0,
                0.0
            },
            PhysicsVector3{
                2.0,
                0.0,
                0.0
            },
            box_geometry),
        box_collider,
        0.25,
        "Box face entry returns earliest hit");

    check_hit(
        state,
        query_collider_segment_hit(
            PhysicsVector3{
                -2.0,
                1.0,
                0.0
            },
            PhysicsVector3{
                2.0,
                1.0,
                0.0
            },
            box_geometry),
        box_collider,
        0.25,
        "Box boundary traversal counts as a hit");

    check_miss(
        state,
        query_collider_segment_hit(
            PhysicsVector3{
                -2.0,
                2.0,
                0.0
            },
            PhysicsVector3{
                2.0,
                2.0,
                0.0
            },
            box_geometry),
        "Box miss returns empty optional");

    check_hit(
        state,
        query_collider_segment_hit(
            physics_zero_vector,
            PhysicsVector3{
                2.0,
                0.0,
                0.0
            },
            box_geometry),
        box_collider,
        0.0,
        "Box start inside returns fraction zero");

    check_hit(
        state,
        query_collider_segment_hit(
            PhysicsVector3{
                -2.0,
                0.0,
                0.0
            },
            PhysicsVector3{
                -1.0,
                0.0,
                0.0
            },
            box_geometry),
        box_collider,
        1.0,
        "Box endpoint-only touch returns fraction one");

    const auto capsule_shape_result =
        CapsuleShape::create(
            1.0,
            2.0,
            PhysicsUnitVector3::
                positive_y());

    if (!capsule_shape_result.has_value())
    {
        return 1;
    }

    const auto capsule_geometry_result =
        ColliderGeometry::create(
            capsule_collider,
            CollisionShape{
                capsule_shape_result.value()
            },
            physics_zero_vector);

    if (!capsule_geometry_result.has_value())
    {
        return 1;
    }

    const ColliderGeometry capsule_geometry =
        capsule_geometry_result.value();

    check_hit(
        state,
        query_collider_segment_hit(
            PhysicsVector3{
                -2.0,
                0.0,
                0.0
            },
            PhysicsVector3{
                2.0,
                0.0,
                0.0
            },
            capsule_geometry),
        capsule_collider,
        0.25,
        "Capsule cylindrical side returns earliest hit");

    check_hit(
        state,
        query_collider_segment_hit(
            PhysicsVector3{
                0.0,
                4.0,
                0.0
            },
            physics_zero_vector,
            capsule_geometry),
        capsule_collider,
        0.25,
        "Capsule spherical cap returns earliest hit");

    check_hit(
        state,
        query_collider_segment_hit(
            PhysicsVector3{
                -2.0,
                0.0,
                1.0
            },
            PhysicsVector3{
                2.0,
                0.0,
                1.0
            },
            capsule_geometry),
        capsule_collider,
        0.5,
        "Capsule tangent counts as a hit");

    check_miss(
        state,
        query_collider_segment_hit(
            PhysicsVector3{
                -2.0,
                0.0,
                2.0
            },
            PhysicsVector3{
                2.0,
                0.0,
                2.0
            },
            capsule_geometry),
        "Capsule miss returns empty optional");

    check_hit(
        state,
        query_collider_segment_hit(
            physics_zero_vector,
            PhysicsVector3{
                2.0,
                0.0,
                0.0
            },
            capsule_geometry),
        capsule_collider,
        0.0,
        "Capsule start inside returns fraction zero");

    const auto arbitrary_axis_shape_result =
        CapsuleShape::create(
            0.5,
            1.0,
            PhysicsUnitVector3::
                positive_x());

    if (!arbitrary_axis_shape_result.has_value())
    {
        return 1;
    }

    const auto
        arbitrary_axis_geometry_result =
            ColliderGeometry::create(
                arbitrary_axis_capsule_collider,
                CollisionShape{
                    arbitrary_axis_shape_result.
                        value()
                },
                physics_zero_vector);

    if (
        !arbitrary_axis_geometry_result.
            has_value())
    {
        return 1;
    }

    check_hit(
        state,
        query_collider_segment_hit(
            PhysicsVector3{
                0.0,
                -2.0,
                0.0
            },
            PhysicsVector3{
                0.0,
                2.0,
                0.0
            },
            arbitrary_axis_geometry_result.
                value()),
        arbitrary_axis_capsule_collider,
        0.375,
        "Capsule supports arbitrary non-default axis");

    std::cout
        << state.checks
        << " checks, "
        << state.failures
        << " failures\n";

    return state.failures == 0
        ? 0
        : 1;
}
