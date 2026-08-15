#pragma once

#include "oros/ai/actor_visual_observation.hpp"
#include "oros/ai/actor_visual_observation_memory.hpp"
#include "oros/ai/actor_visual_perception.hpp"
#include "oros/foundation/result.hpp"
#include "oros/world/entity_id.hpp"
#include "oros/world/world_position.hpp"
#include "oros/world/world_time.hpp"

#include <cstdint>

namespace oros::ai
{
    enum class
        ActorVisualPerceptionMemoryUpdateState :
            std::uint8_t
    {
        invalid = 0,
        preserved,
        remembered
    };

    [[nodiscard]]
    constexpr bool
    is_valid_actor_visual_perception_memory_update_state(
        const ActorVisualPerceptionMemoryUpdateState
            state)
        noexcept
    {
        return
            state ==
                ActorVisualPerceptionMemoryUpdateState::
                    preserved ||
            state ==
                ActorVisualPerceptionMemoryUpdateState::
                    remembered;
    }

    [[nodiscard]]
    foundation::Result<
        ActorVisualPerceptionMemoryUpdateState>
    apply_actor_visual_perception_to_memory(
        ActorVisualObservationMemory& memory,
        ActorVisualPerceptionState
            perception_state,
        world::EntityId observer,
        world::EntityId target,
        const world::WorldPosition&
            target_position,
        world::WorldTime observed_at,
        ActorVisualObservationConfidence
            confidence);
}