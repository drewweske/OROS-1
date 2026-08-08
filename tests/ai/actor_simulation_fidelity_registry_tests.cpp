#include "oros/ai/actor_simulation_fidelity_registry.hpp"

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
                    const ActorSimulationFidelityRegistry&>().
                        states_in_entity_order()),
            std::span<
                const ActorSimulationFidelityState>>);

    static_assert(
        std::is_nothrow_move_constructible_v<
            ActorSimulationFidelityRegistry>);

    static_assert(
        std::is_nothrow_move_assignable_v<
            ActorSimulationFidelityRegistry>);

    TestState state{};

    ActorSimulationFidelityRegistry registry{};

    check(
        state,
        registry.empty(),
        "New registry is empty");

    check(
        state,
        registry.size() == 0U,
        "New registry has zero actors");

    check(
        state,
        registry.states_in_entity_order().empty(),
        "New registry exposes an empty ordered view");

    check(
        state,
        !registry.contains(invalid_entity_id),
        "Registry does not contain invalid identity");

    const Result<ActorSimulationFidelity>
        invalid_lookup =
            registry.fidelity(
                invalid_entity_id);

    check(
        state,
        !invalid_lookup.has_value() &&
            invalid_lookup.error().code ==
                ErrorCode::invalid_argument,
        "Invalid lookup reports invalid_argument");

    const Result<
        ActorSimulationFidelityTransition>
        invalid_actor_transition =
            registry.transition(
                invalid_entity_id,
                ActorSimulationFidelity::
                    deep_local);

    check(
        state,
        !invalid_actor_transition.has_value() &&
            invalid_actor_transition.error().code ==
                ErrorCode::invalid_argument,
        "Invalid transition actor reports invalid_argument");

    const Status invalid_remove =
        registry.remove(
            invalid_entity_id);

    check(
        state,
        !invalid_remove.has_value() &&
            invalid_remove.error().code ==
                ErrorCode::invalid_argument,
        "Invalid removal reports invalid_argument");

    const Status invalid_insert =
        registry.insert(
            invalid_entity_id,
            ActorSimulationFidelity::
                statistical_distant);

    check(
        state,
        !invalid_insert.has_value() &&
            invalid_insert.error().code ==
                ErrorCode::invalid_argument,
        "Invalid insertion actor reports invalid_argument");

    check(
        state,
        registry.empty(),
        "Rejected invalid actor insertion does not mutate registry");

    const EntityId early_actor{
        1ULL,
        2ULL
    };

    const EntityId middle_actor{
        1ULL,
        9ULL
    };

    const EntityId late_actor{
        2ULL,
        1ULL
    };

    const EntityId absent_actor{
        3ULL,
        1ULL
    };

    const Status invalid_fidelity_insert =
        registry.insert(
            early_actor,
            ActorSimulationFidelity::invalid);

    check(
        state,
        !invalid_fidelity_insert.has_value() &&
            invalid_fidelity_insert.error().code ==
                ErrorCode::invalid_argument,
        "Invalid insertion fidelity reports invalid_argument");

    check(
        state,
        registry.empty(),
        "Rejected invalid fidelity does not mutate registry");

    const Status insert_late =
        registry.insert(
            late_actor,
            ActorSimulationFidelity::
                deep_local);

    check(
        state,
        insert_late.has_value(),
        "Registry accepts late actor first");

    const Status insert_early =
        registry.insert(
            early_actor,
            ActorSimulationFidelity::
                statistical_distant);

    check(
        state,
        insert_early.has_value(),
        "Registry accepts earlier actor after later actor");

    const Status insert_middle =
        registry.insert(
            middle_actor,
            ActorSimulationFidelity::
                deep_local);

    check(
        state,
        insert_middle.has_value(),
        "Registry accepts middle actor last");

    check(
        state,
        registry.size() == 3U,
        "Registry owns three actors");

    check(
        state,
        !registry.empty(),
        "Populated registry is not empty");

    const std::span<
        const ActorSimulationFidelityState>
        ordered =
            registry.states_in_entity_order();

    check(
        state,
        ordered.size() == 3U,
        "Ordered view contains every actor");

    check(
        state,
        ordered.size() == 3U &&
            ordered[0].actor() ==
                early_actor,
        "Canonical order begins with earliest EntityId");

    check(
        state,
        ordered.size() == 3U &&
            ordered[1].actor() ==
                middle_actor,
        "Canonical order places middle EntityId second");

    check(
        state,
        ordered.size() == 3U &&
            ordered[2].actor() ==
                late_actor,
        "Canonical order places later namespace last");

    check(
        state,
        ordered.size() == 3U &&
            ordered[0].fidelity() ==
                ActorSimulationFidelity::
                    statistical_distant,
        "Ordered state preserves early actor fidelity");

    check(
        state,
        ordered.size() == 3U &&
            ordered[1].fidelity() ==
                ActorSimulationFidelity::
                    deep_local,
        "Ordered state preserves middle actor fidelity");

    check(
        state,
        ordered.size() == 3U &&
            ordered[2].fidelity() ==
                ActorSimulationFidelity::
                    deep_local,
        "Ordered state preserves late actor fidelity");

    check(
        state,
        registry.contains(early_actor),
        "Registry contains early actor");

    check(
        state,
        registry.contains(middle_actor),
        "Registry contains middle actor");

    check(
        state,
        registry.contains(late_actor),
        "Registry contains late actor");

    check(
        state,
        !registry.contains(absent_actor),
        "Registry rejects absent valid actor");

    const Result<ActorSimulationFidelity>
        early_fidelity =
            registry.fidelity(
                early_actor);

    check(
        state,
        early_fidelity.has_value() &&
            early_fidelity.value() ==
                ActorSimulationFidelity::
                    statistical_distant,
        "Fidelity lookup resolves early actor");

    const Result<ActorSimulationFidelity>
        absent_fidelity =
            registry.fidelity(
                absent_actor);

    check(
        state,
        !absent_fidelity.has_value() &&
            absent_fidelity.error().code ==
                ErrorCode::not_found,
        "Absent fidelity lookup reports not_found");

    const Status duplicate_insert =
        registry.insert(
            early_actor,
            ActorSimulationFidelity::
                deep_local);

    check(
        state,
        !duplicate_insert.has_value() &&
            duplicate_insert.error().code ==
                ErrorCode::invalid_state,
        "Duplicate actor insertion reports invalid_state");

    check(
        state,
        registry.size() == 3U,
        "Duplicate insertion does not change actor count");

    const Result<ActorSimulationFidelity>
        fidelity_after_duplicate =
            registry.fidelity(
                early_actor);

    check(
        state,
        fidelity_after_duplicate.has_value() &&
            fidelity_after_duplicate.value() ==
                ActorSimulationFidelity::
                    statistical_distant,
        "Duplicate insertion cannot overwrite fidelity");

    const Result<
        ActorSimulationFidelityTransition>
        unchanged_transition =
            registry.transition(
                early_actor,
                ActorSimulationFidelity::
                    statistical_distant);

    check(
        state,
        unchanged_transition.has_value() &&
            unchanged_transition.value() ==
                ActorSimulationFidelityTransition::
                    unchanged,
        "Registry reports an unchanged fidelity transition");

    const Result<ActorSimulationFidelity>
        fidelity_after_unchanged_transition =
            registry.fidelity(
                early_actor);

    check(
        state,
        fidelity_after_unchanged_transition.
                has_value() &&
            fidelity_after_unchanged_transition.
                value() ==
                ActorSimulationFidelity::
                    statistical_distant,
        "Unchanged registry transition preserves fidelity");

    const Result<
        ActorSimulationFidelityTransition>
        invalid_target_transition =
            registry.transition(
                early_actor,
                ActorSimulationFidelity::invalid);

    check(
        state,
        !invalid_target_transition.has_value() &&
            invalid_target_transition.error().code ==
                ErrorCode::invalid_argument,
        "Invalid target transition reports invalid_argument");

    const Result<ActorSimulationFidelity>
        fidelity_after_invalid_transition =
            registry.fidelity(
                early_actor);

    check(
        state,
        fidelity_after_invalid_transition.has_value() &&
            fidelity_after_invalid_transition.value() ==
                ActorSimulationFidelity::
                    statistical_distant,
        "Rejected transition preserves actor fidelity");

    const Result<
        ActorSimulationFidelityTransition>
        missing_transition =
            registry.transition(
                absent_actor,
                ActorSimulationFidelity::
                    deep_local);

    check(
        state,
        !missing_transition.has_value() &&
            missing_transition.error().code ==
                ErrorCode::not_found,
        "Missing actor transition reports not_found");

    const Result<
        ActorSimulationFidelityTransition>
        promotion =
            registry.transition(
                early_actor,
                ActorSimulationFidelity::
                    deep_local);

    check(
        state,
        promotion.has_value() &&
            promotion.value() ==
                ActorSimulationFidelityTransition::
                    promotion_to_deep_local,
        "Registry promotes distant actor explicitly");

    const Result<ActorSimulationFidelity>
        promoted_fidelity =
            registry.fidelity(
                early_actor);

    check(
        state,
        promoted_fidelity.has_value() &&
            promoted_fidelity.value() ==
                ActorSimulationFidelity::
                    deep_local,
        "Promotion updates only the selected actor fidelity");

    const std::span<
        const ActorSimulationFidelityState>
        after_promotion =
            registry.states_in_entity_order();

    check(
        state,
        after_promotion.size() == 3U &&
            after_promotion[0].actor() ==
                early_actor &&
            after_promotion[1].actor() ==
                middle_actor &&
            after_promotion[2].actor() ==
                late_actor,
        "Fidelity transition cannot perturb canonical actor order");

    const Result<
        ActorSimulationFidelityTransition>
        demotion =
            registry.transition(
                middle_actor,
                ActorSimulationFidelity::
                    statistical_distant);

    check(
        state,
        demotion.has_value() &&
            demotion.value() ==
                ActorSimulationFidelityTransition::
                    demotion_to_statistical_distant,
        "Registry demotes deep-local actor explicitly");

    const Status remove_middle =
        registry.remove(
            middle_actor);

    check(
        state,
        remove_middle.has_value(),
        "Registry removes an existing actor");

    check(
        state,
        registry.size() == 2U,
        "Removal decrements actor count");

    check(
        state,
        !registry.contains(middle_actor),
        "Removed actor is no longer present");

    const std::span<
        const ActorSimulationFidelityState>
        after_removal =
            registry.states_in_entity_order();

    check(
        state,
        after_removal.size() == 2U &&
            after_removal[0].actor() ==
                early_actor &&
            after_removal[1].actor() ==
                late_actor,
        "Removal preserves canonical order of survivors");

    const Status remove_missing =
        registry.remove(
            middle_actor);

    check(
        state,
        !remove_missing.has_value() &&
            remove_missing.error().code ==
                ErrorCode::not_found,
        "Repeated removal reports not_found");

    check(
        state,
        registry.size() == 2U,
        "Rejected removal does not mutate registry");

    const Status reinsert_middle =
        registry.insert(
            middle_actor,
            ActorSimulationFidelity::
                statistical_distant);

    check(
        state,
        reinsert_middle.has_value(),
        "Removed identity may be registered again");

    const std::span<
        const ActorSimulationFidelityState>
        restored_order =
            registry.states_in_entity_order();

    check(
        state,
        restored_order.size() == 3U &&
            restored_order[0].actor() ==
                early_actor &&
            restored_order[1].actor() ==
                middle_actor &&
            restored_order[2].actor() ==
                late_actor,
        "Reinsertion restores canonical EntityId order");

    ActorSimulationFidelityRegistry
        alternate_registry{};

    const Status alternate_middle =
        alternate_registry.insert(
            middle_actor,
            ActorSimulationFidelity::
                statistical_distant);

    const Status alternate_late =
        alternate_registry.insert(
            late_actor,
            ActorSimulationFidelity::
                deep_local);

    const Status alternate_early =
        alternate_registry.insert(
            early_actor,
            ActorSimulationFidelity::
                deep_local);

    check(
        state,
        alternate_middle.has_value() &&
            alternate_late.has_value() &&
            alternate_early.has_value(),
        "Alternate insertion order succeeds");

    const std::span<
        const ActorSimulationFidelityState>
        canonical_a =
            registry.states_in_entity_order();

    const std::span<
        const ActorSimulationFidelityState>
        canonical_b =
            alternate_registry.
                states_in_entity_order();

    bool canonical_match =
        canonical_a.size() ==
            canonical_b.size();

    if (canonical_match)
    {
        for (
            std::size_t index = 0U;
            index < canonical_a.size();
            ++index)
        {
            if (
                canonical_a[index].actor() !=
                    canonical_b[index].actor() ||
                canonical_a[index].fidelity() !=
                    canonical_b[index].fidelity())
            {
                canonical_match = false;
                break;
            }
        }
    }

    check(
        state,
        canonical_match,
        "Canonical state sequence is independent of insertion order");

    std::cout
        << state.checks
        << " checks, "
        << state.failures
        << " failures\n";

    return state.failures == 0
        ? 0
        : 1;
}