#pragma once

#include "oros/ai/actor_simulation_fidelity_registry.hpp"
#include "oros/ai/actor_vision_state_registry.hpp"
#include "oros/ai/actor_visual_observation_memory.hpp"
#include "oros/ai/actor_visual_perception.hpp"
#include "oros/foundation/result.hpp"
#include "oros/physical_world/world_cell_collider_registry.hpp"
#include "oros/world/world.hpp"
#include "oros/world/world_time.hpp"

#include <cstddef>
#include <limits>
#include <span>
#include <vector>

namespace oros::ai
{
    // The caller selects the observable pairs; discovery and visibility
    // budgets are separate responsibilities. Input order is irrelevant.
    struct ActorVisualPerceptionTickRequest final
    {
        world::EntityId observer{};
        world::EntityId target{};
    };

    struct ActorVisualPerceptionTickObservation final
    {
        world::EntityId observer{};
        world::EntityId target{};
        ActorVisualPerceptionState perception{
            ActorVisualPerceptionState::invalid
        };
    };

    struct ActorVisualPerceptionTickReport final
    {
        // Evaluated pairs in (observer, target) EntityId order.
        // Deep-local observers only; distant ones are skipped.
        std::vector<ActorVisualPerceptionTickObservation> observations{};
        std::size_t skipped_distant_pairs{};
        // True if a visible pair caused transactional memory staging.
        // Identical same-time observations can still stage a no-op;
        // false means original memory storage was never rebuilt.
        bool visual_memory_staged{};
    };

    // Fully deterministic for a given World, collider residency, actor
    // registries, request set, memory and explicit simulation WorldTime.
    //
    // Rejects duplicate pairs and invalid/missing World-owned entities.
    // A failed batch leaves the original visual memory unchanged, even
    // when an earlier pair would otherwise update it.
    //
    // maximum_requests is a hard caller-supplied budget. If the
    // request set exceeds it, the entire batch fails without any
    // expensive query, allocation or visual-memory mutation.
    //
    // A memory copy is staged only when the batch includes at least one
    // visible pair. Purely skipped/nonvisible batches keep their memory
    // storage untouched. This does NOT eliminate copying for visible
    // updates, and no throughput gains are assumed without profiling.
    // The API does not discover NPCs or targets.
    [[nodiscard]]
    foundation::Result<ActorVisualPerceptionTickReport>
    observe_actor_visual_perception_tick(
        const world::World& world,
        const physical_world::WorldCellColliderRegistry& colliders,
        const ActorVisionStateRegistry& vision_states,
        const ActorSimulationFidelityRegistry& fidelities,
        ActorVisualObservationMemory& memory,
        std::span<const ActorVisualPerceptionTickRequest> requests,
        world::WorldTime observed_at,
        std::size_t maximum_requests =
            (std::numeric_limits<std::size_t>::max)());
}
