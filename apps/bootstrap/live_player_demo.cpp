#include "live_player_demo.hpp"

#include "oros/world/world_position.hpp"

#include <string>
#include <utility>

namespace oros::bootstrap
{
    namespace
    {
        foundation::Status
        rollback_live_player_entity(
            world::World& world,
            const world::EntityId entity)
        {
            if (!entity.is_valid() ||
                !world.contains(entity))
            {
                return {};
            }

            return world.destroy_entity(
                entity);
        }

        foundation::Status
        rollback_live_player_entities(
            world::World& world,
            const world::EntityId player_entity,
            const world::EntityId floor_entity)
        {
            foundation::Status floor_status =
                rollback_live_player_entity(
                    world,
                    floor_entity);

            foundation::Status player_status =
                rollback_live_player_entity(
                    world,
                    player_entity);

            if (!floor_status.has_value())
            {
                return foundation::fail(
                    floor_status.error().code,
                    floor_status.error().message);
            }

            if (!player_status.has_value())
            {
                return foundation::fail(
                    player_status.error().code,
                    player_status.error().message);
            }

            return {};
        }

        foundation::Result<LivePlayerDemo>
        fail_live_player_creation(
            world::World& world,
            const world::EntityId player_entity,
            const world::EntityId floor_entity,
            const foundation::ErrorCode failure_code,
            std::string failure_message)
        {
            foundation::Status rollback_status =
                rollback_live_player_entities(
                    world,
                    player_entity,
                    floor_entity);

            if (!rollback_status.has_value())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "Live-player composition rollback "
                    "failed while handling an earlier "
                    "creation failure: " +
                        rollback_status.
                            error().
                            message);
            }

            return foundation::fail(
                failure_code,
                std::move(failure_message));
        }
    }

    foundation::Result<LivePlayerDemo>
    create_live_player_demo(
        world::World& world)
    {
        foundation::Result<world::EntityId>
            player_entity_result =
                world.create_entity();

        if (!player_entity_result.has_value())
        {
            return foundation::fail(
                player_entity_result.error().code,
                player_entity_result.error().message);
        }

        const world::EntityId player_entity =
            player_entity_result.value();

        foundation::Result<world::EntityId>
            floor_entity_result =
                world.create_entity();

        if (!floor_entity_result.has_value())
        {
            return fail_live_player_creation(
                world,
                player_entity,
                world::EntityId{},
                floor_entity_result.error().code,
                floor_entity_result.error().message);
        }

        const world::EntityId floor_entity =
            floor_entity_result.value();

        foundation::Result<world::WorldPosition>
            player_position_result =
                world::WorldPosition::create(
                    world::WorldCell{},
                    world::LocalPosition{
                        0.0,
                        0.0,
                        0.0
                    });

        if (!player_position_result.has_value())
        {
            return fail_live_player_creation(
                world,
                player_entity,
                floor_entity,
                player_position_result.error().code,
                player_position_result.error().message);
        }

        foundation::Status player_position_status =
            world.add_position(
                player_entity,
                std::move(
                    player_position_result.value()));

        if (!player_position_status.has_value())
        {
            return fail_live_player_creation(
                world,
                player_entity,
                floor_entity,
                player_position_status.error().code,
                player_position_status.error().message);
        }

        foundation::Result<PhysicalWorldDemo>
            physical_world_result =
                create_physical_world_demo(
                    world.world_namespace(),
                    player_entity,
                    floor_entity);

        if (!physical_world_result.has_value())
        {
            return fail_live_player_creation(
                world,
                player_entity,
                floor_entity,
                physical_world_result.error().code,
                physical_world_result.error().message);
        }

        PhysicalWorldDemo physical_world{
            std::move(
                physical_world_result.value())
        };

        if (!world.contains(player_entity) ||
            !world.contains(floor_entity) ||
            world.find_position(player_entity) ==
                nullptr ||
            physical_world.
                    player_collider.
                    owner !=
                player_entity ||
            physical_world.
                    floor_collider.
                    owner !=
                floor_entity)
        {
            return fail_live_player_creation(
                world,
                player_entity,
                floor_entity,
                foundation::ErrorCode::
                    invalid_state,
                "Live-player composition did not "
                "preserve its persistent World and "
                "collider identity contracts.");
        }

        return LivePlayerDemo{
            player_entity,
            floor_entity,
            std::move(physical_world)
        };
    }

    foundation::Status
    step_live_player_demo_fixed_tick(
        world::World& world,
        LivePlayerDemo& demo,
        const physical_world::
            WorldFirstPersonControllerCommand& command,
        const foundation::Nanoseconds simulation_step)
    {
        if (!demo.player_entity.is_valid() ||
            !demo.floor_entity.is_valid() ||
            demo.player_entity == demo.floor_entity)
        {
            return foundation::fail(
                foundation::ErrorCode::invalid_state,
                "Live-player simulation requires "
                "distinct valid player and floor "
                "identities.");
        }

        if (!world.contains(demo.player_entity) ||
            !world.contains(demo.floor_entity))
        {
            return foundation::fail(
                foundation::ErrorCode::invalid_state,
                "Live-player simulation identities "
                "are no longer owned by the World.");
        }

        if (demo.physical_world.
                player_collider.
                owner !=
                demo.player_entity ||
            demo.physical_world.
                floor_collider.
                owner !=
                demo.floor_entity)
        {
            return foundation::fail(
                foundation::ErrorCode::invalid_state,
                "Live-player collider identity "
                "continuity was not preserved.");
        }

        world::WorldPosition* player_position =
            world.find_position(
                demo.player_entity);

        if (player_position == nullptr)
        {
            return foundation::fail(
                foundation::ErrorCode::invalid_state,
                "Live player lost its authoritative "
                "WorldPosition during simulation.");
        }

        foundation::Result<world::WorldPosition>
            controller_step_result =
                physical_world::
                    step_world_first_person_controller(
                        demo.physical_world.registry,
                        world.world_namespace(),
                        demo.physical_world.
                            player_collider,
                        demo.physical_world.
                            player_capsule,
                        *player_position,
                        command,
                        demo.physical_world.
                            traversal_settings,
                        demo.physical_world.
                            controller_settings,
                        simulation_step);

        if (!controller_step_result.has_value())
        {
            return foundation::fail(
                controller_step_result.error().code,
                controller_step_result.error().message);
        }

        *player_position =
            std::move(
                controller_step_result.value());

        if (!player_position->is_normalized())
        {
            return foundation::fail(
                foundation::ErrorCode::invalid_state,
                "Live controller produced a "
                "non-normalized WorldPosition.");
        }

        return {};
    }
}
