#include "oros/physical_world/world_capsule_depenetration.hpp"

#include "oros/physical_world/world_capsule_contact_query.hpp"
#include "oros/physics/collision_contact.hpp"
#include "oros/physics/physics_vector.hpp"

#include "world_capsule_contact_normal.hpp"

#include <cstddef>

namespace oros::physical_world
{
    namespace
    {
        inline constexpr
            physics::PhysicsScalar
            capsule_penetration_resolution_tolerance{
                physics::
                    physics_vector_zero_tolerance
            };
    }

    foundation::Result<
        world::WorldPosition>
    resolve_world_capsule_penetration(
        const WorldCellColliderRegistry& registry,
        const std::uint64_t world_namespace,
        const physics::ColliderId capsule_collider,
        const physics::CapsuleShape& capsule_shape,
        const world::WorldPosition&
            capsule_center,
        const std::size_t maximum_iterations)
    {
        if (maximum_iterations == 0U)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Capsule depenetration requires at "
                "least one resolution iteration.");
        }

        world::WorldPosition
            resolved_center =
                capsule_center;

        for (std::size_t iteration = 0U;
             iteration < maximum_iterations;
             ++iteration)
        {
            const auto contacts_result =
                query_world_capsule_contacts(
                    registry,
                    world_namespace,
                    capsule_collider,
                    capsule_shape,
                    resolved_center);

            if (!contacts_result.has_value())
            {
                return foundation::fail(
                    contacts_result.
                        error().
                        code,
                    contacts_result.
                        error().
                        message);
            }

            const physics::CollisionContact*
                deepest_contact{};

            for (const physics::
                     CollisionContact&
                     contact :
                 contacts_result.value())
            {
                if (contact.
                        penetration_depth() <=
                    capsule_penetration_resolution_tolerance)
                {
                    continue;
                }

                if (deepest_contact == nullptr ||
                    contact.
                        penetration_depth() >
                        deepest_contact->
                            penetration_depth())
                {
                    deepest_contact =
                        &contact;
                }
            }

            if (deepest_contact == nullptr)
            {
                return resolved_center;
            }

            const auto outward_normal_result =
                detail::
                    capsule_outward_contact_normal(
                        *deepest_contact,
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

            const physics::PhysicsVector3
                correction =
                    outward_normal_result.
                        value() *
                    deepest_contact->
                        penetration_depth();

            const auto translated_result =
                resolved_center.translated(
                    world::WorldDisplacement{
                        correction.x,
                        correction.y,
                        correction.z
                    });

            if (!translated_result.has_value())
            {
                return foundation::fail(
                    translated_result.
                        error().
                        code,
                    translated_result.
                        error().
                        message);
            }

            resolved_center =
                translated_result.value();
        }

        const auto final_contacts_result =
            query_world_capsule_contacts(
                registry,
                world_namespace,
                capsule_collider,
                capsule_shape,
                resolved_center);

        if (!final_contacts_result.has_value())
        {
            return foundation::fail(
                final_contacts_result.
                    error().
                    code,
                final_contacts_result.
                    error().
                    message);
        }

        for (const physics::CollisionContact&
                 contact :
             final_contacts_result.value())
        {
            if (contact.penetration_depth() >
                capsule_penetration_resolution_tolerance)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_state,
                    "Capsule penetration could not "
                    "be fully resolved within the "
                    "requested iteration limit.");
            }
        }

        return resolved_center;
    }
}