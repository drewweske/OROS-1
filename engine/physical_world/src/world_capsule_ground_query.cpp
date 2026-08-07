#include "oros/physical_world/world_capsule_ground_query.hpp"

#include "oros/physical_world/world_capsule_contact_query.hpp"
#include "oros/physics/physics_vector.hpp"

#include <cmath>
#include <optional>

namespace oros::physical_world
{
    foundation::Result<
        std::optional<
            physics::CollisionContact>>
    query_world_capsule_ground_contact(
        const WorldCellColliderRegistry& registry,
        const std::uint64_t world_namespace,
        const physics::ColliderId capsule_collider,
        const physics::CapsuleShape& capsule_shape,
        const world::WorldPosition&
            capsule_center,
        const WorldCapsuleTraversalSettings&
            traversal_settings)
    {
        const auto contacts_result =
            query_world_capsule_contacts(
                registry,
                world_namespace,
                capsule_collider,
                capsule_shape,
                capsule_center);

        if (!contacts_result.has_value())
        {
            return foundation::fail(
                contacts_result.error().code,
                contacts_result.error().message);
        }

        const physics::CollisionContact*
            best_contact{};

        physics::PhysicsScalar
            best_up_dot{};

        for (const physics::CollisionContact&
                 contact :
             contacts_result.value())
        {
            if (!contact.pair().contains(
                    capsule_collider))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "Capsule ground query received "
                    "a contact that does not contain "
                    "the capsule collider identity.");
            }

            const physics::PhysicsVector3
                canonical_normal =
                    contact.normal().vector();

            const physics::PhysicsVector3
                capsule_outward_normal =
                    contact.pair().first_collider() ==
                            capsule_collider
                        ? -canonical_normal
                        : canonical_normal;

            const physics::PhysicsScalar
                up_dot =
                    physics::dot(
                        capsule_outward_normal,
                        traversal_settings.
                            up_direction().
                            vector());

            if (!std::isfinite(up_dot))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "Capsule ground query produced "
                    "a non-finite ground normal "
                    "alignment.");
            }

            if (up_dot <
                traversal_settings.
                    minimum_walkable_up_dot())
            {
                continue;
            }

            if (best_contact == nullptr)
            {
                best_contact = &contact;
                best_up_dot = up_dot;
                continue;
            }

            if (up_dot > best_up_dot)
            {
                best_contact = &contact;
                best_up_dot = up_dot;
                continue;
            }

            if (up_dot == best_up_dot &&
                contact.penetration_depth() >
                    best_contact->
                        penetration_depth())
            {
                best_contact = &contact;
                best_up_dot = up_dot;
            }
        }

        if (best_contact == nullptr)
        {
            return std::optional<
                physics::CollisionContact>{};
        }

        return std::optional<
            physics::CollisionContact>{
                *best_contact
            };
    }
}