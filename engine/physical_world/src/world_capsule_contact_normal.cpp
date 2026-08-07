#include "world_capsule_contact_normal.hpp"

namespace oros::physical_world::detail
{
    foundation::Result<
        physics::PhysicsVector3>
    capsule_outward_contact_normal(
        const physics::CollisionContact& contact,
        const physics::ColliderId capsule_collider)
    {
        if (!contact.pair().contains(
                capsule_collider))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "Capsule contact-normal conversion "
                "requires a contact that contains "
                "the capsule collider identity.");
        }

        const physics::PhysicsVector3
            canonical_normal =
                contact.normal().vector();

        if (contact.pair().first_collider() ==
            capsule_collider)
        {
            return -canonical_normal;
        }

        return canonical_normal;
    }
}