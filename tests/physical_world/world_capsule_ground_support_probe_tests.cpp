#include "oros/physical_world/world_capsule_contact_query.hpp"
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
            << "\nWorld capsule ground support probe "
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

    const ColliderId floor_collider{
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

    const auto floor_shape_result =
        BoxShape::create(
            PhysicsVector3{
                5.0,
                0.5,
                5.0
            });

    const auto settings_result =
        WorldCapsuleTraversalSettings::create(
            PhysicsUnitVector3::positive_y(),
            1.0,
            0.5,
            0.25);

    check(
        state,
        capsule_shape_result.has_value(),
        "Support-probe capsule shape is created");

    check(
        state,
        floor_shape_result.has_value(),
        "Support-probe floor shape is created");

    check(
        state,
        settings_result.has_value(),
        "Support-probe traversal settings are created");

    if (!capsule_shape_result.has_value() ||
        !floor_shape_result.has_value() ||
        !settings_result.has_value())
    {
        return finish(state);
    }

    const auto tiny_gap_position_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                0.0,
                physics_vector_zero_tolerance *
                    0.5,
                0.0
            });

    const auto outside_probe_position_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                0.0,
                physics_vector_zero_tolerance *
                    2.0,
                0.0
            });

    check(
        state,
        tiny_gap_position_result.has_value() &&
            outside_probe_position_result.
                has_value(),
        "Support-probe world positions are created");

    if (!tiny_gap_position_result.has_value() ||
        !outside_probe_position_result.has_value())
    {
        return finish(state);
    }

    const auto floor_geometry_result =
        ColliderGeometry::create(
            floor_collider,
            floor_shape_result.value(),
            PhysicsVector3{
                0.0,
                -2.5,
                0.0
            });

    check(
        state,
        floor_geometry_result.has_value(),
        "Support-probe floor geometry is created");

    if (!floor_geometry_result.has_value())
    {
        return finish(state);
    }

    const WorldCellKey cell_key{
        world_namespace,
        WorldCell{}
    };

    const std::array<
        ColliderGeometry,
        1U>
        colliders{
            floor_geometry_result.value()
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
        "Support-probe collider set is created");

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
        "Support-probe collider payload serializes");

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
        "Support-probe world snapshot is created");

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
        "Support-probe residency is created");

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
        "Support-probe cell becomes resident");

    WorldCellColliderRegistry registry{};

    check(
        state,
        registry.synchronize(
            residency).
            has_value(),
        "Support-probe collider activates");

    check(
        state,
        registry.is_valid() &&
            registry.collider_count() ==
                1U,
        "Support-probe registry is valid");

    const auto exact_contacts_result =
        query_world_capsule_contacts(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            tiny_gap_position_result.value());

    check(
        state,
        exact_contacts_result.has_value(),
        "Tiny-gap exact contact query succeeds");

    check(
        state,
        exact_contacts_result.has_value() &&
            exact_contacts_result.
                value().
                empty(),
        "Tiny positive support gap has no exact collision contact");

    const auto tiny_support_result =
        query_world_capsule_ground_contact(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            tiny_gap_position_result.value(),
            settings_result.value());

    check(
        state,
        tiny_support_result.has_value() &&
            tiny_support_result.
                value().
                has_value() &&
            tiny_support_result.
                value()->
                pair().
                contains(
                    floor_collider),
        "Numerical support probe preserves a tiny separated ground contact");

    const auto outside_support_result =
        query_world_capsule_ground_contact(
            registry,
            world_namespace,
            query_collider,
            capsule_shape_result.value(),
            outside_probe_position_result.value(),
            settings_result.value());

    check(
        state,
        outside_support_result.has_value() &&
            !outside_support_result.
                value().
                has_value(),
        "Numerical support probe does not become gameplay ground snapping");

    return finish(state);
}
