#include "oros/physical_world/world_capsule_ground_motion.hpp"

#include "oros/physical_world/world_capsule_ground_query.hpp"
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
#include <cmath>
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
            << "\nWorld capsule step-up "
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
    using namespace oros::physical_world;
    using namespace oros::physics;
    using namespace oros::streaming;
    using namespace oros::world;

    TestState state{};

    constexpr std::uint64_t
        world_namespace{
            0x4F524F5300000008ULL
        };

    const ColliderId query_collider{
        EntityId{
            world_namespace,
            900ULL
        },
        1U
    };

    const ColliderId low_floor_id{
        EntityId{
            world_namespace,
            100ULL
        },
        1U
    };

    const ColliderId low_step_id{
        EntityId{
            world_namespace,
            200ULL
        },
        1U
    };

    const ColliderId high_floor_id{
        EntityId{
            world_namespace,
            300ULL
        },
        1U
    };

    const ColliderId high_step_id{
        EntityId{
            world_namespace,
            400ULL
        },
        1U
    };

    const ColliderId x_floor_id{
        EntityId{
            world_namespace,
            500ULL
        },
        1U
    };

    const ColliderId x_step_id{
        EntityId{
            world_namespace,
            600ULL
        },
        1U
    };

    const auto capsule_shape_result =
        CapsuleShape::create(
            1.0,
            1.0,
            PhysicsUnitVector3::
                positive_y());

    const auto x_capsule_shape_result =
        CapsuleShape::create(
            1.0,
            1.0,
            PhysicsUnitVector3::
                positive_x());

    const auto floor_shape_result =
        BoxShape::create(
            PhysicsVector3{
                5.0,
                0.5,
                5.0
            });

    const auto low_step_shape_result =
        BoxShape::create(
            PhysicsVector3{
                1.0,
                0.2,
                2.0
            });

    const auto high_step_shape_result =
        BoxShape::create(
            PhysicsVector3{
                1.0,
                0.3,
                2.0
            });

    const auto x_floor_shape_result =
        BoxShape::create(
            PhysicsVector3{
                0.5,
                5.0,
                5.0
            });

    const auto x_step_shape_result =
        BoxShape::create(
            PhysicsVector3{
                0.2,
                1.0,
                2.0
            });

    check(
        state,
        capsule_shape_result.has_value(),
        "Step-up capsule shape is created");

    check(
        state,
        x_capsule_shape_result.has_value(),
        "Positive-X-up step capsule shape is created");

    check(
        state,
        floor_shape_result.has_value(),
        "Step-up floor shape is created");

    check(
        state,
        low_step_shape_result.has_value(),
        "Walkable-height step shape is created");

    check(
        state,
        high_step_shape_result.has_value(),
        "Over-height step shape is created");

    check(
        state,
        x_floor_shape_result.has_value(),
        "Positive-X-up support shape is created");

    check(
        state,
        x_step_shape_result.has_value(),
        "Positive-X-up raised-step shape is created");

    if (!capsule_shape_result.has_value() ||
        !x_capsule_shape_result.has_value() ||
        !floor_shape_result.has_value() ||
        !low_step_shape_result.has_value() ||
        !high_step_shape_result.has_value() ||
        !x_floor_shape_result.has_value() ||
        !x_step_shape_result.has_value())
    {
        return finish(state);
    }

    const auto settings_result =
        WorldCapsuleTraversalSettings::create(
            PhysicsUnitVector3::positive_y(),
            0.70,
            0.5,
            0.25);

    const auto tight_snap_settings_result =
        WorldCapsuleTraversalSettings::create(
            PhysicsUnitVector3::positive_y(),
            0.70,
            0.5,
            0.05);

    const auto x_settings_result =
        WorldCapsuleTraversalSettings::create(
            PhysicsUnitVector3::positive_x(),
            0.70,
            0.5,
            0.25);

    check(
        state,
        settings_result.has_value(),
        "Step-up traversal settings are created");

    check(
        state,
        tight_snap_settings_result.has_value(),
        "Step-up tight-snap traversal settings are created");

    check(
        state,
        x_settings_result.has_value(),
        "Positive-X-up step traversal settings are created");

    if (!settings_result.has_value() ||
        !tight_snap_settings_result.has_value() ||
        !x_settings_result.has_value())
    {
        return finish(state);
    }

    const auto low_floor_result =
        ColliderGeometry::create(
            low_floor_id,
            floor_shape_result.value(),
            PhysicsVector3{
                0.0,
                -2.5,
                0.0
            });

    const auto low_step_result =
        ColliderGeometry::create(
            low_step_id,
            low_step_shape_result.value(),
            PhysicsVector3{
                1.5,
                -1.8,
                0.0
            });

    const auto high_floor_result =
        ColliderGeometry::create(
            high_floor_id,
            floor_shape_result.value(),
            PhysicsVector3{
                20.0,
                -2.5,
                0.0
            });

    const auto high_step_result =
        ColliderGeometry::create(
            high_step_id,
            high_step_shape_result.value(),
            PhysicsVector3{
                21.5,
                -1.7,
                0.0
            });

    const auto x_floor_result =
        ColliderGeometry::create(
            x_floor_id,
            x_floor_shape_result.value(),
            PhysicsVector3{
                -2.5,
                40.0,
                0.0
            });

    const auto x_step_result =
        ColliderGeometry::create(
            x_step_id,
            x_step_shape_result.value(),
            PhysicsVector3{
                -1.8,
                41.5,
                0.0
            });

    check(
        state,
        low_floor_result.has_value(),
        "Walkable-step floor geometry is created");

    check(
        state,
        low_step_result.has_value(),
        "Walkable-height step geometry is created");

    check(
        state,
        high_floor_result.has_value(),
        "Over-height-step floor geometry is created");

    check(
        state,
        high_step_result.has_value(),
        "Over-height step geometry is created");

    check(
        state,
        x_floor_result.has_value(),
        "Positive-X-up lower support is created");

    check(
        state,
        x_step_result.has_value(),
        "Positive-X-up raised support is created");

    if (!low_floor_result.has_value() ||
        !low_step_result.has_value() ||
        !high_floor_result.has_value() ||
        !high_step_result.has_value() ||
        !x_floor_result.has_value() ||
        !x_step_result.has_value())
    {
        return finish(state);
    }

    const WorldCellKey cell_key{
        world_namespace,
        WorldCell{}
    };

    const std::array<
        ColliderGeometry,
        6U>
        colliders{
            low_floor_result.value(),
            low_step_result.value(),
            high_floor_result.value(),
            high_step_result.value(),
            x_floor_result.value(),
            x_step_result.value()
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
        "Step-up collider set is created");

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
        "Step-up collider payload serializes");

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
        "Step-up world snapshot is created");

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
        "Step-up residency is created");

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
        "Step-up cell becomes resident");

    WorldCellColliderRegistry registry{};

    check(
        state,
        registry.synchronize(
            residency).
            has_value(),
        "Step-up colliders activate");

    check(
        state,
        registry.is_valid() &&
            registry.collider_count() ==
                6U,
        "Step-up registry is valid");

    const auto low_start_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                -2.0,
                0.0,
                0.0
            });

    const auto high_start_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                18.0,
                0.0,
                0.0
            });

    const auto x_start_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                0.0,
                38.0,
                0.0
            });

    check(
        state,
        low_start_result.has_value() &&
            high_start_result.has_value() &&
            x_start_result.has_value(),
        "Step-up start positions are created");

    if (!low_start_result.has_value() ||
        !high_start_result.has_value() ||
        !x_start_result.has_value())
    {
        return finish(state);
    }

    const auto low_initial_ground_result =
        query_world_capsule_ground_contact(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            low_start_result.value(),
            settings_result.value());

    check(
        state,
        low_initial_ground_result.has_value() &&
            low_initial_ground_result.
                value().
                has_value() &&
            low_initial_ground_result.
                value()->
                pair().
                contains(
                    low_floor_id),
        "Walkable-step fixture starts grounded");

    const auto low_motion_result =
        move_world_capsule_along_ground(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            low_start_result.value(),
            WorldDisplacement{
                4.0,
                0.0,
                0.0
            },
            settings_result.value(),
            0.1,
            64U,
            4U);

    check(
        state,
        low_motion_result.has_value(),
        "Walkable-height step motion resolves");

    if (!low_motion_result.has_value())
    {
        return finish(state);
    }

    const auto low_displacement_result =
        low_start_result.
            value().
            displacement_to(
                low_motion_result.value());

    check(
        state,
        low_displacement_result.has_value(),
        "Walkable-height step displacement can be measured");

    check(
        state,
        low_displacement_result.has_value() &&
            nearly_equal(
                low_displacement_result.
                    value().
                    x,
                4.0) &&
            nearly_equal(
                low_displacement_result.
                    value().
                    y,
                0.4) &&
            nearly_equal(
                low_displacement_result.
                    value().
                    z,
                0.0),
        "Step within maximum height preserves forward travel and climbs");

    const auto low_final_ground_result =
        query_world_capsule_ground_contact(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            low_motion_result.value(),
            settings_result.value());

    check(
        state,
        low_final_ground_result.has_value() &&
            low_final_ground_result.
                value().
                has_value() &&
            low_final_ground_result.
                value()->
                pair().
                contains(
                    low_step_id),
        "Successful step-up ends on the raised walkable support");

    const auto tight_snap_motion_result =
        move_world_capsule_along_ground(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            low_start_result.value(),
            WorldDisplacement{
                4.0,
                0.0,
                0.0
            },
            tight_snap_settings_result.value(),
            0.1,
            64U,
            4U);

    check(
        state,
        tight_snap_motion_result.has_value(),
        "Step-up is independent of ordinary ground-snap distance");

    if (!tight_snap_motion_result.has_value())
    {
        return finish(state);
    }

    const auto tight_snap_displacement_result =
        low_start_result.
            value().
            displacement_to(
                tight_snap_motion_result.value());

    check(
        state,
        tight_snap_displacement_result.has_value() &&
            nearly_equal(
                tight_snap_displacement_result.
                    value().
                    x,
                4.0) &&
            nearly_equal(
                tight_snap_displacement_result.
                    value().
                    y,
                0.4) &&
            nearly_equal(
                tight_snap_displacement_result.
                    value().
                    z,
                0.0),
        "Tight ordinary snap still completes the valid step");

    const auto tight_snap_ground_result =
        query_world_capsule_ground_contact(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            tight_snap_motion_result.value(),
            tight_snap_settings_result.value());

    check(
        state,
        tight_snap_ground_result.has_value() &&
            tight_snap_ground_result.
                value().
                has_value() &&
            tight_snap_ground_result.
                value()->
                pair().
                contains(
                    low_step_id),
        "Tight-snap step ends on the raised support");

    const auto high_initial_ground_result =
        query_world_capsule_ground_contact(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            high_start_result.value(),
            settings_result.value());

    check(
        state,
        high_initial_ground_result.has_value() &&
            high_initial_ground_result.
                value().
                has_value() &&
            high_initial_ground_result.
                value()->
                pair().
                contains(
                    high_floor_id),
        "Over-height-step fixture starts grounded");

    const auto high_motion_result =
        move_world_capsule_along_ground(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            high_start_result.value(),
            WorldDisplacement{
                4.0,
                0.0,
                0.0
            },
            settings_result.value(),
            0.1,
            64U,
            4U);

    check(
        state,
        high_motion_result.has_value(),
        "Over-height step motion resolves");

    if (!high_motion_result.has_value())
    {
        return finish(state);
    }

    const auto high_displacement_result =
        high_start_result.
            value().
            displacement_to(
                high_motion_result.value());

    check(
        state,
        high_displacement_result.has_value() &&
            high_displacement_result.
                value().
                x >
                0.0 &&
            high_displacement_result.
                value().
                x <
                4.0 &&
            nearly_equal(
                high_displacement_result.
                    value().
                    y,
                0.0) &&
            nearly_equal(
                high_displacement_result.
                    value().
                    z,
                0.0),
        "Step above maximum height remains blocked");

    const auto high_final_ground_result =
        query_world_capsule_ground_contact(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            high_motion_result.value(),
            settings_result.value());

    check(
        state,
        high_final_ground_result.has_value() &&
            high_final_ground_result.
                value().
                has_value() &&
            high_final_ground_result.
                value()->
                pair().
                contains(
                    high_floor_id),
        "Blocked over-height step remains on the lower support");

    const auto x_initial_ground_result =
        query_world_capsule_ground_contact(
            registry,
            world_namespace,
            query_collider,
            x_capsule_shape_result.value(),
            x_start_result.value(),
            x_settings_result.value());

    check(
        state,
        x_initial_ground_result.has_value() &&
            x_initial_ground_result.
                value().
                has_value() &&
            x_initial_ground_result.
                value()->
                pair().
                contains(
                    x_floor_id),
        "Positive-X-up step fixture starts grounded");

    const auto x_motion_result =
        move_world_capsule_along_ground(
            registry,
            world_namespace,
            query_collider,
            x_capsule_shape_result.value(),
            x_start_result.value(),
            WorldDisplacement{
                0.0,
                4.0,
                0.0
            },
            x_settings_result.value(),
            0.1,
            64U,
            4U);

    check(
        state,
        x_motion_result.has_value(),
        "Positive-X-up step motion resolves");

    if (!x_motion_result.has_value())
    {
        return finish(state);
    }

    const auto x_displacement_result =
        x_start_result.
            value().
            displacement_to(
                x_motion_result.value());

    check(
        state,
        x_displacement_result.has_value() &&
            nearly_equal(
                x_displacement_result.
                    value().
                    x,
                0.4) &&
            nearly_equal(
                x_displacement_result.
                    value().
                    y,
                4.0) &&
            nearly_equal(
                x_displacement_result.
                    value().
                    z,
                0.0),
        "Step-up follows explicit positive-X up direction");

    const auto x_final_ground_result =
        query_world_capsule_ground_contact(
            registry,
            world_namespace,
            query_collider,
            x_capsule_shape_result.value(),
            x_motion_result.value(),
            x_settings_result.value());

    check(
        state,
        x_final_ground_result.has_value() &&
            x_final_ground_result.
                value().
                has_value() &&
            x_final_ground_result.
                value()->
                pair().
                contains(
                    x_step_id),
        "Positive-X-up step ends on raised support");

    return finish(state);
}
