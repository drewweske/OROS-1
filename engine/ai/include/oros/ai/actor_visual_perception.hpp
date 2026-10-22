#pragma once

#include "oros/ai/actor_vision_profile.hpp"
#include "oros/foundation/result.hpp"
#include "oros/physical_world/world_cell_collider_registry.hpp"
#include "oros/world/entity_id.hpp"
#include "oros/world/world_position.hpp"

#include <cstdint>

namespace oros::ai
{
    enum class ActorVisualPerceptionState :
        std::uint8_t
    {
        invalid = 0,
        out_of_range,
        outside_field_of_view,
        visible,
        occluded,
        unavailable
    };

    [[nodiscard]]
    constexpr bool
    is_valid_actor_visual_perception_state(
        const ActorVisualPerceptionState state)
        noexcept
    {
        return
            state ==
                ActorVisualPerceptionState::
                    out_of_range ||
            state ==
                ActorVisualPerceptionState::
                    outside_field_of_view ||
            state ==
                ActorVisualPerceptionState::
                    visible ||
            state ==
                ActorVisualPerceptionState::
                    occluded ||
            state ==
                ActorVisualPerceptionState::
                    unavailable;
    }

    [[nodiscard]]
    foundation::Result<
        ActorVisualPerceptionState>
    query_actor_visual_perception(
        const physical_world::
            WorldCellColliderRegistry& registry,
        world::EntityId observer,
        const world::WorldPosition&
            observer_position,
        const ActorVisionDirection&
            observer_forward,
        const ActorVisionProfile&
            vision_profile,
        world::EntityId target,
        const world::WorldPosition&
            target_position);
}