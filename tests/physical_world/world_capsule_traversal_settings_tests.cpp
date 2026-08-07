#include "oros/physical_world/world_capsule_traversal_settings.hpp"

#include "oros/foundation/error.hpp"
#include "oros/physics/physics_vector.hpp"

#include <iostream>
#include <limits>
#include <string_view>

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

    int finish(
        const TestState& state)
    {
        std::cout
            << "\nWorld capsule traversal settings "
            << "test summary: "
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
    using namespace oros::foundation;
    using namespace oros::physical_world;
    using namespace oros::physics;

    TestState state{};

    const PhysicsUnitVector3 up =
        PhysicsUnitVector3::positive_y();

    const auto valid_result =
        WorldCapsuleTraversalSettings::create(
            up,
            0.75,
            0.5,
            0.25);

    check(
        state,
        valid_result.has_value(),
        "Valid traversal settings are created");

    if (!valid_result.has_value())
    {
        return finish(state);
    }

    const WorldCapsuleTraversalSettings settings =
        valid_result.value();

    check(
        state,
        settings.up_direction() == up,
        "Traversal settings preserve up direction");

    check(
        state,
        settings.minimum_walkable_up_dot() ==
            0.75,
        "Traversal settings preserve walkable up dot");

    check(
        state,
        settings.maximum_step_height() ==
            0.5,
        "Traversal settings preserve maximum step height");

    check(
        state,
        settings.maximum_ground_snap_distance() ==
            0.25,
        "Traversal settings preserve ground snap distance");

    const auto minimum_dot_result =
        WorldCapsuleTraversalSettings::create(
            up,
            1.0,
            0.0,
            0.0);

    check(
        state,
        minimum_dot_result.has_value(),
        "Traversal settings accept walkable up dot one");

    const auto zero_dot_result =
        WorldCapsuleTraversalSettings::create(
            up,
            0.0,
            0.0,
            0.0);

    check(
        state,
        !zero_dot_result.has_value() &&
            zero_dot_result.error().code ==
                ErrorCode::invalid_argument,
        "Traversal settings reject zero walkable up dot");

    const auto negative_dot_result =
        WorldCapsuleTraversalSettings::create(
            up,
            -0.25,
            0.0,
            0.0);

    check(
        state,
        !negative_dot_result.has_value() &&
            negative_dot_result.error().code ==
                ErrorCode::invalid_argument,
        "Traversal settings reject negative walkable up dot");

    const auto excessive_dot_result =
        WorldCapsuleTraversalSettings::create(
            up,
            1.25,
            0.0,
            0.0);

    check(
        state,
        !excessive_dot_result.has_value() &&
            excessive_dot_result.error().code ==
                ErrorCode::invalid_argument,
        "Traversal settings reject walkable up dot above one");

    const auto nan_dot_result =
        WorldCapsuleTraversalSettings::create(
            up,
            std::numeric_limits<
                PhysicsScalar>::
                quiet_NaN(),
            0.0,
            0.0);

    check(
        state,
        !nan_dot_result.has_value() &&
            nan_dot_result.error().code ==
                ErrorCode::invalid_argument,
        "Traversal settings reject NaN walkable up dot");

    const auto infinite_dot_result =
        WorldCapsuleTraversalSettings::create(
            up,
            std::numeric_limits<
                PhysicsScalar>::
                infinity(),
            0.0,
            0.0);

    check(
        state,
        !infinite_dot_result.has_value() &&
            infinite_dot_result.error().code ==
                ErrorCode::invalid_argument,
        "Traversal settings reject infinite walkable up dot");

    const auto zero_step_result =
        WorldCapsuleTraversalSettings::create(
            up,
            0.75,
            0.0,
            0.0);

    check(
        state,
        zero_step_result.has_value(),
        "Traversal settings allow disabled step height");

    const auto negative_step_result =
        WorldCapsuleTraversalSettings::create(
            up,
            0.75,
            -0.25,
            0.0);

    check(
        state,
        !negative_step_result.has_value() &&
            negative_step_result.error().code ==
                ErrorCode::invalid_argument,
        "Traversal settings reject negative step height");

    const auto nan_step_result =
        WorldCapsuleTraversalSettings::create(
            up,
            0.75,
            std::numeric_limits<
                PhysicsScalar>::
                quiet_NaN(),
            0.0);

    check(
        state,
        !nan_step_result.has_value() &&
            nan_step_result.error().code ==
                ErrorCode::invalid_argument,
        "Traversal settings reject NaN step height");

    const auto infinite_step_result =
        WorldCapsuleTraversalSettings::create(
            up,
            0.75,
            std::numeric_limits<
                PhysicsScalar>::
                infinity(),
            0.0);

    check(
        state,
        !infinite_step_result.has_value() &&
            infinite_step_result.error().code ==
                ErrorCode::invalid_argument,
        "Traversal settings reject infinite step height");

    const auto zero_snap_result =
        WorldCapsuleTraversalSettings::create(
            up,
            0.75,
            0.0,
            0.0);

    check(
        state,
        zero_snap_result.has_value(),
        "Traversal settings allow disabled ground snap");

    const auto negative_snap_result =
        WorldCapsuleTraversalSettings::create(
            up,
            0.75,
            0.0,
            -0.25);

    check(
        state,
        !negative_snap_result.has_value() &&
            negative_snap_result.error().code ==
                ErrorCode::invalid_argument,
        "Traversal settings reject negative ground snap distance");

    const auto nan_snap_result =
        WorldCapsuleTraversalSettings::create(
            up,
            0.75,
            0.0,
            std::numeric_limits<
                PhysicsScalar>::
                quiet_NaN());

    check(
        state,
        !nan_snap_result.has_value() &&
            nan_snap_result.error().code ==
                ErrorCode::invalid_argument,
        "Traversal settings reject NaN ground snap distance");

    const auto infinite_snap_result =
        WorldCapsuleTraversalSettings::create(
            up,
            0.75,
            0.0,
            std::numeric_limits<
                PhysicsScalar>::
                infinity());

    check(
        state,
        !infinite_snap_result.has_value() &&
            infinite_snap_result.error().code ==
                ErrorCode::invalid_argument,
        "Traversal settings reject infinite ground snap distance");

    return finish(state);
}