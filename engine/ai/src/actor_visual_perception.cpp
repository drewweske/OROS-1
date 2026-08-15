#include "oros/ai/actor_visual_perception.hpp"

#include "oros/physical_world/world_segment_query.hpp"

namespace oros::ai
{
    foundation::Result<
        ActorVisualPerceptionState>
    query_actor_visual_perception(
        const physical_world::
            WorldCellColliderRegistry& registry,
        const world::EntityId observer,
        const world::WorldPosition&
            observer_position,
        const ActorVisionDirection&
            observer_forward,
        const ActorVisionProfile&
            vision_profile,
        const world::EntityId target,
        const world::WorldPosition&
            target_position)
    {
        if (
            !observer.is_valid() ||
            !target.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor visual perception requires "
                "valid observer and target EntityId "
                "values.");
        }

        if (
            observer.world_namespace !=
            target.world_namespace)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor visual perception observer "
                "and target must belong to the same "
                "world namespace.");
        }

        if (observer == target)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor visual perception requires "
                "distinct observer and target "
                "EntityId values.");
        }

        const auto vision_field_result =
            vision_profile.classify(
                observer_position,
                observer_forward,
                target_position);

        if (!vision_field_result.has_value())
        {
            return foundation::fail(
                vision_field_result.error().code,
                vision_field_result.error().message);
        }

        switch (vision_field_result.value())
        {
        case ActorVisionFieldState::out_of_range:
            return
                ActorVisualPerceptionState::
                    out_of_range;

        case ActorVisionFieldState::
            outside_field_of_view:
            return
                ActorVisualPerceptionState::
                    outside_field_of_view;

        case ActorVisionFieldState::
            within_vision_field:
            break;

        case ActorVisionFieldState::invalid:
        default:
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Actor vision field classification "
                "returned an invalid state.");
        }

        const auto filter_result =
            physical_world::
                WorldSegmentQueryFilter::
                    create_excluding_owner(
                        observer);

        if (!filter_result.has_value())
        {
            return foundation::fail(
                filter_result.error().code,
                filter_result.error().message);
        }

        const auto segment_result =
            physical_world::
                query_world_segment(
                    registry,
                    observer.world_namespace,
                    observer_position,
                    target_position,
                    filter_result.value());

        if (!segment_result.has_value())
        {
            return foundation::fail(
                segment_result.error().code,
                segment_result.error().message);
        }

        const physical_world::
            WorldSegmentQueryResult&
            segment =
                segment_result.value();

        if (!segment.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Physical World returned an invalid "
                "segment-query result to visual "
                "perception.");
        }

        switch (segment.state())
        {
        case physical_world::
            WorldSegmentQueryState::clear:
            return
                ActorVisualPerceptionState::
                    visible;

        case physical_world::
            WorldSegmentQueryState::unavailable:
            return
                ActorVisualPerceptionState::
                    unavailable;

        case physical_world::
            WorldSegmentQueryState::blocked:
        {
            if (!segment.hit().has_value())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_state,
                    "Blocked Physical World segment "
                    "query did not provide a collider "
                    "hit.");
            }

            if (
                segment.hit()->
                        collider().
                        owner ==
                    target)
            {
                return
                    ActorVisualPerceptionState::
                        visible;
            }

            return
                ActorVisualPerceptionState::
                    occluded;
        }

        case physical_world::
            WorldSegmentQueryState::invalid:
        default:
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Physical World returned an invalid "
                "segment state to visual perception.");
        }
    }
}