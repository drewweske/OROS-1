#include "oros/physical_world/world_capsule_ground_motion.hpp"

#include "oros/foundation/error.hpp"
#include "oros/physical_world/world_capsule_contact_query.hpp"
#include "oros/physical_world/world_capsule_ground_query.hpp"
#include "oros/physical_world/world_capsule_ground_snap.hpp"
#include "oros/physical_world/world_capsule_motion.hpp"
#include "oros/physics/physics_vector.hpp"

#include "world_capsule_contact_normal.hpp"
#include "world_capsule_substep_plan.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace oros::physical_world
{
    namespace
    {
        [[nodiscard]]
        foundation::Result<bool>
        candidate_has_blocking_steep_contact(
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
                        "Grounded capsule steep-contact "
                        "classification produced a "
                        "non-finite up alignment.");
                }

                if (up_dot >
                        physics::
                            physics_vector_zero_tolerance &&
                    up_dot <
                        traversal_settings.
                            minimum_walkable_up_dot())
                {
                    return true;
                }
            }

            return false;
        }

        [[nodiscard]]
        foundation::Result<bool>
        motion_matches_displacement(
            const world::WorldPosition& start,
            const world::WorldPosition& end,
            const physics::PhysicsVector3
                expected_displacement)
        {
            const auto actual_result =
                start.displacement_to(
                    end);

            if (!actual_result.has_value())
            {
                return foundation::fail(
                    actual_result.error().code,
                    actual_result.error().message);
            }

            const physics::PhysicsVector3
                actual_displacement{
                    actual_result.value().x,
                    actual_result.value().y,
                    actual_result.value().z
                };

            if (!actual_displacement.is_finite())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "Grounded capsule movement "
                    "produced a non-finite resolved "
                    "displacement.");
            }

            return physics::nearly_equal(
                actual_displacement,
                expected_displacement);
        }

        [[nodiscard]]
        foundation::Result<bool>
        candidate_has_step_obstruction(
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
                        "Grounded capsule step-contact "
                        "classification produced a "
                        "non-finite up alignment.");
                }

                if (std::abs(up_dot) <=
                    physics::
                        physics_unit_length_tolerance)
                {
                    return true;
                }
            }

            return false;
        }

        [[nodiscard]]
        foundation::Result<
            std::optional<
                world::WorldPosition>>
        try_step_up_over_obstruction(
            const WorldCellColliderRegistry& registry,
            const std::uint64_t world_namespace,
            const physics::ColliderId capsule_collider,
            const physics::CapsuleShape& capsule_shape,
            const world::WorldPosition&
                grounded_start,
            const physics::PhysicsVector3
                remaining_ground_displacement,
            const WorldCapsuleTraversalSettings&
                traversal_settings,
            const physics::PhysicsScalar
                maximum_substep_distance,
            const std::size_t maximum_substeps,
            const std::size_t
                maximum_depenetration_iterations)
        {
            const physics::PhysicsScalar
                maximum_step_height =
                    traversal_settings.
                        maximum_step_height();

            if (maximum_step_height <=
                physics::
                    physics_vector_zero_tolerance)
            {
                return std::optional<
                    world::WorldPosition>{};
            }

            if (!remaining_ground_displacement.
                    is_finite())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "Grounded capsule step-up "
                    "received a non-finite remaining "
                    "displacement.");
            }

            const physics::PhysicsVector3&
                up_direction =
                    traversal_settings.
                        up_direction().
                        vector();

            const auto initial_ground_result =
                query_world_capsule_ground_contact(
                    registry,
                    world_namespace,
                    capsule_collider,
                    capsule_shape,
                    grounded_start,
                    traversal_settings);

            if (!initial_ground_result.has_value())
            {
                return foundation::fail(
                    initial_ground_result.error().code,
                    initial_ground_result.error().message);
            }

            if (!initial_ground_result.
                    value().
                    has_value())
            {
                return std::optional<
                    world::WorldPosition>{};
            }

            const physics::PhysicsVector3
                lift_displacement =
                    up_direction *
                    maximum_step_height;

            const auto lifted_result =
                move_world_capsule(
                    registry,
                    world_namespace,
                    capsule_collider,
                    capsule_shape,
                    grounded_start,
                    world::WorldDisplacement{
                        lift_displacement.x,
                        lift_displacement.y,
                        lift_displacement.z
                    },
                    maximum_substep_distance,
                    maximum_substeps,
                    maximum_depenetration_iterations);

            if (!lifted_result.has_value())
            {
                return foundation::fail(
                    lifted_result.error().code,
                    lifted_result.error().message);
            }

            const auto lift_matches_result =
                motion_matches_displacement(
                    grounded_start,
                    lifted_result.value(),
                    lift_displacement);

            if (!lift_matches_result.has_value())
            {
                return foundation::fail(
                    lift_matches_result.error().code,
                    lift_matches_result.error().message);
            }

            if (!lift_matches_result.value())
            {
                return std::optional<
                    world::WorldPosition>{};
            }

            const auto forward_result =
                move_world_capsule(
                    registry,
                    world_namespace,
                    capsule_collider,
                    capsule_shape,
                    lifted_result.value(),
                    world::WorldDisplacement{
                        remaining_ground_displacement.x,
                        remaining_ground_displacement.y,
                        remaining_ground_displacement.z
                    },
                    maximum_substep_distance,
                    maximum_substeps,
                    maximum_depenetration_iterations);

            if (!forward_result.has_value())
            {
                return foundation::fail(
                    forward_result.error().code,
                    forward_result.error().message);
            }

            const auto forward_matches_result =
                motion_matches_displacement(
                    lifted_result.value(),
                    forward_result.value(),
                    remaining_ground_displacement);

            if (!forward_matches_result.has_value())
            {
                return foundation::fail(
                    forward_matches_result.error().code,
                    forward_matches_result.error().message);
            }

            if (!forward_matches_result.value())
            {
                return std::optional<
                    world::WorldPosition>{};
            }

            const physics::PhysicsScalar
                ordinary_snap_distance =
                    traversal_settings.
                        maximum_ground_snap_distance();

            const physics::PhysicsScalar
                step_snap_distance =
                    ordinary_snap_distance >
                            maximum_step_height
                        ? ordinary_snap_distance
                        : maximum_step_height;

            const auto step_snap_settings_result =
                WorldCapsuleTraversalSettings::create(
                    traversal_settings.
                        up_direction(),
                    traversal_settings.
                        minimum_walkable_up_dot(),
                    maximum_step_height,
                    step_snap_distance);

            if (!step_snap_settings_result.has_value())
            {
                return foundation::fail(
                    step_snap_settings_result.
                        error().
                        code,
                    step_snap_settings_result.
                        error().
                        message);
            }

            const auto snapped_result =
                snap_world_capsule_to_ground(
                    registry,
                    world_namespace,
                    capsule_collider,
                    capsule_shape,
                    forward_result.value(),
                    step_snap_settings_result.value(),
                    maximum_substep_distance,
                    maximum_substeps,
                    maximum_depenetration_iterations);

            if (!snapped_result.has_value())
            {
                return foundation::fail(
                    snapped_result.error().code,
                    snapped_result.error().message);
            }

            const auto ground_result =
                query_world_capsule_ground_contact(
                    registry,
                    world_namespace,
                    capsule_collider,
                    capsule_shape,
                    snapped_result.value(),
                    traversal_settings);

            if (!ground_result.has_value())
            {
                return foundation::fail(
                    ground_result.error().code,
                    ground_result.error().message);
            }

            if (!ground_result.value().has_value())
            {
                return std::optional<
                    world::WorldPosition>{};
            }

            const auto step_displacement_result =
                grounded_start.displacement_to(
                    snapped_result.value());

            if (!step_displacement_result.has_value())
            {
                return foundation::fail(
                    step_displacement_result.
                        error().
                        code,
                    step_displacement_result.
                        error().
                        message);
            }

            const physics::PhysicsVector3
                step_displacement{
                    step_displacement_result.value().x,
                    step_displacement_result.value().y,
                    step_displacement_result.value().z
                };

            if (!step_displacement.is_finite())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "Grounded capsule step-up "
                    "produced a non-finite resolved "
                    "displacement.");
            }

            const physics::PhysicsScalar
                resolved_step_height =
                    physics::dot(
                        step_displacement,
                        up_direction);

            if (!std::isfinite(
                    resolved_step_height))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "Grounded capsule step-up "
                    "produced a non-finite resolved "
                    "step height.");
            }

            if (resolved_step_height <=
                    physics::
                        physics_vector_zero_tolerance ||
                resolved_step_height >
                    maximum_step_height +
                        physics::
                            physics_unit_length_tolerance)
            {
                return std::optional<
                    world::WorldPosition>{};
            }

            return std::optional<
                world::WorldPosition>{
                    snapped_result.value()
                };
        }

        [[nodiscard]]
        foundation::Result<
            world::WorldPosition>
        move_ground_displacement_with_steep_guard(
            const WorldCellColliderRegistry& registry,
            const std::uint64_t world_namespace,
            const physics::ColliderId capsule_collider,
            const physics::CapsuleShape& capsule_shape,
            const world::WorldPosition&
                grounded_start,
            const physics::PhysicsVector3
                ground_displacement,
            const WorldCapsuleTraversalSettings&
                traversal_settings,
            const physics::PhysicsScalar
                maximum_substep_distance,
            const std::size_t maximum_substeps,
            const std::size_t
                maximum_depenetration_iterations)
        {
            if (!std::isfinite(
                    maximum_substep_distance) ||
                maximum_substep_distance <= 0.0)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Grounded capsule movement requires "
                    "a finite positive maximum substep "
                    "distance.");
            }

            if (maximum_substeps == 0U)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Grounded capsule movement requires "
                    "at least one motion substep.");
            }

            if (maximum_depenetration_iterations == 0U)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Grounded capsule movement requires "
                    "at least one depenetration "
                    "iteration.");
            }

            if (ground_displacement ==
                physics::physics_zero_vector)
            {
                return move_world_capsule(
                    registry,
                    world_namespace,
                    capsule_collider,
                    capsule_shape,
                    grounded_start,
                    world::WorldDisplacement{},
                    maximum_substep_distance,
                    maximum_substeps,
                    maximum_depenetration_iterations);
            }

            const physics::PhysicsScalar
                displacement_length =
                    ground_displacement.length();

            if (!std::isfinite(
                    displacement_length))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Grounded capsule displacement "
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
                    "Grounded capsule movement exceeds "
                    "the requested deterministic "
                    "substep budget.");
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
                    ground_displacement *
                    inverse_substep_count;

            const physics::PhysicsScalar
                substep_length =
                    substep_displacement.length();

            if (!std::isfinite(substep_length) ||
                substep_length <= 0.0)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "Grounded capsule movement produced "
                    "an invalid non-zero substep "
                    "length.");
            }

            world::WorldPosition current_center =
                grounded_start;

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

                const auto blocking_result =
                    candidate_has_blocking_steep_contact(
                        registry,
                        world_namespace,
                        capsule_collider,
                        capsule_shape,
                        candidate_center_result.value(),
                        traversal_settings);

                if (!blocking_result.has_value())
                {
                    return foundation::fail(
                        blocking_result.error().code,
                        blocking_result.error().message);
                }

                if (blocking_result.value())
                {
                    const std::size_t
                        remaining_substep_count =
                            substep_count -
                            substep;

                    const physics::PhysicsVector3
                        remaining_ground_displacement =
                            substep_displacement *
                            static_cast<
                                physics::PhysicsScalar>(
                                    remaining_substep_count);

                    const auto step_result =
                        try_step_up_over_obstruction(
                            registry,
                            world_namespace,
                            capsule_collider,
                            capsule_shape,
                            current_center,
                            remaining_ground_displacement,
                            traversal_settings,
                            maximum_substep_distance,
                            maximum_substeps,
                            maximum_depenetration_iterations);

                    if (!step_result.has_value())
                    {
                        return foundation::fail(
                            step_result.error().code,
                            step_result.error().message);
                    }

                    if (step_result.
                            value().
                            has_value())
                    {
                        return step_result.
                            value().
                            value();
                    }

                    return current_center;
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
                        substep_length,
                        1U,
                        maximum_depenetration_iterations);

                if (!moved_result.has_value())
                {
                    return foundation::fail(
                        moved_result.error().code,
                        moved_result.error().message);
                }

                const auto complete_substep_result =
                    motion_matches_displacement(
                        current_center,
                        moved_result.value(),
                        substep_displacement);

                if (!complete_substep_result.has_value())
                {
                    return foundation::fail(
                        complete_substep_result.
                            error().
                            code,
                        complete_substep_result.
                            error().
                            message);
                }

                if (!complete_substep_result.value())
                {
                    const auto step_obstruction_result =
                        candidate_has_step_obstruction(
                            registry,
                            world_namespace,
                            capsule_collider,
                            capsule_shape,
                            candidate_center_result.value(),
                            traversal_settings);

                    if (!step_obstruction_result.
                            has_value())
                    {
                        return foundation::fail(
                            step_obstruction_result.
                                error().
                                code,
                            step_obstruction_result.
                                error().
                                message);
                    }

                    if (step_obstruction_result.value())
                    {
                        const std::size_t
                            remaining_substep_count =
                                substep_count -
                                substep;

                        const physics::PhysicsVector3
                            remaining_ground_displacement =
                                substep_displacement *
                                static_cast<
                                    physics::PhysicsScalar>(
                                        remaining_substep_count);

                        const auto step_result =
                            try_step_up_over_obstruction(
                                registry,
                                world_namespace,
                                capsule_collider,
                                capsule_shape,
                                current_center,
                                remaining_ground_displacement,
                                traversal_settings,
                                maximum_substep_distance,
                                maximum_substeps,
                                maximum_depenetration_iterations);

                        if (!step_result.has_value())
                        {
                            return foundation::fail(
                                step_result.error().code,
                                step_result.error().message);
                        }

                        if (step_result.
                                value().
                                has_value())
                        {
                            return step_result.
                                value().
                                value();
                        }
                    }
                }

                current_center =
                    moved_result.value();
            }

            return current_center;
        }
    }

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
            move_ground_displacement_with_steep_guard(
                registry,
                world_namespace,
                capsule_collider,
                capsule_shape,
                grounded_start,
                ground_displacement,
                traversal_settings,
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