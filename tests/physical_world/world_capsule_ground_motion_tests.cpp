#include "oros/physical_world/world_capsule_ground_motion.hpp"

#include "oros/foundation/error.hpp"

#include "oros/physical_world/world_capsule_ground_query.hpp"
#include "oros/physical_world/world_capsule_ground_snap.hpp"
#include "oros/physical_world/world_capsule_motion.hpp"
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
#include <limits>
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
            << "\nWorld capsule ground motion "
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

    const ColliderId floor_id{
        EntityId{
            world_namespace,
            100ULL
        },
        1U
    };

    const ColliderId slope_support_id{
        EntityId{
            world_namespace,
            200ULL
        },
        1U
    };

    const ColliderId x_support_id{
        EntityId{
            world_namespace,
            300ULL
        },
        1U
    };

    const ColliderId transition_floor_id{
        EntityId{
            world_namespace,
            400ULL
        },
        1U
    };

    const ColliderId steep_obstacle_id{
        EntityId{
            world_namespace,
            500ULL
        },
        1U
    };

    const ColliderId guard_wall_id{
        EntityId{
            world_namespace,
            600ULL
        },
        1U
    };

    const ColliderId guard_floor_id{
        EntityId{
            world_namespace,
            650ULL
        },
        1U
    };

    const ColliderId ledge_floor_id{
        EntityId{
            world_namespace,
            700ULL
        },
        1U
    };

    const ColliderId x_steep_obstacle_id{
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
                5.0,
                0.5,
                5.0
            });

    const auto slope_shape_result =
        SphereShape::create(
            1.0);

    const auto wall_shape_result =
        BoxShape::create(
            PhysicsVector3{
                0.5,
                5.0,
                5.0
            });

    check(
        state,
        capsule_shape_result.has_value(),
        "Ground-motion capsule shape is created");

    check(
        state,
        floor_shape_result.has_value(),
        "Ground-motion floor shape is created");

    check(
        state,
        slope_shape_result.has_value(),
        "Ground-motion slope support shape is created");

    check(
        state,
        wall_shape_result.has_value(),
        "Ground-motion wall shape is created");

    if (!capsule_shape_result.has_value() ||
        !floor_shape_result.has_value() ||
        !slope_shape_result.has_value() ||
        !wall_shape_result.has_value())
    {
        return finish(state);
    }

    const auto y_settings_result =
        WorldCapsuleTraversalSettings::create(
            PhysicsUnitVector3::positive_y(),
            0.70,
            0.5,
            0.25);

    const auto x_settings_result =
        WorldCapsuleTraversalSettings::create(
            PhysicsUnitVector3::positive_x(),
            1.0,
            0.5,
            0.25);

    check(
        state,
        y_settings_result.has_value(),
        "Positive-Y ground-motion settings are created");

    check(
        state,
        x_settings_result.has_value(),
        "Positive-X ground-motion settings are created");

    if (!y_settings_result.has_value() ||
        !x_settings_result.has_value())
    {
        return finish(state);
    }

    const auto flat_start_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                0.0,
                0.0,
                0.0
            });

    const auto slope_start_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                20.0,
                0.0,
                0.0
            });

    const auto x_support_start_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                40.0,
                0.0,
                0.0
            });

    const auto unsupported_start_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                120.0,
                0.0,
                0.0
            });

    const auto steep_transition_start_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                58.0,
                0.0,
                0.0
            });

    const auto guard_wall_start_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                78.0,
                0.0,
                0.0
            });

    const auto ledge_start_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                98.0,
                0.0,
                0.0
            });

    check(
        state,
        flat_start_result.has_value() &&
            slope_start_result.has_value() &&
            x_support_start_result.has_value() &&
            unsupported_start_result.has_value() &&
            steep_transition_start_result.has_value() &&
            guard_wall_start_result.has_value() &&
            ledge_start_result.has_value(),
        "Ground-motion world positions are created");

    if (!flat_start_result.has_value() ||
        !slope_start_result.has_value() ||
        !x_support_start_result.has_value() ||
        !unsupported_start_result.has_value() ||
        !steep_transition_start_result.has_value() ||
        !guard_wall_start_result.has_value() ||
        !ledge_start_result.has_value())
    {
        return finish(state);
    }

    const auto floor_geometry_result =
        ColliderGeometry::create(
            floor_id,
            floor_shape_result.value(),
            PhysicsVector3{
                0.0,
                -2.5,
                0.0
            });

    const auto slope_geometry_result =
        ColliderGeometry::create(
            slope_support_id,
            slope_shape_result.value(),
            PhysicsVector3{
                21.2,
                -2.2,
                0.0
            });

    const auto x_support_geometry_result =
        ColliderGeometry::create(
            x_support_id,
            wall_shape_result.value(),
            PhysicsVector3{
                38.5,
                0.0,
                0.0
            });

    const auto transition_floor_geometry_result =
        ColliderGeometry::create(
            transition_floor_id,
            floor_shape_result.value(),
            PhysicsVector3{
                60.0,
                -2.5,
                0.0
            });

    const auto steep_obstacle_geometry_result =
        ColliderGeometry::create(
            steep_obstacle_id,
            slope_shape_result.value(),
            PhysicsVector3{
                61.9,
                -1.7,
                0.0
            });

    const auto guard_wall_geometry_result =
        ColliderGeometry::create(
            guard_wall_id,
            wall_shape_result.value(),
            PhysicsVector3{
                80.5,
                0.0,
                0.0
            });

    const auto guard_floor_geometry_result =
        ColliderGeometry::create(
            guard_floor_id,
            floor_shape_result.value(),
            PhysicsVector3{
                80.0,
                -2.5,
                0.0
            });

    const auto ledge_floor_geometry_result =
        ColliderGeometry::create(
            ledge_floor_id,
            floor_shape_result.value(),
            PhysicsVector3{
                96.0,
                -2.5,
                0.0
            });

    const auto x_steep_obstacle_geometry_result =
        ColliderGeometry::create(
            x_steep_obstacle_id,
            slope_shape_result.value(),
            PhysicsVector3{
                38.7,
                3.9,
                0.0
            });

    check(
        state,
        floor_geometry_result.has_value(),
        "Ground-motion floor geometry is created");

    check(
        state,
        slope_geometry_result.has_value(),
        "Ground-motion slope geometry is created");

    check(
        state,
        x_support_geometry_result.has_value(),
        "Ground-motion positive-X support is created");

    check(
        state,
        transition_floor_geometry_result.has_value(),
        "Steep-transition floor geometry is created");

    check(
        state,
        steep_obstacle_geometry_result.has_value(),
        "Steep-transition obstacle geometry is created");

    check(
        state,
        guard_wall_geometry_result.has_value(),
        "Steep-guard vertical wall geometry is created");

    check(
        state,
        guard_floor_geometry_result.has_value(),
        "Steep-guard vertical wall floor geometry is created");

    check(
        state,
        ledge_floor_geometry_result.has_value(),
        "Steep-guard ledge floor geometry is created");

    check(
        state,
        x_steep_obstacle_geometry_result.has_value(),
        "Positive-X-up steep obstacle geometry is created");

    if (!floor_geometry_result.has_value() ||
        !slope_geometry_result.has_value() ||
        !x_support_geometry_result.has_value() ||
        !transition_floor_geometry_result.has_value() ||
        !steep_obstacle_geometry_result.has_value() ||
        !guard_wall_geometry_result.has_value() ||
        !guard_floor_geometry_result.has_value() ||
        !ledge_floor_geometry_result.has_value() ||
        !x_steep_obstacle_geometry_result.has_value())
    {
        return finish(state);
    }

    const WorldCellKey cell_key{
        world_namespace,
        WorldCell{}
    };

    const std::array<
        ColliderGeometry,
        9U>
        colliders{
            floor_geometry_result.value(),
            slope_geometry_result.value(),
            x_support_geometry_result.value(),
            transition_floor_geometry_result.value(),
            steep_obstacle_geometry_result.value(),
            guard_wall_geometry_result.value(),
            guard_floor_geometry_result.value(),
            ledge_floor_geometry_result.value(),
            x_steep_obstacle_geometry_result.value()
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
        "Ground-motion collider set is created");

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
        "Ground-motion collider payload serializes");

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
        "Ground-motion world snapshot is created");

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
        "Ground-motion residency is created");

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
        "Ground-motion cell becomes resident");

    WorldCellColliderRegistry registry{};

    check(
        state,
        registry.synchronize(
            residency).
            has_value(),
        "Ground-motion colliders activate");

    check(
        state,
        registry.is_valid() &&
            registry.collider_count() ==
                9U,
        "Ground-motion registry is valid");

    const auto non_finite_result =
        move_world_capsule_along_ground(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            flat_start_result.value(),
            WorldDisplacement{
                std::numeric_limits<double>::
                    quiet_NaN(),
                0.0,
                0.0
            },
            y_settings_result.value(),
            0.1,
            16U,
            4U);

    check(
        state,
        !non_finite_result.has_value() &&
            non_finite_result.
                error().
                code ==
            ErrorCode::invalid_argument,
        "Ground motion rejects non-finite displacement");

    const auto unsupported_result =
        move_world_capsule_along_ground(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            unsupported_start_result.value(),
            WorldDisplacement{
                0.5,
                0.0,
                0.0
            },
            y_settings_result.value(),
            0.1,
            16U,
            4U);

    check(
        state,
        !unsupported_result.has_value() &&
            unsupported_result.
                error().
                code ==
            ErrorCode::invalid_state,
        "Ground motion requires initial walkable support");

    const auto flat_motion_result =
        move_world_capsule_along_ground(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            flat_start_result.value(),
            WorldDisplacement{
                0.5,
                0.0,
                0.0
            },
            y_settings_result.value(),
            0.1,
            16U,
            4U);

    check(
        state,
        flat_motion_result.has_value(),
        "Flat ground motion succeeds");

    if (!flat_motion_result.has_value())
    {
        return finish(state);
    }

    const auto flat_displacement_result =
        flat_start_result.
            value().
            displacement_to(
                flat_motion_result.value());

    check(
        state,
        flat_displacement_result.has_value() &&
            nearly_equal(
                flat_displacement_result.
                    value().
                    x,
                0.5) &&
            nearly_equal(
                flat_displacement_result.
                    value().
                    y,
                0.0) &&
            nearly_equal(
                flat_displacement_result.
                    value().
                    z,
                0.0),
        "Flat support preserves planar displacement");

    const auto flat_ground_result =
        query_world_capsule_ground_contact(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            flat_motion_result.value(),
            y_settings_result.value());

    check(
        state,
        flat_ground_result.has_value() &&
            flat_ground_result.
                value().
                has_value() &&
            flat_ground_result.
                value()->
                pair().
                contains(
                    floor_id),
        "Flat motion remains on walkable ground");

    const auto slope_initial_ground_result =
        query_world_capsule_ground_contact(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            slope_start_result.value(),
            y_settings_result.value());

    check(
        state,
        slope_initial_ground_result.has_value() &&
            slope_initial_ground_result.
                value().
                has_value() &&
            slope_initial_ground_result.
                value()->
                pair().
                contains(
                    slope_support_id),
        "Slope start initially has walkable support");

    const auto slope_zero_motion_result =
        move_world_capsule(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            slope_start_result.value(),
            WorldDisplacement{},
            0.1,
            16U,
            4U);

    check(
        state,
        slope_zero_motion_result.has_value(),
        "Slope zero-motion depenetration succeeds");

    if (!slope_zero_motion_result.has_value())
    {
        return finish(state);
    }

    const auto slope_after_depenetration_ground_result =
        query_world_capsule_ground_contact(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            slope_zero_motion_result.value(),
            y_settings_result.value());

    check(
        state,
        slope_after_depenetration_ground_result.
                has_value() &&
            slope_after_depenetration_ground_result.
                value().
                has_value() &&
            slope_after_depenetration_ground_result.
                value()->
                pair().
                contains(
                    slope_support_id),
        "Slope remains grounded after zero-motion depenetration");

    const auto slope_direct_down_result =
        move_world_capsule(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            slope_zero_motion_result.value(),
            WorldDisplacement{
                0.0,
                -0.25,
                0.0
            },
            0.1,
            16U,
            4U);

    check(
        state,
        slope_direct_down_result.has_value(),
        "Direct downward movement from depenetrated slope succeeds");

    const auto slope_direct_snap_result =
        snap_world_capsule_to_ground(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            slope_start_result.value(),
            y_settings_result.value(),
            0.1,
            16U,
            4U);

    check(
        state,
        slope_direct_snap_result.has_value(),
        "Direct slope ground snap succeeds");

    if (!slope_direct_snap_result.has_value())
    {
        return finish(state);
    }

    const auto slope_after_snap_ground_result =
        query_world_capsule_ground_contact(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            slope_direct_snap_result.value(),
            y_settings_result.value());

    check(
        state,
        slope_after_snap_ground_result.has_value() &&
            slope_after_snap_ground_result.
                value().
                has_value() &&
            slope_after_snap_ground_result.
                value()->
                pair().
                contains(
                    slope_support_id),
        "Direct slope ground snap ends on walkable support");

    const auto slope_baseline_result =
        move_world_capsule_along_ground(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            slope_start_result.value(),
            WorldDisplacement{},
            y_settings_result.value(),
            0.1,
            16U,
            4U);

    check(
        state,
        slope_baseline_result.has_value(),
        "Slope baseline resolves onto walkable support");

    if (!slope_baseline_result.has_value())
    {
        return finish(state);
    }

    const auto slope_baseline_ground_result =
        query_world_capsule_ground_contact(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            slope_baseline_result.value(),
            y_settings_result.value());

    check(
        state,
        slope_baseline_ground_result.has_value() &&
            slope_baseline_ground_result.
                value().
                has_value() &&
            slope_baseline_ground_result.
                value()->
                pair().
                contains(
                    slope_support_id),
        "Resolved slope baseline is grounded");

    const auto slope_motion_result =
        move_world_capsule_along_ground(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            slope_baseline_result.value(),
            WorldDisplacement{
                0.5,
                0.0,
                0.0
            },
            y_settings_result.value(),
            0.1,
            16U,
            4U);

    check(
        state,
        slope_motion_result.has_value(),
        "Walkable slope motion succeeds");

    if (!slope_motion_result.has_value())
    {
        return finish(state);
    }

    const auto slope_displacement_result =
        slope_baseline_result.
            value().
            displacement_to(
                slope_motion_result.value());

    check(
        state,
        slope_displacement_result.has_value(),
        "Slope displacement can be measured");

    check(
        state,
        slope_displacement_result.has_value() &&
            slope_displacement_result.
                value().
                x >
                0.0 &&
            slope_displacement_result.
                value().
                y >
                0.0 &&
            nearly_equal(
                slope_displacement_result.
                    value().
                    z,
                0.0),
        "Walkable curved support produces forward uphill motion without lateral drift");
    const auto slope_ground_result =
        query_world_capsule_ground_contact(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            slope_motion_result.value(),
            y_settings_result.value());

    check(
        state,
        slope_ground_result.has_value() &&
            slope_ground_result.
                value().
                has_value() &&
            slope_ground_result.
                value()->
                pair().
                contains(
                    slope_support_id),
        "Slope motion reacquires walkable support");

    const auto x_motion_result =
        move_world_capsule_along_ground(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            x_support_start_result.value(),
            WorldDisplacement{
                0.0,
                0.5,
                0.0
            },
            x_settings_result.value(),
            0.1,
            16U,
            4U);

    check(
        state,
        x_motion_result.has_value(),
        "Positive-X-up ground motion succeeds");

    if (!x_motion_result.has_value())
    {
        return finish(state);
    }

    const auto x_displacement_result =
        x_support_start_result.
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
                0.0) &&
            nearly_equal(
                x_displacement_result.
                    value().
                    y,
                0.5) &&
            nearly_equal(
                x_displacement_result.
                    value().
                    z,
                0.0),
        "Ground motion follows explicit positive-X up direction");

    const auto x_ground_result =
        query_world_capsule_ground_contact(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            x_motion_result.value(),
            x_settings_result.value());

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
                    x_support_id),
        "Positive-X traversal remains grounded");

    const auto x_steep_initial_ground_result =
        query_world_capsule_ground_contact(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            x_support_start_result.value(),
            x_settings_result.value());

    check(
        state,
        x_steep_initial_ground_result.has_value() &&
            x_steep_initial_ground_result.
                value().
                has_value() &&
            x_steep_initial_ground_result.
                value()->
                pair().
                contains(
                    x_support_id),
        "Positive-X steep transition starts on walkable support");

    const auto x_steep_result =
        move_world_capsule_along_ground(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            x_support_start_result.value(),
            WorldDisplacement{
                0.0,
                3.0,
                0.0
            },
            x_settings_result.value(),
            0.1,
            64U,
            4U);

    check(
        state,
        x_steep_result.has_value(),
        "Positive-X steep-transition grounded motion succeeds");

    if (!x_steep_result.has_value())
    {
        return finish(state);
    }

    const auto x_steep_displacement_result =
        x_support_start_result.
            value().
            displacement_to(
                x_steep_result.value());

    check(
        state,
        x_steep_displacement_result.has_value() &&
            nearly_equal(
                x_steep_displacement_result.
                    value().
                    x,
                0.0) &&
            x_steep_displacement_result.
                value().
                y >
                0.0 &&
            x_steep_displacement_result.
                value().
                y <
                3.0 &&
            nearly_equal(
                x_steep_displacement_result.
                    value().
                    z,
                0.0),
        "Steep guard follows explicit positive-X up direction");

    const auto x_steep_ground_result =
        query_world_capsule_ground_contact(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            x_steep_result.value(),
            x_settings_result.value());

    check(
        state,
        x_steep_ground_result.has_value() &&
            x_steep_ground_result.
                value().
                has_value() &&
            x_steep_ground_result.
                value()->
                pair().
                contains(
                    x_support_id),
        "Positive-X steep block preserves walkable support");

    const auto steep_transition_initial_ground_result =
        query_world_capsule_ground_contact(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            steep_transition_start_result.value(),
            y_settings_result.value());

    check(
        state,
        steep_transition_initial_ground_result.
                has_value() &&
            steep_transition_initial_ground_result.
                value().
                has_value() &&
            steep_transition_initial_ground_result.
                value()->
                pair().
                contains(
                    transition_floor_id),
        "Steep-transition start is grounded on its flat floor");

    const auto steep_transition_result =
        move_world_capsule_along_ground(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            steep_transition_start_result.value(),
            WorldDisplacement{
                3.0,
                0.0,
                0.0
            },
            y_settings_result.value(),
            0.1,
            64U,
            4U);

    check(
        state,
        steep_transition_result.has_value(),
        "Steep-transition grounded motion succeeds");

    if (!steep_transition_result.has_value())
    {
        return finish(state);
    }

    const auto steep_transition_displacement_result =
        steep_transition_start_result.
            value().
            displacement_to(
                steep_transition_result.value());

    check(
        state,
        steep_transition_displacement_result.
            has_value(),
        "Steep-transition displacement can be measured");

    check(
        state,
        steep_transition_displacement_result.
                has_value() &&
            steep_transition_displacement_result.
                value().
                x >
                0.0 &&
            steep_transition_displacement_result.
                value().
                x <
                3.0 &&
            nearly_equal(
                steep_transition_displacement_result.
                    value().
                    y,
                0.0) &&
            nearly_equal(
                steep_transition_displacement_result.
                    value().
                    z,
                0.0),
        "Non-walkable steep surface blocks upward grounded climbing");

    const auto steep_transition_ground_result =
        query_world_capsule_ground_contact(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            steep_transition_result.value(),
            y_settings_result.value());

    check(
        state,
        steep_transition_ground_result.
                has_value() &&
            steep_transition_ground_result.
                value().
                has_value() &&
            steep_transition_ground_result.
                value()->
                pair().
                contains(
                    transition_floor_id),
        "Blocked steep transition remains grounded on the flat floor");

    const auto guard_wall_result =
        move_world_capsule_along_ground(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            guard_wall_start_result.value(),
            WorldDisplacement{
                3.0,
                0.0,
                0.0
            },
            y_settings_result.value(),
            0.1,
            64U,
            4U);

    check(
        state,
        guard_wall_result.has_value(),
        "Vertical-wall grounded motion succeeds");

    if (!guard_wall_result.has_value())
    {
        return finish(state);
    }

    const auto guard_wall_displacement_result =
        guard_wall_start_result.
            value().
            displacement_to(
                guard_wall_result.value());

    check(
        state,
        guard_wall_displacement_result.has_value() &&
            guard_wall_displacement_result.
                value().
                x >
                0.0 &&
            guard_wall_displacement_result.
                value().
                x <
                3.0 &&
            nearly_equal(
                guard_wall_displacement_result.
                    value().
                    y,
                0.0) &&
            nearly_equal(
                guard_wall_displacement_result.
                    value().
                    z,
                0.0),
        "Vertical wall blocks horizontal motion without steep-slope climbing");

    const auto guard_wall_ground_result =
        query_world_capsule_ground_contact(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            guard_wall_result.value(),
            y_settings_result.value());

    check(
        state,
        guard_wall_ground_result.has_value() &&
            guard_wall_ground_result.
                value().
                has_value() &&
            guard_wall_ground_result.
                value()->
                pair().
                contains(
                    guard_floor_id),
        "Vertical-wall blocking preserves walkable support");

    const auto ledge_initial_ground_result =
        query_world_capsule_ground_contact(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            ledge_start_result.value(),
            y_settings_result.value());

    check(
        state,
        ledge_initial_ground_result.has_value() &&
            ledge_initial_ground_result.
                value().
                has_value() &&
            ledge_initial_ground_result.
                value()->
                pair().
                contains(
                    ledge_floor_id),
        "Ledge departure starts grounded");

    const auto ledge_result =
        move_world_capsule_along_ground(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            ledge_start_result.value(),
            WorldDisplacement{
                5.0,
                0.0,
                0.0
            },
            y_settings_result.value(),
            0.1,
            64U,
            4U);

    check(
        state,
        ledge_result.has_value(),
        "Ledge departure grounded motion succeeds");

    if (!ledge_result.has_value())
    {
        return finish(state);
    }

    const auto ledge_displacement_result =
        ledge_start_result.
            value().
            displacement_to(
                ledge_result.value());

    check(
        state,
        ledge_displacement_result.has_value() &&
            nearly_equal(
                ledge_displacement_result.
                    value().
                    x,
                5.0) &&
            nearly_equal(
                ledge_displacement_result.
                    value().
                    y,
                0.0) &&
            nearly_equal(
                ledge_displacement_result.
                    value().
                    z,
                0.0),
        "Steep guard does not block unsupported ledge departure");

    const auto ledge_ground_result =
        query_world_capsule_ground_contact(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            ledge_result.value(),
            y_settings_result.value());

    check(
        state,
        ledge_ground_result.has_value() &&
            !ledge_ground_result.
                value().
                has_value(),
        "Ledge departure may end without walkable support");

    const auto budget_result =
        move_world_capsule_along_ground(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            flat_start_result.value(),
            WorldDisplacement{
                0.5,
                0.0,
                0.0
            },
            y_settings_result.value(),
            0.1,
            4U,
            4U);

    check(
        state,
        !budget_result.has_value() &&
            budget_result.
                error().
                code ==
            ErrorCode::invalid_state,
        "Ground motion propagates deterministic substep budget exhaustion");

    return finish(state);
}