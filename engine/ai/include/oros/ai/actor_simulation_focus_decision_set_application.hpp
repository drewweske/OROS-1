#pragma once

#include "oros/ai/actor_simulation_fidelity_registry.hpp"
#include "oros/ai/actor_simulation_focus_decision_set.hpp"
#include "oros/foundation/result.hpp"

namespace oros::ai
{
    [[nodiscard]]
    foundation::Status
    apply_actor_simulation_focus_decision_set_if_fidelities_match(
        ActorSimulationFidelityRegistry& registry,
        const ActorSimulationFocusDecisionSet& decision_set);
}