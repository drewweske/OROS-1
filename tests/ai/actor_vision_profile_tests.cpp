#include "oros/ai/actor_vision_profile.hpp"

#include "oros/foundation/error.hpp"

#include <cmath>
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
            << "\nActor vision profile test summary: "
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
    using namespace oros::ai;
    using namespace oros::foundation;
    using namespace oros::world;

    TestState state{};

    check(
        state,
        !is_valid_actor_vision_field_state(
            ActorVisionFieldState::invalid) &&
            is_valid_actor_vision_field_state(
                ActorVisionFieldState::
                    within_vision_field) &&
            is_valid_actor_vision_field_state(
                ActorVisionFieldState::
                    out_of_range) &&
            is_valid_actor_vision_field_state(
                ActorVisionFieldState::
                    outside_field_of_view),
        "Vision field states expose an explicit invalid sentinel");

    const double infinity =
        (std::numeric_limits<double>::
            infinity)();

    const double quiet_nan =
        (std::numeric_limits<double>::
            quiet_NaN)();

    const Result<ActorVisionDirection>
        zero_direction_result =
            ActorVisionDirection::create(
                WorldDisplacement{});

    check(
        state,
        !zero_direction_result.has_value() &&
            zero_direction_result.error().code ==
                ErrorCode::invalid_argument,
        "Zero vision direction is rejected");

    const Result<ActorVisionDirection>
        nan_direction_result =
            ActorVisionDirection::create(
                WorldDisplacement{
                    quiet_nan,
                    0.0,
                    0.0
                });

    check(
        state,
        !nan_direction_result.has_value() &&
            nan_direction_result.error().code ==
                ErrorCode::invalid_argument,
        "NaN vision direction is rejected");

    const Result<ActorVisionDirection>
        infinite_direction_result =
            ActorVisionDirection::create(
                WorldDisplacement{
                    infinity,
                    0.0,
                    0.0
                });

    check(
        state,
        !infinite_direction_result.has_value() &&
            infinite_direction_result.error().code ==
                ErrorCode::invalid_argument,
        "Infinite vision direction is rejected");

    const Result<ActorVisionDirection>
        forward_result =
            ActorVisionDirection::create(
                WorldDisplacement{
                    2.0,
                    0.0,
                    0.0
                });

    check(
        state,
        forward_result.has_value(),
        "Finite non-zero vision direction is created");

    if (!forward_result.has_value())
    {
        return finish(state);
    }

    const ActorVisionDirection forward =
        forward_result.value();

    check(
        state,
        forward.is_valid() &&
            forward.vector().x == 1.0 &&
            forward.vector().y == 0.0 &&
            forward.vector().z == 0.0,
        "Vision direction is stored as a normalized unit vector");

    const Result<ActorVisionDirection>
        diagonal_result =
            ActorVisionDirection::create(
                WorldDisplacement{
                    3.0,
                    4.0,
                    0.0
                });

    check(
        state,
        diagonal_result.has_value() &&
            diagonal_result.value().is_valid() &&
            std::abs(
                diagonal_result.value().
                    vector().x -
                0.6) <= 1.0e-12 &&
            std::abs(
                diagonal_result.value().
                    vector().y -
                0.8) <= 1.0e-12,
        "Arbitrary facing vectors normalize deterministically");

    const Result<ActorVisionProfile>
        negative_range_result =
            ActorVisionProfile::create(
                -1.0,
                90.0);

    check(
        state,
        !negative_range_result.has_value() &&
            negative_range_result.error().code ==
                ErrorCode::invalid_argument,
        "Negative vision range is rejected");

    const Result<ActorVisionProfile>
        negative_fov_result =
            ActorVisionProfile::create(
                10.0,
                -1.0);

    check(
        state,
        !negative_fov_result.has_value() &&
            negative_fov_result.error().code ==
                ErrorCode::invalid_argument,
        "Negative field of view is rejected");

    const Result<ActorVisionProfile>
        oversized_fov_result =
            ActorVisionProfile::create(
                10.0,
                360.001);

    check(
        state,
        !oversized_fov_result.has_value() &&
            oversized_fov_result.error().code ==
                ErrorCode::invalid_argument,
        "Field of view above 360 degrees is rejected");

    const Result<ActorVisionProfile>
        nan_range_result =
            ActorVisionProfile::create(
                quiet_nan,
                90.0);

    check(
        state,
        !nan_range_result.has_value() &&
            nan_range_result.error().code ==
                ErrorCode::invalid_argument,
        "NaN vision range is rejected");

    const Result<ActorVisionProfile>
        infinite_fov_result =
            ActorVisionProfile::create(
                10.0,
                infinity);

    check(
        state,
        !infinite_fov_result.has_value() &&
            infinite_fov_result.error().code ==
                ErrorCode::invalid_argument,
        "Infinite field of view is rejected");

    const Result<ActorVisionProfile>
        profile_result =
            ActorVisionProfile::create(
                10.0,
                90.0);

    const Result<ActorVisionProfile>
        zero_profile_result =
            ActorVisionProfile::create(
                0.0,
                0.0);

    const Result<ActorVisionProfile>
        half_sphere_profile_result =
            ActorVisionProfile::create(
                10.0,
                180.0);

    const Result<ActorVisionProfile>
        full_sphere_profile_result =
            ActorVisionProfile::create(
                10.0,
                360.0);

    const Result<ActorVisionProfile>
        zero_fov_profile_result =
            ActorVisionProfile::create(
                10.0,
                0.0);

    check(
        state,
        profile_result.has_value() &&
            zero_profile_result.has_value() &&
            half_sphere_profile_result.has_value() &&
            full_sphere_profile_result.has_value() &&
            zero_fov_profile_result.has_value(),
        "Valid and boundary vision profiles are created");

    if (
        !profile_result.has_value() ||
        !zero_profile_result.has_value() ||
        !half_sphere_profile_result.has_value() ||
        !full_sphere_profile_result.has_value() ||
        !zero_fov_profile_result.has_value())
    {
        return finish(state);
    }

    const ActorVisionProfile profile =
        profile_result.value();

    check(
        state,
        profile.is_valid() &&
            profile.maximum_range_meters() ==
                10.0 &&
            profile.field_of_view_degrees() ==
                90.0 &&
            profile.minimum_forward_dot() >
                0.70 &&
            profile.minimum_forward_dot() <
                0.71,
        "Vision profile preserves range and total cone FOV");

    check(
        state,
        half_sphere_profile_result.value().
                minimum_forward_dot() ==
                0.0 &&
            full_sphere_profile_result.value().
                minimum_forward_dot() ==
                -1.0 &&
            zero_fov_profile_result.value().
                minimum_forward_dot() ==
                1.0,
        "Canonical 0/180/360 degree thresholds are exact");

    const Result<WorldPosition>
        origin_result =
            WorldPosition::origin();

    const Result<WorldPosition>
        front_result =
            WorldPosition::create(
                WorldCell{},
                LocalPosition{
                    5.0,
                    0.0,
                    0.0
                });

    const Result<WorldPosition>
        range_boundary_result =
            WorldPosition::create(
                WorldCell{},
                LocalPosition{
                    10.0,
                    0.0,
                    0.0
                });

    const Result<WorldPosition>
        beyond_range_result =
            WorldPosition::create(
                WorldCell{},
                LocalPosition{
                    10.001,
                    0.0,
                    0.0
                });

    const Result<WorldPosition>
        side_result =
            WorldPosition::create(
                WorldCell{},
                LocalPosition{
                    0.0,
                    5.0,
                    0.0
                });

    const Result<WorldPosition>
        behind_result =
            WorldPosition::create(
                WorldCell{},
                LocalPosition{
                    -5.0,
                    0.0,
                    0.0
                });

    const Result<WorldPosition>
        behind_distant_result =
            WorldPosition::create(
                WorldCell{},
                LocalPosition{
                    -20.0,
                    0.0,
                    0.0
                });

    const Result<WorldPosition>
        slight_lateral_result =
            WorldPosition::create(
                WorldCell{},
                LocalPosition{
                    5.0,
                    0.001,
                    0.0
                });

    check(
        state,
        origin_result.has_value() &&
            front_result.has_value() &&
            range_boundary_result.has_value() &&
            beyond_range_result.has_value() &&
            side_result.has_value() &&
            behind_result.has_value() &&
            behind_distant_result.has_value() &&
            slight_lateral_result.has_value(),
        "Vision classification fixtures are created");

    if (
        !origin_result.has_value() ||
        !front_result.has_value() ||
        !range_boundary_result.has_value() ||
        !beyond_range_result.has_value() ||
        !side_result.has_value() ||
        !behind_result.has_value() ||
        !behind_distant_result.has_value() ||
        !slight_lateral_result.has_value())
    {
        return finish(state);
    }

    const WorldPosition origin =
        origin_result.value();

    const Result<ActorVisionFieldState>
        frontState =
            profile.classify(
                origin,
                forward,
                front_result.value());

    check(
        state,
        frontState.has_value() &&
            frontState.value() ==
                ActorVisionFieldState::
                    within_vision_field,
        "Target in front and inside range enters the vision field");

    const Result<ActorVisionFieldState>
        rangeBoundaryState =
            profile.classify(
                origin,
                forward,
                range_boundary_result.value());

    check(
        state,
        rangeBoundaryState.has_value() &&
            rangeBoundaryState.value() ==
                ActorVisionFieldState::
                    within_vision_field,
        "Maximum vision range is inclusive");

    const Result<ActorVisionFieldState>
        beyondRangeState =
            profile.classify(
                origin,
                forward,
                beyond_range_result.value());

    check(
        state,
        beyondRangeState.has_value() &&
            beyondRangeState.value() ==
                ActorVisionFieldState::
                    out_of_range,
        "Target beyond maximum range is classified out of range");

    const Result<ActorVisionFieldState>
        sideState =
            profile.classify(
                origin,
                forward,
                side_result.value());

    check(
        state,
        sideState.has_value() &&
            sideState.value() ==
                ActorVisionFieldState::
                    outside_field_of_view,
        "Orthogonal target is outside a 90-degree vision cone");

    const Result<ActorVisionFieldState>
        behindState =
            profile.classify(
                origin,
                forward,
                behind_result.value());

    check(
        state,
        behindState.has_value() &&
            behindState.value() ==
                ActorVisionFieldState::
                    outside_field_of_view,
        "Target behind observer is outside a 90-degree vision cone");

    const Result<ActorVisionFieldState>
        distantPrecedenceState =
            profile.classify(
                origin,
                forward,
                behind_distant_result.value());

    check(
        state,
        distantPrecedenceState.has_value() &&
            distantPrecedenceState.value() ==
                ActorVisionFieldState::
                    out_of_range,
        "Range classification deterministically precedes FOV classification");

    const Result<ActorVisionFieldState>
        coincidentState =
            zero_profile_result.value().
                classify(
                    origin,
                    forward,
                    origin);

    check(
        state,
        coincidentState.has_value() &&
            coincidentState.value() ==
                ActorVisionFieldState::
                    within_vision_field,
        "Coincident target is inside even a zero-range profile");

    const Result<ActorVisionFieldState>
        halfSphereSideState =
            half_sphere_profile_result.value().
                classify(
                    origin,
                    forward,
                    side_result.value());

    check(
        state,
        halfSphereSideState.has_value() &&
            halfSphereSideState.value() ==
                ActorVisionFieldState::
                    within_vision_field,
        "180-degree FOV includes its orthogonal boundary");

    const Result<ActorVisionFieldState>
        fullSphereBehindState =
            full_sphere_profile_result.value().
                classify(
                    origin,
                    forward,
                    behind_result.value());

    check(
        state,
        fullSphereBehindState.has_value() &&
            fullSphereBehindState.value() ==
                ActorVisionFieldState::
                    within_vision_field,
        "360-degree FOV includes a target directly behind");

    const Result<ActorVisionFieldState>
        zeroFovFrontState =
            zero_fov_profile_result.value().
                classify(
                    origin,
                    forward,
                    front_result.value());

    check(
        state,
        zeroFovFrontState.has_value() &&
            zeroFovFrontState.value() ==
                ActorVisionFieldState::
                    within_vision_field,
        "Zero-degree FOV includes an exactly forward target");

    const Result<ActorVisionFieldState>
        zeroFovLateralState =
            zero_fov_profile_result.value().
                classify(
                    origin,
                    forward,
                    slight_lateral_result.value());

    check(
        state,
        zeroFovLateralState.has_value() &&
            zeroFovLateralState.value() ==
                ActorVisionFieldState::
                    outside_field_of_view,
        "Zero-degree FOV rejects any lateral target offset");

    const Result<WorldPosition>
        boundary_observer_result =
            WorldPosition::create(
                WorldCell{},
                LocalPosition{
                    500.0,
                    0.0,
                    0.0
                });

    const Result<WorldPosition>
        boundary_target_result =
            WorldPosition::create(
                WorldCell{
                    1,
                    0,
                    0
                },
                LocalPosition{
                    -500.0,
                    0.0,
                    0.0
                });

    const Result<ActorVisionProfile>
        boundary_profile_result =
            ActorVisionProfile::create(
                24.0,
                0.0);

    check(
        state,
        boundary_observer_result.has_value() &&
            boundary_target_result.has_value() &&
            boundary_profile_result.has_value(),
        "Cross-cell vision fixtures are created");

    if (
        !boundary_observer_result.has_value() ||
        !boundary_target_result.has_value() ||
        !boundary_profile_result.has_value())
    {
        return finish(state);
    }

    const Result<ActorVisionFieldState>
        boundaryState =
            boundary_profile_result.value().
                classify(
                    boundary_observer_result.value(),
                    forward,
                    boundary_target_result.value());

    check(
        state,
        boundaryState.has_value() &&
            boundaryState.value() ==
                ActorVisionFieldState::
                    within_vision_field,
        "Vision classification uses WorldPosition displacement across cell boundaries");

    return finish(state);
}