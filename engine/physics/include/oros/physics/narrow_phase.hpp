#pragma once

#include "oros/foundation/result.hpp"
#include "oros/physics/collider_geometry.hpp"
#include "oros/physics/collision_contact.hpp"

#include <optional>

namespace oros::physics
{
    [[nodiscard]]
    foundation::Result<
        std::optional<CollisionContact>>
    generate_sphere_sphere_contact(
        const ColliderGeometry& first_geometry,
        const ColliderGeometry& second_geometry);
}