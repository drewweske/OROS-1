#pragma once

#include "navigation_demo.hpp"

#include "oros/ai/actor_faction_membership_set.hpp"
#include "oros/ai/actor_schedule.hpp"
#include "oros/ai/actor_schedule_execution_state.hpp"
#include "oros/ai/actor_simulation_fidelity_registry.hpp"
#include "oros/ai/actor_simulation_focus_policy.hpp"
#include "oros/ai/actor_simulation_focus_source_registry.hpp"
#include "oros/foundation/result.hpp"
#include "oros/world/world.hpp"

namespace oros::bootstrap
{
    struct LiveAiActorDemo final
    {
        world::EntityId actor{};

        ai::ActorSchedule
            schedule{};

        ai::ActorScheduleExecutionState
            schedule_execution;

        ai::ActorFactionMembershipSet
            faction_memberships{};

        ai::ActorSimulationFidelityRegistry
            fidelity_registry{};
    };

    [[nodiscard]]
    foundation::Result<LiveAiActorDemo>
    create_live_ai_actor_demo(
        world::World& world,
        const NavigationDemo& navigation_demo);

    [[nodiscard]]
    foundation::Result<
        ai::ActorSimulationFidelityTransition>
    apply_live_ai_actor_simulation_focus(
        world::World& world,
        LiveAiActorDemo& actor_demo,
        const ai::ActorSimulationFocusPolicy& policy,
        const ai::ActorSimulationFocusSourceRegistry&
            focus_sources);
}
