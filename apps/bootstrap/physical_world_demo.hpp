#pragma once

#include "oros/foundation/result.hpp"
#include "oros/physical_world/world_capsule_traversal_settings.hpp"
#include "oros/physical_world/world_cell_collider_registry.hpp"
#include "oros/physical_world/world_first_person_controller.hpp"
#include "oros/physics/collider_id.hpp"
#include "oros/physics/collision_shape.hpp"
#include "oros/streaming/world_cell_residency.hpp"
#include "oros/world/entity_id.hpp"

#include <cstdint>

namespace oros::bootstrap
{
    struct PhysicalWorldDemo final
    {
        streaming::WorldCellResidency residency;
        physical_world::WorldCellColliderRegistry registry;

        physics::ColliderId player_collider{};
        physics::ColliderId floor_collider{};

        physics::CapsuleShape player_capsule;

        physical_world::WorldCapsuleTraversalSettings
            traversal_settings;

        physical_world::WorldFirstPersonControllerSettings
            controller_settings;
    };

    [[nodiscard]]
    foundation::Result<PhysicalWorldDemo>
    create_physical_world_demo(
        std::uint64_t world_namespace,
        world::EntityId player_entity,
        world::EntityId floor_entity);
}
