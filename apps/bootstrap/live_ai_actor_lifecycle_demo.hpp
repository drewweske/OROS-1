#pragma once

#include "live_ai_actor_demo.hpp"

#include "oros/foundation/result.hpp"
#include "oros/world/world.hpp"

#include <span>

namespace oros::bootstrap
{
    // Explicit terminal NPC despawn. Validates the entire supplied
    // live roster before any mutation. On success, the World entity
    // and WorldPosition are destroyed, the retired NPC's sensing
    // configuration is removed and every roster member drops facts
    // about the retired entity (including observer-owned facts).
    //
    // Caller MUST remove the retired pointer from subsequent rosters;
    // the LiveAiActorDemo object itself is intentionally not destroyed.
    // This prototype owns only visual/fidelity cleanup; schedules,
    // faction relations, and wider gameplay ownership are future work.
    [[nodiscard]]
    foundation::Status retire_live_ai_actor(
        world::World& world,
        LiveAiActorDemo& retiring,
        std::span<LiveAiActorDemo* const> active_roster);
}
