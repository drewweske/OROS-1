#include "live_ai_actor_lifecycle_demo.hpp"

#include "oros/foundation/error.hpp"

#include <cstddef>

namespace oros::bootstrap
{
    foundation::Status retire_live_ai_actor(
        world::World& world,
        LiveAiActorDemo& retiring,
        const std::span<LiveAiActorDemo* const> active_roster)
    {
        if (!retiring.actor.is_valid() ||
            !world.contains(retiring.actor) ||
            active_roster.empty())
        {
            return foundation::fail(
                foundation::ErrorCode::invalid_state,
                "Retiring an NPC requires a living World identity "
                "and a nonempty active roster.");
        }

        std::size_t retiree_count{};
        // Validate first: invalid rosters must not partially destroy
        // entities, discard memories or invalidate fidelity state.
        for (std::size_t i = 0U; i < active_roster.size(); ++i)
        {
            const LiveAiActorDemo* const actor = active_roster[i];
            if (actor == nullptr ||
                !actor->actor.is_valid() ||
                !world.contains(actor->actor) ||
                !actor->vision_state.has_value() ||
                !actor->vision_state->is_valid() ||
                actor->vision_state->actor() != actor->actor ||
                actor->fidelity_registry.size() != 1U ||
                !actor->fidelity_registry.contains(actor->actor))
            {
                return foundation::fail(
                    foundation::ErrorCode::invalid_state,
                    "Retirement requires valid live actor state.");
            }

            if (actor == &retiring)
            {
                ++retiree_count;
            }

            for (std::size_t j = 0U; j < i; ++j)
            {
                if (active_roster[j] == actor ||
                    active_roster[j]->actor == actor->actor)
                {
                    return foundation::fail(
                        foundation::ErrorCode::invalid_state,
                        "Retirement roster contains duplicate actors.");
                }
            }
            for (const auto& fact :
                actor->visual_memory.observations_in_canonical_order())
            {
                if (fact.observer() != actor->actor)
                {
                    return foundation::fail(
                        foundation::ErrorCode::invalid_state,
                        "Retirement roster memory belongs to "
                        "another observer.");
                }
            }
        }

        if (retiree_count != 1U)
        {
            return foundation::fail(
                foundation::ErrorCode::invalid_state,
                "Retired NPC must occur exactly once in the roster.");
        }

        const world::EntityId retired_id = retiring.actor;
        // World destruction removes the authoritative position.
        // The validated post-destruction purges are no-allocating.
        const auto destroyed = world.destroy_entity(retired_id);
        if (!destroyed.has_value())
        {
            return foundation::fail(
                destroyed.error().code, destroyed.error().message);
        }

        for (LiveAiActorDemo* const actor : active_roster)
        {
            static_cast<void>(
                actor->visual_memory.forget_entity(retired_id));
        }

        const auto removed =
            retiring.fidelity_registry.remove(retired_id);
        if (!removed.has_value())
        {
            // All preconditions were checked before World mutation;
            // reaching here indicates an internal contract failure.
            return foundation::fail(
                foundation::ErrorCode::internal_failure,
                "Validated retired NPC fidelity disappeared.");
        }
        retiring.vision_state.reset();
        return {};
    }
}
