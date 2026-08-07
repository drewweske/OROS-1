#pragma once

#include "oros/foundation/result.hpp"
#include "oros/physics/collider_id.hpp"
#include "oros/physics/collision_contact.hpp"
#include "oros/physics/physics_vector.hpp"

namespace oros::physical_world::detail
{
    [[nodiscard]]
    foundation::Result<
        physics::PhysicsVector3>
    capsule_outward_contact_normal(
        const physics::CollisionContact& contact,
        physics::ColliderId capsule_collider);
}