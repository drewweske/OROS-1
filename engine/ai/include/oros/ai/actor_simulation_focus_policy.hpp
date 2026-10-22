#pragma once

#include "oros/ai/actor_simulation_fidelity_state.hpp"
#include "oros/foundation/result.hpp"
#include "oros/world/world_position.hpp"

#include <span>

namespace oros::ai
{
    class ActorSimulationFocusPolicy final
    {
    public:
        [[nodiscard]]
        static foundation::Result<
            ActorSimulationFocusPolicy>
        create(
            double promotion_distance_meters,
            double retention_distance_meters);

        [[nodiscard]]
        foundation::Result<
            ActorSimulationFidelity>
        evaluate(
            ActorSimulationFidelity current_fidelity,
            const world::WorldPosition& actor_position,
            std::span<
                const world::WorldPosition>
                simulation_focus_positions) const;

        [[nodiscard]]
        double promotion_distance_meters()
            const noexcept;

        [[nodiscard]]
        double retention_distance_meters()
            const noexcept;

    private:
        ActorSimulationFocusPolicy(
            double promotion_distance_meters,
            double retention_distance_meters)
            noexcept;

        double promotion_distance_meters_{};
        double retention_distance_meters_{};
    };
}