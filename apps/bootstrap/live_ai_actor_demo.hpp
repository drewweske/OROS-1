#pragma once

#include "navigation_demo.hpp"

#include "oros/ai/actor_faction_membership_set.hpp"
#include "oros/ai/actor_schedule.hpp"
#include "oros/ai/actor_schedule_execution_state.hpp"
#include "oros/ai/actor_simulation_fidelity_registry.hpp"
#include "oros/ai/actor_simulation_focus_policy.hpp"
#include "oros/ai/actor_simulation_focus_source_registry.hpp"
#include "oros/ai/actor_vision_profile.hpp"
#include "oros/ai/actor_vision_state.hpp"
#include "oros/ai/actor_visual_observation.hpp"
#include "oros/ai/actor_visual_observation_memory.hpp"
#include "oros/ai/actor_visual_perception.hpp"

#include "oros/foundation/result.hpp"

#include "oros/physical_world/world_cell_collider_registry.hpp"

#include "oros/world/entity_id.hpp"
#include "oros/world/world.hpp"
#include "oros/world/world_time.hpp"

#include <optional>

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

        // The actor retains observations independently
        // of current visibility and simulation fidelity.
        ai::ActorVisualObservationMemory
            visual_memory{};

        // This actor's validated facing and sensing settings.
        // Initialized during the next composition increment.
        std::optional<ai::ActorVisionState>
            vision_state{};
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

    // Evaluate an observer's view of a target.
    // Visible targets update remembered observations.
    // Unseen targets retain previous knowledge.
    //
    // Implemented in live_ai_actor_demo.cpp.
    [[nodiscard]]
    foundation::Result<
        ai::ActorVisualPerceptionState>
    observe_live_ai_actor_target(
        const world::World& world,
        LiveAiActorDemo& actor_demo,
        const physical_world::
            WorldCellColliderRegistry& colliders,
        world::EntityId target,
        const ai::ActorVisionDirection&
            observer_forward,
        const ai::ActorVisionProfile&
            vision_profile,
        world::WorldTime observed_at,
        ai::ActorVisualObservationConfidence
            confidence);
}
