#include "oros/physics/collision_shape_bounds.hpp"

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
            << "\nCollision-shape bounds test summary: "
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

    static_assert(
        std::is_same_v<
            decltype(
                collision_shape_half_extents(
                    std::declval<
                        const CollisionShape&>())),
            PhysicsVector3>);

    static_assert(
        std::is_same_v<
            decltype(
                collision_shape_bounds(
                    std::declval<
                        const CollisionShape&>(),
                    std::declval<
                        PhysicsVector3>())),
            oros::foundation::Result<
                AxisAlignedBounds>>);

    TestState state{};

    const auto sphere_result =
        SphereShape::create(
            2.5);

    const auto box_result =
        BoxShape::create(
            PhysicsVector3{
                1.0,
                2.0,
                3.0
            });

    const auto x_capsule_result =
        CapsuleShape::create(
            0.5,
            2.0,
            PhysicsUnitVector3::
                positive_x());

    const auto y_capsule_result =
        CapsuleShape::create(
            0.5,
            2.0,
            PhysicsUnitVector3::
                positive_y());

    const auto z_capsule_result =
        CapsuleShape::create(
            0.5,
            2.0,
            PhysicsUnitVector3::
                positive_z());

    const auto diagonal_axis_result =
        PhysicsUnitVector3::create(
            PhysicsVector3{
                1.0,
                2.0,
                2.0
            });

    check(
        state,
        sphere_result.has_value(),
        "Sphere fixture is valid");

    check(
        state,
        box_result.has_value(),
        "Box fixture is valid");

    check(
        state,
        x_capsule_result.has_value(),
        "X-axis capsule fixture is valid");

    check(
        state,
        y_capsule_result.has_value(),
        "Y-axis capsule fixture is valid");

    check(
        state,
        z_capsule_result.has_value(),
        "Z-axis capsule fixture is valid");

    check(
        state,
        diagonal_axis_result.has_value(),
        "Diagonal capsule axis fixture is valid");

    if (!sphere_result.has_value() ||
        !box_result.has_value() ||
        !x_capsule_result.has_value() ||
        !y_capsule_result.has_value() ||
        !z_capsule_result.has_value() ||
        !diagonal_axis_result.has_value())
    {
        return finish(state);
    }

    const auto diagonal_capsule_result =
        CapsuleShape::create(
            0.5,
            3.0,
            diagonal_axis_result.value());

    check(
        state,
        diagonal_capsule_result.
            has_value(),
        "Diagonal capsule fixture is valid");

    if (!diagonal_capsule_result.
            has_value())
    {
        return finish(state);
    }

    const CollisionShape sphere{
        sphere_result.value()
    };

    const CollisionShape box{
        box_result.value()
    };

    const CollisionShape x_capsule{
        x_capsule_result.value()
    };

    const CollisionShape y_capsule{
        y_capsule_result.value()
    };

    const CollisionShape z_capsule{
        z_capsule_result.value()
    };

    const CollisionShape
        diagonal_capsule{
            diagonal_capsule_result.
                value()
        };

    check(
        state,
        collision_shape_half_extents(
            sphere) ==
            PhysicsVector3{
                2.5,
                2.5,
                2.5
            },
        "Sphere half extents equal radius on every axis");

    check(
        state,
        collision_shape_half_extents(
            box) ==
            PhysicsVector3{
                1.0,
                2.0,
                3.0
            },
        "Box half extents preserve shape dimensions");

    check(
        state,
        collision_shape_half_extents(
            x_capsule) ==
            PhysicsVector3{
                2.5,
                0.5,
                0.5
            },
        "X-axis capsule projects segment onto X");

    check(
        state,
        collision_shape_half_extents(
            y_capsule) ==
            PhysicsVector3{
                0.5,
                2.5,
                0.5
            },
        "Y-axis capsule projects segment onto Y");

    check(
        state,
        collision_shape_half_extents(
            z_capsule) ==
            PhysicsVector3{
                0.5,
                0.5,
                2.5
            },
        "Z-axis capsule projects segment onto Z");

    check(
        state,
        nearly_equal(
            collision_shape_half_extents(
                diagonal_capsule),
            PhysicsVector3{
                1.5,
                2.5,
                2.5
            }),
        "Diagonal capsule projects its segment onto every axis");

    const PhysicsVector3 center{
        10.0,
        20.0,
        30.0
    };

    const auto sphere_bounds_result =
        collision_shape_bounds(
            sphere,
            center);

    check(
        state,
        sphere_bounds_result.
            has_value(),
        "Sphere bounds accept finite center");

    check(
        state,
        sphere_bounds_result.
            has_value() &&
            sphere_bounds_result.
                value().
                minimum() ==
                PhysicsVector3{
                    7.5,
                    17.5,
                    27.5
                },
        "Sphere bounds calculate minimum");

    check(
        state,
        sphere_bounds_result.
            has_value() &&
            sphere_bounds_result.
                value().
                maximum() ==
                PhysicsVector3{
                    12.5,
                    22.5,
                    32.5
                },
        "Sphere bounds calculate maximum");

    check(
        state,
        sphere_bounds_result.
            has_value() &&
            sphere_bounds_result.
                value().
                center() ==
                center,
        "Sphere bounds preserve center");

    check(
        state,
        sphere_bounds_result.
            has_value() &&
            sphere_bounds_result.
                value().
                half_extents() ==
                PhysicsVector3{
                    2.5,
                    2.5,
                    2.5
                },
        "Sphere bounds preserve shape half extents");

    const auto box_bounds_result =
        collision_shape_bounds(
            box,
            center);

    check(
        state,
        box_bounds_result.
            has_value(),
        "Box bounds accept finite center");

    check(
        state,
        box_bounds_result.
            has_value() &&
            box_bounds_result.
                value().
                minimum() ==
                PhysicsVector3{
                    9.0,
                    18.0,
                    27.0
                },
        "Box bounds calculate minimum");

    check(
        state,
        box_bounds_result.
            has_value() &&
            box_bounds_result.
                value().
                maximum() ==
                PhysicsVector3{
                    11.0,
                    22.0,
                    33.0
                },
        "Box bounds calculate maximum");

    const auto x_capsule_bounds_result =
        collision_shape_bounds(
            x_capsule,
            center);

    check(
        state,
        x_capsule_bounds_result.
            has_value(),
        "X-axis capsule bounds accept finite center");

    check(
        state,
        x_capsule_bounds_result.
            has_value() &&
            x_capsule_bounds_result.
                value().
                minimum() ==
                PhysicsVector3{
                    7.5,
                    19.5,
                    29.5
                },
        "X-axis capsule bounds calculate minimum");

    check(
        state,
        x_capsule_bounds_result.
            has_value() &&
            x_capsule_bounds_result.
                value().
                maximum() ==
                PhysicsVector3{
                    12.5,
                    20.5,
                    30.5
                },
        "X-axis capsule bounds calculate maximum");

    const auto diagonal_bounds_result =
        collision_shape_bounds(
            diagonal_capsule,
            center);

    check(
        state,
        diagonal_bounds_result.
            has_value(),
        "Diagonal capsule bounds accept finite center");

    check(
        state,
        diagonal_bounds_result.
            has_value() &&
            nearly_equal(
                diagonal_bounds_result.
                    value().
                    minimum(),
                PhysicsVector3{
                    8.5,
                    17.5,
                    27.5
                }),
        "Diagonal capsule bounds calculate minimum");

    check(
        state,
        diagonal_bounds_result.
            has_value() &&
            nearly_equal(
                diagonal_bounds_result.
                    value().
                    maximum(),
                PhysicsVector3{
                    11.5,
                    22.5,
                    32.5
                }),
        "Diagonal capsule bounds calculate maximum");

    const auto zero_segment_capsule_result =
        CapsuleShape::create(
            1.25,
            0.0,
            PhysicsUnitVector3::
                positive_y());

    check(
        state,
        zero_segment_capsule_result.
            has_value(),
        "Zero-segment capsule fixture is valid");

    if (!zero_segment_capsule_result.
            has_value())
    {
        return finish(state);
    }

    const CollisionShape
        zero_segment_capsule{
            zero_segment_capsule_result.
                value()
        };

    check(
        state,
        collision_shape_half_extents(
            zero_segment_capsule) ==
            PhysicsVector3{
                1.25,
                1.25,
                1.25
            },
        "Zero-segment capsule has spherical half extents");

    check_failure(
        state,
        collision_shape_bounds(
            sphere,
            PhysicsVector3{
                std::numeric_limits<
                    PhysicsScalar>::
                    quiet_NaN(),
                0.0,
                0.0
            }),
        ErrorCode::invalid_argument,
        "Shape bounds reject NaN center");

    check_failure(
        state,
        collision_shape_bounds(
            box,
            PhysicsVector3{
                0.0,
                std::numeric_limits<
                    PhysicsScalar>::
                    infinity(),
                0.0
            }),
        ErrorCode::invalid_argument,
        "Shape bounds reject infinite center");

    const PhysicsScalar maximum =
        std::numeric_limits<
            PhysicsScalar>::max();

    check_failure(
        state,
        collision_shape_bounds(
            sphere,
            PhysicsVector3{
                maximum,
                0.0,
                0.0
            }),
        ErrorCode::invalid_argument,
        "Shape bounds reject overflowing positive endpoint");

    check_failure(
        state,
        collision_shape_bounds(
            sphere,
            PhysicsVector3{
                -maximum,
                0.0,
                0.0
            }),
        ErrorCode::invalid_argument,
        "Shape bounds reject overflowing negative endpoint");

    const auto maximum_sphere_result =
        SphereShape::create(
            maximum);

    check(
        state,
        maximum_sphere_result.
            has_value(),
        "Maximum-radius sphere fixture is valid");

    if (!maximum_sphere_result.
            has_value())
    {
        return finish(state);
    }

    const CollisionShape maximum_sphere{
        maximum_sphere_result.value()
    };

    const auto maximum_sphere_bounds_result =
        collision_shape_bounds(
            maximum_sphere,
            physics_zero_vector);

    check(
        state,
        maximum_sphere_bounds_result.
            has_value(),
        "Maximum-radius sphere has finite origin bounds");

    check(
        state,
        maximum_sphere_bounds_result.
            has_value() &&
            maximum_sphere_bounds_result.
                value().
                minimum() ==
                PhysicsVector3{
                    -maximum,
                    -maximum,
                    -maximum
                },
        "Maximum-radius sphere preserves finite minimum");

    check(
        state,
        maximum_sphere_bounds_result.
            has_value() &&
            maximum_sphere_bounds_result.
                value().
                maximum() ==
                PhysicsVector3{
                    maximum,
                    maximum,
                    maximum
                },
        "Maximum-radius sphere preserves finite maximum");

    CollisionShape copied_shape =
        diagonal_capsule;

    check(
        state,
        collision_shape_half_extents(
            copied_shape) ==
            collision_shape_half_extents(
                diagonal_capsule),
        "Copied shape produces equivalent half extents");

    CollisionShape moved_shape =
        std::move(
            copied_shape);

    check(
        state,
        collision_shape_half_extents(
            moved_shape) ==
            collision_shape_half_extents(
                diagonal_capsule),
        "Moved shape produces equivalent half extents");

    return finish(state);
}