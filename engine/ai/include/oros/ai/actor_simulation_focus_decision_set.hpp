#pragma once

#include "oros/ai/actor_simulation_fidelity_registry.hpp"
#include "oros/ai/actor_simulation_focus_decision.hpp"
#include "oros/ai/actor_simulation_focus_policy.hpp"
#include "oros/ai/actor_simulation_focus_source_registry.hpp"
#include "oros/ai/actor_simulation_position_snapshot_set.hpp"
#include "oros/foundation/result.hpp"

#include <cstddef>
#include <span>
#include <vector>

namespace oros::ai
{
    class ActorSimulationFocusDecisionSet final
    {
    public:
        [[nodiscard]]
        static foundation::Result<
            ActorSimulationFocusDecisionSet>
        evaluate(
            const ActorSimulationFocusPolicy& policy,
            const ActorSimulationFidelityRegistry&
                fidelity_registry,
            const ActorSimulationPositionSnapshotSet&
                actor_positions,
            const ActorSimulationFocusSourceRegistry&
                focus_sources);

        ActorSimulationFocusDecisionSet(
            const ActorSimulationFocusDecisionSet&) =
                default;

        ActorSimulationFocusDecisionSet&
        operator=(
            const ActorSimulationFocusDecisionSet&) =
                default;

        ActorSimulationFocusDecisionSet(
            ActorSimulationFocusDecisionSet&&)
            noexcept = default;

        ActorSimulationFocusDecisionSet&
        operator=(
            ActorSimulationFocusDecisionSet&&)
            noexcept = default;

        [[nodiscard]]
        std::size_t size() const noexcept;

        [[nodiscard]]
        bool empty() const noexcept;

        [[nodiscard]]
        std::span<
            const ActorSimulationFocusDecision>
        decisions_in_entity_order()
            const noexcept;

    private:
        explicit
        ActorSimulationFocusDecisionSet(
            std::vector<
                ActorSimulationFocusDecision>
                    decisions)
            noexcept;

        std::vector<
            ActorSimulationFocusDecision>
            decisions_{};
    };
}