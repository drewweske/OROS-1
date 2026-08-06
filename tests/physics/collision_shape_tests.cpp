#include "oros/physics/collision_shape.hpp"

#include "oros/foundation/error.hpp"

#include <cstdint>
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
            << "\nCollision shape test summary: "
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
            std::underlying_type_t<
                CollisionShapeKind>,
            std::uint8_t>);

    static_assert(
        !std::is_default_constructible_v<
            SphereShape>);

    static_assert(
        !std::is_default_constructible_v<
            BoxShape>);

    static_assert(
        !std::is_default_constructible_v<
            CapsuleShape>);

    static_assert(
        std::is_copy_constructible_v<
            SphereShape>);

    static_assert(
        std::is_copy_constructible_v<
            BoxShape>);

    static_assert(
        std::is_copy_constructible_v<
            CapsuleShape>);

    static_assert(
        std::is_move_constructible_v<
            SphereShape>);

    static_assert(
        std::is_move_constructible_v<
            BoxShape>);

    static_assert(
        std::is_move_constructible_v<
            CapsuleShape>);

    static_assert(
        std::variant_size_v<
            CollisionShape> ==
            3U);

    TestState state{};

    check_failure(
        state,
        SphereShape::create(
            0.0),
        ErrorCode::invalid_argument,
        "Sphere rejects zero radius");

    check_failure(
        state,
        SphereShape::create(
            -1.0),
        ErrorCode::invalid_argument,
        "Sphere rejects negative radius");

    check_failure(
        state,
        SphereShape::create(
            std::numeric_limits<
                PhysicsScalar>::infinity()),
        ErrorCode::invalid_argument,
        "Sphere rejects infinite radius");

    check_failure(
        state,
        SphereShape::create(
            std::numeric_limits<
                PhysicsScalar>::quiet_NaN()),
        ErrorCode::invalid_argument,
        "Sphere rejects NaN radius");

    const auto sphere_result =
        SphereShape::create(
            2.5);

    check(
        state,
        sphere_result.has_value(),
        "Sphere accepts finite positive radius");

    if (!sphere_result.has_value())
    {
        return finish(state);
    }

    const SphereShape sphere =
        sphere_result.value();

    check(
        state,
        nearly_equal(
            sphere.radius(),
            2.5),
        "Sphere preserves radius");

    check(
        state,
        sphere.kind() ==
            CollisionShapeKind::sphere,
        "Sphere reports sphere kind");

    const PhysicsScalar maximum =
        std::numeric_limits<
            PhysicsScalar>::max();

    const auto maximum_sphere_result =
        SphereShape::create(
            maximum);

    check(
        state,
        maximum_sphere_result.
            has_value(),
        "Sphere accepts maximum finite radius");

    check(
        state,
        maximum_sphere_result.
            has_value() &&
            maximum_sphere_result.
                value().
                radius() ==
                maximum,
        "Maximum sphere radius is preserved");

    check_failure(
        state,
        BoxShape::create(
            PhysicsVector3{
                0.0,
                1.0,
                1.0
            }),
        ErrorCode::invalid_argument,
        "Box rejects zero X half extent");

    check_failure(
        state,
        BoxShape::create(
            PhysicsVector3{
                1.0,
                0.0,
                1.0
            }),
        ErrorCode::invalid_argument,
        "Box rejects zero Y half extent");

    check_failure(
        state,
        BoxShape::create(
            PhysicsVector3{
                1.0,
                1.0,
                0.0
            }),
        ErrorCode::invalid_argument,
        "Box rejects zero Z half extent");

    check_failure(
        state,
        BoxShape::create(
            PhysicsVector3{
                -1.0,
                1.0,
                1.0
            }),
        ErrorCode::invalid_argument,
        "Box rejects negative half extent");

    check_failure(
        state,
        BoxShape::create(
            PhysicsVector3{
                std::numeric_limits<
                    PhysicsScalar>::infinity(),
                1.0,
                1.0
            }),
        ErrorCode::invalid_argument,
        "Box rejects infinite half extent");

    check_failure(
        state,
        BoxShape::create(
            PhysicsVector3{
                1.0,
                std::numeric_limits<
                    PhysicsScalar>::quiet_NaN(),
                1.0
            }),
        ErrorCode::invalid_argument,
        "Box rejects NaN half extent");

    const PhysicsVector3
        box_half_extents{
            1.0,
            2.0,
            3.0
        };

    const auto box_result =
        BoxShape::create(
            box_half_extents);

    check(
        state,
        box_result.has_value(),
        "Box accepts finite positive half extents");

    if (!box_result.has_value())
    {
        return finish(state);
    }

    const BoxShape box =
        box_result.value();

    check(
        state,
        box.half_extents() ==
            box_half_extents,
        "Box preserves half extents");

    check(
        state,
        box.kind() ==
            CollisionShapeKind::box,
        "Box reports box kind");

    const auto maximum_box_result =
        BoxShape::create(
            PhysicsVector3{
                maximum,
                maximum,
                maximum
            });

    check(
        state,
        maximum_box_result.
            has_value(),
        "Box accepts maximum finite half extents");

    check(
        state,
        maximum_box_result.
            has_value() &&
            maximum_box_result.
                value().
                half_extents() ==
                PhysicsVector3{
                    maximum,
                    maximum,
                    maximum
                },
        "Maximum box half extents are preserved");

    const PhysicsUnitVector3
        positive_y =
            PhysicsUnitVector3::
                positive_y();

    check_failure(
        state,
        CapsuleShape::create(
            0.0,
            1.0,
            positive_y),
        ErrorCode::invalid_argument,
        "Capsule rejects zero radius");

    check_failure(
        state,
        CapsuleShape::create(
            -1.0,
            1.0,
            positive_y),
        ErrorCode::invalid_argument,
        "Capsule rejects negative radius");

    check_failure(
        state,
        CapsuleShape::create(
            std::numeric_limits<
                PhysicsScalar>::infinity(),
            1.0,
            positive_y),
        ErrorCode::invalid_argument,
        "Capsule rejects infinite radius");

    check_failure(
        state,
        CapsuleShape::create(
            std::numeric_limits<
                PhysicsScalar>::quiet_NaN(),
            1.0,
            positive_y),
        ErrorCode::invalid_argument,
        "Capsule rejects NaN radius");

    check_failure(
        state,
        CapsuleShape::create(
            1.0,
            -1.0,
            positive_y),
        ErrorCode::invalid_argument,
        "Capsule rejects negative half-segment length");

    check_failure(
        state,
        CapsuleShape::create(
            1.0,
            std::numeric_limits<
                PhysicsScalar>::infinity(),
            positive_y),
        ErrorCode::invalid_argument,
        "Capsule rejects infinite half-segment length");

    check_failure(
        state,
        CapsuleShape::create(
            1.0,
            std::numeric_limits<
                PhysicsScalar>::quiet_NaN(),
            positive_y),
        ErrorCode::invalid_argument,
        "Capsule rejects NaN half-segment length");

    check_failure(
        state,
        CapsuleShape::create(
            maximum,
            maximum,
            positive_y),
        ErrorCode::invalid_argument,
        "Capsule rejects overflowing total half height");

    const auto zero_segment_capsule_result =
        CapsuleShape::create(
            1.0,
            0.0,
            positive_y);

    check(
        state,
        zero_segment_capsule_result.
            has_value(),
        "Capsule accepts zero half-segment length");

    check(
        state,
        zero_segment_capsule_result.
            has_value() &&
            nearly_equal(
                zero_segment_capsule_result.
                    value().
                    total_half_height(),
                1.0),
        "Zero-segment capsule half height equals radius");

    const auto capsule_result =
        CapsuleShape::create(
            0.5,
            1.25,
            positive_y);

    check(
        state,
        capsule_result.has_value(),
        "Capsule accepts valid dimensions and axis");

    if (!capsule_result.has_value())
    {
        return finish(state);
    }

    const CapsuleShape capsule =
        capsule_result.value();

    check(
        state,
        nearly_equal(
            capsule.radius(),
            0.5),
        "Capsule preserves radius");

    check(
        state,
        nearly_equal(
            capsule.
                half_segment_length(),
            1.25),
        "Capsule preserves half-segment length");

    check(
        state,
        nearly_equal(
            capsule.
                total_half_height(),
            1.75),
        "Capsule calculates total half height");

    check(
        state,
        capsule.axis() ==
            positive_y,
        "Capsule preserves unit axis");

    check(
        state,
        capsule.axis().is_valid(),
        "Capsule axis remains valid");

    check(
        state,
        capsule.kind() ==
            CollisionShapeKind::capsule,
        "Capsule reports capsule kind");

    const auto maximum_capsule_result =
        CapsuleShape::create(
            maximum,
            0.0,
            positive_y);

    check(
        state,
        maximum_capsule_result.
            has_value(),
        "Capsule accepts maximum finite radius with zero segment");

    check(
        state,
        maximum_capsule_result.
            has_value() &&
            maximum_capsule_result.
                value().
                total_half_height() ==
                maximum,
        "Maximum capsule half height remains finite");

    SphereShape copied_sphere =
        sphere;

    check(
        state,
        copied_sphere ==
            sphere,
        "Sphere copy preserves value");

    SphereShape moved_sphere =
        std::move(
            copied_sphere);

    check(
        state,
        moved_sphere ==
            sphere,
        "Sphere move preserves value");

    BoxShape copied_box =
        box;

    check(
        state,
        copied_box ==
            box,
        "Box copy preserves value");

    BoxShape moved_box =
        std::move(
            copied_box);

    check(
        state,
        moved_box ==
            box,
        "Box move preserves value");

    CapsuleShape copied_capsule =
        capsule;

    check(
        state,
        copied_capsule ==
            capsule,
        "Capsule copy preserves value");

    CapsuleShape moved_capsule =
        std::move(
            copied_capsule);

    check(
        state,
        moved_capsule ==
            capsule,
        "Capsule move preserves value");

    const CollisionShape
        sphere_shape{
            sphere
        };

    const CollisionShape
        box_shape{
            box
        };

    const CollisionShape
        capsule_shape{
            capsule
        };

    check(
        state,
        std::holds_alternative<
            SphereShape>(
            sphere_shape),
        "Collision shape stores sphere alternative");

    check(
        state,
        std::holds_alternative<
            BoxShape>(
            box_shape),
        "Collision shape stores box alternative");

    check(
        state,
        std::holds_alternative<
            CapsuleShape>(
            capsule_shape),
        "Collision shape stores capsule alternative");

    check(
        state,
        std::get<SphereShape>(
            sphere_shape) ==
            sphere,
        "Sphere variant preserves shape value");

    check(
        state,
        std::get<BoxShape>(
            box_shape) ==
            box,
        "Box variant preserves shape value");

    check(
        state,
        std::get<CapsuleShape>(
            capsule_shape) ==
            capsule,
        "Capsule variant preserves shape value");

    check(
        state,
        collision_shape_kind(
            sphere_shape) ==
            CollisionShapeKind::sphere,
        "Variant dispatch reports sphere kind");

    check(
        state,
        collision_shape_kind(
            box_shape) ==
            CollisionShapeKind::box,
        "Variant dispatch reports box kind");

    check(
        state,
        collision_shape_kind(
            capsule_shape) ==
            CollisionShapeKind::capsule,
        "Variant dispatch reports capsule kind");

    CollisionShape copied_shape =
        capsule_shape;

    check(
        state,
        copied_shape ==
            capsule_shape,
        "Collision shape copy preserves alternative and value");

    CollisionShape moved_shape =
        std::move(
            copied_shape);

    check(
        state,
        moved_shape ==
            capsule_shape,
        "Collision shape move preserves alternative and value");

    return finish(state);
}