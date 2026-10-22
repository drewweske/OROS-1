#pragma once

#include "live_ai_actor_demo.hpp"

#include "oros/ai/actor_visual_perception_tick.hpp"
#include "oros/foundation/result.hpp"
#include "oros/physical_world/world_cell_collider_registry.hpp"
#include "oros/world/world.hpp"

#include <span>

namespace oros::bootstrap
{
    // Bridge live actor-owned vision, fidelity and memory to the
    // reusable multi-NPC tick. No persistent duplicate registries.
    //
    // Caller supplies stable pointers and target identities. All
    // observations and memory replacements are transactional.
    // This bootstrap adapter stages copies per tick; it is not the
    // eventual large-population perception scheduler.
    [[nodiscard]]
    foundation::Result<ai::ActorVisualPerceptionTickReport>
    observe_live_ai_actor_group(
        const world::World& world,
        const physical_world::WorldCellColliderRegistry& colliders,
        std::span<LiveAiActorDemo* const> actors,
        std::span<const world::EntityId> targets,
        world::WorldTime observed_at);
}
