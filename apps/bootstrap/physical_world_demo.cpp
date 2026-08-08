#include "physical_world_demo.hpp"

#include "oros/foundation/error.hpp"
#include "oros/physical_world/world_cell_collider_payload.hpp"
#include "oros/physical_world/world_cell_collider_set.hpp"
#include "oros/physics/collider_geometry.hpp"
#include "oros/physics/physics_vector.hpp"
#include "oros/streaming/world_cell_key.hpp"
#include "oros/streaming/world_cell_revision_id.hpp"
#include "oros/streaming/world_cell_snapshot.hpp"
#include "oros/world/world_position.hpp"

#include <array>
#include <cstddef>
#include <span>
#include <utility>
#include <vector>

namespace oros::bootstrap
{
    foundation::Result<PhysicalWorldDemo>
    create_physical_world_demo(
        const std::uint64_t world_namespace,
        const world::EntityId player_entity,
        const world::EntityId floor_entity)
    {
        if (world_namespace == 0ULL)
        {
            return foundation::fail(
                foundation::ErrorCode::invalid_argument,
                "Bootstrap physical world requires a "
                "non-zero world namespace.");
        }

        if (!player_entity.is_valid() ||
            player_entity.world_namespace !=
                world_namespace)
        {
            return foundation::fail(
                foundation::ErrorCode::invalid_argument,
                "Bootstrap player identity must belong "
                "to the physical world namespace.");
        }

        if (!floor_entity.is_valid() ||
            floor_entity.world_namespace !=
                world_namespace)
        {
            return foundation::fail(
                foundation::ErrorCode::invalid_argument,
                "Bootstrap floor identity must belong "
                "to the physical world namespace.");
        }

        if (player_entity == floor_entity)
        {
            return foundation::fail(
                foundation::ErrorCode::invalid_argument,
                "Bootstrap player and floor require "
                "distinct persistent entity identities.");
        }

        const physics::ColliderId player_collider{
            player_entity,
            1U
        };

        const physics::ColliderId floor_collider{
            floor_entity,
            1U
        };

        foundation::Result<physics::CapsuleShape>
            capsule_result =
                physics::CapsuleShape::create(
                    1.0,
                    1.0,
                    physics::PhysicsUnitVector3::
                        positive_y());

        if (!capsule_result.has_value())
        {
            return foundation::fail(
                capsule_result.error().code,
                capsule_result.error().message);
        }

        foundation::Result<physics::BoxShape>
            floor_shape_result =
                physics::BoxShape::create(
                    physics::PhysicsVector3{
                        256.0,
                        0.5,
                        256.0
                    });

        if (!floor_shape_result.has_value())
        {
            return foundation::fail(
                floor_shape_result.error().code,
                floor_shape_result.error().message);
        }

        foundation::Result<
            physical_world::
                WorldCapsuleTraversalSettings>
            traversal_result =
                physical_world::
                    WorldCapsuleTraversalSettings::
                        create(
                            physics::
                                PhysicsUnitVector3::
                                    positive_y(),
                            0.70,
                            0.5,
                            0.25);

        if (!traversal_result.has_value())
        {
            return foundation::fail(
                traversal_result.error().code,
                traversal_result.error().message);
        }

        foundation::Result<
            physical_world::
                WorldFirstPersonControllerSettings>
            controller_result =
                physical_world::
                    WorldFirstPersonControllerSettings::
                        create(
                            6.0,
                            0.125,
                            64U,
                            4U);

        if (!controller_result.has_value())
        {
            return foundation::fail(
                controller_result.error().code,
                controller_result.error().message);
        }

        foundation::Result<
            physics::ColliderGeometry>
            floor_geometry_result =
                physics::ColliderGeometry::create(
                    floor_collider,
                    floor_shape_result.value(),
                    physics::PhysicsVector3{
                        0.0,
                        -2.5,
                        0.0
                    });

        if (!floor_geometry_result.has_value())
        {
            return foundation::fail(
                floor_geometry_result.error().code,
                floor_geometry_result.error().message);
        }

        const streaming::WorldCellKey cell_key{
            world_namespace,
            world::WorldCell{}
        };

        const std::array<
            physics::ColliderGeometry,
            1U>
            colliders{
                floor_geometry_result.value()
            };

        foundation::Result<
            physical_world::WorldCellColliderSet>
            collider_set_result =
                physical_world::
                    WorldCellColliderSet::create(
                        cell_key,
                        std::span<
                            const physics::
                                ColliderGeometry>{
                                    colliders
                                });

        if (!collider_set_result.has_value())
        {
            return foundation::fail(
                collider_set_result.error().code,
                collider_set_result.error().message);
        }

        foundation::Result<
            std::vector<std::byte>>
            payload_result =
                physical_world::
                    serialize_world_cell_collider_payload(
                        collider_set_result.value());

        if (!payload_result.has_value())
        {
            return foundation::fail(
                payload_result.error().code,
                payload_result.error().message);
        }

        foundation::Result<
            streaming::WorldCellSnapshot>
            snapshot_result =
                streaming::WorldCellSnapshot::create(
                    cell_key,
                    1ULL,
                    std::span<const std::byte>{
                        payload_result.value()
                    });

        if (!snapshot_result.has_value())
        {
            return foundation::fail(
                snapshot_result.error().code,
                snapshot_result.error().message);
        }

        foundation::Result<
            streaming::WorldCellResidency>
            residency_result =
                streaming::WorldCellResidency::create(
                    cell_key);

        if (!residency_result.has_value())
        {
            return foundation::fail(
                residency_result.error().code,
                residency_result.error().message);
        }

        streaming::WorldCellResidency residency{
            std::move(
                residency_result.value())
        };

        const streaming::WorldCellRevisionId
            revision{
                cell_key,
                snapshot_result.value().revision()
            };

        constexpr std::uint64_t request_id{
            1ULL
        };

        foundation::Status queue_status =
            residency.queue_load(
                revision,
                request_id);

        if (!queue_status.has_value())
        {
            return foundation::fail(
                queue_status.error().code,
                queue_status.error().message);
        }

        foundation::Status begin_status =
            residency.begin_load(
                request_id);

        if (!begin_status.has_value())
        {
            return foundation::fail(
                begin_status.error().code,
                begin_status.error().message);
        }

        foundation::Status complete_status =
            residency.complete_load(
                request_id,
                std::move(
                    snapshot_result.value()));

        if (!complete_status.has_value())
        {
            return foundation::fail(
                complete_status.error().code,
                complete_status.error().message);
        }

        physical_world::WorldCellColliderRegistry
            registry{};

        foundation::Status synchronize_status =
            registry.synchronize(
                residency);

        if (!synchronize_status.has_value())
        {
            return foundation::fail(
                synchronize_status.error().code,
                synchronize_status.error().message);
        }

        if (!residency.is_valid() ||
            !residency.is_resident() ||
            !registry.is_valid() ||
            registry.active_cell_count() != 1U ||
            registry.collider_count() != 1U ||
            !registry.contains(floor_collider))
        {
            return foundation::fail(
                foundation::ErrorCode::invalid_state,
                "Bootstrap physical world did not "
                "activate exactly one resident floor "
                "collider.");
        }

        return PhysicalWorldDemo{
            std::move(residency),
            std::move(registry),
            player_collider,
            floor_collider,
            std::move(
                capsule_result.value()),
            std::move(
                traversal_result.value()),
            std::move(
                controller_result.value())
        };
    }
}
