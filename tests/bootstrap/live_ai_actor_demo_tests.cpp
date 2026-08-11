#include "live_ai_actor_demo.hpp"
#include "navigation_demo.hpp"

#include "oros/ai/navigation_obstacle_overlay.hpp"
#include "oros/ai/navigation_search.hpp"
#include "oros/foundation/result.hpp"
#include "oros/world/world.hpp"

#include <cstdint>
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

    const oros::ai::NavigationNodeRecord*
    find_navigation_node(
        const oros::bootstrap::NavigationDemo&
            navigation_demo,
        const oros::ai::NavigationNodeId node)
        noexcept
    {
        const oros::ai::NavigationCellTopology*
            cell_topology =
                navigation_demo.
                    topology.
                    find_cell_topology(
                        node.cell);

        if (cell_topology == nullptr)
        {
            return nullptr;
        }

        for (
            const oros::ai::
                NavigationNodeRecord&
                record :
            cell_topology->
                nodes_in_canonical_order())
        {
            if (record.id() == node)
            {
                return &record;
            }
        }

        return nullptr;
    }
}

int main()
{
    using namespace oros::ai;
    using namespace oros::bootstrap;
    using namespace oros::foundation;
    using namespace oros::world;

    static_assert(
        !std::is_default_constructible_v<
            LiveAiActorDemo>);

    static_assert(
        !std::is_copy_constructible_v<
            LiveAiActorDemo>);

    static_assert(
        !std::is_copy_assignable_v<
            LiveAiActorDemo>);

    static_assert(
        std::is_nothrow_move_constructible_v<
            LiveAiActorDemo>);

    static_assert(
        std::is_nothrow_move_assignable_v<
            LiveAiActorDemo>);

    TestState state{};

    Result<NavigationDemo>
        navigation_result =
            create_navigation_demo();

    check(
        state,
        navigation_result.has_value(),
        "NavigationDemo prerequisite constructs");

    if (!navigation_result.has_value())
    {
        return 1;
    }

    NavigationDemo navigation_demo{
        std::move(
            navigation_result.value())
    };

    Result<World>
        world_result =
            World::create(
                0x4F524F53ULL);

    check(
        state,
        world_result.has_value(),
        "Fresh World prerequisite constructs");

    if (!world_result.has_value())
    {
        return 1;
    }

    World world{
        std::move(
            world_result.value())
    };

    check(
        state,
        world.entity_count() == 0U &&
            world.position_count() == 0U,
        "Focused World starts without entities or positions");

    Result<LiveAiActorDemo>
        actor_result =
            create_live_ai_actor_demo(
                world,
                navigation_demo);

    check(
        state,
        actor_result.has_value(),
        "create_live_ai_actor_demo succeeds");

    if (!actor_result.has_value())
    {
        return 1;
    }

    LiveAiActorDemo actor_demo{
        std::move(
            actor_result.value())
    };

    check(
        state,
        actor_demo.actor.is_valid(),
        "Live AI actor EntityId is valid");

    check(
        state,
        world.entity_count() == 1U &&
            world.position_count() == 1U,
        "Focused fixture owns exactly one World actor and position");

    check(
        state,
        world.contains(
            actor_demo.actor),
        "World contains the live AI actor");

    const NavigationNodeRecord*
        start_record =
            find_navigation_node(
                navigation_demo,
                navigation_demo.start_node);

    check(
        state,
        start_record != nullptr,
        "Navigation start-node record is supplied");

    const WorldPosition*
        actor_position =
            world.find_position(
                actor_demo.actor);

    check(
        state,
        start_record != nullptr &&
            actor_position != nullptr &&
            *actor_position ==
                start_record->position(),
        "Actor World position equals navigation start-node position");

    const std::span<
        const ActorScheduledActivity>
        activities =
            actor_demo.
                schedule.
                activities_in_time_order();

    check(
        state,
        activities.size() == 1U,
        "Live AI actor schedule contains exactly one activity");

    check(
        state,
        activities.size() == 1U &&
            activities.front().
                    intent().
                    intent_namespace() ==
                "oros" &&
            activities.front().
                    intent().
                    intent_name() ==
                "work",
        "Scheduled intent is oros work");

    check(
        state,
        activities.size() == 1U &&
            activities.front().
                    window().
                    start_inclusive().
                    microseconds_since_epoch() ==
                0ULL &&
            activities.front().
                    window().
                    end_exclusive().
                    microseconds_since_epoch() ==
                1'000'000ULL,
        "Schedule window is exactly zero through one million microseconds");

    const auto&
        persistent_intent =
            actor_demo.
                schedule_execution.
                persistent_intent();

    check(
        state,
        actor_demo.
                schedule_execution.
                actor() ==
            actor_demo.actor,
        "Schedule execution state uses live actor EntityId");

    check(
        state,
        persistent_intent.has_value() &&
            persistent_intent->
                    intent_namespace() ==
                "oros" &&
            persistent_intent->
                    intent_name() ==
                "work",
        "Persistent execution intent is oros work");

    check(
        state,
        !actor_demo.
            schedule_execution.
            is_interrupted(),
        "Live AI actor begins in non-interrupted schedule state");

    const std::span<
        const ActorFactionMembership>
        memberships =
            actor_demo.
                faction_memberships.
                memberships_in_canonical_order();

    check(
        state,
        memberships.size() == 1U,
        "Live AI actor has exactly one faction membership");

    check(
        state,
        memberships.size() == 1U &&
            memberships.front().actor() ==
                actor_demo.actor &&
            memberships.front().
                    faction().
                    faction_namespace() ==
                "oros" &&
            memberships.front().
                    faction().
                    faction_name() ==
                "citizens",
        "Faction membership is actor plus oros citizens");

    check(
        state,
        actor_demo.
            fidelity_registry.
            contains(
                actor_demo.actor),
        "Fidelity registry contains live AI actor");

    const Result<
        ActorSimulationFidelity>
        fidelity_result =
            actor_demo.
                fidelity_registry.
                fidelity(
                    actor_demo.actor);

    check(
        state,
        fidelity_result.has_value() &&
            fidelity_result.value() ==
                ActorSimulationFidelity::
                    deep_local,
        "Live AI actor fidelity is deep_local");

    const NavigationObstacleOverlay
        empty_overlay{};

    Result<NavigationSearchResult>
        actor_navigation_result =
            search_navigation_route(
                navigation_demo.topology,
                empty_overlay,
                navigation_demo.start_node,
                navigation_demo.
                    complete_destination);

    check(
        state,
        actor_navigation_result.
                has_value() &&
            actor_navigation_result.
                value().kind() ==
            NavigationSearchResultKind::
                complete &&
            actor_navigation_result.
                value().route() !=
            nullptr,
        "Actor-on-behalf navigation search is complete");

    if (
        !actor_navigation_result.has_value() ||
        actor_navigation_result.
            value().route() ==
            nullptr)
    {
        return 1;
    }

    const std::span<
        const NavigationNodeId>
        actor_route_nodes =
            actor_navigation_result.
                value().
                route()->
                nodes_in_traversal_order();

    check(
        state,
        !actor_route_nodes.empty() &&
            actor_route_nodes.front() ==
                navigation_demo.start_node,
        "Actor-on-behalf route begins at navigation start node");

    const EntityId
        continuity_actor =
            actor_demo.actor;

    const WorldPosition
        continuity_position =
            *actor_position;

    Status
        interruption_status =
            actor_demo.
                schedule_execution.
                begin_interruption();

    check(
        state,
        interruption_status.has_value(),
        "Live AI actor schedule interruption begins");

    if (!interruption_status.has_value())
    {
        return 1;
    }

    const auto&
        interrupted_intent =
            actor_demo.
                schedule_execution.
                persistent_intent();

    check(
        state,
        interrupted_intent.has_value() &&
            interrupted_intent->
                    intent_namespace() ==
                "oros" &&
            interrupted_intent->
                    intent_name() ==
                "work" &&
            actor_demo.
                schedule_execution.
                is_interrupted(),
        "Interrupted actor retains persistent oros work intent");

    Result<ActorSimulationFocusPolicy>
        focus_policy_result =
            ActorSimulationFocusPolicy::
                create(
                    1.0,
                    2.0);

    check(
        state,
        focus_policy_result.has_value(),
        "Fidelity round-trip focus policy constructs");

    if (!focus_policy_result.has_value())
    {
        return 1;
    }

    const ActorSimulationFocusSourceRegistry
        empty_focus_sources{};

    Result<
        ActorSimulationFidelityTransition>
        demotion_result =
            apply_live_ai_actor_simulation_focus(
                world,
                actor_demo,
                focus_policy_result.value(),
                empty_focus_sources);

    check(
        state,
        demotion_result.has_value() &&
            demotion_result.value() ==
                ActorSimulationFidelityTransition::
                    demotion_to_statistical_distant,
        "Empty focus deterministically demotes live actor");

    if (!demotion_result.has_value())
    {
        return 1;
    }

    const Result<
        ActorSimulationFidelity>
        distant_fidelity_result =
            actor_demo.
                fidelity_registry.
                fidelity(
                    actor_demo.actor);

    const WorldPosition*
        distant_actor_position =
            world.find_position(
                actor_demo.actor);

    const auto&
        distant_intent =
            actor_demo.
                schedule_execution.
                persistent_intent();

    const auto
        distant_memberships =
            actor_demo.
                faction_memberships.
                memberships_in_canonical_order();

    check(
        state,
        actor_demo.actor ==
                continuity_actor &&
            world.contains(
                continuity_actor) &&
            distant_actor_position !=
                nullptr &&
            *distant_actor_position ==
                continuity_position &&
            distant_fidelity_result.
                has_value() &&
            distant_fidelity_result.
                    value() ==
                ActorSimulationFidelity::
                    statistical_distant,
        "Demotion preserves actor identity and World position");

    check(
        state,
        distant_intent.has_value() &&
            distant_intent->
                    intent_namespace() ==
                "oros" &&
            distant_intent->
                    intent_name() ==
                "work" &&
            actor_demo.
                schedule_execution.
                is_interrupted(),
        "Demotion preserves persistent interrupted intent");

    check(
        state,
        distant_memberships.size() ==
                1U &&
            distant_memberships.front().
                    actor() ==
                continuity_actor &&
            distant_memberships.front().
                    faction().
                    faction_namespace() ==
                "oros" &&
            distant_memberships.front().
                    faction().
                    faction_name() ==
                "citizens",
        "Demotion preserves faction membership");

    Result<EntityId>
        focus_entity_result =
            world.create_entity();

    check(
        state,
        focus_entity_result.has_value(),
        "Stable World focus entity is created");

    if (!focus_entity_result.has_value())
    {
        return 1;
    }

    const EntityId focus_entity =
        focus_entity_result.value();

    check(
        state,
        focus_entity.is_valid() &&
            focus_entity !=
                continuity_actor,
        "Focus entity is distinct from live AI actor");

    Status
        focus_position_status =
            world.add_position(
                focus_entity,
                continuity_position);

    check(
        state,
        focus_position_status.has_value(),
        "Stable World focus entity receives World position");

    if (!focus_position_status.has_value())
    {
        return 1;
    }

    const WorldPosition*
        focus_position =
            world.find_position(
                focus_entity);

    check(
        state,
        focus_position != nullptr &&
            *focus_position ==
                continuity_position,
        "Focus source uses actual World-owned position");

    if (focus_position == nullptr)
    {
        return 1;
    }

    ActorSimulationFocusSourceRegistry
        focus_sources{};

    Status
        focus_source_status =
            focus_sources.insert(
                focus_entity,
                *focus_position);

    check(
        state,
        focus_source_status.has_value(),
        "World focus entity enters focus-source registry");

    if (!focus_source_status.has_value())
    {
        return 1;
    }

    Result<
        ActorSimulationFidelityTransition>
        promotion_result =
            apply_live_ai_actor_simulation_focus(
                world,
                actor_demo,
                focus_policy_result.value(),
                focus_sources);

    check(
        state,
        promotion_result.has_value() &&
            promotion_result.value() ==
                ActorSimulationFidelityTransition::
                    promotion_to_deep_local,
        "Nearby stable focus deterministically promotes live actor");

    if (!promotion_result.has_value())
    {
        return 1;
    }

    const Result<
        ActorSimulationFidelity>
        promoted_fidelity_result =
            actor_demo.
                fidelity_registry.
                fidelity(
                    actor_demo.actor);

    const WorldPosition*
        promoted_actor_position =
            world.find_position(
                actor_demo.actor);

    const auto&
        promoted_intent =
            actor_demo.
                schedule_execution.
                persistent_intent();

    const auto
        promoted_memberships =
            actor_demo.
                faction_memberships.
                memberships_in_canonical_order();

    check(
        state,
        actor_demo.actor ==
                continuity_actor &&
            promoted_actor_position !=
                nullptr &&
            *promoted_actor_position ==
                continuity_position &&
            promoted_fidelity_result.
                has_value() &&
            promoted_fidelity_result.
                    value() ==
                ActorSimulationFidelity::
                    deep_local,
        "Promotion restores deep_local without changing identity or position");

    check(
        state,
        promoted_intent.has_value() &&
            promoted_intent->
                    intent_namespace() ==
                "oros" &&
            promoted_intent->
                    intent_name() ==
                "work" &&
            actor_demo.
                schedule_execution.
                is_interrupted(),
        "Promotion preserves persistent interrupted intent");

    check(
        state,
        promoted_memberships.size() ==
                1U &&
            promoted_memberships.front().
                    actor() ==
                continuity_actor &&
            promoted_memberships.front().
                    faction().
                    faction_namespace() ==
                "oros" &&
            promoted_memberships.front().
                    faction().
                    faction_name() ==
                "citizens",
        "Promotion preserves faction membership");

    Result<NavigationSearchResult>
        post_promotion_navigation_result =
            search_navigation_route(
                navigation_demo.topology,
                empty_overlay,
                navigation_demo.start_node,
                navigation_demo.
                    complete_destination);

    check(
        state,
        post_promotion_navigation_result.
                has_value() &&
            post_promotion_navigation_result.
                value().kind() ==
            NavigationSearchResultKind::
                complete &&
            post_promotion_navigation_result.
                value().route() !=
            nullptr,
        "Navigation is freshly reacquired after promotion");

    if (
        !post_promotion_navigation_result.
            has_value() ||
        post_promotion_navigation_result.
            value().route() ==
            nullptr)
    {
        return 1;
    }

    const auto
        post_promotion_route_nodes =
            post_promotion_navigation_result.
                value().
                route()->
                nodes_in_traversal_order();

    check(
        state,
        !post_promotion_route_nodes.empty() &&
            post_promotion_route_nodes.front() ==
                navigation_demo.start_node,
        "Reacquired route begins at live actor navigation anchor");

    NavigationDemo
        malformed_navigation =
            navigation_demo;

    malformed_navigation.start_node =
        NavigationNodeId{
            navigation_demo.
                start_node.
                cell,
            999ULL
        };

    Result<World>
        malformed_world_result =
            World::create(
                0x4F524F54ULL);

    if (!malformed_world_result.has_value())
    {
        return 1;
    }

    World malformed_world{
        std::move(
            malformed_world_result.value())
    };

    Result<LiveAiActorDemo>
        malformed_actor_result =
            create_live_ai_actor_demo(
                malformed_world,
                malformed_navigation);

    check(
        state,
        !malformed_actor_result.
                has_value() &&
            malformed_actor_result.
                error().code ==
            ErrorCode::invalid_argument,
        "Malformed navigation anchor is rejected");

    check(
        state,
        malformed_world.
                entity_count() ==
            0U &&
            malformed_world.
                position_count() ==
            0U,
        "Malformed navigation anchor creates no orphan World entity");

    std::cout
        << state.checks
        << " checks, "
        << state.failures
        << " failures\n";

    return state.failures == 0
        ? 0
        : 1;
}
