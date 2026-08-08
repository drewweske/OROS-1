#include "oros/ai/actor_simulation_focus_decision.hpp"

#include <iostream>
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
        policy_result =
            ActorSimulationFocusPolicy::create(
                10.0,
                20.0);

    check(
        state,
        policy_result.has_value(),
        "Focus decision policy is valid");

    const Result<WorldPosition>
        actor_position_result =
            WorldPosition::origin();

    const Result<WorldPosition>
        near_position_result =
            WorldPosition::create(
                WorldCell{},
                LocalPosition{
                    10.0,
                    0.0,
                    0.0
                });

    const Result<WorldPosition>
        hysteresis_position_result =
            WorldPosition::create(
                WorldCell{},
                LocalPosition{
                    15.0,
                    0.0,
                    0.0
                });

    const Result<WorldPosition>
        far_position_result =
            WorldPosition::create(
                WorldCell{},
                LocalPosition{
                    100.0,
                    0.0,
                    0.0
                });

    check(
        state,
        actor_position_result.has_value() &&
            near_position_result.has_value() &&
            hysteresis_position_result.has_value() &&
            far_position_result.has_value(),
        "Focus decision position fixtures are valid");

    if (
        !policy_result.has_value() ||
        !actor_position_result.has_value() ||
        !near_position_result.has_value() ||
        !hysteresis_position_result.has_value() ||
        !far_position_result.has_value())
    {
        return 1;
    }

    const ActorSimulationFocusPolicy&
        policy =
            policy_result.value();

    const WorldPosition actor_position =
        actor_position_result.value();

    const EntityId actor{
        0x4F524F53ULL,
        101ULL
    };

    const Result<ActorSimulationFidelityState>
        distant_state_result =
            ActorSimulationFidelityState::create(
                actor,
                ActorSimulationFidelity::
                    statistical_distant);

    const Result<ActorSimulationFidelityState>
        local_state_result =
            ActorSimulationFidelityState::create(
                actor,
                ActorSimulationFidelity::
                    deep_local);

    check(
        state,
        distant_state_result.has_value() &&
            local_state_result.has_value(),
        "Actor fidelity-state fixtures are valid");

    if (
        !distant_state_result.has_value() ||
        !local_state_result.has_value())
    {
        return 1;
    }

    ActorSimulationFocusSourceRegistry
        no_sources{};

    const Result<ActorSimulationFocusDecision>
        empty_distant_decision =
            ActorSimulationFocusDecision::evaluate(
                policy,
                distant_state_result.value(),
                actor_position,
                no_sources);

    check(
        state,
        empty_distant_decision.has_value(),
        "Distant actor evaluates with no focus sources");

    check(
        state,
        empty_distant_decision.has_value() &&
            empty_distant_decision.value().
                actor() ==
                actor,
        "Focus decision preserves persistent actor identity");

    check(
        state,
        empty_distant_decision.has_value() &&
            empty_distant_decision.value().
                current_fidelity() ==
                ActorSimulationFidelity::
                    statistical_distant,
        "Decision records current distant fidelity");

    check(
        state,
        empty_distant_decision.has_value() &&
            empty_distant_decision.value().
                focus_fidelity() ==
                ActorSimulationFidelity::
                    statistical_distant &&
            empty_distant_decision.value().
                transition() ==
                ActorSimulationFidelityTransition::
                    unchanged,
        "No focus leaves distant actor unchanged");

    const Result<ActorSimulationFocusDecision>
        empty_local_decision =
            ActorSimulationFocusDecision::evaluate(
                policy,
                local_state_result.value(),
                actor_position,
                no_sources);

    check(
        state,
        empty_local_decision.has_value() &&
            empty_local_decision.value().
                focus_fidelity() ==
                ActorSimulationFidelity::
                    statistical_distant &&
            empty_local_decision.value().
                transition() ==
                ActorSimulationFidelityTransition::
                    demotion_to_statistical_distant,
        "No focus recommends demotion for deep-local actor");

    check(
        state,
        local_state_result.value().fidelity() ==
            ActorSimulationFidelity::
                deep_local,
        "Decision evaluation does not mutate actor fidelity state");

    const EntityId far_source{
        1ULL,
        20ULL
    };

    const EntityId near_source{
        1ULL,
        10ULL
    };

    ActorSimulationFocusSourceRegistry
        far_only{};

    const Status insert_far =
        far_only.insert(
            far_source,
            far_position_result.value());

    check(
        state,
        insert_far.has_value(),
        "Far focus source inserts");

    const Result<ActorSimulationFocusDecision>
        far_distant_decision =
            ActorSimulationFocusDecision::evaluate(
                policy,
                distant_state_result.value(),
                actor_position,
                far_only);

    check(
        state,
        far_distant_decision.has_value() &&
            far_distant_decision.value().
                focus_fidelity() ==
                ActorSimulationFidelity::
                    statistical_distant &&
            far_distant_decision.value().
                transition() ==
                ActorSimulationFidelityTransition::
                    unchanged,
        "Far focus does not promote distant actor");

    const Status update_far_to_near =
        far_only.update_position(
            far_source,
            near_position_result.value());

    check(
        state,
        update_far_to_near.has_value(),
        "Focus-source position update succeeds");

    const Result<ActorSimulationFocusDecision>
        updated_near_decision =
            ActorSimulationFocusDecision::evaluate(
                policy,
                distant_state_result.value(),
                actor_position,
                far_only);

    check(
        state,
        updated_near_decision.has_value() &&
            updated_near_decision.value().
                focus_fidelity() ==
                ActorSimulationFidelity::
                    deep_local &&
            updated_near_decision.value().
                transition() ==
                ActorSimulationFidelityTransition::
                    promotion_to_deep_local,
        "Updated near focus recommends explicit promotion");

    check(
        state,
        distant_state_result.value().fidelity() ==
            ActorSimulationFidelity::
                statistical_distant,
        "Promotion decision remains read-only");

    ActorSimulationFocusSourceRegistry
        hysteresis_sources{};

    const Status insert_hysteresis =
        hysteresis_sources.insert(
            near_source,
            hysteresis_position_result.value());

    check(
        state,
        insert_hysteresis.has_value(),
        "Hysteresis-band focus source inserts");

    const Result<ActorSimulationFocusDecision>
        hysteresis_distant =
            ActorSimulationFocusDecision::evaluate(
                policy,
                distant_state_result.value(),
                actor_position,
                hysteresis_sources);

    const Result<ActorSimulationFocusDecision>
        hysteresis_local =
            ActorSimulationFocusDecision::evaluate(
                policy,
                local_state_result.value(),
                actor_position,
                hysteresis_sources);

    check(
        state,
        hysteresis_distant.has_value() &&
            hysteresis_distant.value().
                focus_fidelity() ==
                ActorSimulationFidelity::
                    statistical_distant &&
            hysteresis_distant.value().
                transition() ==
                ActorSimulationFidelityTransition::
                    unchanged,
        "Hysteresis band does not promote distant actor");

    check(
        state,
        hysteresis_local.has_value() &&
            hysteresis_local.value().
                focus_fidelity() ==
                ActorSimulationFidelity::
                    deep_local &&
            hysteresis_local.value().
                transition() ==
                ActorSimulationFidelityTransition::
                    unchanged,
        "Hysteresis band retains deep-local actor");

    ActorSimulationFocusSourceRegistry
        insertion_order_a{};

    const Status a_far =
        insertion_order_a.insert(
            far_source,
            far_position_result.value());

    const Status a_near =
        insertion_order_a.insert(
            near_source,
            near_position_result.value());

    ActorSimulationFocusSourceRegistry
        insertion_order_b{};

    const Status b_near =
        insertion_order_b.insert(
            near_source,
            near_position_result.value());

    const Status b_far =
        insertion_order_b.insert(
            far_source,
            far_position_result.value());

    check(
        state,
        a_far.has_value() &&
            a_near.has_value() &&
            b_near.has_value() &&
            b_far.has_value(),
        "Equivalent focus registries accept different insertion histories");

    const Result<ActorSimulationFocusDecision>
        order_a =
            ActorSimulationFocusDecision::evaluate(
                policy,
                distant_state_result.value(),
                actor_position,
                insertion_order_a);

    const Result<ActorSimulationFocusDecision>
        order_b =
            ActorSimulationFocusDecision::evaluate(
                policy,
                distant_state_result.value(),
                actor_position,
                insertion_order_b);

    check(
        state,
        order_a.has_value() &&
            order_b.has_value() &&
            order_a.value().actor() ==
                order_b.value().actor() &&
            order_a.value().current_fidelity() ==
                order_b.value().current_fidelity() &&
            order_a.value().focus_fidelity() ==
                order_b.value().focus_fidelity() &&
            order_a.value().transition() ==
                order_b.value().transition(),
        "Focus decision is independent of source insertion history");

    check(
        state,
        insertion_order_a.size() == 2U &&
            insertion_order_b.size() == 2U,
        "Decision evaluation does not mutate focus-source registries");

    const Result<WorldPosition>
        large_actor_result =
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
        large_focus_result =
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
        large_actor_result.has_value() &&
            large_focus_result.has_value(),
        "Large-world decision fixtures are valid");

    if (
        large_actor_result.has_value() &&
        large_focus_result.has_value())
    {
        ActorSimulationFocusSourceRegistry
            large_sources{};

        const Status insert_large =
            large_sources.insert(
                EntityId{
                    9ULL,
                    1ULL
                },
                large_focus_result.value());

        check(
            state,
            insert_large.has_value(),
            "Large-world focus source inserts");

        const Result<ActorSimulationFocusDecision>
            large_decision =
                ActorSimulationFocusDecision::evaluate(
                    policy,
                    distant_state_result.value(),
                    large_actor_result.value(),
                    large_sources);

        check(
            state,
            large_decision.has_value() &&
                large_decision.value().
                    focus_fidelity() ==
                    ActorSimulationFidelity::
                        deep_local &&
                large_decision.value().
                    transition() ==
                    ActorSimulationFidelityTransition::
                        promotion_to_deep_local,
            "Composition preserves cell-aware large-world focus evaluation");
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