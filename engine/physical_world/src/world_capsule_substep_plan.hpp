#pragma once

#include "oros/physics/physics_vector.hpp"

#include <cmath>
#include <cstddef>
#include <optional>

namespace oros::physical_world::detail
{
    [[nodiscard]]
    inline std::optional<std::size_t>
    try_compute_deterministic_substep_count(
        const physics::PhysicsScalar distance,
        const physics::PhysicsScalar
            maximum_substep_distance,
        const std::size_t maximum_substeps)
        noexcept
    {
        if (!std::isfinite(distance) ||
            distance < 0.0 ||
            !std::isfinite(
                maximum_substep_distance) ||
            maximum_substep_distance <= 0.0 ||
            maximum_substeps == 0U)
        {
            return std::nullopt;
        }

        const physics::PhysicsScalar
            required_substeps =
                std::ceil(
                    distance /
                    maximum_substep_distance);

        if (!std::isfinite(required_substeps) ||
            required_substeps >
                static_cast<
                    physics::PhysicsScalar>(
                        maximum_substeps))
        {
            return std::nullopt;
        }

        std::size_t substep_count =
            static_cast<std::size_t>(
                required_substeps);

        if (substep_count == 0U)
        {
            substep_count = 1U;
        }

        return substep_count;
    }
}
