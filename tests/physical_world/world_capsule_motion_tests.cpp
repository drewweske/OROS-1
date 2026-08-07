#include "oros/physical_world/world_capsule_motion.hpp"

#include "oros/foundation/error.hpp"
#include "oros/physical_world/world_capsule_contact_query.hpp"
#include "oros/physical_world/world_cell_collider_payload.hpp"
#include "oros/physical_world/world_cell_collider_set.hpp"
#include "oros/physics/collider_geometry.hpp"
#include "oros/physics/collider_id.hpp"
#include "oros/physics/collision_contact.hpp"
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
#include <vector>

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
            << "\nWorld capsule motion test "
            << "summary: "
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
                std::move(
                    snapshot)).
                has_value();
    }

    [[nodiscard]]
    bool has_positive_penetration(
        const std::vector<
            oros::physics::CollisionContact>&
            contacts)
    {
        for (const oros::physics::
                 CollisionContact&
                 contact :
             contacts)
        {
            if (contact.penetration_depth() >
                0.0)
            {
                return true;
            }
        }

        return false;
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

    const ColliderId wall_collider{
        EntityId{
            world_namespace,
            100ULL
        },
        1U
    };

    const auto capsule_shape_result =
        CapsuleShape::create(
            1.0,
            1.0,
            PhysicsUnitVector3::
                positive_y());

    const auto wall_shape_result =
        BoxShape::create(
            PhysicsVector3{
                0.5,
                10.0,
                10.0
            });

    check(
        state,
        capsule_shape_result.has_value(),
        "Motion capsule shape is created");

    check(
        state,
        wall_shape_result.has_value(),
        "Motion wall shape is created");

    if (!capsule_shape_result.has_value() ||
        !wall_shape_result.has_value())
    {
        return finish(state);
    }

    const auto origin_result =
        WorldPosition::origin();

    check(
        state,
        origin_result.has_value(),
        "Motion world origin is created");

    if (!origin_result.has_value())
    {
        return finish(state);
    }

    WorldCellColliderRegistry empty_registry{};

    const auto non_finite_displacement_result =
        move_world_capsule(
            empty_registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            origin_result.value(),
            WorldDisplacement{
                std::numeric_limits<double>::
                    quiet_NaN(),
                0.0,
                0.0
            },
            0.25,
            16U,
            4U);

    check(
        state,
        !non_finite_displacement_result.
                has_value() &&
            non_finite_displacement_result.
                error().
                code ==
            ErrorCode::invalid_argument,
        "Motion rejects non-finite displacement");

    const auto zero_substep_distance_result =
        move_world_capsule(
            empty_registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            origin_result.value(),
            WorldDisplacement{},
            0.0,
            16U,
            4U);

    check(
        state,
        !zero_substep_distance_result.
                has_value() &&
            zero_substep_distance_result.
                error().
                code ==
            ErrorCode::invalid_argument,
        "Motion rejects zero maximum substep distance");

    const auto infinite_substep_distance_result =
        move_world_capsule(
            empty_registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            origin_result.value(),
            WorldDisplacement{},
            std::numeric_limits<double>::
                infinity(),
            16U,
            4U);

    check(
        state,
        !infinite_substep_distance_result.
                has_value() &&
            infinite_substep_distance_result.
                error().
                code ==
            ErrorCode::invalid_argument,
        "Motion rejects non-finite maximum substep distance");

    const auto zero_substeps_result =
        move_world_capsule(
            empty_registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            origin_result.value(),
            WorldDisplacement{},
            0.25,
            0U,
            4U);

    check(
        state,
        !zero_substeps_result.has_value() &&
            zero_substeps_result.
                error().
                code ==
            ErrorCode::invalid_argument,
        "Motion rejects zero maximum substeps");

    const auto zero_depenetration_result =
        move_world_capsule(
            empty_registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            origin_result.value(),
            WorldDisplacement{},
            0.25,
            16U,
            0U);

    check(
        state,
        !zero_depenetration_result.
                has_value() &&
            zero_depenetration_result.
                error().
                code ==
            ErrorCode::invalid_argument,
        "Motion rejects zero depenetration iterations");

    const auto zero_motion_result =
        move_world_capsule(
            empty_registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            origin_result.value(),
            WorldDisplacement{},
            0.25,
            16U,
            4U);

    check(
        state,
        zero_motion_result.has_value(),
        "Zero motion succeeds");

    check(
        state,
        zero_motion_result.has_value() &&
            zero_motion_result.value() ==
                origin_result.value(),
        "Zero motion preserves position");

    const auto empty_motion_result =
        move_world_capsule(
            empty_registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            origin_result.value(),
            WorldDisplacement{
                1.0,
                0.0,
                0.0
            },
            0.25,
            4U,
            4U);

    check(
        state,
        empty_motion_result.has_value(),
        "Empty-world segmented motion succeeds");

    const auto expected_empty_motion_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                1.0,
                0.0,
                0.0
            });

    check(
        state,
        expected_empty_motion_result.has_value(),
        "Expected empty-world motion position is created");

    check(
        state,
        empty_motion_result.has_value() &&
            expected_empty_motion_result.
                has_value() &&
            empty_motion_result.value() ==
                expected_empty_motion_result.value(),
        "Empty-world motion applies complete displacement");

    const auto budget_exhaustion_result =
        move_world_capsule(
            empty_registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            origin_result.value(),
            WorldDisplacement{
                2.0,
                0.0,
                0.0
            },
            0.5,
            3U,
            4U);

    check(
        state,
        !budget_exhaustion_result.
                has_value() &&
            budget_exhaustion_result.
                error().
                code ==
            ErrorCode::invalid_state,
        "Motion reports deterministic substep budget exhaustion");

    const auto boundary_start_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                511.75,
                0.0,
                0.0
            });

    const auto boundary_expected_result =
        WorldPosition::create(
            WorldCell{
                1,
                0,
                0
            },
            LocalPosition{
                -511.25,
                0.0,
                0.0
            });

    check(
        state,
        boundary_start_result.has_value() &&
            boundary_expected_result.has_value(),
        "Cross-cell motion fixtures are created");

    if (!boundary_start_result.has_value() ||
        !boundary_expected_result.has_value())
    {
        return finish(state);
    }

    const auto boundary_motion_result =
        move_world_capsule(
            empty_registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            boundary_start_result.value(),
            WorldDisplacement{
                1.0,
                0.0,
                0.0
            },
            0.25,
            4U,
            4U);

    check(
        state,
        boundary_motion_result.has_value(),
        "Cross-cell segmented motion succeeds");

    check(
        state,
        boundary_motion_result.has_value() &&
            boundary_motion_result.value() ==
                boundary_expected_result.value(),
        "Cross-cell motion normalizes the final world position");

    const WorldCellKey wall_cell{
        world_namespace,
        WorldCell{}
    };

    const auto wall_geometry_result =
        ColliderGeometry::create(
            wall_collider,
            wall_shape_result.value(),
            PhysicsVector3{
                2.0,
                0.0,
                0.0
            });

    check(
        state,
        wall_geometry_result.has_value(),
        "Motion wall geometry is created");

    if (!wall_geometry_result.has_value())
    {
        return finish(state);
    }

    const std::array<
        ColliderGeometry,
        1U>
        wall_colliders{
            wall_geometry_result.value()
        };

    const auto wall_set_result =
        WorldCellColliderSet::create(
            wall_cell,
            std::span<
                const ColliderGeometry>{
                    wall_colliders
                });

    check(
        state,
        wall_set_result.has_value(),
        "Motion wall collider set is created");

    if (!wall_set_result.has_value())
    {
        return finish(state);
    }

    const auto wall_payload_result =
        serialize_world_cell_collider_payload(
            wall_set_result.value());

    check(
        state,
        wall_payload_result.has_value(),
        "Motion wall payload serializes");

    if (!wall_payload_result.has_value())
    {
        return finish(state);
    }

    const auto wall_snapshot_result =
        WorldCellSnapshot::create(
            wall_cell,
            1ULL,
            std::span<const std::byte>{
                wall_payload_result.value()
            });

    check(
        state,
        wall_snapshot_result.has_value(),
        "Motion wall snapshot is created");

    if (!wall_snapshot_result.has_value())
    {
        return finish(state);
    }

    auto wall_residency_result =
        WorldCellResidency::create(
            wall_cell);

    check(
        state,
        wall_residency_result.has_value(),
        "Motion wall residency is created");

    if (!wall_residency_result.has_value())
    {
        return finish(state);
    }

    WorldCellResidency wall_residency =
        std::move(
            wall_residency_result.value());

    check(
        state,
        make_resident(
            wall_residency,
            wall_snapshot_result.value(),
            101ULL),
        "Motion wall cell becomes resident");

    WorldCellColliderRegistry wall_registry{};

    check(
        state,
        wall_registry.synchronize(
            wall_residency).
            has_value(),
        "Motion wall collider activates");

    check(
        state,
        wall_registry.is_valid() &&
            wall_registry.collider_count() ==
                1U,
        "Motion wall registry is valid");

    const auto blocked_motion_result =
        move_world_capsule(
            wall_registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            origin_result.value(),
            WorldDisplacement{
                2.0,
                0.0,
                0.0
            },
            0.25,
            8U,
            4U);

    check(
        state,
        blocked_motion_result.has_value(),
        "Segmented wall motion succeeds");

    if (!blocked_motion_result.has_value())
    {
        return finish(state);
    }

    const auto blocked_displacement_result =
        origin_result.
            value().
            displacement_to(
                blocked_motion_result.value());

    check(
        state,
        blocked_displacement_result.has_value(),
        "Blocked motion displacement can be measured");

    check(
        state,
        blocked_displacement_result.has_value() &&
            nearly_equal(
                blocked_displacement_result.
                    value().
                    x,
                0.5) &&
            nearly_equal(
                blocked_displacement_result.
                    value().
                    y,
                0.0) &&
            nearly_equal(
                blocked_displacement_result.
                    value().
                    z,
                0.0),
        "Small deterministic substeps stop the capsule at the wall");

    const auto blocked_contacts_result =
        query_world_capsule_contacts(
            wall_registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            blocked_motion_result.value());

    check(
        state,
        blocked_contacts_result.has_value() &&
            !has_positive_penetration(
                blocked_contacts_result.value()),
        "Wall-blocked capsule finishes without positive penetration");

    const auto sliding_motion_result =
        move_world_capsule(
            wall_registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            origin_result.value(),
            WorldDisplacement{
                2.0,
                0.0,
                2.0
            },
            0.25,
            16U,
            4U);

    check(
        state,
        sliding_motion_result.has_value(),
        "Diagonal segmented wall motion succeeds");

    if (!sliding_motion_result.has_value())
    {
        return finish(state);
    }

    const auto sliding_displacement_result =
        origin_result.
            value().
            displacement_to(
                sliding_motion_result.value());

    check(
        state,
        sliding_displacement_result.has_value(),
        "Sliding displacement can be measured");

    check(
        state,
        sliding_displacement_result.has_value() &&
            nearly_equal(
                sliding_displacement_result.
                    value().
                    x,
                0.5) &&
            nearly_equal(
                sliding_displacement_result.
                    value().
                    z,
                2.0),
        "Wall correction preserves tangential motion");

    const auto sliding_contacts_result =
        query_world_capsule_contacts(
            wall_registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            sliding_motion_result.value());

    check(
        state,
        sliding_contacts_result.has_value() &&
            !has_positive_penetration(
                sliding_contacts_result.value()),
        "Sliding capsule finishes without positive penetration");

    const auto overlapping_start_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                1.0,
                0.0,
                0.0
            });

    check(
        state,
        overlapping_start_result.has_value(),
        "Overlapping motion start position is created");

    if (!overlapping_start_result.has_value())
    {
        return finish(state);
    }

    const auto initial_recovery_result =
        move_world_capsule(
            wall_registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            overlapping_start_result.value(),
            WorldDisplacement{},
            0.25,
            8U,
            4U);

    check(
        state,
        initial_recovery_result.has_value(),
        "Motion resolves initial penetration before moving");

    if (!initial_recovery_result.has_value())
    {
        return finish(state);
    }

    const auto recovery_displacement_result =
        origin_result.
            value().
            displacement_to(
                initial_recovery_result.value());

    check(
        state,
        recovery_displacement_result.has_value() &&
            nearly_equal(
                recovery_displacement_result.
                    value().
                    x,
                0.5),
        "Zero desired motion still performs initial depenetration");

    const auto recovery_contacts_result =
        query_world_capsule_contacts(
            wall_registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            initial_recovery_result.value());

    check(
        state,
        recovery_contacts_result.has_value() &&
            !has_positive_penetration(
                recovery_contacts_result.value()),
        "Initial recovery finishes without positive penetration");

    return finish(state);
}