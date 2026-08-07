#include "oros/physical_world/world_capsule_ground_query.hpp"

#include "oros/foundation/error.hpp"
#include "oros/physical_world/world_cell_collider_payload.hpp"
#include "oros/physical_world/world_cell_collider_set.hpp"
#include "oros/physics/collider_geometry.hpp"
#include "oros/physics/collider_id.hpp"
#include "oros/physics/collision_shape.hpp"
#include "oros/physics/physics_vector.hpp"
#include "oros/streaming/world_cell_residency.hpp"
#include "oros/streaming/world_cell_revision_id.hpp"
#include "oros/streaming/world_cell_snapshot.hpp"
#include "oros/world/entity_id.hpp"
#include "oros/world/world_position.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <string_view>
#include <utility>

namespace
{
    struct TestState final
    {
        int checks{};
        int failures{};
    };

    void check(
        TestState& state,
        const bool condition,
        const std::string_view name)
    {
        ++state.checks;

        if (condition)
        {
            std::cout
                << "[pass] "
                << name
                << '\n';

            return;
        }

        ++state.failures;

        std::cerr
            << "[fail] "
            << name
            << '\n';
    }

    int finish(
        const TestState& state)
    {
        std::cout
            << "\nWorld capsule ground query "
            << "test summary: "
            << state.checks - state.failures
            << '/'
            << state.checks
            << " passed.\n";

        return
            state.failures == 0
                ? 0
                : 1;
    }

    [[nodiscard]]
    bool make_resident(
        oros::streaming::WorldCellResidency&
            residency,
        oros::streaming::WorldCellSnapshot
            snapshot,
        const std::uint64_t request_id)
    {
        const oros::streaming::
            WorldCellRevisionId
            revision{
                snapshot.key(),
                snapshot.revision()
            };

        if (!residency.queue_load(
                revision,
                request_id).
                has_value())
        {
            return false;
        }

        if (!residency.begin_load(
                request_id).
                has_value())
        {
            return false;
        }

        return
            residency.complete_load(
                request_id,
                std::move(snapshot)).
                has_value();
    }
}

int main()
{
    using namespace oros::foundation;
    using namespace oros::physical_world;
    using namespace oros::physics;
    using namespace oros::streaming;
    using namespace oros::world;

    TestState state{};

    constexpr std::uint64_t
        world_namespace{
            0x4F524F5300000008ULL
        };

    const ColliderId query_high{
        EntityId{
            world_namespace,
            900ULL
        },
        1U
    };

    const ColliderId query_low{
        EntityId{
            world_namespace,
            50ULL
        },
        1U
    };

    const ColliderId shallow_floor_id{
        EntityId{
            world_namespace,
            100ULL
        },
        1U
    };

    const ColliderId deep_floor_id{
        EntityId{
            world_namespace,
            200ULL
        },
        1U
    };

    const ColliderId left_wall_id{
        EntityId{
            world_namespace,
            300ULL
        },
        1U
    };

    const ColliderId slope_support_id{
        EntityId{
            world_namespace,
            400ULL
        },
        1U
    };

    const auto capsule_shape_result =
        CapsuleShape::create(
            1.0,
            1.0,
            PhysicsUnitVector3::
                positive_y());

    const auto floor_shape_result =
        BoxShape::create(
            PhysicsVector3{
                10.0,
                0.5,
                10.0
            });

    const auto wall_shape_result =
        BoxShape::create(
            PhysicsVector3{
                0.5,
                10.0,
                10.0
            });

    const auto slope_support_shape_result =
        SphereShape::create(
            1.0);

    check(
        state,
        capsule_shape_result.has_value(),
        "Ground-query capsule shape is created");

    check(
        state,
        floor_shape_result.has_value(),
        "Ground-query floor shape is created");

    check(
        state,
        wall_shape_result.has_value(),
        "Ground-query wall shape is created");

    check(
        state,
        slope_support_shape_result.has_value(),
        "Ground-query slope support shape is created");

    if (!capsule_shape_result.has_value() ||
        !floor_shape_result.has_value() ||
        !wall_shape_result.has_value() ||
        !slope_support_shape_result.has_value())
    {
        return finish(state);
    }

    const auto origin_result =
        WorldPosition::origin();

    check(
        state,
        origin_result.has_value(),
        "Ground-query world origin is created");

    if (!origin_result.has_value())
    {
        return finish(state);
    }

    const auto y_settings_result =
        WorldCapsuleTraversalSettings::create(
            PhysicsUnitVector3::positive_y(),
            1.0,
            0.5,
            0.25);

    const auto x_settings_result =
        WorldCapsuleTraversalSettings::create(
            PhysicsUnitVector3::positive_x(),
            1.0,
            0.5,
            0.25);

    const auto z_settings_result =
        WorldCapsuleTraversalSettings::create(
            PhysicsUnitVector3::positive_z(),
            1.0,
            0.5,
            0.25);

    const auto walkable_slope_settings_result =
        WorldCapsuleTraversalSettings::create(
            PhysicsUnitVector3::positive_y(),
            0.70,
            0.5,
            0.25);

    const auto steep_slope_settings_result =
        WorldCapsuleTraversalSettings::create(
            PhysicsUnitVector3::positive_y(),
            0.72,
            0.5,
            0.25);

    check(
        state,
        y_settings_result.has_value(),
        "Positive-Y traversal settings are created");

    check(
        state,
        x_settings_result.has_value(),
        "Positive-X traversal settings are created");

    check(
        state,
        z_settings_result.has_value(),
        "Positive-Z traversal settings are created");

    check(
        state,
        walkable_slope_settings_result.has_value(),
        "Walkable-slope traversal settings are created");

    check(
        state,
        steep_slope_settings_result.has_value(),
        "Steep-slope traversal settings are created");

    if (!y_settings_result.has_value() ||
        !x_settings_result.has_value() ||
        !z_settings_result.has_value() ||
        !walkable_slope_settings_result.has_value() ||
        !steep_slope_settings_result.has_value())
    {
        return finish(state);
    }

    WorldCellColliderRegistry empty_registry{};

    const auto empty_result =
        query_world_capsule_ground_contact(
            empty_registry,
            world_namespace,
            query_high,
            capsule_shape_result.value(),
            origin_result.value(),
            y_settings_result.value());

    check(
        state,
        empty_result.has_value(),
        "Empty-world ground query succeeds");

    check(
        state,
        empty_result.has_value() &&
            !empty_result.value().has_value(),
        "Empty world has no ground contact");

    const auto invalid_namespace_result =
        query_world_capsule_ground_contact(
            empty_registry,
            0ULL,
            query_high,
            capsule_shape_result.value(),
            origin_result.value(),
            y_settings_result.value());

    check(
        state,
        !invalid_namespace_result.has_value() &&
            invalid_namespace_result.
                error().
                code ==
            ErrorCode::invalid_argument,
        "Ground query propagates invalid namespace");

    const WorldCellKey cell_key{
        world_namespace,
        WorldCell{}
    };

    const auto shallow_floor_result =
        ColliderGeometry::create(
            shallow_floor_id,
            floor_shape_result.value(),
            PhysicsVector3{
                0.0,
                -2.5,
                0.0
            });

    const auto deep_floor_result =
        ColliderGeometry::create(
            deep_floor_id,
            floor_shape_result.value(),
            PhysicsVector3{
                0.0,
                -2.25,
                0.0
            });

    const auto left_wall_result =
        ColliderGeometry::create(
            left_wall_id,
            wall_shape_result.value(),
            PhysicsVector3{
                -1.5,
                0.0,
                0.0
            });

    const auto slope_support_result =
        ColliderGeometry::create(
            slope_support_id,
            slope_support_shape_result.value(),
            PhysicsVector3{
                21.2,
                -2.2,
                0.0
            });

    check(
        state,
        shallow_floor_result.has_value(),
        "Touching floor geometry is created");

    check(
        state,
        deep_floor_result.has_value(),
        "Penetrating floor geometry is created");

    check(
        state,
        left_wall_result.has_value(),
        "Touching wall geometry is created");

    check(
        state,
        slope_support_result.has_value(),
        "Sloped sphere support geometry is created");

    if (!shallow_floor_result.has_value() ||
        !deep_floor_result.has_value() ||
        !left_wall_result.has_value() ||
        !slope_support_result.has_value())
    {
        return finish(state);
    }

    const std::array<
        ColliderGeometry,
        4U>
        colliders{
            shallow_floor_result.value(),
            deep_floor_result.value(),
            left_wall_result.value(),
            slope_support_result.value()
        };

    const auto collider_set_result =
        WorldCellColliderSet::create(
            cell_key,
            std::span<
                const ColliderGeometry>{
                    colliders
                });

    check(
        state,
        collider_set_result.has_value(),
        "Ground-query collider set is created");

    if (!collider_set_result.has_value())
    {
        return finish(state);
    }

    const auto payload_result =
        serialize_world_cell_collider_payload(
            collider_set_result.value());

    check(
        state,
        payload_result.has_value(),
        "Ground-query collider payload serializes");

    if (!payload_result.has_value())
    {
        return finish(state);
    }

    const auto snapshot_result =
        WorldCellSnapshot::create(
            cell_key,
            1ULL,
            std::span<const std::byte>{
                payload_result.value()
            });

    check(
        state,
        snapshot_result.has_value(),
        "Ground-query world snapshot is created");

    if (!snapshot_result.has_value())
    {
        return finish(state);
    }

    auto residency_result =
        WorldCellResidency::create(
            cell_key);

    check(
        state,
        residency_result.has_value(),
        "Ground-query cell residency is created");

    if (!residency_result.has_value())
    {
        return finish(state);
    }

    WorldCellResidency residency =
        std::move(
            residency_result.value());

    check(
        state,
        make_resident(
            residency,
            snapshot_result.value(),
            101ULL),
        "Ground-query cell becomes resident");

    WorldCellColliderRegistry registry{};

    check(
        state,
        registry.synchronize(
            residency).
            has_value(),
        "Ground-query colliders activate");

    check(
        state,
        registry.is_valid() &&
            registry.collider_count() ==
                4U,
        "Ground-query registry is valid");

    const auto y_ground_high_result =
        query_world_capsule_ground_contact(
            registry,
            world_namespace,
            query_high,
            capsule_shape_result.value(),
            origin_result.value(),
            y_settings_result.value());

    check(
        state,
        y_ground_high_result.has_value(),
        "Positive-Y ground query succeeds");

    check(
        state,
        y_ground_high_result.has_value() &&
            y_ground_high_result.
                value().
                has_value(),
        "Positive-Y ground query finds walkable ground");

    check(
        state,
        y_ground_high_result.has_value() &&
            y_ground_high_result.
                value().
                has_value() &&
            y_ground_high_result.
                value()->
                pair().
                contains(
                    deep_floor_id),
        "Deeper equally aligned floor is selected deterministically");

    const auto y_ground_low_result =
        query_world_capsule_ground_contact(
            registry,
            world_namespace,
            query_low,
            capsule_shape_result.value(),
            origin_result.value(),
            y_settings_result.value());

    check(
        state,
        y_ground_low_result.has_value() &&
            y_ground_low_result.
                value().
                has_value(),
        "Ground query succeeds when capsule identity sorts first");

    check(
        state,
        y_ground_low_result.has_value() &&
            y_ground_low_result.
                value().
                has_value() &&
            y_ground_low_result.
                value()->
                pair().
                contains(
                    deep_floor_id),
        "Canonical normal orientation preserves ground classification");

    const auto x_ground_result =
        query_world_capsule_ground_contact(
            registry,
            world_namespace,
            query_high,
            capsule_shape_result.value(),
            origin_result.value(),
            x_settings_result.value());

    check(
        state,
        x_ground_result.has_value(),
        "Positive-X ground query succeeds");

    check(
        state,
        x_ground_result.has_value() &&
            x_ground_result.
                value().
                has_value() &&
            x_ground_result.
                value()->
                pair().
                contains(
                    left_wall_id),
        "Explicit positive-X up direction classifies the left wall as ground");

    const auto z_ground_result =
        query_world_capsule_ground_contact(
            registry,
            world_namespace,
            query_high,
            capsule_shape_result.value(),
            origin_result.value(),
            z_settings_result.value());

    check(
        state,
        z_ground_result.has_value(),
        "Positive-Z ground query succeeds");

    check(
        state,
        z_ground_result.has_value() &&
            !z_ground_result.
                value().
                has_value(),
        "Contacts without sufficient positive-Z alignment are rejected");

    const auto slope_position_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                20.0,
                0.0,
                0.0
            });

    check(
        state,
        slope_position_result.has_value(),
        "Sloped-support query position is created");

    if (!slope_position_result.has_value())
    {
        return finish(state);
    }

    const auto walkable_slope_result =
        query_world_capsule_ground_contact(
            registry,
            world_namespace,
            query_high,
            capsule_shape_result.value(),
            slope_position_result.value(),
            walkable_slope_settings_result.value());

    check(
        state,
        walkable_slope_result.has_value() &&
            walkable_slope_result.
                value().
                has_value(),
        "Non-axis-aligned support passes the walkable slope threshold");

    check(
        state,
        walkable_slope_result.has_value() &&
            walkable_slope_result.
                value().
                has_value() &&
            walkable_slope_result.
                value()->
                pair().
                contains(
                    slope_support_id),
        "Walkable slope query selects the spherical support");

    const auto steep_slope_result =
        query_world_capsule_ground_contact(
            registry,
            world_namespace,
            query_high,
            capsule_shape_result.value(),
            slope_position_result.value(),
            steep_slope_settings_result.value());

    check(
        state,
        steep_slope_result.has_value() &&
            !steep_slope_result.
                value().
                has_value(),
        "Same non-axis-aligned support is rejected above the slope threshold");

    return finish(state);
}