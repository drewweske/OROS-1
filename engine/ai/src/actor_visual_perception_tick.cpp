#include "oros/ai/actor_visual_perception_tick.hpp"

#include "oros/ai/actor_visual_perception_memory_update.hpp"

#include <algorithm>
#include <new>
#include <utility>
#include <vector>

namespace oros::ai
{
    foundation::Result<ActorVisualPerceptionTickReport>
    observe_actor_visual_perception_tick(
        const world::World& world,
        const physical_world::WorldCellColliderRegistry& colliders,
        const ActorVisionStateRegistry& vision_states,
        const ActorSimulationFidelityRegistry& fidelities,
        ActorVisualObservationMemory& memory,
        const std::span<const ActorVisualPerceptionTickRequest> requests,
        const world::WorldTime observed_at)
    {
        auto less = [](
            const ActorVisualPerceptionTickRequest& a,
            const ActorVisualPerceptionTickRequest& b) noexcept
        {
            if (a.observer != b.observer)
            {
                return a.observer < b.observer;
            }
            return a.target < b.target;
        };

        try
        {
            // Sort first: input iteration order cannot alter observations.
            std::vector<ActorVisualPerceptionTickRequest> sorted{
                requests.begin(), requests.end()
            };
            std::sort(sorted.begin(), sorted.end(), less);

            // Validate the entire request set before mutating even a
            // staged memory. A World may have destroyed an old actor.
            for (std::size_t i = 0U; i < sorted.size(); ++i)
            {
                const auto request = sorted[i];
                if (
                    !request.observer.is_valid() ||
                    !request.target.is_valid() ||
                    request.observer == request.target ||
                    request.observer.world_namespace !=
                        world.world_namespace() ||
                    request.target.world_namespace !=
                        world.world_namespace())
                {
                    return foundation::fail(
                        foundation::ErrorCode::invalid_argument,
                        "Perception batch requires distinct World-namespace entities.");
                }

                if (
                    i > 0U &&
                    sorted[i - 1U].observer == request.observer &&
                    sorted[i - 1U].target == request.target)
                {
                    return foundation::fail(
                        foundation::ErrorCode::invalid_argument,
                        "Perception batch rejects duplicate observer-target pairs.");
                }

                const ActorVisionState* vision =
                    vision_states.find(request.observer);
                const auto fidelity =
                    fidelities.fidelity(request.observer);
                const world::WorldPosition* observer_position =
                    world.find_position(request.observer);
                const world::WorldPosition* target_position =
                    world.find_position(request.target);

                if (
                    !world.contains(request.observer) ||
                    !world.contains(request.target) ||
                    vision == nullptr ||
                    !vision->is_valid() ||
                    vision->actor() != request.observer ||
                    !fidelity.has_value() ||
                    observer_position == nullptr ||
                    target_position == nullptr ||
                    !observer_position->is_normalized() ||
                    !target_position->is_normalized())
                {
                    return foundation::fail(
                        foundation::ErrorCode::invalid_state,
                        "Perception batch requires World-owned positioned "
                        "entities with actor vision and fidelity.");
                }
            }

            // Nothing reaches the caller until every requested observation
            // has been evaluated and all memory updates have succeeded.
            ActorVisualObservationMemory staged_memory{memory};
            ActorVisualPerceptionTickReport report{};
            report.observations.reserve(sorted.size());

            for (const auto request : sorted)
            {
                const auto fidelity =
                    fidelities.fidelity(request.observer);
                if (!fidelity.has_value())
                {
                    return foundation::fail(
                        fidelity.error().code,
                        fidelity.error().message);
                }

                if (
                    fidelity.value() !=
                    ActorSimulationFidelity::deep_local)
                {
                    ++report.skipped_distant_pairs;
                    continue;
                }

                const ActorVisionState* vision =
                    vision_states.find(request.observer);
                // All lookups were verified in the preflight above.
                const world::WorldPosition* observer_position =
                    world.find_position(request.observer);
                const world::WorldPosition* target_position =
                    world.find_position(request.target);

                const auto perception =
                    query_actor_visual_perception(
                        colliders,
                        request.observer,
                        *observer_position,
                        vision->forward(),
                        vision->profile(),
                        request.target,
                        *target_position);

                if (!perception.has_value())
                {
                    return foundation::fail(
                        perception.error().code,
                        perception.error().message);
                }

                const auto memory_update =
                    apply_actor_visual_perception_to_memory(
                        staged_memory,
                        perception.value(),
                        request.observer,
                        request.target,
                        *target_position,
                        observed_at,
                        vision->confidence());

                if (!memory_update.has_value())
                {
                    return foundation::fail(
                        memory_update.error().code,
                        memory_update.error().message);
                }

                report.observations.push_back({
                    request.observer, request.target,
                    perception.value()
                });
            }

            memory = std::move(staged_memory);
            return report;
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::out_of_memory,
                "Perception batch could not allocate working storage.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::internal_failure,
                "Perception batch failed while processing its requests.");
        }
    }
}
