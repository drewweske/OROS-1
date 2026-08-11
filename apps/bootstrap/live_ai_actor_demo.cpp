#include "live_ai_actor_demo.hpp"

#include "oros/ai/actor_activity_intent_key.hpp"
#include "oros/ai/actor_faction_membership.hpp"
#include "oros/ai/actor_scheduled_activity.hpp"
#include "oros/ai/actor_schedule_window.hpp"
#include "oros/ai/actor_simulation_focus_decision_set.hpp"
#include "oros/ai/actor_simulation_focus_decision_set_application.hpp"
#include "oros/ai/actor_simulation_position_snapshot_set.hpp"
#include "oros/ai/faction_key.hpp"
#include "oros/ai/navigation_node_record.hpp"
#include "oros/world/world_time.hpp"

#include <expected>
#include <utility>

namespace oros::bootstrap
{
    namespace
    {
        [[nodiscard]]
        const ai::NavigationNodeRecord*
        find_navigation_anchor(
            const NavigationDemo& navigation_demo)
            noexcept
        {
            if (
                !navigation_demo.
                    start_node.
                    is_valid())
            {
                return nullptr;
            }

            const ai::NavigationCellTopology*
                cell_topology =
                    navigation_demo.
                        topology.
                        find_cell_topology(
                            navigation_demo.
                                start_node.
                                cell);

            if (cell_topology == nullptr)
            {
                return nullptr;
            }

            for (
                const ai::NavigationNodeRecord&
                    node :
                cell_topology->
                    nodes_in_canonical_order())
            {
                if (
                    node.id() ==
                    navigation_demo.start_node)
                {
                    return &node;
                }
            }

            return nullptr;
        }

        [[nodiscard]]
        foundation::Result<LiveAiActorDemo>
        rollback_actor_and_fail(
            world::World& world,
            const world::EntityId actor,
            foundation::Error error)
        {
            const foundation::Status
                rollback_status =
                    world.destroy_entity(
                        actor);

            if (!rollback_status.has_value())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "Live AI actor composition "
                    "failed and World entity "
                    "rollback also failed.");
            }

            return
                std::unexpected<
                    foundation::Error>{
                        std::move(error)
                    };
        }
    }

    foundation::Result<LiveAiActorDemo>
    create_live_ai_actor_demo(
        world::World& world,
        const NavigationDemo& navigation_demo)
    {
        const ai::NavigationNodeRecord*
            navigation_anchor =
                find_navigation_anchor(
                    navigation_demo);

        if (navigation_anchor == nullptr)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Live AI actor composition "
                "requires a supplied navigation "
                "start node.");
        }

        foundation::Result<
            world::EntityId>
            actor_result =
                world.create_entity();

        if (!actor_result.has_value())
        {
            return
                std::unexpected<
                    foundation::Error>{
                        std::move(
                            actor_result.error())
                    };
        }

        const world::EntityId actor =
            actor_result.value();

        foundation::Status
            position_status =
                world.add_position(
                    actor,
                    navigation_anchor->
                        position());

        if (!position_status.has_value())
        {
            return rollback_actor_and_fail(
                world,
                actor,
                std::move(
                    position_status.error()));
        }

        foundation::Result<
            ai::ActorActivityIntentKey>
            intent_result =
                ai::ActorActivityIntentKey::
                    create(
                        "oros",
                        "work");

        if (!intent_result.has_value())
        {
            return rollback_actor_and_fail(
                world,
                actor,
                std::move(
                    intent_result.error()));
        }

        foundation::Result<
            ai::ActorScheduleWindow>
            window_result =
                ai::ActorScheduleWindow::
                    create(
                        world::WorldTime::
                            epoch(),
                        world::WorldTime::
                            from_microseconds_since_epoch(
                                1'000'000ULL));

        if (!window_result.has_value())
        {
            return rollback_actor_and_fail(
                world,
                actor,
                std::move(
                    window_result.error()));
        }

        ai::ActorScheduledActivity
            activity{
                std::move(
                    intent_result.value()),
                std::move(
                    window_result.value())
            };

        ai::ActorSchedule schedule{};

        foundation::Status
            schedule_status =
                schedule.insert(
                    activity);

        if (!schedule_status.has_value())
        {
            return rollback_actor_and_fail(
                world,
                actor,
                std::move(
                    schedule_status.error()));
        }

        foundation::Result<
            ai::ActorScheduleExecutionState>
            schedule_execution_result =
                ai::ActorScheduleExecutionState::
                    create_following(
                        actor);

        if (
            !schedule_execution_result.
                has_value())
        {
            return rollback_actor_and_fail(
                world,
                actor,
                std::move(
                    schedule_execution_result.
                        error()));
        }

        foundation::Status
            synchronization_status =
                schedule_execution_result.
                    value().
                    synchronize_following_intent_from_schedule(
                        schedule,
                        world::WorldTime::
                            from_microseconds_since_epoch(
                                500'000ULL));

        if (!synchronization_status.has_value())
        {
            return rollback_actor_and_fail(
                world,
                actor,
                std::move(
                    synchronization_status.
                        error()));
        }

        foundation::Result<
            ai::FactionKey>
            faction_result =
                ai::FactionKey::create(
                    "oros",
                    "citizens");

        if (!faction_result.has_value())
        {
            return rollback_actor_and_fail(
                world,
                actor,
                std::move(
                    faction_result.error()));
        }

        foundation::Result<
            ai::ActorFactionMembership>
            membership_result =
                ai::ActorFactionMembership::
                    create(
                        actor,
                        faction_result.value());

        if (!membership_result.has_value())
        {
            return rollback_actor_and_fail(
                world,
                actor,
                std::move(
                    membership_result.error()));
        }

        ai::ActorFactionMembershipSet
            faction_memberships{};

        foundation::Status
            membership_status =
                faction_memberships.insert(
                    membership_result.value());

        if (!membership_status.has_value())
        {
            return rollback_actor_and_fail(
                world,
                actor,
                std::move(
                    membership_status.error()));
        }

        ai::ActorSimulationFidelityRegistry
            fidelity_registry{};

        foundation::Status
            fidelity_status =
                fidelity_registry.insert(
                    actor,
                    ai::ActorSimulationFidelity::
                        deep_local);

        if (!fidelity_status.has_value())
        {
            return rollback_actor_and_fail(
                world,
                actor,
                std::move(
                    fidelity_status.error()));
        }

        return LiveAiActorDemo{
            actor,
            std::move(schedule),
            std::move(
                schedule_execution_result.
                    value()),
            std::move(
                faction_memberships),
            std::move(
                fidelity_registry)
        };
    }

    foundation::Result<
        ai::ActorSimulationFidelityTransition>
    apply_live_ai_actor_simulation_focus(
        world::World& world,
        LiveAiActorDemo& actor_demo,
        const ai::ActorSimulationFocusPolicy& policy,
        const ai::ActorSimulationFocusSourceRegistry&
            focus_sources)
    {
        if (
            !actor_demo.actor.is_valid() ||
            !world.contains(
                actor_demo.actor) ||
            actor_demo.
                    fidelity_registry.
                    size() !=
                1U ||
            !actor_demo.
                fidelity_registry.
                contains(
                    actor_demo.actor))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Live AI actor focus application "
                "requires one World-owned actor "
                "with one matching fidelity state.");
        }

        const world::WorldPosition*
            actor_position =
                world.find_position(
                    actor_demo.actor);

        if (
            actor_position == nullptr ||
            !actor_position->is_normalized())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Live AI actor focus application "
                "requires an authoritative "
                "normalized WorldPosition.");
        }

        ai::ActorSimulationPositionSnapshotSet
            position_snapshots{};

        foundation::Status
            snapshot_status =
                position_snapshots.insert(
                    actor_demo.actor,
                    *actor_position);

        if (!snapshot_status.has_value())
        {
            return foundation::fail(
                snapshot_status.error().code,
                snapshot_status.error().message);
        }

        foundation::Result<
            ai::ActorSimulationFocusDecisionSet>
            decision_set_result =
                ai::ActorSimulationFocusDecisionSet::
                    evaluate(
                        policy,
                        actor_demo.
                            fidelity_registry,
                        position_snapshots,
                        focus_sources);

        if (!decision_set_result.has_value())
        {
            return foundation::fail(
                decision_set_result.error().code,
                decision_set_result.error().message);
        }

        const ai::ActorSimulationFocusDecisionSet&
            decision_set =
                decision_set_result.value();

        const auto decisions =
            decision_set.
                decisions_in_entity_order();

        if (
            decisions.size() != 1U ||
            decisions.front().actor() !=
                actor_demo.actor ||
            decisions.front().transition() ==
                ai::
                    ActorSimulationFidelityTransition::
                        invalid)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "Live AI actor focus evaluation "
                "did not produce exactly one valid "
                "decision for the live actor.");
        }

        const ai::
            ActorSimulationFidelityTransition
            transition =
                decisions.front().
                    transition();

        foundation::Status
            application_status =
                ai::
                    apply_actor_simulation_focus_decision_set_if_fidelities_match(
                        actor_demo.
                            fidelity_registry,
                        decision_set);

        if (!application_status.has_value())
        {
            return foundation::fail(
                application_status.error().code,
                application_status.error().message);
        }

        return transition;
    }
}
