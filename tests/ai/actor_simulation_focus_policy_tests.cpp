#include "oros/ai/actor_simulation_focus_policy.hpp"

#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <span>
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
}

int main()
{
    using namespace oros::ai;
    using namespace oros::foundation;
    using namespace oros::world;

    TestState state{};

    const Result<ActorSimulationFocusPolicy>
        negative_promotion =
            ActorSimulationFocusPolicy::create(
                -1.0,
                20.0);

    check(
        state,
        !negative_promotion.has_value() &&
            negative_promotion.error().code ==
                ErrorCode::invalid_argument,
        "Negative promotion distance is rejected");

    const Result<ActorSimulationFocusPolicy>
        negative_retention =
            ActorSimulationFocusPolicy::create(
                10.0,
                -1.0);

    check(
        state,
        !negative_retention.has_value() &&
            negative_retention.error().code ==
                ErrorCode::invalid_argument,
        "Negative retention distance is rejected");

    const Result<ActorSimulationFocusPolicy>
        inverted_hysteresis =
            ActorSimulationFocusPolicy::create(
                21.0,
                20.0);

    check(
        state,
        !inverted_hysteresis.has_value() &&
            inverted_hysteresis.error().code ==
                ErrorCode::invalid_argument,
        "Promotion radius cannot exceed retention radius");

    const double infinity =
        (std::numeric_limits<double>::infinity)();

    const double quiet_nan =
        (std::numeric_limits<double>::quiet_NaN)();

    const Result<ActorSimulationFocusPolicy>
        infinite_policy =
            ActorSimulationFocusPolicy::create(
                infinity,
                infinity);

    check(
        state,
        !infinite_policy.has_value() &&
            infinite_policy.error().code ==
                ErrorCode::invalid_argument,
        "Infinite focus distances are rejected");

    const Result<ActorSimulationFocusPolicy>
        nan_policy =
            ActorSimulationFocusPolicy::create(
                quiet_nan,
                20.0);

    check(
        state,
        !nan_policy.has_value() &&
            nan_policy.error().code ==
                ErrorCode::invalid_argument,
        "NaN focus distances are rejected");

    const Result<ActorSimulationFocusPolicy>
        policy_result =
            ActorSimulationFocusPolicy::create(
                10.0,
                20.0);

    check(
        state,
        policy_result.has_value(),
        "Valid hysteresis policy is created");

    if (!policy_result.has_value())
    {
        std::cerr
            << "Cannot continue policy test.\n";

        return 1;
    }

    const ActorSimulationFocusPolicy policy =
        policy_result.value();

    check(
        state,
        policy.promotion_distance_meters() ==
            10.0,
        "Promotion distance is preserved");

    check(
        state,
        policy.retention_distance_meters() ==
            20.0,
        "Retention distance is preserved");

    const Result<WorldPosition>
        actor_result =
            WorldPosition::origin();

    check(
        state,
        actor_result.has_value(),
        "Actor origin position is available");

    if (!actor_result.has_value())
    {
        return 1;
    }

    const WorldPosition actor_position =
        actor_result.value();

    const std::span<
        const WorldPosition>
        no_focuses{};

    const Result<ActorSimulationFidelity>
        no_focus_distant =
            policy.evaluate(
                ActorSimulationFidelity::
                    statistical_distant,
                actor_position,
                no_focuses);

    check(
        state,
        no_focus_distant.has_value() &&
            no_focus_distant.value() ==
                ActorSimulationFidelity::
                    statistical_distant,
        "No focus keeps a distant actor statistical");

    const Result<ActorSimulationFidelity>
        no_focus_local =
            policy.evaluate(
                ActorSimulationFidelity::
                    deep_local,
                actor_position,
                no_focuses);

    check(
        state,
        no_focus_local.has_value() &&
            no_focus_local.value() ==
                ActorSimulationFidelity::
                    statistical_distant,
        "No focus releases a deep-local actor");

    const Result<ActorSimulationFidelity>
        invalid_current =
            policy.evaluate(
                ActorSimulationFidelity::invalid,
                actor_position,
                no_focuses);

    check(
        state,
        !invalid_current.has_value() &&
            invalid_current.error().code ==
                ErrorCode::invalid_argument,
        "Invalid current fidelity is rejected");

    const Result<WorldPosition>
        promotion_boundary_result =
            WorldPosition::create(
                WorldCell{},
                LocalPosition{
                    10.0,
                    0.0,
                    0.0
                });

    const Result<WorldPosition>
        hysteresis_result =
            WorldPosition::create(
                WorldCell{},
                LocalPosition{
                    15.0,
                    0.0,
                    0.0
                });

    const Result<WorldPosition>
        retention_boundary_result =
            WorldPosition::create(
                WorldCell{},
                LocalPosition{
                    20.0,
                    0.0,
                    0.0
                });

    const Result<WorldPosition>
        outside_retention_result =
            WorldPosition::create(
                WorldCell{},
                LocalPosition{
                    20.25,
                    0.0,
                    0.0
                });

    const Result<WorldPosition>
        far_focus_result =
            WorldPosition::create(
                WorldCell{},
                LocalPosition{
                    100.0,
                    0.0,
                    0.0
                });

    check(
        state,
        promotion_boundary_result.has_value() &&
            hysteresis_result.has_value() &&
            retention_boundary_result.has_value() &&
            outside_retention_result.has_value() &&
            far_focus_result.has_value(),
        "Local focus fixtures are valid");

    if (
        !promotion_boundary_result.has_value() ||
        !hysteresis_result.has_value() ||
        !retention_boundary_result.has_value() ||
        !outside_retention_result.has_value() ||
        !far_focus_result.has_value())
    {
        return 1;
    }

    const std::array<WorldPosition, 1>
        promotion_boundary{
            promotion_boundary_result.value()
        };

    const Result<ActorSimulationFidelity>
        promote_at_boundary =
            policy.evaluate(
                ActorSimulationFidelity::
                    statistical_distant,
                actor_position,
                promotion_boundary);

    check(
        state,
        promote_at_boundary.has_value() &&
            promote_at_boundary.value() ==
                ActorSimulationFidelity::
                    deep_local,
        "Distant actor promotes exactly at promotion boundary");

    const std::array<WorldPosition, 1>
        hysteresis_focus{
            hysteresis_result.value()
        };

    const Result<ActorSimulationFidelity>
        distant_in_hysteresis =
            policy.evaluate(
                ActorSimulationFidelity::
                    statistical_distant,
                actor_position,
                hysteresis_focus);

    check(
        state,
        distant_in_hysteresis.has_value() &&
            distant_in_hysteresis.value() ==
                ActorSimulationFidelity::
                    statistical_distant,
        "Distant actor does not promote inside hysteresis band");

    const Result<ActorSimulationFidelity>
        local_in_hysteresis =
            policy.evaluate(
                ActorSimulationFidelity::
                    deep_local,
                actor_position,
                hysteresis_focus);

    check(
        state,
        local_in_hysteresis.has_value() &&
            local_in_hysteresis.value() ==
                ActorSimulationFidelity::
                    deep_local,
        "Deep-local actor remains deep inside hysteresis band");

    const std::array<WorldPosition, 1>
        retention_boundary{
            retention_boundary_result.value()
        };

    const Result<ActorSimulationFidelity>
        retain_at_boundary =
            policy.evaluate(
                ActorSimulationFidelity::
                    deep_local,
                actor_position,
                retention_boundary);

    check(
        state,
        retain_at_boundary.has_value() &&
            retain_at_boundary.value() ==
                ActorSimulationFidelity::
                    deep_local,
        "Deep-local actor is retained exactly at retention boundary");

    const std::array<WorldPosition, 1>
        outside_retention{
            outside_retention_result.value()
        };

    const Result<ActorSimulationFidelity>
        demote_outside_boundary =
            policy.evaluate(
                ActorSimulationFidelity::
                    deep_local,
                actor_position,
                outside_retention);

    check(
        state,
        demote_outside_boundary.has_value() &&
            demote_outside_boundary.value() ==
                ActorSimulationFidelity::
                    statistical_distant,
        "Deep-local actor demotes beyond retention boundary");

    const std::array<WorldPosition, 2>
        far_then_near{
            far_focus_result.value(),
            promotion_boundary_result.value()
        };

    const std::array<WorldPosition, 2>
        near_then_far{
            promotion_boundary_result.value(),
            far_focus_result.value()
        };

    const Result<ActorSimulationFidelity>
        order_a =
            policy.evaluate(
                ActorSimulationFidelity::
                    statistical_distant,
                actor_position,
                far_then_near);

    const Result<ActorSimulationFidelity>
        order_b =
            policy.evaluate(
                ActorSimulationFidelity::
                    statistical_distant,
                actor_position,
                near_then_far);

    check(
        state,
        order_a.has_value() &&
            order_b.has_value() &&
            order_a.value() ==
                ActorSimulationFidelity::
                    deep_local &&
            order_b.value() ==
                ActorSimulationFidelity::
                    deep_local,
        "Focus ordering cannot change fidelity decision");

    const Result<WorldPosition>
        large_world_actor_result =
            WorldPosition::create(
                WorldCell{
                    1'000'000,
                    0,
                    -1'000'000
                },
                LocalPosition{
                    510.0,
                    0.0,
                    -510.0
                });

    const Result<WorldPosition>
        cross_cell_focus_result =
            WorldPosition::create(
                WorldCell{
                    1'000'001,
                    0,
                    -1'000'001
                },
                LocalPosition{
                    -510.0,
                    0.0,
                    510.0
                });

    check(
        state,
        large_world_actor_result.has_value() &&
            cross_cell_focus_result.has_value(),
        "Large-world cross-cell fixtures are valid");

    if (
        !large_world_actor_result.has_value() ||
        !cross_cell_focus_result.has_value())
    {
        return 1;
    }

    const std::array<WorldPosition, 1>
        cross_cell_focus{
            cross_cell_focus_result.value()
        };

    const Result<ActorSimulationFidelity>
        cross_cell_decision =
            policy.evaluate(
                ActorSimulationFidelity::
                    statistical_distant,
                large_world_actor_result.value(),
                cross_cell_focus);

    check(
        state,
        cross_cell_decision.has_value() &&
            cross_cell_decision.value() ==
                ActorSimulationFidelity::
                    deep_local,
        "Focus policy respects cell-relative large-world distance");

    const Result<ActorSimulationFocusPolicy>
        zero_policy_result =
            ActorSimulationFocusPolicy::create(
                0.0,
                0.0);

    check(
        state,
        zero_policy_result.has_value(),
        "Zero-distance policy is valid");

    if (zero_policy_result.has_value())
    {
        const std::array<WorldPosition, 1>
            coincident_focus{
                actor_position
            };

        const Result<ActorSimulationFidelity>
            coincident_decision =
                zero_policy_result.value().
                    evaluate(
                        ActorSimulationFidelity::
                            statistical_distant,
                        actor_position,
                        coincident_focus);

        check(
            state,
            coincident_decision.has_value() &&
                coincident_decision.value() ==
                    ActorSimulationFidelity::
                        deep_local,
            "Coincident focus promotes with zero radius");
    }

    std::cout
        << state.checks
        << " checks, "
        << state.failures
        << " failures\n";

    return state.failures == 0
        ? 0
        : 1;
}