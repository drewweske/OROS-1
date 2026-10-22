#include "oros/ai/actor_visual_perception_memory_update.hpp"

#include <utility>

namespace oros::ai
{
    foundation::Result<
        ActorVisualPerceptionMemoryUpdateState>
    apply_actor_visual_perception_to_memory(
        ActorVisualObservationMemory& memory,
        const ActorVisualPerceptionState
            perception_state,
        const world::EntityId observer,
        const world::EntityId target,
        const world::WorldPosition&
            target_position,
        const world::WorldTime observed_at,
        ActorVisualObservationConfidence
            confidence)
    {
        if (
            !is_valid_actor_visual_perception_state(
                perception_state))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Visual perception memory update "
                "requires a valid perception state.");
        }

        if (
            !observer.is_valid() ||
            !target.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Visual perception memory update "
                "requires valid observer and target "
                "EntityId values.");
        }

        if (
            observer.world_namespace !=
            target.world_namespace)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Visual perception memory update "
                "observer and target must belong to "
                "the same world namespace.");
        }

        if (observer == target)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Visual perception memory update "
                "requires distinct observer and "
                "target EntityId values.");
        }

        switch (perception_state)
        {
        case ActorVisualPerceptionState::
            out_of_range:
        case ActorVisualPerceptionState::
            outside_field_of_view:
        case ActorVisualPerceptionState::
            occluded:
        case ActorVisualPerceptionState::
            unavailable:
            return
                ActorVisualPerceptionMemoryUpdateState::
                    preserved;

        case ActorVisualPerceptionState::
            visible:
            break;

        case ActorVisualPerceptionState::
            invalid:
        default:
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Visual perception memory update "
                "received an invalid perception "
                "state after validation.");
        }

        const auto observation_result =
            ActorVisualObservation::create(
                observer,
                target,
                target_position,
                observed_at,
                std::move(confidence));

        if (!observation_result.has_value())
        {
            return foundation::fail(
                observation_result.error().code,
                observation_result.error().message);
        }

        const foundation::Status
            remember_status =
                memory.remember(
                    observation_result.value());

        if (!remember_status.has_value())
        {
            return foundation::fail(
                remember_status.error().code,
                remember_status.error().message);
        }

        return
            ActorVisualPerceptionMemoryUpdateState::
                remembered;
    }
}