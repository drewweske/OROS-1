#include "oros/physical_world/world_capsule_ground_motion.hpp"

#include "oros/foundation/error.hpp"
#include "oros/physical_world/world_capsule_ground_query.hpp"
#include "oros/physical_world/world_capsule_ground_snap.hpp"
#include "oros/physical_world/world_capsule_motion.hpp"
#include "oros/physics/physics_vector.hpp"

#include "world_capsule_contact_normal.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace oros::physical_world
{
    foundation::Result<
        world::WorldPosition>
    move_world_capsule_along_ground(
        const WorldCellColliderRegistry& registry,
        const std::uint64_t world_namespace,
        const physics::ColliderId capsule_collider,
        const physics::CapsuleShape& capsule_shape,
        const world::WorldPosition&
            capsule_center,
        const world::WorldDisplacement
            desired_planar_displacement,
        const WorldCapsuleTraversalSettings&
            traversal_settings,
        const physics::PhysicsScalar
            maximum_substep_distance,
        const std::size_t maximum_substeps,
        const std::size_t
            maximum_depenetration_iterations)
    {
        const physics::PhysicsVector3
            requested_displacement{
                desired_planar_displacement.x,
                desired_planar_displacement.y,
                desired_planar_displacement.z
            };

        if (!requested_displacement.is_finite())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Grounded capsule movement requires "
                "a finite desired displacement.");
        }

        const auto grounded_start_result =
            snap_world_capsule_to_ground(
                registry,
                world_namespace,
                capsule_collider,
                capsule_shape,
                capsule_center,
                traversal_settings,
                maximum_substep_distance,
                maximum_substeps,
                maximum_depenetration_iterations);

        if (!grounded_start_result.has_value())
        {
            return foundation::fail(
                grounded_start_result.error().code,
                grounded_start_result.error().message);
        }

        const world::WorldPosition grounded_start =
            grounded_start_result.value();

        const auto ground_contact_result =
            query_world_capsule_ground_contact(
                registry,
                world_namespace,
                capsule_collider,
                capsule_shape,
                grounded_start,
                traversal_settings);

        if (!ground_contact_result.has_value())
        {
            return foundation::fail(
                ground_contact_result.error().code,
                ground_contact_result.error().message);
        }

        if (!ground_contact_result.
                value().
                has_value())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Grounded capsule movement requires "
                "an initial walkable ground contact.");
        }

        const auto ground_normal_result =
            detail::
                capsule_outward_contact_normal(
                    ground_contact_result.
                        value().
                        value(),
                    capsule_collider);

        if (!ground_normal_result.has_value())
        {
            return foundation::fail(
                ground_normal_result.error().code,
                ground_normal_result.error().message);
        }

        const physics::PhysicsVector3&
            up_direction =
                traversal_settings.
                    up_direction().
                    vector();

        const physics::PhysicsVector3
            ground_normal =
                ground_normal_result.value();

        const physics::PhysicsScalar
            requested_up_component =
                physics::dot(
                    requested_displacement,
                    up_direction);

        if (!std::isfinite(
                requested_up_component))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Grounded capsule movement produced "
                "a non-finite up-axis projection.");
        }

        const physics::PhysicsVector3
            planar_displacement =
                requested_displacement -
                up_direction *
                    requested_up_component;

        if (!planar_displacement.is_finite())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Grounded capsule planar "
                "displacement exceeds the finite "
                "physics range.");
        }

        const physics::PhysicsScalar
            ground_normal_component =
                physics::dot(
                    planar_displacement,
                    ground_normal);

        if (!std::isfinite(
                ground_normal_component))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Grounded capsule movement produced "
                "a non-finite support-plane "
                "projection.");
        }

        const physics::PhysicsVector3
            ground_displacement =
                planar_displacement -
                ground_normal *
                    ground_normal_component;

        if (!ground_displacement.is_finite())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Grounded capsule support-plane "
                "displacement exceeds the finite "
                "physics range.");
        }

        const auto moved_result =
            move_world_capsule(
                registry,
                world_namespace,
                capsule_collider,
                capsule_shape,
                grounded_start,
                world::WorldDisplacement{
                    ground_displacement.x,
                    ground_displacement.y,
                    ground_displacement.z
                },
                maximum_substep_distance,
                maximum_substeps,
                maximum_depenetration_iterations);

        if (!moved_result.has_value())
        {
            return foundation::fail(
                moved_result.error().code,
                moved_result.error().message);
        }

        const auto snapped_result =
            snap_world_capsule_to_ground(
                registry,
                world_namespace,
                capsule_collider,
                capsule_shape,
                moved_result.value(),
                traversal_settings,
                maximum_substep_distance,
                maximum_substeps,
                maximum_depenetration_iterations);

        if (!snapped_result.has_value())
        {
            return foundation::fail(
                snapped_result.error().code,
                snapped_result.error().message);
        }

        return snapped_result.value();
    }
}