#include "oros/physical_world/world_capsule_depenetration.hpp"

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
            << "\nWorld capsule depenetration test "
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

    const WorldCellKey origin_cell{
        world_namespace,
        WorldCell{
            0,
            0,
            0
        }
    };

    const ColliderId single_collider{
        EntityId{
            world_namespace,
            100ULL
        },
        1U
    };

    const ColliderId multiple_near_collider{
        EntityId{
            world_namespace,
            200ULL
        },
        1U
    };

    const ColliderId multiple_far_collider{
        EntityId{
            world_namespace,
            300ULL
        },
        1U
    };

    const ColliderId trapped_right_collider{
        EntityId{
            world_namespace,
            400ULL
        },
        1U
    };

    const ColliderId trapped_left_collider{
        EntityId{
            world_namespace,
            500ULL
        },
        1U
    };

    const ColliderId boundary_collider{
        EntityId{
            world_namespace,
            600ULL
        },
        1U
    };

    const ColliderId query_after_world_ids{
        EntityId{
            world_namespace,
            900ULL
        },
        1U
    };

    const ColliderId query_before_world_ids{
        EntityId{
            world_namespace,
            50ULL
        },
        1U
    };

    const auto capsule_shape_result =
        CapsuleShape::create(
            1.0,
            1.0,
            PhysicsUnitVector3::
                positive_y());

    const auto sphere_shape_result =
        SphereShape::create(
            1.0);

    check(
        state,
        capsule_shape_result.has_value(),
        "Depenetration capsule shape is created");

    check(
        state,
        sphere_shape_result.has_value(),
        "Depenetration sphere shape is created");

    if (!capsule_shape_result.has_value() ||
        !sphere_shape_result.has_value())
    {
        return finish(state);
    }

    const auto single_geometry_result =
        ColliderGeometry::create(
            single_collider,
            sphere_shape_result.value(),
            PhysicsVector3{
                1.5,
                0.0,
                0.0
            });

    const auto multiple_near_geometry_result =
        ColliderGeometry::create(
            multiple_near_collider,
            sphere_shape_result.value(),
            PhysicsVector3{
                101.5,
                0.0,
                0.0
            });

    const auto multiple_far_geometry_result =
        ColliderGeometry::create(
            multiple_far_collider,
            sphere_shape_result.value(),
            PhysicsVector3{
                101.75,
                0.0,
                0.0
            });

    const auto trapped_right_geometry_result =
        ColliderGeometry::create(
            trapped_right_collider,
            sphere_shape_result.value(),
            PhysicsVector3{
                201.5,
                0.0,
                0.0
            });

    const auto trapped_left_geometry_result =
        ColliderGeometry::create(
            trapped_left_collider,
            sphere_shape_result.value(),
            PhysicsVector3{
                198.5,
                0.0,
                0.0
            });

    const auto boundary_geometry_result =
        ColliderGeometry::create(
            boundary_collider,
            sphere_shape_result.value(),
            PhysicsVector3{
                510.25,
                0.0,
                0.0
            });

    check(
        state,
        single_geometry_result.has_value() &&
            multiple_near_geometry_result.
                has_value() &&
            multiple_far_geometry_result.
                has_value() &&
            trapped_right_geometry_result.
                has_value() &&
            trapped_left_geometry_result.
                has_value() &&
            boundary_geometry_result.has_value(),
        "Depenetration world geometries are created");

    if (!single_geometry_result.has_value() ||
        !multiple_near_geometry_result.has_value() ||
        !multiple_far_geometry_result.has_value() ||
        !trapped_right_geometry_result.has_value() ||
        !trapped_left_geometry_result.has_value() ||
        !boundary_geometry_result.has_value())
    {
        return finish(state);
    }

    const std::array<
        ColliderGeometry,
        6U>
        colliders{
            boundary_geometry_result.value(),
            trapped_left_geometry_result.value(),
            multiple_far_geometry_result.value(),
            single_geometry_result.value(),
            trapped_right_geometry_result.value(),
            multiple_near_geometry_result.value()
        };

    const auto collider_set_result =
        WorldCellColliderSet::create(
            origin_cell,
            std::span<
                const ColliderGeometry>{
                    colliders
                });

    check(
        state,
        collider_set_result.has_value(),
        "Depenetration collider set is created");

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
        "Depenetration collider payload serializes");

    if (!payload_result.has_value())
    {
        return finish(state);
    }

    const auto snapshot_result =
        WorldCellSnapshot::create(
            origin_cell,
            1ULL,
            std::span<const std::byte>{
                payload_result.value()
            });

    check(
        state,
        snapshot_result.has_value(),
        "Depenetration snapshot is created");

    if (!snapshot_result.has_value())
    {
        return finish(state);
    }

    auto residency_result =
        WorldCellResidency::create(
            origin_cell);

    check(
        state,
        residency_result.has_value(),
        "Depenetration residency is created");

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
        "Depenetration world cell becomes resident");

    WorldCellColliderRegistry registry{};

    check(
        state,
        registry.synchronize(
            residency).
            has_value(),
        "Depenetration colliders activate");

    check(
        state,
        registry.is_valid() &&
            registry.collider_count() ==
                6U,
        "Depenetration registry contains six colliders");

    const auto origin_position_result =
        WorldPosition::create(
            origin_cell.cell,
            LocalPosition{});

    const auto separated_position_result =
        WorldPosition::create(
            origin_cell.cell,
            LocalPosition{
                300.0,
                0.0,
                0.0
            });

    const auto multiple_position_result =
        WorldPosition::create(
            origin_cell.cell,
            LocalPosition{
                100.0,
                0.0,
                0.0
            });

    const auto trapped_position_result =
        WorldPosition::create(
            origin_cell.cell,
            LocalPosition{
                200.0,
                0.0,
                0.0
            });

    const auto boundary_position_result =
        WorldPosition::create(
            origin_cell.cell,
            LocalPosition{
                511.75,
                0.0,
                0.0
            });

    check(
        state,
        origin_position_result.has_value() &&
            separated_position_result.has_value() &&
            multiple_position_result.has_value() &&
            trapped_position_result.has_value() &&
            boundary_position_result.has_value(),
        "Depenetration query positions are created");

    if (!origin_position_result.has_value() ||
        !separated_position_result.has_value() ||
        !multiple_position_result.has_value() ||
        !trapped_position_result.has_value() ||
        !boundary_position_result.has_value())
    {
        return finish(state);
    }

    WorldCellColliderRegistry empty_registry{};

    const auto zero_iteration_result =
        resolve_world_capsule_penetration(
            empty_registry,
            world_namespace,
            query_after_world_ids,
            capsule_shape_result.value(),
            origin_position_result.value(),
            0U);

    check(
        state,
        !zero_iteration_result.has_value() &&
            zero_iteration_result.
                error().
                code ==
            ErrorCode::invalid_argument,
        "Depenetration rejects zero iterations");

    const auto invalid_namespace_result =
        resolve_world_capsule_penetration(
            empty_registry,
            0ULL,
            query_after_world_ids,
            capsule_shape_result.value(),
            origin_position_result.value(),
            1U);

    check(
        state,
        !invalid_namespace_result.has_value() &&
            invalid_namespace_result.
                error().
                code ==
            ErrorCode::invalid_argument,
        "Depenetration propagates invalid namespace");

    const auto empty_result =
        resolve_world_capsule_penetration(
            empty_registry,
            world_namespace,
            query_after_world_ids,
            capsule_shape_result.value(),
            origin_position_result.value(),
            1U);

    check(
        state,
        empty_result.has_value(),
        "Empty-world depenetration succeeds");

    check(
        state,
        empty_result.has_value() &&
            empty_result.value() ==
                origin_position_result.value(),
        "Empty-world depenetration preserves position");

    const auto separated_result =
        resolve_world_capsule_penetration(
            registry,
            world_namespace,
            query_after_world_ids,
            capsule_shape_result.value(),
            separated_position_result.value(),
            1U);

    check(
        state,
        separated_result.has_value(),
        "Separated depenetration succeeds");

    check(
        state,
        separated_result.has_value() &&
            separated_result.value() ==
                separated_position_result.value(),
        "Separated capsule position is unchanged");

    const auto expected_single_result =
        WorldPosition::create(
            origin_cell.cell,
            LocalPosition{
                -0.5,
                0.0,
                0.0
            });

    check(
        state,
        expected_single_result.has_value(),
        "Expected single-contact position is created");

    if (!expected_single_result.has_value())
    {
        return finish(state);
    }

    const auto single_after_result =
        resolve_world_capsule_penetration(
            registry,
            world_namespace,
            query_after_world_ids,
            capsule_shape_result.value(),
            origin_position_result.value(),
            1U);

    check(
        state,
        single_after_result.has_value(),
        "Single contact resolves when capsule identity sorts after world identity");

    check(
        state,
        single_after_result.has_value() &&
            single_after_result.value() ==
                expected_single_result.value(),
        "Canonical second-collider normal pushes capsule outward");

    const auto single_before_result =
        resolve_world_capsule_penetration(
            registry,
            world_namespace,
            query_before_world_ids,
            capsule_shape_result.value(),
            origin_position_result.value(),
            1U);

    check(
        state,
        single_before_result.has_value(),
        "Single contact resolves when capsule identity sorts before world identity");

    check(
        state,
        single_before_result.has_value() &&
            single_before_result.value() ==
                expected_single_result.value(),
        "Canonical first-collider normal is reversed for capsule push-out");

    if (!single_after_result.has_value())
    {
        return finish(state);
    }

    const auto single_contacts_result =
        query_world_capsule_contacts(
            registry,
            world_namespace,
            query_after_world_ids,
            capsule_shape_result.value(),
            single_after_result.value());

    check(
        state,
        single_contacts_result.has_value(),
        "Resolved single-contact position can be queried");

    check(
        state,
        single_contacts_result.has_value() &&
            !has_positive_penetration(
                single_contacts_result.value()),
        "Resolved single-contact position has no positive penetration");

    const auto expected_multiple_result =
        WorldPosition::create(
            origin_cell.cell,
            LocalPosition{
                99.5,
                0.0,
                0.0
            });

    check(
        state,
        expected_multiple_result.has_value(),
        "Expected multiple-contact position is created");

    if (!expected_multiple_result.has_value())
    {
        return finish(state);
    }

    const auto multiple_result =
        resolve_world_capsule_penetration(
            registry,
            world_namespace,
            query_after_world_ids,
            capsule_shape_result.value(),
            multiple_position_result.value(),
            1U);

    check(
        state,
        multiple_result.has_value(),
        "Multiple same-direction penetrations resolve");

    check(
        state,
        multiple_result.has_value() &&
            multiple_result.value() ==
                expected_multiple_result.value(),
        "Deepest penetration determines the correction");

    if (!multiple_result.has_value())
    {
        return finish(state);
    }

    const auto multiple_contacts_result =
        query_world_capsule_contacts(
            registry,
            world_namespace,
            query_after_world_ids,
            capsule_shape_result.value(),
            multiple_result.value());

    check(
        state,
        multiple_contacts_result.has_value() &&
            !has_positive_penetration(
                multiple_contacts_result.value()),
        "Deepest correction clears all same-direction penetrations");

    const auto trapped_result =
        resolve_world_capsule_penetration(
            registry,
            world_namespace,
            query_after_world_ids,
            capsule_shape_result.value(),
            trapped_position_result.value(),
            1U);

    check(
        state,
        !trapped_result.has_value() &&
            trapped_result.
                error().
                code ==
            ErrorCode::invalid_state,
        "Iteration exhaustion reports unresolved penetration");

    const auto expected_boundary_result =
        WorldPosition::create(
            WorldCell{
                1,
                0,
                0
            },
            LocalPosition{
                -511.75,
                0.0,
                0.0
            });

    check(
        state,
        expected_boundary_result.has_value(),
        "Expected cross-cell resolved position is created");

    if (!expected_boundary_result.has_value())
    {
        return finish(state);
    }

    const auto boundary_result =
        resolve_world_capsule_penetration(
            registry,
            world_namespace,
            query_after_world_ids,
            capsule_shape_result.value(),
            boundary_position_result.value(),
            1U);

    check(
        state,
        boundary_result.has_value(),
        "Boundary penetration resolves");

    check(
        state,
        boundary_result.has_value() &&
            boundary_result.value() ==
                expected_boundary_result.value(),
        "Depenetration correction normalizes across the world-cell boundary");

    if (!boundary_result.has_value())
    {
        return finish(state);
    }

    const auto boundary_contacts_result =
        query_world_capsule_contacts(
            registry,
            world_namespace,
            query_after_world_ids,
            capsule_shape_result.value(),
            boundary_result.value());

    check(
        state,
        boundary_contacts_result.has_value() &&
            !has_positive_penetration(
                boundary_contacts_result.value()),
        "Cross-cell resolved position has no positive penetration");

    return finish(state);
}