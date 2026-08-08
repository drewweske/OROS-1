#include "oros/physical_world/world_capsule_ground_snap.hpp"

#include "oros/foundation/error.hpp"
#include "oros/physical_world/world_capsule_contact_query.hpp"
#include "oros/physical_world/world_capsule_ground_query.hpp"
#include "oros/physical_world/world_capsule_motion.hpp"
#include "oros/physics/physics_vector.hpp"

#include "world_capsule_contact_normal.hpp"
#include "world_capsule_substep_plan.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace oros::physical_world
{
    namespace
    {
        [[nodiscard]]
        foundation::Result<bool>
        candidate_has_non_walkable_penetration(
            const WorldCellColliderRegistry& registry,
            const std::uint64_t world_namespace,
            const physics::ColliderId capsule_collider,
            const physics::CapsuleShape& capsule_shape,
            const world::WorldPosition&
                candidate_center,
            const WorldCapsuleTraversalSettings&
                traversal_settings)
        {
            const auto contacts_result =
                query_world_capsule_contacts(
                    registry,
                    world_namespace,
                    capsule_collider,
                    capsule_shape,
                    candidate_center);

            if (!contacts_result.has_value())
            {
                return foundation::fail(
                    contacts_result.error().code,
                    contacts_result.error().message);
            }

            for (const physics::CollisionContact&
                     contact :
                 contacts_result.value())
            {
                if (contact.penetration_depth() <=
                    physics::
                        physics_vector_zero_tolerance)
                {
                    continue;
                }

                const auto outward_normal_result =
                    detail::
                        capsule_outward_contact_normal(
                            contact,
                            capsule_collider);

                if (!outward_normal_result.has_value())
                {
                    return foundation::fail(
                        outward_normal_result.
                            error().
                            code,
                        outward_normal_result.
                            error().
                            message);
                }

                const physics::PhysicsScalar
                    up_dot =
                        physics::dot(
                            outward_normal_result.value(),
                            traversal_settings.
                                up_direction().
                                vector());

                if (!std::isfinite(up_dot))
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            internal_failure,
                        "Ground snap contact "
                        "classification produced a "
                        "non-finite up alignment.");
                }

                if (up_dot <
                    traversal_settings.
                        minimum_walkable_up_dot())
                {
                    return true;
                }
            }

            return false;
        }

        [[nodiscard]]
        foundation::Result<
            world::WorldPosition>
        snap_resolved_capsule_to_ground(
            const WorldCellColliderRegistry& registry,
            const std::uint64_t world_namespace,
            const physics::ColliderId capsule_collider,
            const physics::CapsuleShape& capsule_shape,
            const world::WorldPosition&
                resolved_start,
            const WorldCapsuleTraversalSettings&
                traversal_settings,
            const physics::PhysicsScalar
                snap_distance,
            const physics::PhysicsScalar
                maximum_substep_distance,
            const std::size_t maximum_substeps,
            const std::size_t
                maximum_depenetration_iterations)
        {
            const auto substep_count_result =
                detail::
                    try_compute_deterministic_substep_count(
                        snap_distance,
                        maximum_substep_distance,
                        maximum_substeps);

            if (!substep_count_result.has_value())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_state,
                    "Ground snap exceeds the "
                    "requested deterministic "
                    "substep budget.");
            }

            const std::size_t substep_count =
                substep_count_result.value();

            const physics::PhysicsScalar
                substep_distance =
                    snap_distance /
                    static_cast<
                        physics::PhysicsScalar>(
                            substep_count);

            if (!std::isfinite(substep_distance) ||
                substep_distance <= 0.0)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "Ground snap produced an invalid "
                    "non-zero substep distance.");
            }

            const physics::PhysicsVector3
                substep_displacement =
                    traversal_settings.
                        up_direction().
                        vector() *
                    -substep_distance;

            world::WorldPosition current_center =
                resolved_start;

            for (std::size_t substep = 0U;
                 substep < substep_count;
                 ++substep)
            {
                const auto candidate_result =
                    current_center.translated(
                        world::WorldDisplacement{
                            substep_displacement.x,
                            substep_displacement.y,
                            substep_displacement.z
                        });

                if (!candidate_result.has_value())
                {
                    return foundation::fail(
                        candidate_result.
                            error().
                            code,
                        candidate_result.
                            error().
                            message);
                }

                const auto blocking_result =
                    candidate_has_non_walkable_penetration(
                        registry,
                        world_namespace,
                        capsule_collider,
                        capsule_shape,
                        candidate_result.value(),
                        traversal_settings);

                if (!blocking_result.has_value())
                {
                    return foundation::fail(
                        blocking_result.error().code,
                        blocking_result.error().message);
                }

                if (blocking_result.value())
                {
                    return resolved_start;
                }

                const auto moved_result =
                    move_world_capsule(
                        registry,
                        world_namespace,
                        capsule_collider,
                        capsule_shape,
                        current_center,
                        world::WorldDisplacement{
                            substep_displacement.x,
                            substep_displacement.y,
                            substep_displacement.z
                        },
                        maximum_substep_distance,
                        1U,
                        maximum_depenetration_iterations);

                if (!moved_result.has_value())
                {
                    return foundation::fail(
                        moved_result.error().code,
                        moved_result.error().message);
                }

                current_center =
                    moved_result.value();

                const auto ground_result =
                    query_world_capsule_ground_contact(
                        registry,
                        world_namespace,
                        capsule_collider,
                        capsule_shape,
                        current_center,
                        traversal_settings);

                if (!ground_result.has_value())
                {
                    return foundation::fail(
                        ground_result.error().code,
                        ground_result.error().message);
                }

                if (ground_result.value().has_value())
                {
                    return current_center;
                }
            }

            return resolved_start;
        }
    }

    foundation::Result<
        world::WorldPosition>
    snap_world_capsule_to_ground(
        const WorldCellColliderRegistry& registry,
        const std::uint64_t world_namespace,
        const physics::ColliderId capsule_collider,
        const physics::CapsuleShape& capsule_shape,
        const world::WorldPosition&
            capsule_center,
        const WorldCapsuleTraversalSettings&
            traversal_settings,
        const physics::PhysicsScalar
            maximum_substep_distance,
        const std::size_t maximum_substeps,
        const std::size_t
            maximum_depenetration_iterations)
    {
        const auto resolved_start_result =
            move_world_capsule(
                registry,
                world_namespace,
                capsule_collider,
                capsule_shape,
                capsule_center,
                world::WorldDisplacement{
                    0.0,
                    0.0,
                    0.0
                },
                maximum_substep_distance,
                maximum_substeps,
                maximum_depenetration_iterations);

        if (!resolved_start_result.has_value())
        {
            return foundation::fail(
                resolved_start_result.error().code,
                resolved_start_result.error().message);
        }

        const world::WorldPosition
            resolved_start =
                resolved_start_result.value();

        const auto initial_ground_result =
            query_world_capsule_ground_contact(
                registry,
                world_namespace,
                capsule_collider,
                capsule_shape,
                resolved_start,
                traversal_settings);

        if (!initial_ground_result.has_value())
        {
            return foundation::fail(
                initial_ground_result.error().code,
                initial_ground_result.error().message);
        }

        if (initial_ground_result.value().has_value())
        {
            return resolved_start;
        }

        const physics::PhysicsScalar
            snap_distance =
                traversal_settings.
                    maximum_ground_snap_distance();

        if (snap_distance == 0.0)
        {
            return resolved_start;
        }

        return snap_resolved_capsule_to_ground(
            registry,
            world_namespace,
            capsule_collider,
            capsule_shape,
            resolved_start,
            traversal_settings,
            snap_distance,
            maximum_substep_distance,
            maximum_substeps,
            maximum_depenetration_iterations);
    }
}
