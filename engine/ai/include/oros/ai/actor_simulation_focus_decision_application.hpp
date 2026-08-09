#pragma once

#include "oros/ai/actor_simulation_fidelity_registry.hpp"
#include "oros/ai/actor_simulation_focus_decision.hpp"
#include "oros/foundation/result.hpp"

namespace oros::ai
{
    [[nodiscard]]
    foundation::Result<
        ActorSimulationFidelityTransition>
    apply_actor_simulation_focus_decision_if_fidelity_matches(
        ActorSimulationFidelityRegistry& registry,
        const ActorSimulationFocusDecision& decision);
}