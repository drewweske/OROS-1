#include "oros/physical_world/world_capsule_ground_snap.hpp"

#include "oros/foundation/error.hpp"
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
            << "\nWorld capsule ground snap "
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

    const ColliderId query_collider{
        EntityId{
            world_namespace,
            900ULL
        },
        1U
    };

    const ColliderId near_floor_id{
        EntityId{
            world_namespace,
            100ULL
        },
        1U
    };

    const ColliderId far_floor_id{
        EntityId{
            world_namespace,
            200ULL
        },
        1U
    };

    const ColliderId boundary_floor_id{
        EntityId{
            world_namespace,
            300ULL
        },
        1U
    };

    const ColliderId left_wall_id{
        EntityId{
            world_namespace,
            400ULL
        },
        1U
    };

    const ColliderId penetrating_floor_id{
        EntityId{
            world_namespace,
            500ULL
        },
        1U
    };

    const ColliderId side_wall_id{
        EntityId{
            world_namespace,
            600ULL
        },
        1U
    };

    const ColliderId intermediate_floor_id{
        EntityId{
            world_namespace,
            700ULL
        },
        1U
    };

    const ColliderId intermediate_blocker_id{
        EntityId{
            world_namespace,
            800ULL
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
                2.0,
                0.5,
                2.0
            });

    const auto wall_shape_result =
        BoxShape::create(
            PhysicsVector3{
                0.5,
                2.0,
                2.0
            });

    const auto sphere_shape_result =
        SphereShape::create(
            1.0);

    check(
        state,
        capsule_shape_result.has_value(),
        "Ground-snap capsule shape is created");

    check(
        state,
        floor_shape_result.has_value(),
        "Ground-snap floor shape is created");

    check(
        state,
        wall_shape_result.has_value(),
        "Ground-snap wall shape is created");

    check(
        state,
        sphere_shape_result.has_value(),
        "Ground-snap blocker sphere shape is created");

    if (!capsule_shape_result.has_value() ||
        !floor_shape_result.has_value() ||
        !wall_shape_result.has_value() ||
        !sphere_shape_result.has_value())
    {
        return finish(state);
    }

    const auto y_settings_result =
        WorldCapsuleTraversalSettings::create(
            PhysicsUnitVector3::positive_y(),
            1.0,
            0.5,
            0.25);

    const auto y_disabled_settings_result =
        WorldCapsuleTraversalSettings::create(
            PhysicsUnitVector3::positive_y(),
            1.0,
            0.5,
            0.0);

    const auto x_settings_result =
        WorldCapsuleTraversalSettings::create(
            PhysicsUnitVector3::positive_x(),
            1.0,
            0.5,
            0.25);

    check(
        state,
        y_settings_result.has_value(),
        "Positive-Y ground-snap settings are created");

    check(
        state,
        y_disabled_settings_result.has_value(),
        "Disabled ground-snap settings are created");

    check(
        state,
        x_settings_result.has_value(),
        "Positive-X ground-snap settings are created");

    if (!y_settings_result.has_value() ||
        !y_disabled_settings_result.has_value() ||
        !x_settings_result.has_value())
    {
        return finish(state);
    }

    const WorldCellKey cell_key{
        world_namespace,
        WorldCell{}
    };

    const auto near_floor_result =
        ColliderGeometry::create(
            near_floor_id,
            floor_shape_result.value(),
            PhysicsVector3{
                0.0,
                -2.6,
                0.0
            });

    const auto far_floor_result =
        ColliderGeometry::create(
            far_floor_id,
            floor_shape_result.value(),
            PhysicsVector3{
                20.0,
                -2.8,
                0.0
            });

    const auto boundary_floor_result =
        ColliderGeometry::create(
            boundary_floor_id,
            floor_shape_result.value(),
            PhysicsVector3{
                40.0,
                -2.75,
                0.0
            });

    const auto left_wall_result =
        ColliderGeometry::create(
            left_wall_id,
            wall_shape_result.value(),
            PhysicsVector3{
                58.4,
                0.0,
                0.0
            });

    const auto penetrating_floor_result =
        ColliderGeometry::create(
            penetrating_floor_id,
            floor_shape_result.value(),
            PhysicsVector3{
                80.0,
                -2.25,
                0.0
            });

    const auto side_wall_result =
        ColliderGeometry::create(
            side_wall_id,
            wall_shape_result.value(),
            PhysicsVector3{
                101.5,
                0.0,
                0.0
            });

    const auto intermediate_floor_result =
        ColliderGeometry::create(
            intermediate_floor_id,
            floor_shape_result.value(),
            PhysicsVector3{
                120.0,
                -2.748,
                0.0
            });

    const auto intermediate_blocker_result =
        ColliderGeometry::create(
            intermediate_blocker_id,
            sphere_shape_result.value(),
            PhysicsVector3{
                121.85,
                -2.0,
                0.0
            });

    check(
        state,
        near_floor_result.has_value(),
        "Near-floor geometry is created");

    check(
        state,
        far_floor_result.has_value(),
        "Far-floor geometry is created");

    check(
        state,
        boundary_floor_result.has_value(),
        "Boundary-floor geometry is created");

    check(
        state,
        left_wall_result.has_value(),
        "Positive-X support wall is created");

    check(
        state,
        penetrating_floor_result.has_value(),
        "Penetrating-floor geometry is created");

    check(
        state,
        side_wall_result.has_value(),
        "Non-walkable side-wall geometry is created");

    check(
        state,
        intermediate_floor_result.has_value(),
        "Intermediate-blocker walkable floor is created");

    check(
        state,
        intermediate_blocker_result.has_value(),
        "Intermediate non-walkable blocker is created");

    if (!near_floor_result.has_value() ||
        !far_floor_result.has_value() ||
        !boundary_floor_result.has_value() ||
        !left_wall_result.has_value() ||
        !penetrating_floor_result.has_value() ||
        !side_wall_result.has_value() ||
        !intermediate_floor_result.has_value() ||
        !intermediate_blocker_result.has_value())
    {
        return finish(state);
    }

    const std::array<
        ColliderGeometry,
        8U>
        colliders{
            near_floor_result.value(),
            far_floor_result.value(),
            boundary_floor_result.value(),
            left_wall_result.value(),
            penetrating_floor_result.value(),
            side_wall_result.value(),
            intermediate_floor_result.value(),
            intermediate_blocker_result.value()
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
        "Ground-snap collider set is created");

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
        "Ground-snap collider payload serializes");

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
        "Ground-snap world snapshot is created");

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
        "Ground-snap residency is created");

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
        "Ground-snap cell becomes resident");

    WorldCellColliderRegistry registry{};

    check(
        state,
        registry.synchronize(
            residency).
            has_value(),
        "Ground-snap colliders activate");

    check(
        state,
        registry.is_valid() &&
            registry.collider_count() ==
                8U,
        "Ground-snap registry is valid");

    const auto near_start_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                0.0,
                0.0,
                0.0
            });

    const auto far_start_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                20.0,
                0.0,
                0.0
            });

    const auto boundary_start_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                40.0,
                0.0,
                0.0
            });

    const auto x_start_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                60.0,
                0.0,
                0.0
            });

    const auto penetration_start_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                80.0,
                0.0,
                0.0
            });

    const auto side_wall_start_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                100.0,
                0.0,
                0.0
            });

    const auto intermediate_start_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                120.0,
                0.0,
                0.0
            });

    check(
        state,
        near_start_result.has_value() &&
            far_start_result.has_value() &&
            boundary_start_result.has_value() &&
            x_start_result.has_value() &&
            penetration_start_result.has_value() &&
            side_wall_start_result.has_value() &&
            intermediate_start_result.has_value(),
        "Ground-snap start positions are created");

    if (!near_start_result.has_value() ||
        !far_start_result.has_value() ||
        !boundary_start_result.has_value() ||
        !x_start_result.has_value() ||
        !penetration_start_result.has_value() ||
        !side_wall_start_result.has_value() ||
        !intermediate_start_result.has_value())
    {
        return finish(state);
    }

    const auto expected_near_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                0.0,
                -0.1,
                0.0
            });

    const auto expected_boundary_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                40.0,
                -0.25,
                0.0
            });

    const auto expected_x_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                59.9,
                0.0,
                0.0
            });

    const auto expected_penetration_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                80.0,
                0.25,
                0.0
            });

    check(
        state,
        expected_near_result.has_value() &&
            expected_boundary_result.has_value() &&
            expected_x_result.has_value() &&
            expected_penetration_result.has_value(),
        "Expected ground-snap positions are created");

    if (!expected_near_result.has_value() ||
        !expected_boundary_result.has_value() ||
        !expected_x_result.has_value() ||
        !expected_penetration_result.has_value())
    {
        return finish(state);
    }

    const auto near_snap_result =
        snap_world_capsule_to_ground(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            near_start_result.value(),
            y_settings_result.value(),
            0.1,
            4U,
            4U);

    check(
        state,
        near_snap_result.has_value(),
        "Capsule snaps across a small ground gap");

    check(
        state,
        near_snap_result.has_value() &&
            nearly_equal(
                near_snap_result.
                    value().
                    local().
                    y,
                expected_near_result.
                    value().
                    local().
                    y),
        "Small-gap snap lands on the floor");

    const auto near_ground_result =
        near_snap_result.has_value()
            ? query_world_capsule_ground_contact(
                  registry,
                  world_namespace,
                  query_collider,
                  capsule_shape_result.value(),
                  near_snap_result.value(),
                  y_settings_result.value())
            : decltype(
                  query_world_capsule_ground_contact(
                      registry,
                      world_namespace,
                      query_collider,
                      capsule_shape_result.value(),
                      near_start_result.value(),
                      y_settings_result.value())){
                  oros::foundation::fail(
                      ErrorCode::internal_failure,
                      "Near snap did not produce a position.")
              };

    check(
        state,
        near_ground_result.has_value() &&
            near_ground_result.
                value().
                has_value() &&
            near_ground_result.
                value()->
                pair().
                contains(
                    near_floor_id),
        "Small-gap snap ends on walkable ground");

    const auto repeated_snap_result =
        near_snap_result.has_value()
            ? snap_world_capsule_to_ground(
                  registry,
                  world_namespace,
                  query_collider,
                  capsule_shape_result.value(),
                  near_snap_result.value(),
                  y_settings_result.value(),
                  0.1,
                  4U,
                  4U)
            : decltype(
                  snap_world_capsule_to_ground(
                      registry,
                      world_namespace,
                      query_collider,
                      capsule_shape_result.value(),
                      near_start_result.value(),
                      y_settings_result.value(),
                      0.1,
                      4U,
                      4U)){
                  oros::foundation::fail(
                      ErrorCode::internal_failure,
                      "Near snap did not produce a position.")
              };

    check(
        state,
        repeated_snap_result.has_value() &&
            near_snap_result.has_value() &&
            repeated_snap_result.value() ==
                near_snap_result.value(),
        "Repeated snap is stable when already grounded");

    const auto disabled_snap_result =
        snap_world_capsule_to_ground(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            near_start_result.value(),
            y_disabled_settings_result.value(),
            0.1,
            4U,
            4U);

    check(
        state,
        disabled_snap_result.has_value() &&
            disabled_snap_result.value() ==
                near_start_result.value(),
        "Zero ground-snap distance disables snapping");

    const auto far_snap_result =
        snap_world_capsule_to_ground(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            far_start_result.value(),
            y_settings_result.value(),
            0.1,
            4U,
            4U);

    check(
        state,
        far_snap_result.has_value() &&
            far_snap_result.value() ==
                far_start_result.value(),
        "Ground beyond snap distance is rejected");

    const auto boundary_snap_result =
        snap_world_capsule_to_ground(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            boundary_start_result.value(),
            y_settings_result.value(),
            0.1,
            4U,
            4U);

    check(
        state,
        boundary_snap_result.has_value(),
        "Ground at exact snap distance succeeds");

    check(
        state,
        boundary_snap_result.has_value() &&
            nearly_equal(
                boundary_snap_result.
                    value().
                    local().
                    y,
                expected_boundary_result.
                    value().
                    local().
                    y),
        "Exact-distance snap preserves touching support");

    const auto x_snap_result =
        snap_world_capsule_to_ground(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            x_start_result.value(),
            x_settings_result.value(),
            0.1,
            4U,
            4U);

    check(
        state,
        x_snap_result.has_value(),
        "Positive-X ground snap succeeds");

    check(
        state,
        x_snap_result.has_value() &&
            nearly_equal(
                x_snap_result.
                    value().
                    local().
                    x,
                expected_x_result.
                    value().
                    local().
                    x),
        "Ground snap follows explicit positive-X up direction");

    const auto x_ground_result =
        x_snap_result.has_value()
            ? query_world_capsule_ground_contact(
                  registry,
                  world_namespace,
                  query_collider,
                  capsule_shape_result.value(),
                  x_snap_result.value(),
                  x_settings_result.value())
            : decltype(
                  query_world_capsule_ground_contact(
                      registry,
                      world_namespace,
                      query_collider,
                      capsule_shape_result.value(),
                      x_start_result.value(),
                      x_settings_result.value())){
                  oros::foundation::fail(
                      ErrorCode::internal_failure,
                      "Positive-X snap did not produce a position.")
              };

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
        "Positive-X snap ends on its walkable support");

    const auto side_wall_snap_result =
        snap_world_capsule_to_ground(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            side_wall_start_result.value(),
            y_settings_result.value(),
            0.1,
            4U,
            4U);

    check(
        state,
        side_wall_snap_result.has_value() &&
            side_wall_snap_result.value() ==
                side_wall_start_result.value(),
        "Non-walkable wall contact does not cause a ground snap");

    const auto intermediate_initial_ground_result =
        query_world_capsule_ground_contact(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            intermediate_start_result.value(),
            y_settings_result.value());

    check(
        state,
        intermediate_initial_ground_result.has_value() &&
            !intermediate_initial_ground_result.
                value().
                has_value(),
        "Intermediate-blocker fixture starts unsupported");

    const auto intermediate_snap_result =
        snap_world_capsule_to_ground(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            intermediate_start_result.value(),
            y_settings_result.value(),
            0.1,
            4U,
            4U);

    check(
        state,
        intermediate_snap_result.has_value(),
        "Intermediate-blocker ground snap resolves");

    check(
        state,
        intermediate_snap_result.has_value() &&
            intermediate_snap_result.value() ==
                intermediate_start_result.value(),
        "Non-walkable intermediate contact blocks ground snap without lateral steering");

    const auto penetration_snap_result =
        snap_world_capsule_to_ground(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            penetration_start_result.value(),
            y_settings_result.value(),
            0.1,
            4U,
            4U);

    check(
        state,
        penetration_snap_result.has_value(),
        "Ground snap recovers initial penetration");

    check(
        state,
        penetration_snap_result.has_value() &&
            nearly_equal(
                penetration_snap_result.
                    value().
                    local().
                    y,
                expected_penetration_result.
                    value().
                    local().
                    y),
        "Initial penetration resolves to supported position");

    const auto penetration_ground_result =
        penetration_snap_result.has_value()
            ? query_world_capsule_ground_contact(
                  registry,
                  world_namespace,
                  query_collider,
                  capsule_shape_result.value(),
                  penetration_snap_result.value(),
                  y_settings_result.value())
            : decltype(
                  query_world_capsule_ground_contact(
                      registry,
                      world_namespace,
                      query_collider,
                      capsule_shape_result.value(),
                      penetration_start_result.value(),
                      y_settings_result.value())){
                  oros::foundation::fail(
                      ErrorCode::internal_failure,
                      "Penetration snap did not produce a position.")
              };

    check(
        state,
        penetration_ground_result.has_value() &&
            penetration_ground_result.
                value().
                has_value() &&
            penetration_ground_result.
                value()->
                pair().
                contains(
                    penetrating_floor_id),
        "Recovered penetration remains grounded");

    const auto budget_result =
        snap_world_capsule_to_ground(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            near_start_result.value(),
            y_settings_result.value(),
            0.1,
            2U,
            4U);

    check(
        state,
        !budget_result.has_value() &&
            budget_result.
                error().
                code ==
            ErrorCode::invalid_state,
        "Ground snap propagates deterministic substep budget exhaustion");

    return finish(state);
}