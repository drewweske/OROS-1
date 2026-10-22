#include "oros/ai/actor_simulation_focus_decision_set_application.hpp"

#include "oros/ai/actor_simulation_focus_policy.hpp"
#include "oros/ai/actor_simulation_focus_source_registry.hpp"
#include "oros/ai/actor_simulation_position_snapshot_set.hpp"

#include <iostream>
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

    bool seed_registry(
        oros::ai::ActorSimulationFidelityRegistry& registry,
        const oros::world::EntityId early,
        const oros::world::EntityId middle,
        const oros::world::EntityId late)
    {
        return
            registry.insert(
                late,
                oros::ai::ActorSimulationFidelity::
                    deep_local).
                has_value() &&
            registry.insert(
                early,
                oros::ai::ActorSimulationFidelity::
                    statistical_distant).
                has_value() &&
            registry.insert(
                middle,
                oros::ai::ActorSimulationFidelity::
                    deep_local).
                has_value();
    }

    bool has_fidelity(
        const oros::ai::ActorSimulationFidelityRegistry& registry,
        const oros::world::EntityId actor,
        const oros::ai::ActorSimulationFidelity expected)
    {
        const oros::foundation::Result<
            oros::ai::ActorSimulationFidelity>
            result =
                registry.fidelity(actor);

        return
            result.has_value() &&
            result.value() == expected;
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

    const Result<WorldPosition>
        origin_result =
            WorldPosition::origin();

    const Result<WorldPosition>
        near_result =
            WorldPosition::create(
                WorldCell{},
                LocalPosition{
                    5.0,
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
        far_result =
            WorldPosition::create(
                WorldCell{},
                LocalPosition{
                    100.0,
                    0.0,
                    0.0
                });

    check(
        state,
        policy_result.has_value() &&
            origin_result.has_value() &&
            near_result.has_value() &&
            hysteresis_result.has_value() &&
            far_result.has_value(),
        "Batch-application fixtures are valid");

    if (
        !policy_result.has_value() ||
        !origin_result.has_value() ||
        !near_result.has_value() ||
        !hysteresis_result.has_value() ||
        !far_result.has_value())
    {
        return 1;
    }

    const EntityId actor_early{
        0x4F524F53ULL,
        10ULL
    };

    const EntityId actor_middle{
        0x4F524F53ULL,
        20ULL
    };

    const EntityId actor_late{
        0x4F524F53ULL,
        30ULL
    };

    const EntityId replacement_actor{
        0x4F524F53ULL,
        25ULL
    };

    const EntityId extra_actor{
        0x4F524F53ULL,
        40ULL
    };

    const EntityId focus_entity{
        0x4F524F53ULL,
        999ULL
    };

    ActorSimulationFidelityRegistry
        evaluation_registry{};

    check(
        state,
        seed_registry(
            evaluation_registry,
            actor_early,
            actor_middle,
            actor_late),
        "Evaluation fidelity registry constructs successfully");

    ActorSimulationPositionSnapshotSet
        positions{};

    const Status position_early =
        positions.insert(
            actor_early,
            near_result.value());

    const Status position_middle =
        positions.insert(
            actor_middle,
            hysteresis_result.value());

    const Status position_late =
        positions.insert(
            actor_late,
            far_result.value());

    check(
        state,
        position_early.has_value() &&
            position_middle.has_value() &&
            position_late.has_value(),
        "Evaluation position phase constructs successfully");

    ActorSimulationFocusSourceRegistry
        focus_sources{};

    const Status focus_insert =
        focus_sources.insert(
            focus_entity,
            origin_result.value());

    check(
        state,
        focus_insert.has_value(),
        "Evaluation focus source constructs successfully");

    if (
        !position_early.has_value() ||
        !position_middle.has_value() ||
        !position_late.has_value() ||
        !focus_insert.has_value())
    {
        return 1;
    }

    const Result<
        ActorSimulationFocusDecisionSet>
        decision_set_result =
            ActorSimulationFocusDecisionSet::evaluate(
                policy_result.value(),
                evaluation_registry,
                positions,
                focus_sources);

    check(
        state,
        decision_set_result.has_value(),
        "Canonical focus decision set evaluates successfully");

    if (!decision_set_result.has_value())
    {
        return 1;
    }

    const ActorSimulationFocusDecisionSet&
        decision_set =
            decision_set_result.value();

    const std::span<
        const ActorSimulationFocusDecision>
        decisions =
            decision_set.
                decisions_in_entity_order();

    check(
        state,
        decisions.size() == 3U &&
            decisions[0].transition() ==
                ActorSimulationFidelityTransition::
                    promotion_to_deep_local &&
            decisions[1].transition() ==
                ActorSimulationFidelityTransition::
                    unchanged &&
            decisions[2].transition() ==
                ActorSimulationFidelityTransition::
                    demotion_to_statistical_distant,
        "Batch fixture contains promotion unchanged and demotion");

    ActorSimulationFidelityRegistry
        matching_registry{};

    check(
        state,
        seed_registry(
            matching_registry,
            actor_early,
            actor_middle,
            actor_late),
        "Matching application registry constructs successfully");

    const Status matching_application =
        apply_actor_simulation_focus_decision_set_if_fidelities_match(
            matching_registry,
            decision_set);

    check(
        state,
        matching_application.has_value(),
        "Matching decision set applies successfully");

    check(
        state,
        has_fidelity(
            matching_registry,
            actor_early,
            ActorSimulationFidelity::
                deep_local) &&
            has_fidelity(
                matching_registry,
                actor_middle,
                ActorSimulationFidelity::
                    deep_local) &&
            has_fidelity(
                matching_registry,
                actor_late,
                ActorSimulationFidelity::
                    statistical_distant),
        "Successful batch applies every canonical target fidelity");

    const std::span<
        const ActorSimulationFidelityState>
        matching_order =
            matching_registry.
                states_in_entity_order();

    check(
        state,
        matching_order.size() == 3U &&
            matching_order[0].actor() ==
                actor_early &&
            matching_order[1].actor() ==
                actor_middle &&
            matching_order[2].actor() ==
                actor_late,
        "Successful batch preserves canonical actor order");

    const Status reapplied =
        apply_actor_simulation_focus_decision_set_if_fidelities_match(
            matching_registry,
            decision_set);

    check(
        state,
        !reapplied.has_value() &&
            reapplied.error().code ==
                ErrorCode::invalid_state,
        "Reapplying an already-consumed decision set is stale");

    check(
        state,
        has_fidelity(
            matching_registry,
            actor_early,
            ActorSimulationFidelity::
                deep_local) &&
            has_fidelity(
                matching_registry,
                actor_middle,
                ActorSimulationFidelity::
                    deep_local) &&
            has_fidelity(
                matching_registry,
                actor_late,
                ActorSimulationFidelity::
                    statistical_distant),
        "Rejected stale reapplication cannot change applied state");

    ActorSimulationFidelityRegistry
        late_stale_registry{};

    check(
        state,
        seed_registry(
            late_stale_registry,
            actor_early,
            actor_middle,
            actor_late),
        "Late-stale registry constructs successfully");

    const Result<
        ActorSimulationFidelityTransition>
        make_late_stale =
            late_stale_registry.transition(
                actor_late,
                ActorSimulationFidelity::
                    statistical_distant);

    check(
        state,
        make_late_stale.has_value(),
        "Late actor is changed after decision evaluation");

    const Status late_stale_application =
        apply_actor_simulation_focus_decision_set_if_fidelities_match(
            late_stale_registry,
            decision_set);

    check(
        state,
        !late_stale_application.has_value() &&
            late_stale_application.error().code ==
                ErrorCode::invalid_state,
        "Late stale fidelity rejects the entire batch during preflight");

    check(
        state,
        has_fidelity(
            late_stale_registry,
            actor_early,
            ActorSimulationFidelity::
                statistical_distant) &&
            has_fidelity(
                late_stale_registry,
                actor_middle,
                ActorSimulationFidelity::
                    deep_local) &&
            has_fidelity(
                late_stale_registry,
                actor_late,
                ActorSimulationFidelity::
                    statistical_distant),
        "Late preflight failure causes zero earlier actor mutations");

    ActorSimulationFidelityRegistry
        size_drift_registry{};

    check(
        state,
        seed_registry(
            size_drift_registry,
            actor_early,
            actor_middle,
            actor_late),
        "Size-drift registry base constructs successfully");

    const Status insert_extra =
        size_drift_registry.insert(
            extra_actor,
            ActorSimulationFidelity::
                deep_local);

    check(
        state,
        insert_extra.has_value(),
        "Actor universe can drift after decision evaluation");

    const Status size_drift_application =
        apply_actor_simulation_focus_decision_set_if_fidelities_match(
            size_drift_registry,
            decision_set);

    check(
        state,
        !size_drift_application.has_value() &&
            size_drift_application.error().code ==
                ErrorCode::invalid_state,
        "Actor-count drift rejects the complete decision set");

    check(
        state,
        size_drift_registry.size() == 4U &&
            has_fidelity(
                size_drift_registry,
                actor_early,
                ActorSimulationFidelity::
                    statistical_distant) &&
            has_fidelity(
                size_drift_registry,
                actor_late,
                ActorSimulationFidelity::
                    deep_local),
        "Actor-count drift failure causes zero fidelity mutations");

    ActorSimulationFidelityRegistry
        identity_drift_registry{};

    check(
        state,
        seed_registry(
            identity_drift_registry,
            actor_early,
            actor_middle,
            actor_late),
        "Identity-drift registry base constructs successfully");

    const Status remove_middle =
        identity_drift_registry.remove(
            actor_middle);

    const Status insert_replacement =
        identity_drift_registry.insert(
            replacement_actor,
            ActorSimulationFidelity::
                deep_local);

    check(
        state,
        remove_middle.has_value() &&
            insert_replacement.has_value() &&
            identity_drift_registry.size() == 3U,
        "Same-size actor identity drift constructs successfully");

    const Status identity_drift_application =
        apply_actor_simulation_focus_decision_set_if_fidelities_match(
            identity_drift_registry,
            decision_set);

    check(
        state,
        !identity_drift_application.has_value() &&
            identity_drift_application.error().code ==
                ErrorCode::invalid_state,
        "Same-size actor identity drift rejects the batch");

    check(
        state,
        has_fidelity(
            identity_drift_registry,
            actor_early,
            ActorSimulationFidelity::
                statistical_distant) &&
            has_fidelity(
                identity_drift_registry,
                replacement_actor,
                ActorSimulationFidelity::
                    deep_local) &&
            has_fidelity(
                identity_drift_registry,
                actor_late,
                ActorSimulationFidelity::
                    deep_local),
        "Identity preflight failure causes zero fidelity mutations");

    ActorSimulationFidelityRegistry
        empty_evaluation_registry{};

    ActorSimulationPositionSnapshotSet
        empty_positions{};

    const Result<
        ActorSimulationFocusDecisionSet>
        empty_decision_set_result =
            ActorSimulationFocusDecisionSet::evaluate(
                policy_result.value(),
                empty_evaluation_registry,
                empty_positions,
                focus_sources);

    check(
        state,
        empty_decision_set_result.has_value() &&
            empty_decision_set_result.value().empty(),
        "Empty authority evaluates to an empty decision set");

    ActorSimulationFidelityRegistry
        empty_application_registry{};

    const Status empty_application =
        apply_actor_simulation_focus_decision_set_if_fidelities_match(
            empty_application_registry,
            empty_decision_set_result.value());

    check(
        state,
        empty_application.has_value() &&
            empty_application_registry.empty(),
        "Empty decision-set application is a successful no-op");

    std::cout
        << state.checks
        << " checks, "
        << state.failures
        << " failures\n";

    return state.failures == 0
        ? 0
        : 1;
}