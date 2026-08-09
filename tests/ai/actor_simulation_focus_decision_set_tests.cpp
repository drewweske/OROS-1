#include "oros/ai/actor_simulation_focus_decision_set.hpp"

#include <iostream>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>

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

    bool decisions_match(
        const std::span<
            const oros::ai::
                ActorSimulationFocusDecision>
            first,
        const std::span<
            const oros::ai::
                ActorSimulationFocusDecision>
            second)
    {
        if (first.size() != second.size())
        {
            return false;
        }

        for (
            std::size_t index = 0U;
            index < first.size();
            ++index)
        {
            if (
                first[index].actor() !=
                    second[index].actor() ||
                first[index].
                        current_fidelity() !=
                    second[index].
                        current_fidelity() ||
                first[index].
                        focus_fidelity() !=
                    second[index].
                        focus_fidelity() ||
                first[index].transition() !=
                    second[index].transition())
            {
                return false;
            }
        }

        return true;
    }
}

int main()
{
    using namespace oros::ai;
    using namespace oros::foundation;
    using namespace oros::world;

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const ActorSimulationFocusDecisionSet&>().
                        decisions_in_entity_order()),
            std::span<
                const ActorSimulationFocusDecision>>);

    static_assert(
        std::is_nothrow_move_constructible_v<
            ActorSimulationFocusDecisionSet>);

    static_assert(
        std::is_nothrow_move_assignable_v<
            ActorSimulationFocusDecisionSet>);

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

    const Result<WorldPosition>
        extra_result =
            WorldPosition::create(
                WorldCell{},
                LocalPosition{
                    200.0,
                    0.0,
                    0.0
                });

    check(
        state,
        policy_result.has_value() &&
            origin_result.has_value() &&
            near_result.has_value() &&
            hysteresis_result.has_value() &&
            far_result.has_value() &&
            extra_result.has_value(),
        "Decision-set fixtures are valid");

    if (
        !policy_result.has_value() ||
        !origin_result.has_value() ||
        !near_result.has_value() ||
        !hysteresis_result.has_value() ||
        !far_result.has_value() ||
        !extra_result.has_value())
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

    const EntityId extra_before{
        0x4F524F53ULL,
        5ULL
    };

    const EntityId extra_between{
        0x4F524F53ULL,
        25ULL
    };

    const EntityId focus_entity{
        0x4F524F53ULL,
        999ULL
    };

    ActorSimulationFidelityRegistry
        fidelity_registry{};

    const Status insert_late_fidelity =
        fidelity_registry.insert(
            actor_late,
            ActorSimulationFidelity::
                deep_local);

    const Status insert_early_fidelity =
        fidelity_registry.insert(
            actor_early,
            ActorSimulationFidelity::
                statistical_distant);

    const Status insert_middle_fidelity =
        fidelity_registry.insert(
            actor_middle,
            ActorSimulationFidelity::
                deep_local);

    check(
        state,
        insert_late_fidelity.has_value() &&
            insert_early_fidelity.has_value() &&
            insert_middle_fidelity.has_value(),
        "Fidelity actors insert successfully out of order");

    ActorSimulationPositionSnapshotSet
        positions{};

    const Status insert_middle_position =
        positions.insert(
            actor_middle,
            hysteresis_result.value());

    const Status insert_extra_between =
        positions.insert(
            extra_between,
            extra_result.value());

    const Status insert_late_position =
        positions.insert(
            actor_late,
            far_result.value());

    const Status insert_extra_before =
        positions.insert(
            extra_before,
            extra_result.value());

    const Status insert_early_position =
        positions.insert(
            actor_early,
            near_result.value());

    check(
        state,
        insert_middle_position.has_value() &&
            insert_extra_between.has_value() &&
            insert_late_position.has_value() &&
            insert_extra_before.has_value() &&
            insert_early_position.has_value(),
        "Phase-local positions accept a deterministic superset");

    ActorSimulationFocusSourceRegistry
        focus_sources{};

    const Status insert_focus =
        focus_sources.insert(
            focus_entity,
            origin_result.value());

    check(
        state,
        insert_focus.has_value(),
        "Focus source inserts for decision-set evaluation");

    if (
        !insert_late_fidelity.has_value() ||
        !insert_early_fidelity.has_value() ||
        !insert_middle_fidelity.has_value() ||
        !insert_middle_position.has_value() ||
        !insert_extra_between.has_value() ||
        !insert_late_position.has_value() ||
        !insert_extra_before.has_value() ||
        !insert_early_position.has_value() ||
        !insert_focus.has_value())
    {
        return 1;
    }

    const Result<
        ActorSimulationFocusDecisionSet>
        decision_set_result =
            ActorSimulationFocusDecisionSet::
                evaluate(
                    policy_result.value(),
                    fidelity_registry,
                    positions,
                    focus_sources);

    check(
        state,
        decision_set_result.has_value(),
        "Complete phase inputs produce a decision set");

    if (!decision_set_result.has_value())
    {
        return 1;
    }

    const ActorSimulationFocusDecisionSet&
        decision_set =
            decision_set_result.value();

    check(
        state,
        decision_set.size() == 3U &&
            !decision_set.empty(),
        "Decision set contains exactly the registered fidelity actors");

    const std::span<
        const ActorSimulationFocusDecision>
        decisions =
            decision_set.
                decisions_in_entity_order();

    check(
        state,
        decisions.size() == 3U &&
            decisions[0].actor() ==
                actor_early &&
            decisions[1].actor() ==
                actor_middle &&
            decisions[2].actor() ==
                actor_late,
        "Decision output follows canonical fidelity actor order");

    check(
        state,
        decisions.size() == 3U &&
            decisions[0].
                    current_fidelity() ==
                ActorSimulationFidelity::
                    statistical_distant &&
            decisions[0].
                    focus_fidelity() ==
                ActorSimulationFidelity::
                    deep_local &&
            decisions[0].
                    transition() ==
                ActorSimulationFidelityTransition::
                    promotion_to_deep_local,
        "Near distant actor receives promotion decision");

    check(
        state,
        decisions.size() == 3U &&
            decisions[1].
                    current_fidelity() ==
                ActorSimulationFidelity::
                    deep_local &&
            decisions[1].
                    focus_fidelity() ==
                ActorSimulationFidelity::
                    deep_local &&
            decisions[1].
                    transition() ==
                ActorSimulationFidelityTransition::
                    unchanged,
        "Hysteresis-band local actor remains deep-local");

    check(
        state,
        decisions.size() == 3U &&
            decisions[2].
                    current_fidelity() ==
                ActorSimulationFidelity::
                    deep_local &&
            decisions[2].
                    focus_fidelity() ==
                ActorSimulationFidelity::
                    statistical_distant &&
            decisions[2].
                    transition() ==
                ActorSimulationFidelityTransition::
                    demotion_to_statistical_distant,
        "Far local actor receives demotion decision");

    check(
        state,
        positions.size() == 5U,
        "Extra position snapshots remain inert and preserved");

    check(
        state,
        focus_sources.size() == 1U,
        "Decision-set evaluation does not mutate focus sources");

    const Result<ActorSimulationFidelity>
        early_after =
            fidelity_registry.fidelity(
                actor_early);

    const Result<ActorSimulationFidelity>
        middle_after =
            fidelity_registry.fidelity(
                actor_middle);

    const Result<ActorSimulationFidelity>
        late_after =
            fidelity_registry.fidelity(
                actor_late);

    check(
        state,
        early_after.has_value() &&
            middle_after.has_value() &&
            late_after.has_value() &&
            early_after.value() ==
                ActorSimulationFidelity::
                    statistical_distant &&
            middle_after.value() ==
                ActorSimulationFidelity::
                    deep_local &&
            late_after.value() ==
                ActorSimulationFidelity::
                    deep_local,
        "Decision-set evaluation cannot mutate authoritative fidelity");

    ActorSimulationFidelityRegistry
        alternate_fidelity{};

    const Status alternate_early_fidelity =
        alternate_fidelity.insert(
            actor_early,
            ActorSimulationFidelity::
                statistical_distant);

    const Status alternate_middle_fidelity =
        alternate_fidelity.insert(
            actor_middle,
            ActorSimulationFidelity::
                deep_local);

    const Status alternate_late_fidelity =
        alternate_fidelity.insert(
            actor_late,
            ActorSimulationFidelity::
                deep_local);

    ActorSimulationPositionSnapshotSet
        alternate_positions{};

    const Status alternate_late_position =
        alternate_positions.insert(
            actor_late,
            far_result.value());

    const Status alternate_early_position =
        alternate_positions.insert(
            actor_early,
            near_result.value());

    const Status alternate_extra_before =
        alternate_positions.insert(
            extra_before,
            extra_result.value());

    const Status alternate_middle_position =
        alternate_positions.insert(
            actor_middle,
            hysteresis_result.value());

    const Status alternate_extra_between =
        alternate_positions.insert(
            extra_between,
            extra_result.value());

    check(
        state,
        alternate_early_fidelity.has_value() &&
            alternate_middle_fidelity.has_value() &&
            alternate_late_fidelity.has_value() &&
            alternate_late_position.has_value() &&
            alternate_early_position.has_value() &&
            alternate_extra_before.has_value() &&
            alternate_middle_position.has_value() &&
            alternate_extra_between.has_value(),
        "Alternate insertion histories construct successfully");

    const Result<
        ActorSimulationFocusDecisionSet>
        alternate_result =
            ActorSimulationFocusDecisionSet::
                evaluate(
                    policy_result.value(),
                    alternate_fidelity,
                    alternate_positions,
                    focus_sources);

    check(
        state,
        alternate_result.has_value(),
        "Alternate insertion histories evaluate successfully");

    check(
        state,
        alternate_result.has_value() &&
            decisions_match(
                decisions,
                alternate_result.value().
                    decisions_in_entity_order()),
        "Decision set is independent of registry insertion history");

    ActorSimulationPositionSnapshotSet
        incomplete_positions{};

    const Status incomplete_early =
        incomplete_positions.insert(
            actor_early,
            near_result.value());

    const Status incomplete_late =
        incomplete_positions.insert(
            actor_late,
            far_result.value());

    check(
        state,
        incomplete_early.has_value() &&
            incomplete_late.has_value(),
        "Incomplete position phase constructs intentionally");

    const Result<
        ActorSimulationFocusDecisionSet>
        missing_position_result =
            ActorSimulationFocusDecisionSet::
                evaluate(
                    policy_result.value(),
                    fidelity_registry,
                    incomplete_positions,
                    focus_sources);

    check(
        state,
        !missing_position_result.has_value() &&
            missing_position_result.error().code ==
                ErrorCode::not_found,
        "Missing registered-actor position is a hard not_found failure");

    const Result<ActorSimulationFidelity>
        middle_after_failure =
            fidelity_registry.fidelity(
                actor_middle);

    check(
        state,
        middle_after_failure.has_value() &&
            middle_after_failure.value() ==
                ActorSimulationFidelity::
                    deep_local,
        "Failed batch evaluation cannot mutate actor fidelity");

    ActorSimulationFidelityRegistry
        empty_fidelity{};

    const Result<
        ActorSimulationFocusDecisionSet>
        empty_result =
            ActorSimulationFocusDecisionSet::
                evaluate(
                    policy_result.value(),
                    empty_fidelity,
                    positions,
                    focus_sources);

    check(
        state,
        empty_result.has_value() &&
            empty_result.value().empty() &&
            empty_result.value().size() == 0U,
        "Empty actor authority produces an empty decision set despite extra snapshots");

    std::cout
        << state.checks
        << " checks, "
        << state.failures
        << " failures\n";

    return state.failures == 0
        ? 0
        : 1;
}