#include "oros/physics/collider_geometry.hpp"

#include "oros/foundation/error.hpp"

#include <iostream>
#include <limits>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

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

    template <typename T>
    void check_failure(
        TestState& state,
        const oros::foundation::Result<T>& result,
        const oros::foundation::ErrorCode
            expected_code,
        const std::string_view name)
    {
        check(
            state,
            !result.has_value() &&
                result.error().code ==
                    expected_code,
            name);
    }

    int finish(
        const TestState& state)
    {
        std::cout
            << "\nCollider geometry test summary: "
            << state.checks - state.failures
            << '/'
            << state.checks
            << " passed.\n";

        return
            state.failures == 0
                ? 0
                : 1;
    }
}

int main()
{
    using namespace oros::physics;
    using oros::foundation::ErrorCode;
    using oros::world::EntityId;

    static_assert(
        !std::is_default_constructible_v<
            ColliderGeometry>);

    static_assert(
        std::is_copy_constructible_v<
            ColliderGeometry>);

    static_assert(
        std::is_move_constructible_v<
            ColliderGeometry>);

    static_assert(
        std::is_copy_assignable_v<
            ColliderGeometry>);

    static_assert(
        std::is_move_assignable_v<
            ColliderGeometry>);

    TestState state{};

    constexpr ColliderId collider_a{
        EntityId{
            1U,
            1U
        },
        1U
    };

    constexpr ColliderId collider_b{
        EntityId{
            1U,
            1U
        },
        2U
    };

    static_assert(
        collider_a.is_valid());

    static_assert(
        collider_b.is_valid());

    const auto sphere_result =
        SphereShape::create(
            2.5);

    check(
        state,
        sphere_result.has_value(),
        "Sphere geometry fixture is valid");

    if (!sphere_result.has_value())
    {
        return finish(state);
    }

    const CollisionShape sphere_shape{
        sphere_result.value()
    };

    const PhysicsVector3 initial_center{
        10.0,
        20.0,
        30.0
    };

    check_failure(
        state,
        ColliderGeometry::create(
            invalid_collider_id,
            sphere_shape,
            initial_center),
        ErrorCode::invalid_argument,
        "Geometry rejects default collider identity");

    const ColliderId invalid_owner{
        EntityId{},
        1U
    };

    check_failure(
        state,
        ColliderGeometry::create(
            invalid_owner,
            sphere_shape,
            initial_center),
        ErrorCode::invalid_argument,
        "Geometry rejects collider with invalid owner");

    const ColliderId invalid_shape_slot{
        EntityId{
            1U,
            1U
        },
        0U
    };

    check_failure(
        state,
        ColliderGeometry::create(
            invalid_shape_slot,
            sphere_shape,
            initial_center),
        ErrorCode::invalid_argument,
        "Geometry rejects collider with shape slot zero");

    check_failure(
        state,
        ColliderGeometry::create(
            collider_a,
            sphere_shape,
            PhysicsVector3{
                std::numeric_limits<
                    PhysicsScalar>::infinity(),
                0.0,
                0.0
            }),
        ErrorCode::invalid_argument,
        "Geometry rejects infinite center");

    check_failure(
        state,
        ColliderGeometry::create(
            collider_a,
            sphere_shape,
            PhysicsVector3{
                0.0,
                std::numeric_limits<
                    PhysicsScalar>::quiet_NaN(),
                0.0
            }),
        ErrorCode::invalid_argument,
        "Geometry rejects NaN center");

    const PhysicsScalar maximum =
        std::numeric_limits<
            PhysicsScalar>::max();

    check_failure(
        state,
        ColliderGeometry::create(
            collider_a,
            sphere_shape,
            PhysicsVector3{
                maximum,
                0.0,
                0.0
            }),
        ErrorCode::invalid_argument,
        "Geometry rejects unrepresentable positive bounds");

    check_failure(
        state,
        ColliderGeometry::create(
            collider_a,
            sphere_shape,
            PhysicsVector3{
                -maximum,
                0.0,
                0.0
            }),
        ErrorCode::invalid_argument,
        "Geometry rejects unrepresentable negative bounds");

    const auto geometry_result =
        ColliderGeometry::create(
            collider_a,
            sphere_shape,
            initial_center);

    check(
        state,
        geometry_result.has_value(),
        "Geometry accepts valid identity shape and center");

    if (!geometry_result.has_value())
    {
        return finish(state);
    }

    const ColliderGeometry geometry =
        geometry_result.value();

    check(
        state,
        geometry.collider() ==
            collider_a,
        "Geometry preserves persistent collider identity");

    check(
        state,
        geometry.shape() ==
            sphere_shape,
        "Geometry preserves collision shape");

    check(
        state,
        std::holds_alternative<
            SphereShape>(
            geometry.shape()),
        "Geometry preserves sphere shape alternative");

    check(
        state,
        std::get<SphereShape>(
            geometry.shape()) ==
            sphere_result.value(),
        "Geometry preserves sphere dimensions");

    check(
        state,
        geometry.center() ==
            initial_center,
        "Geometry preserves world-space center");

    const auto bounds_result =
        geometry.bounds();

    check(
        state,
        bounds_result.has_value(),
        "Geometry derives representable bounds");

    if (!bounds_result.has_value())
    {
        return finish(state);
    }

    check(
        state,
        bounds_result.value().minimum() ==
            PhysicsVector3{
                7.5,
                17.5,
                27.5
            },
        "Geometry bounds preserve sphere minimum");

    check(
        state,
        bounds_result.value().maximum() ==
            PhysicsVector3{
                12.5,
                22.5,
                32.5
            },
        "Geometry bounds preserve sphere maximum");

    check(
        state,
        bounds_result.value().center() ==
            initial_center,
        "Geometry bounds preserve center");

    check(
        state,
        bounds_result.value().
            half_extents() ==
            PhysicsVector3{
                2.5,
                2.5,
                2.5
            },
        "Geometry bounds preserve shape half extents");

    const auto equivalent_geometry_result =
        ColliderGeometry::create(
            collider_a,
            sphere_shape,
            initial_center);

    check(
        state,
        equivalent_geometry_result.has_value(),
        "Equivalent geometry fixture is valid");

    check(
        state,
        equivalent_geometry_result.has_value() &&
            equivalent_geometry_result.value() ==
                geometry,
        "Equivalent geometry values compare equal");

    const auto alternate_identity_result =
        ColliderGeometry::create(
            collider_b,
            sphere_shape,
            initial_center);

    check(
        state,
        alternate_identity_result.has_value(),
        "Alternate identity geometry fixture is valid");

    check(
        state,
        alternate_identity_result.has_value() &&
            alternate_identity_result.value() !=
                geometry,
        "Different collider identities produce different geometry");

    const auto box_result =
        BoxShape::create(
            PhysicsVector3{
                1.0,
                2.0,
                3.0
            });

    check(
        state,
        box_result.has_value(),
        "Box geometry fixture is valid");

    if (!box_result.has_value())
    {
        return finish(state);
    }

    const CollisionShape box_shape{
        box_result.value()
    };

    const auto alternate_shape_result =
        ColliderGeometry::create(
            collider_a,
            box_shape,
            initial_center);

    check(
        state,
        alternate_shape_result.has_value(),
        "Alternate shape geometry fixture is valid");

    check(
        state,
        alternate_shape_result.has_value() &&
            alternate_shape_result.value() !=
                geometry,
        "Different shapes produce different geometry");

    const PhysicsVector3 updated_center{
        -10.0,
        -20.0,
        -30.0
    };

    const auto updated_geometry_result =
        geometry.with_center(
            updated_center);

    check(
        state,
        updated_geometry_result.has_value(),
        "Geometry accepts immutable center replacement");

    if (!updated_geometry_result.has_value())
    {
        return finish(state);
    }

    const ColliderGeometry updated_geometry =
        updated_geometry_result.value();

    check(
        state,
        updated_geometry.collider() ==
            collider_a,
        "Center replacement preserves collider identity");

    check(
        state,
        updated_geometry.shape() ==
            sphere_shape,
        "Center replacement preserves collision shape");

    check(
        state,
        updated_geometry.center() ==
            updated_center,
        "Center replacement preserves new center");

    check(
        state,
        updated_geometry !=
            geometry,
        "Center replacement produces distinct geometry");

    check(
        state,
        geometry.center() ==
            initial_center,
        "Center replacement does not mutate original geometry");

    const auto updated_bounds_result =
        updated_geometry.bounds();

    check(
        state,
        updated_bounds_result.has_value(),
        "Updated geometry derives bounds");

    if (!updated_bounds_result.has_value())
    {
        return finish(state);
    }

    check(
        state,
        updated_bounds_result.
            value().
            minimum() ==
            PhysicsVector3{
                -12.5,
                -22.5,
                -32.5
            },
        "Updated geometry bounds preserve minimum");

    check(
        state,
        updated_bounds_result.
            value().
            maximum() ==
            PhysicsVector3{
                -7.5,
                -17.5,
                -27.5
            },
        "Updated geometry bounds preserve maximum");

    check_failure(
        state,
        geometry.with_center(
            PhysicsVector3{
                maximum,
                0.0,
                0.0
            }),
        ErrorCode::invalid_argument,
        "Center replacement rejects unrepresentable bounds");

    check(
        state,
        geometry.center() ==
            initial_center,
        "Rejected center replacement preserves original geometry");

    const auto same_center_result =
        geometry.with_center(
            initial_center);

    check(
        state,
        same_center_result.has_value(),
        "Geometry accepts equivalent center replacement");

    check(
        state,
        same_center_result.has_value() &&
            same_center_result.value() ==
                geometry,
        "Equivalent center replacement preserves geometry value");

    const auto maximum_sphere_result =
        SphereShape::create(
            maximum);

    check(
        state,
        maximum_sphere_result.has_value(),
        "Maximum sphere fixture is valid");

    if (!maximum_sphere_result.has_value())
    {
        return finish(state);
    }

    const CollisionShape maximum_sphere{
        maximum_sphere_result.value()
    };

    const auto maximum_geometry_result =
        ColliderGeometry::create(
            collider_a,
            maximum_sphere,
            physics_zero_vector);

    check(
        state,
        maximum_geometry_result.has_value(),
        "Geometry accepts maximum finite sphere at origin");

    if (!maximum_geometry_result.has_value())
    {
        return finish(state);
    }

    const auto maximum_bounds_result =
        maximum_geometry_result.
            value().
            bounds();

    check(
        state,
        maximum_bounds_result.has_value(),
        "Maximum geometry derives finite bounds");

    check(
        state,
        maximum_bounds_result.has_value() &&
            maximum_bounds_result.
                value().
                minimum() ==
                PhysicsVector3{
                    -maximum,
                    -maximum,
                    -maximum
                },
        "Maximum geometry preserves minimum bounds");

    check(
        state,
        maximum_bounds_result.has_value() &&
            maximum_bounds_result.
                value().
                maximum() ==
                PhysicsVector3{
                    maximum,
                    maximum,
                    maximum
                },
        "Maximum geometry preserves maximum bounds");

    ColliderGeometry copied_geometry =
        geometry;

    check(
        state,
        copied_geometry ==
            geometry,
        "Geometry copy preserves all values");

    ColliderGeometry moved_geometry =
        std::move(
            copied_geometry);

    check(
        state,
        moved_geometry ==
            geometry,
        "Geometry move preserves all values");

    ColliderGeometry assigned_geometry =
        updated_geometry;

    assigned_geometry =
        geometry;

    check(
        state,
        assigned_geometry ==
            geometry,
        "Geometry copy assignment preserves all values");

    ColliderGeometry move_assigned_geometry =
        updated_geometry;

    move_assigned_geometry =
        std::move(
            assigned_geometry);

    check(
        state,
        move_assigned_geometry ==
            geometry,
        "Geometry move assignment preserves all values");

    return finish(state);
}