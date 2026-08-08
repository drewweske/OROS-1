#include "oros/physical_world/world_first_person_controller.hpp"

#include "oros/physical_world/world_capsule_ground_motion.hpp"

#include <chrono>
#include <cmath>

namespace oros::physical_world
{
    foundation::Result<
        WorldFirstPersonControllerSettings>
    WorldFirstPersonControllerSettings::create(
        const physics::PhysicsScalar
            maximum_ground_speed,
        const physics::PhysicsScalar
            maximum_substep_distance,
        const std::size_t maximum_substeps,
        const std::size_t
            maximum_depenetration_iterations)
    {
        if (!std::isfinite(maximum_ground_speed) ||
            maximum_ground_speed < 0.0)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "First-person controller maximum "
                "ground speed must be finite and "
                "non-negative.");
        }

        if (!std::isfinite(
                maximum_substep_distance) ||
            maximum_substep_distance <= 0.0)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "First-person controller requires "
                "a finite positive maximum "
                "substep distance.");
        }

        if (maximum_substeps == 0U)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "First-person controller requires "
                "at least one motion substep.");
        }

        if (maximum_depenetration_iterations == 0U)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "First-person controller requires "
                "at least one depenetration "
                "iteration.");
        }

        return WorldFirstPersonControllerSettings{
            maximum_ground_speed,
            maximum_substep_distance,
            maximum_substeps,
            maximum_depenetration_iterations
        };
    }

    physics::PhysicsScalar
    WorldFirstPersonControllerSettings::
        maximum_ground_speed() const noexcept
    {
        return maximum_ground_speed_;
    }

    physics::PhysicsScalar
    WorldFirstPersonControllerSettings::
        maximum_substep_distance() const noexcept
    {
        return maximum_substep_distance_;
    }

    std::size_t
    WorldFirstPersonControllerSettings::
        maximum_substeps() const noexcept
    {
        return maximum_substeps_;
    }

    std::size_t
    WorldFirstPersonControllerSettings::
        maximum_depenetration_iterations()
        const noexcept
    {
        return maximum_depenetration_iterations_;
    }

    WorldFirstPersonControllerSettings::
        WorldFirstPersonControllerSettings(
            const physics::PhysicsScalar
                maximum_ground_speed,
            const physics::PhysicsScalar
                maximum_substep_distance,
            const std::size_t maximum_substeps,
            const std::size_t
                maximum_depenetration_iterations)
            noexcept
        : maximum_ground_speed_{
              maximum_ground_speed
          },
          maximum_substep_distance_{
              maximum_substep_distance
          },
          maximum_substeps_{
              maximum_substeps
          },
          maximum_depenetration_iterations_{
              maximum_depenetration_iterations
          }
    {
    }

    foundation::Result<world::WorldPosition>
    step_world_first_person_controller(
        const WorldCellColliderRegistry& registry,
        const std::uint64_t world_namespace,
        const physics::ColliderId capsule_collider,
        const physics::CapsuleShape& capsule_shape,
        const world::WorldPosition& capsule_center,
        const WorldFirstPersonControllerCommand&
            command,
        const WorldCapsuleTraversalSettings&
            traversal_settings,
        const WorldFirstPersonControllerSettings&
            controller_settings,
        const foundation::Nanoseconds
            simulation_step)
    {
        const physics::PhysicsVector3&
            requested_direction =
                command.desired_planar_direction;

        if (!requested_direction.is_finite())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "First-person controller movement "
                "direction must be finite.");
        }

        if (simulation_step <=
            foundation::Nanoseconds::zero())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "First-person controller simulation "
                "step must be positive.");
        }

        const physics::PhysicsVector3&
            up_direction =
                traversal_settings.
                    up_direction().
                    vector();

        const physics::PhysicsScalar
            requested_up_component =
                physics::dot(
                    requested_direction,
                    up_direction);

        if (!std::isfinite(
                requested_up_component))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "First-person controller movement "
                "projection must remain finite.");
        }

        const physics::PhysicsVector3
            planar_direction =
                requested_direction -
                up_direction *
                    requested_up_component;

        if (!planar_direction.is_finite())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "First-person controller planar "
                "direction exceeds the finite "
                "physics range.");
        }

        const physics::PhysicsScalar
            planar_length =
                planar_direction.length();

        if (!std::isfinite(planar_length))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "First-person controller planar "
                "direction magnitude must remain "
                "finite.");
        }

        physics::PhysicsVector3
            bounded_direction =
                planar_direction;

        if (planar_length > 1.0)
        {
            bounded_direction =
                planar_direction *
                (1.0 / planar_length);
        }

        const physics::PhysicsScalar
            simulation_seconds =
                std::chrono::duration<
                    physics::PhysicsScalar>{
                        simulation_step
                    }.
                    count();

        const physics::PhysicsScalar
            movement_scale =
                controller_settings.
                    maximum_ground_speed() *
                simulation_seconds;

        if (!std::isfinite(movement_scale))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "First-person controller movement "
                "distance exceeds the finite "
                "physics range.");
        }

        const physics::PhysicsVector3
            desired_displacement =
                bounded_direction *
                movement_scale;

        if (!desired_displacement.is_finite())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "First-person controller desired "
                "displacement exceeds the finite "
                "physics range.");
        }

        return move_world_capsule_along_ground(
            registry,
            world_namespace,
            capsule_collider,
            capsule_shape,
            capsule_center,
            world::WorldDisplacement{
                desired_displacement.x,
                desired_displacement.y,
                desired_displacement.z
            },
            traversal_settings,
            controller_settings.
                maximum_substep_distance(),
            controller_settings.
                maximum_substeps(),
            controller_settings.
                maximum_depenetration_iterations());
    }
}
