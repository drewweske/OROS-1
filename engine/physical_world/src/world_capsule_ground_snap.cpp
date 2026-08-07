#include "oros/physical_world/world_capsule_ground_snap.hpp"

#include "oros/physical_world/world_capsule_ground_query.hpp"
#include "oros/physical_world/world_capsule_motion.hpp"

#include <cstddef>
#include <cstdint>

namespace oros::physical_world
{
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

        const physics::PhysicsVector3
            snap_displacement =
                traversal_settings.
                    up_direction().
                    vector() *
                -snap_distance;

        const auto candidate_result =
            move_world_capsule(
                registry,
                world_namespace,
                capsule_collider,
                capsule_shape,
                resolved_start,
                world::WorldDisplacement{
                    snap_displacement.x,
                    snap_displacement.y,
                    snap_displacement.z
                },
                maximum_substep_distance,
                maximum_substeps,
                maximum_depenetration_iterations);

        if (!candidate_result.has_value())
        {
            return foundation::fail(
                candidate_result.error().code,
                candidate_result.error().message);
        }

        const auto candidate_ground_result =
            query_world_capsule_ground_contact(
                registry,
                world_namespace,
                capsule_collider,
                capsule_shape,
                candidate_result.value(),
                traversal_settings);

        if (!candidate_ground_result.has_value())
        {
            return foundation::fail(
                candidate_ground_result.error().code,
                candidate_ground_result.error().message);
        }

        if (!candidate_ground_result.
                value().
                has_value())
        {
            return resolved_start;
        }

        return candidate_result.value();
    }
}