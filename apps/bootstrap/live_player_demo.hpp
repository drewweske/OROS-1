#pragma once

#include "physical_world_demo.hpp"

#include "oros/foundation/clock.hpp"
#include "oros/foundation/result.hpp"
#include "oros/physical_world/world_first_person_controller.hpp"
#include "oros/world/entity_id.hpp"
#include "oros/world/world.hpp"

namespace oros::bootstrap
{
    struct LivePlayerDemo final
    {
        world::EntityId player_entity{};
        world::EntityId floor_entity{};

        PhysicalWorldDemo physical_world;
    };

    [[nodiscard]]
    foundation::Result<LivePlayerDemo>
    create_live_player_demo(
        world::World& world);

    [[nodiscard]]
    foundation::Status
    step_live_player_demo_fixed_tick(
        world::World& world,
        LivePlayerDemo& demo,
        const physical_world::
            WorldFirstPersonControllerCommand& command,
        foundation::Nanoseconds simulation_step);
}
