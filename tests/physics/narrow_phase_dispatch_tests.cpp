#include "oros/physics/narrow_phase.hpp"

#include <iostream>
#include <optional>
#include <string_view>
#include <type_traits>
#include <utility>

namespace
{
    using ContactResult =
        oros::foundation::Result<
            std::optional<
                oros::physics::CollisionContact>>;

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

    void check_same_result(
        TestState& state,
        const ContactResult& actual,
        const ContactResult& expected,
        const std::string_view name)
    {
        bool matches =
            actual.has_value() ==
            expected.has_value();

        if (matches &&
            actual.has_value())
        {
            matches =
                actual.value() ==
                expected.value();
        }
        else if (matches)
        {
            matches =
                actual.error().code ==
                    expected.error().code &&
                actual.error().message ==
                    expected.error().message;
        }

        check(
            state,
            matches,
            name);
    }

    int finish(
        const TestState& state)
    {
        std::cout
            << "\nNarrow-phase dispatcher test summary: "
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
    using oros::world::EntityId;

    static_assert(
        std::is_same_v<
            decltype(
                generate_collision_contact(
                    std::declval<
                        const ColliderGeometry&>(),
                    std::declval<
                        const ColliderGeometry&>())),
            ContactResult>);

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

    const auto sphere_result =
        SphereShape::create(
            1.0);

    const auto box_result =
        BoxShape::create(
            PhysicsVector3{
                1.0,
                1.0,
                1.0
            });

    const auto capsule_result =
        CapsuleShape::create(
            0.5,
            1.0,
            PhysicsUnitVector3::positive_y());

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
        capsule_result.has_value(),
        "Capsule fixture is valid");

    if (!sphere_result.has_value() ||
        !box_result.has_value() ||
        !capsule_result.has_value())
    {
        return finish(state);
    }

    const CollisionShape sphere_shape{
        sphere_result.value()
    };

    const CollisionShape box_shape{
        box_result.value()
    };

    const CollisionShape capsule_shape{
        capsule_result.value()
    };

    const auto sphere_a_result =
        ColliderGeometry::create(
            collider_a,
            sphere_shape,
            physics_zero_vector);

    const auto sphere_b_result =
        ColliderGeometry::create(
            collider_b,
            sphere_shape,
            PhysicsVector3{
                1.5,
                0.0,
                0.0
            });

    const auto box_a_result =
        ColliderGeometry::create(
            collider_a,
            box_shape,
            physics_zero_vector);

    const auto box_b_result =
        ColliderGeometry::create(
            collider_b,
            box_shape,
            PhysicsVector3{
                1.5,
                0.0,
                0.0
            });

    const auto capsule_a_result =
        ColliderGeometry::create(
            collider_a,
            capsule_shape,
            physics_zero_vector);

    const auto capsule_b_result =
        ColliderGeometry::create(
            collider_b,
            capsule_shape,
            PhysicsVector3{
                0.75,
                0.0,
                0.0
            });

    check(
        state,
        sphere_a_result.has_value(),
        "Sphere A geometry fixture is valid");

    check(
        state,
        sphere_b_result.has_value(),
        "Sphere B geometry fixture is valid");

    check(
        state,
        box_a_result.has_value(),
        "Box A geometry fixture is valid");

    check(
        state,
        box_b_result.has_value(),
        "Box B geometry fixture is valid");

    check(
        state,
        capsule_a_result.has_value(),
        "Capsule A geometry fixture is valid");

    check(
        state,
        capsule_b_result.has_value(),
        "Capsule B geometry fixture is valid");

    if (!sphere_a_result.has_value() ||
        !sphere_b_result.has_value() ||
        !box_a_result.has_value() ||
        !box_b_result.has_value() ||
        !capsule_a_result.has_value() ||
        !capsule_b_result.has_value())
    {
        return finish(state);
    }

    const ColliderGeometry sphere_a =
        sphere_a_result.value();

    const ColliderGeometry sphere_b =
        sphere_b_result.value();

    const ColliderGeometry box_a =
        box_a_result.value();

    const ColliderGeometry box_b =
        box_b_result.value();

    const ColliderGeometry capsule_a =
        capsule_a_result.value();

    const ColliderGeometry capsule_b =
        capsule_b_result.value();

    check_same_result(
        state,
        generate_collision_contact(
            sphere_a,
            sphere_b),
        generate_sphere_sphere_contact(
            sphere_a,
            sphere_b),
        "Dispatcher routes sphere-sphere");

    check_same_result(
        state,
        generate_collision_contact(
            sphere_b,
            sphere_a),
        generate_sphere_sphere_contact(
            sphere_b,
            sphere_a),
        "Dispatcher routes reversed sphere-sphere");

    check_same_result(
        state,
        generate_collision_contact(
            sphere_a,
            box_b),
        generate_sphere_box_contact(
            sphere_a,
            box_b),
        "Dispatcher routes sphere-box");

    check_same_result(
        state,
        generate_collision_contact(
            box_b,
            sphere_a),
        generate_sphere_box_contact(
            box_b,
            sphere_a),
        "Dispatcher routes box-sphere");

    check_same_result(
        state,
        generate_collision_contact(
            box_a,
            box_b),
        generate_box_box_contact(
            box_a,
            box_b),
        "Dispatcher routes box-box");

    check_same_result(
        state,
        generate_collision_contact(
            box_b,
            box_a),
        generate_box_box_contact(
            box_b,
            box_a),
        "Dispatcher routes reversed box-box");

    check_same_result(
        state,
        generate_collision_contact(
            sphere_a,
            capsule_b),
        generate_sphere_capsule_contact(
            sphere_a,
            capsule_b),
        "Dispatcher routes sphere-capsule");

    check_same_result(
        state,
        generate_collision_contact(
            capsule_b,
            sphere_a),
        generate_sphere_capsule_contact(
            capsule_b,
            sphere_a),
        "Dispatcher routes capsule-sphere");

    check_same_result(
        state,
        generate_collision_contact(
            capsule_a,
            box_b),
        generate_capsule_box_contact(
            capsule_a,
            box_b),
        "Dispatcher routes capsule-box");

    check_same_result(
        state,
        generate_collision_contact(
            box_b,
            capsule_a),
        generate_capsule_box_contact(
            box_b,
            capsule_a),
        "Dispatcher routes box-capsule");

    check_same_result(
        state,
        generate_collision_contact(
            capsule_a,
            capsule_b),
        generate_capsule_capsule_contact(
            capsule_a,
            capsule_b),
        "Dispatcher routes capsule-capsule");

    check_same_result(
        state,
        generate_collision_contact(
            capsule_b,
            capsule_a),
        generate_capsule_capsule_contact(
            capsule_b,
            capsule_a),
        "Dispatcher routes reversed capsule-capsule");

    check_same_result(
        state,
        generate_collision_contact(
            sphere_a,
            sphere_a),
        generate_sphere_sphere_contact(
            sphere_a,
            sphere_a),
        "Dispatcher preserves delegated identity validation");

    return finish(state);
}