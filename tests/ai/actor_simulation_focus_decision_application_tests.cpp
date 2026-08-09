#include "oros/ai/actor_simulation_focus_decision_application.hpp"

#include "oros/ai/actor_simulation_focus_policy.hpp"
#include "oros/ai/actor_simulation_focus_source_registry.hpp"
#include "oros/world/world_position.hpp"

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

    check(
        state,
        policy_result.has_value() &&
            actor_position_result.has_value() &&
            near_position_result.has_value(),
        "Decision-application fixtures are valid");

    if (
        !policy_result.has_value() ||
        !actor_position_result.has_value() ||
        !near_position_result.has_value())
    {
        return 1;
    }

    const EntityId actor{
        0x4F524F53ULL,
        101ULL
    };

    const EntityId other_actor{
        0x4F524F53ULL,
        202ULL
    };

    const EntityId missing_actor{
        0x4F524F53ULL,
        303ULL
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

    const Result<ActorSimulationFidelityState>
        missing_distant_state_result =
            ActorSimulationFidelityState::create(
                missing_actor,
                ActorSimulationFidelity::
                    statistical_distant);

    check(
        state,
        distant_state_result.has_value() &&
            local_state_result.has_value() &&
            missing_distant_state_result.has_value(),
        "Decision actor-state fixtures are valid");

    if (
        !distant_state_result.has_value() ||
        !local_state_result.has_value() ||
        !missing_distant_state_result.has_value())
    {
        return 1;
    }

    ActorSimulationFocusSourceRegistry
        near_sources{};

    const Status insert_focus =
        near_sources.insert(
            EntityId{
                0x4F524F53ULL,
                1ULL
            },
            near_position_result.value());

    check(
        state,
        insert_focus.has_value(),
        "Near simulation focus source inserts");

    if (!insert_focus.has_value())
    {
        return 1;
    }

    ActorSimulationFocusSourceRegistry
        no_sources{};

    const Result<ActorSimulationFocusDecision>
        promotion_decision =
            ActorSimulationFocusDecision::evaluate(
                policy_result.value(),
                distant_state_result.value(),
                actor_position_result.value(),
                near_sources);

    const Result<ActorSimulationFocusDecision>
        demotion_decision =
            ActorSimulationFocusDecision::evaluate(
                policy_result.value(),
                local_state_result.value(),
                actor_position_result.value(),
                no_sources);

    const Result<ActorSimulationFocusDecision>
        unchanged_local_decision =
            ActorSimulationFocusDecision::evaluate(
                policy_result.value(),
                local_state_result.value(),
                actor_position_result.value(),
                near_sources);

    const Result<ActorSimulationFocusDecision>
        unchanged_distant_decision =
            ActorSimulationFocusDecision::evaluate(
                policy_result.value(),
                distant_state_result.value(),
                actor_position_result.value(),
                no_sources);

    const Result<ActorSimulationFocusDecision>
        missing_actor_decision =
            ActorSimulationFocusDecision::evaluate(
                policy_result.value(),
                missing_distant_state_result.value(),
                actor_position_result.value(),
                near_sources);

    check(
        state,
        promotion_decision.has_value() &&
            demotion_decision.has_value() &&
            unchanged_local_decision.has_value() &&
            unchanged_distant_decision.has_value() &&
            missing_actor_decision.has_value(),
        "Application decisions are created successfully");

    if (
        !promotion_decision.has_value() ||
        !demotion_decision.has_value() ||
        !unchanged_local_decision.has_value() ||
        !unchanged_distant_decision.has_value() ||
        !missing_actor_decision.has_value())
    {
        return 1;
    }

    check(
        state,
        promotion_decision.value().
                transition() ==
            ActorSimulationFidelityTransition::
                promotion_to_deep_local &&
            demotion_decision.value().
                transition() ==
            ActorSimulationFidelityTransition::
                demotion_to_statistical_distant &&
            unchanged_local_decision.value().
                transition() ==
            ActorSimulationFidelityTransition::
                unchanged &&
            unchanged_distant_decision.value().
                transition() ==
            ActorSimulationFidelityTransition::
                unchanged,
        "Decision fixtures cover promotion demotion and unchanged");

    ActorSimulationFidelityRegistry
        registry{};

    const Status insert_other =
        registry.insert(
            other_actor,
            ActorSimulationFidelity::
                deep_local);

    const Status insert_actor =
        registry.insert(
            actor,
            ActorSimulationFidelity::
                statistical_distant);

    check(
        state,
        insert_other.has_value() &&
            insert_actor.has_value(),
        "Application registry accepts actors out of order");

    check(
        state,
        registry.size() == 2U,
        "Application registry owns exactly two actors");

    const std::span<
        const ActorSimulationFidelityState>
        initial_order =
            registry.states_in_entity_order();

    check(
        state,
        initial_order.size() == 2U &&
            initial_order[0].actor() ==
                actor &&
            initial_order[1].actor() ==
                other_actor,
        "Registry starts in canonical EntityId order");

    const Result<
        ActorSimulationFidelityTransition>
        promotion_application =
            apply_actor_simulation_focus_decision_if_fidelity_matches(
                registry,
                promotion_decision.value());

    check(
        state,
        promotion_application.has_value() &&
            promotion_application.value() ==
                ActorSimulationFidelityTransition::
                    promotion_to_deep_local,
        "Matching promotion decision applies explicitly");

    const Result<ActorSimulationFidelity>
        promoted_fidelity =
            registry.fidelity(
                actor);

    check(
        state,
        promoted_fidelity.has_value() &&
            promoted_fidelity.value() ==
                ActorSimulationFidelity::
                    deep_local,
        "Promotion application updates authoritative fidelity");

    const Result<ActorSimulationFidelity>
        other_after_promotion =
            registry.fidelity(
                other_actor);

    check(
        state,
        other_after_promotion.has_value() &&
            other_after_promotion.value() ==
                ActorSimulationFidelity::
                    deep_local,
        "Promotion cannot mutate another actor");

    const std::span<
        const ActorSimulationFidelityState>
        order_after_promotion =
            registry.states_in_entity_order();

    check(
        state,
        order_after_promotion.size() == 2U &&
            order_after_promotion[0].actor() ==
                actor &&
            order_after_promotion[1].actor() ==
                other_actor,
        "Decision application cannot perturb canonical actor order");

    const Result<
        ActorSimulationFidelityTransition>
        stale_promotion =
            apply_actor_simulation_focus_decision_if_fidelity_matches(
                registry,
                promotion_decision.value());

    check(
        state,
        !stale_promotion.has_value() &&
            stale_promotion.error().code ==
                ErrorCode::invalid_state,
        "Reapplying promotion decision is rejected as stale");

    const Result<ActorSimulationFidelity>
        after_stale_promotion =
            registry.fidelity(
                actor);

    check(
        state,
        after_stale_promotion.has_value() &&
            after_stale_promotion.value() ==
                ActorSimulationFidelity::
                    deep_local,
        "Rejected stale promotion cannot mutate fidelity");

    const Result<
        ActorSimulationFidelityTransition>
        unchanged_local_application =
            apply_actor_simulation_focus_decision_if_fidelity_matches(
                registry,
                unchanged_local_decision.value());

    check(
        state,
        unchanged_local_application.has_value() &&
            unchanged_local_application.value() ==
                ActorSimulationFidelityTransition::
                    unchanged,
        "Matching unchanged deep-local decision succeeds");

    const Result<ActorSimulationFidelity>
        after_unchanged_local =
            registry.fidelity(
                actor);

    check(
        state,
        after_unchanged_local.has_value() &&
            after_unchanged_local.value() ==
                ActorSimulationFidelity::
                    deep_local,
        "Unchanged application preserves deep-local fidelity");

    const Result<
        ActorSimulationFidelityTransition>
        demotion_application =
            apply_actor_simulation_focus_decision_if_fidelity_matches(
                registry,
                demotion_decision.value());

    check(
        state,
        demotion_application.has_value() &&
            demotion_application.value() ==
                ActorSimulationFidelityTransition::
                    demotion_to_statistical_distant,
        "Matching demotion decision applies explicitly");

    const Result<ActorSimulationFidelity>
        demoted_fidelity =
            registry.fidelity(
                actor);

    check(
        state,
        demoted_fidelity.has_value() &&
            demoted_fidelity.value() ==
                ActorSimulationFidelity::
                    statistical_distant,
        "Demotion application updates authoritative fidelity");

    const Result<
        ActorSimulationFidelityTransition>
        stale_demotion =
            apply_actor_simulation_focus_decision_if_fidelity_matches(
                registry,
                demotion_decision.value());

    check(
        state,
        !stale_demotion.has_value() &&
            stale_demotion.error().code ==
                ErrorCode::invalid_state,
        "Reapplying demotion decision is rejected as stale");

    const Result<
        ActorSimulationFidelityTransition>
        unchanged_distant_application =
            apply_actor_simulation_focus_decision_if_fidelity_matches(
                registry,
                unchanged_distant_decision.value());

    check(
        state,
        unchanged_distant_application.has_value() &&
            unchanged_distant_application.value() ==
                ActorSimulationFidelityTransition::
                    unchanged,
        "Matching unchanged distant decision succeeds");

    const Result<ActorSimulationFidelity>
        after_unchanged_distant =
            registry.fidelity(
                actor);

    check(
        state,
        after_unchanged_distant.has_value() &&
            after_unchanged_distant.value() ==
                ActorSimulationFidelity::
                    statistical_distant,
        "Unchanged application preserves distant fidelity");

    const Result<
        ActorSimulationFidelityTransition>
        missing_application =
            apply_actor_simulation_focus_decision_if_fidelity_matches(
                registry,
                missing_actor_decision.value());

    check(
        state,
        !missing_application.has_value() &&
            missing_application.error().code ==
                ErrorCode::not_found,
        "Decision for absent registry actor reports not_found");

    check(
        state,
        registry.size() == 2U &&
            !registry.contains(
                missing_actor),
        "Missing-actor application cannot alter registry membership");

    const Result<ActorSimulationFidelity>
        other_final =
            registry.fidelity(
                other_actor);

    check(
        state,
        other_final.has_value() &&
            other_final.value() ==
                ActorSimulationFidelity::
                    deep_local,
        "Rejected and successful applications preserve unrelated actor state");

    const std::span<
        const ActorSimulationFidelityState>
        final_order =
            registry.states_in_entity_order();

    check(
        state,
        final_order.size() == 2U &&
            final_order[0].actor() ==
                actor &&
            final_order[1].actor() ==
                other_actor,
        "All applications preserve canonical registry ordering");

    std::cout
        << state.checks
        << " checks, "
        << state.failures
        << " failures\n";

    return state.failures == 0
        ? 0
        : 1;
}