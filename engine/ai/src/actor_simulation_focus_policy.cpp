#include "oros/ai/actor_simulation_focus_policy.hpp"

#include <cmath>

namespace oros::ai
{
    foundation::Result<
        ActorSimulationFocusPolicy>
    ActorSimulationFocusPolicy::create(
        const double promotion_distance_meters,
        const double retention_distance_meters)
    {
        if (
            !std::isfinite(
                promotion_distance_meters) ||
            !std::isfinite(
                retention_distance_meters) ||
            promotion_distance_meters < 0.0 ||
            retention_distance_meters < 0.0)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor simulation focus distances "
                "must be finite and non-negative.");
        }

        if (
            promotion_distance_meters >
                retention_distance_meters)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor simulation promotion distance "
                "cannot exceed retention distance.");
        }

        return ActorSimulationFocusPolicy{
            promotion_distance_meters,
            retention_distance_meters
        };
    }

    foundation::Result<
        ActorSimulationFidelity>
    ActorSimulationFocusPolicy::evaluate(
        const ActorSimulationFidelity
            current_fidelity,
        const world::WorldPosition&
            actor_position,
        const std::span<
            const world::WorldPosition>
            simulation_focus_positions) const
    {
        if (!is_valid_actor_simulation_fidelity(
                current_fidelity))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor simulation focus evaluation "
                "requires a valid current fidelity.");
        }

        if (!actor_position.is_normalized())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor simulation focus evaluation "
                "requires a normalized actor "
                "WorldPosition.");
        }

        if (simulation_focus_positions.empty())
        {
            return
                ActorSimulationFidelity::
                    statistical_distant;
        }

        const double active_distance_meters =
            current_fidelity ==
                ActorSimulationFidelity::
                    statistical_distant
                ? promotion_distance_meters_
                : retention_distance_meters_;

        for (
            const world::WorldPosition&
                focus_position :
            simulation_focus_positions)
        {
            if (!focus_position.is_normalized())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Simulation focus positions must "
                    "be normalized WorldPositions.");
            }

            const foundation::Result<
                world::WorldDisplacement>
                displacement_result =
                    actor_position.displacement_to(
                        focus_position);

            if (!displacement_result.has_value())
            {
                return foundation::fail(
                    displacement_result.error().code,
                    displacement_result.error().message);
            }

            const world::WorldDisplacement&
                displacement =
                    displacement_result.value();

            const double distance_meters =
                std::hypot(
                    displacement.x,
                    displacement.y,
                    displacement.z);

            if (!std::isfinite(distance_meters))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_state,
                    "Simulation focus distance could "
                    "not be represented as a finite "
                    "value.");
            }

            if (
                distance_meters <=
                    active_distance_meters)
            {
                return
                    ActorSimulationFidelity::
                        deep_local;
            }
        }

        return
            ActorSimulationFidelity::
                statistical_distant;
    }

    double
    ActorSimulationFocusPolicy::
        promotion_distance_meters()
        const noexcept
    {
        return promotion_distance_meters_;
    }

    double
    ActorSimulationFocusPolicy::
        retention_distance_meters()
        const noexcept
    {
        return retention_distance_meters_;
    }

    ActorSimulationFocusPolicy::
        ActorSimulationFocusPolicy(
            const double promotion_distance_meters,
            const double retention_distance_meters)
            noexcept
        : promotion_distance_meters_{
              promotion_distance_meters
          },
          retention_distance_meters_{
              retention_distance_meters
          }
    {
    }
}