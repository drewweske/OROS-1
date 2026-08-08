#include "oros/physical_world/world_capsule_motion.hpp"

#include "oros/physical_world/world_capsule_depenetration.hpp"
#include "oros/physics/physics_vector.hpp"

#include "world_capsule_substep_plan.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace oros::physical_world
{
    foundation::Result<
        world::WorldPosition>
    move_world_capsule(
        const WorldCellColliderRegistry& registry,
        const std::uint64_t world_namespace,
        const physics::ColliderId capsule_collider,
        const physics::CapsuleShape& capsule_shape,
        const world::WorldPosition&
            capsule_center,
        const world::WorldDisplacement
            desired_displacement,
        const physics::PhysicsScalar
            maximum_substep_distance,
        const std::size_t maximum_substeps,
        const std::size_t
            maximum_depenetration_iterations)
    {
        const physics::PhysicsVector3
            requested_displacement{
                desired_displacement.x,
                desired_displacement.y,
                desired_displacement.z
            };

        if (!requested_displacement.is_finite())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Capsule movement requires a "
                "finite desired displacement.");
        }

        if (!std::isfinite(
                maximum_substep_distance) ||
            maximum_substep_distance <= 0.0)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Capsule movement requires a "
                "finite positive maximum substep "
                "distance.");
        }

        if (maximum_substeps == 0U)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Capsule movement requires at "
                "least one motion substep.");
        }

        if (maximum_depenetration_iterations == 0U)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Capsule movement requires at "
                "least one depenetration "
                "iteration.");
        }

        const auto initial_center_result =
            resolve_world_capsule_penetration(
                registry,
                world_namespace,
                capsule_collider,
                capsule_shape,
                capsule_center,
                maximum_depenetration_iterations);

        if (!initial_center_result.has_value())
        {
            return foundation::fail(
                initial_center_result.
                    error().
                    code,
                initial_center_result.
                    error().
                    message);
        }

        world::WorldPosition current_center =
            initial_center_result.value();

        if (requested_displacement ==
            physics::physics_zero_vector)
        {
            return current_center;
        }

        const physics::PhysicsScalar
            displacement_length =
                requested_displacement.length();

        if (!std::isfinite(
                displacement_length))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Capsule movement displacement "
                "length must remain finite.");
        }

        const auto substep_count_result =
            detail::
                try_compute_deterministic_substep_count(
                    displacement_length,
                    maximum_substep_distance,
                    maximum_substeps);

        if (!substep_count_result.has_value())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Capsule movement exceeds the "
                "requested deterministic substep "
                "budget.");
        }

        const std::size_t substep_count =
            substep_count_result.value();

        const physics::PhysicsScalar
            inverse_substep_count =
                1.0 /
                static_cast<
                    physics::PhysicsScalar>(
                        substep_count);

        const physics::PhysicsVector3
            substep_displacement =
                requested_displacement *
                inverse_substep_count;

        for (std::size_t substep = 0U;
             substep < substep_count;
             ++substep)
        {
            const auto candidate_center_result =
                current_center.translated(
                    world::WorldDisplacement{
                        substep_displacement.x,
                        substep_displacement.y,
                        substep_displacement.z
                    });

            if (!candidate_center_result.has_value())
            {
                return foundation::fail(
                    candidate_center_result.
                        error().
                        code,
                    candidate_center_result.
                        error().
                        message);
            }

            const auto resolved_center_result =
                resolve_world_capsule_penetration(
                    registry,
                    world_namespace,
                    capsule_collider,
                    capsule_shape,
                    candidate_center_result.value(),
                    maximum_depenetration_iterations);

            if (!resolved_center_result.has_value())
            {
                return foundation::fail(
                    resolved_center_result.
                        error().
                        code,
                    resolved_center_result.
                        error().
                        message);
            }

            current_center =
                resolved_center_result.value();
        }

        return current_center;
    }
}