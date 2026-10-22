#include "live_ai_perception_group_demo.hpp"

#include "oros/ai/actor_simulation_fidelity_registry.hpp"
#include "oros/ai/actor_vision_state_registry.hpp"
#include "oros/ai/actor_visual_observation_memory.hpp"

#include <algorithm>
#include <limits>
#include <new>
#include <utility>
#include <vector>

namespace oros::bootstrap
{
    foundation::Result<ai::ActorVisualPerceptionTickReport>
    observe_live_ai_actor_group(
        const world::World& world,
        const physical_world::WorldCellColliderRegistry& colliders,
        const std::span<LiveAiActorDemo* const> actors,
        const std::span<const world::EntityId> targets,
        const world::WorldTime observed_at,
        const std::size_t maximum_requests)
    {
        if (actors.empty())
        {
            return foundation::fail(
                foundation::ErrorCode::invalid_argument,
                "Live perception group requires at least one NPC.");
        }

        if (
            targets.size() >
                (std::numeric_limits<std::size_t>::max)() /
                    actors.size())
        {
            return foundation::fail(
                foundation::ErrorCode::invalid_argument,
                "Live perception group request count overflows.");
        }

        if (actors.size() * targets.size() > maximum_requests)
        {
            return foundation::fail(
                foundation::ErrorCode::invalid_argument,
                "Live perception group exceeds its request budget.");
        }

        try
        {
            std::vector<LiveAiActorDemo*> ordered{
                actors.begin(), actors.end()
            };

            if (std::any_of(
                    ordered.begin(), ordered.end(),
                    [](const LiveAiActorDemo* actor)
                    {
                        return actor == nullptr;
                    }))
            {
                return foundation::fail(
                    foundation::ErrorCode::invalid_argument,
                    "Live perception group contains a null actor.");
            }

            std::sort(
                ordered.begin(), ordered.end(),
                [](const LiveAiActorDemo* a, const LiveAiActorDemo* b)
                {
                    return a->actor < b->actor;
                });

            ai::ActorVisionStateRegistry visions{};
            ai::ActorSimulationFidelityRegistry fidelities{};
            ai::ActorVisualObservationMemory combined_memory{};

            for (std::size_t i = 0U; i < ordered.size(); ++i)
            {
                LiveAiActorDemo& actor = *ordered[i];

                if (
                    !actor.actor.is_valid() ||
                    actor.actor.world_namespace !=
                        world.world_namespace() ||
                    !world.contains(actor.actor) ||
                    !actor.vision_state.has_value() ||
                    !actor.vision_state->is_valid() ||
                    actor.vision_state->actor() != actor.actor ||
                    actor.fidelity_registry.size() != 1U ||
                    !actor.fidelity_registry.contains(actor.actor) ||
                    (i > 0U &&
                        ordered[i - 1U]->actor == actor.actor))
                {
                    return foundation::fail(
                        foundation::ErrorCode::invalid_state,
                        "Live perception group requires unique, "
                        "World-owned NPC state.");
                }

                const auto fidelity =
                    actor.fidelity_registry.fidelity(actor.actor);
                if (!fidelity.has_value())
                {
                    return foundation::fail(
                        fidelity.error().code,
                        fidelity.error().message);
                }

                const auto vision_status =
                    visions.insert(*actor.vision_state);
                const auto fidelity_status =
                    fidelities.insert(actor.actor, fidelity.value());

                if (!vision_status.has_value() ||
                    !fidelity_status.has_value())
                {
                    return foundation::fail(
                        foundation::ErrorCode::invalid_state,
                        "Cannot assemble live NPC vision/fidelity.");
                }

                for (const auto& memory :
                    actor.visual_memory.observations_in_canonical_order())
                {
                    if (memory.observer() != actor.actor)
                    {
                        return foundation::fail(
                            foundation::ErrorCode::invalid_state,
                            "NPC memory contains another observer's fact.");
                    }

                    const auto remembered =
                        combined_memory.remember(memory);
                    if (!remembered.has_value())
                    {
                        return foundation::fail(
                            remembered.error().code,
                            remembered.error().message);
                    }
                }
            }

            std::vector<ai::ActorVisualPerceptionTickRequest>
                requests{};
            requests.reserve(ordered.size() * targets.size());

            for (const LiveAiActorDemo* actor : ordered)
            {
                for (const world::EntityId target : targets)
                {
                    requests.push_back({actor->actor, target});
                }
            }

            auto result = ai::observe_actor_visual_perception_tick(
                world, colliders, visions, fidelities,
                combined_memory, requests, observed_at,
                maximum_requests);

            if (!result.has_value())
            {
                return foundation::fail(
                    result.error().code,
                    result.error().message);
            }

            // The reusable tick stages memory only on visibility.
            // Skip expensive actor-by-actor rebuilding when every
            // pair was distant, occluded, out of range or unavailable.
            if (!result->visual_memory_staged)
            {
                return std::move(result.value());
            }

            // Prepare replacement memories before committing any
            // actor. Later allocation failure cannot corrupt peers.
            std::vector<ai::ActorVisualObservationMemory> staged{};
            staged.reserve(ordered.size());

            for (const LiveAiActorDemo* actor : ordered)
            {
                ai::ActorVisualObservationMemory next{};
                const auto owned =
                    combined_memory.observations_for_observer(actor->actor);

                if (!owned.has_value())
                {
                    return foundation::fail(
                        owned.error().code,
                        owned.error().message);
                }

                // Combined memory is already sorted and unique.
                // Bulk replacement validates that contract, copies
                // each actor's slice once, and publishes atomically.
                const auto status =
                    next.replace_canonical(owned.value());
                if (!status.has_value())
                {
                    return foundation::fail(
                        status.error().code,
                        status.error().message);
                }

                staged.push_back(std::move(next));
            }

            for (std::size_t i = 0U; i < ordered.size(); ++i)
            {
                ordered[i]->visual_memory = std::move(staged[i]);
            }

            return std::move(result.value());
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::out_of_memory,
                "Live perception group could not allocate storage.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::internal_failure,
                "Live perception group failed unexpectedly.");
        }
    }
}
