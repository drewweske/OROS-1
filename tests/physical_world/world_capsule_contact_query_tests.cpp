#include "oros/physical_world/world_capsule_contact_query.hpp"

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
            << "\nWorld capsule contact query test "
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

    constexpr std::uint64_t
        other_world_namespace{
            0x4F524F5300000009ULL
        };

    const WorldCellKey origin_cell{
        world_namespace,
        WorldCell{
            0,
            0,
            0
        }
    };

    const WorldCellKey right_cell{
        world_namespace,
        WorldCell{
            1,
            0,
            0
        }
    };

    const ColliderId sphere_collider{
        EntityId{
            world_namespace,
            100ULL
        },
        1U
    };

    const ColliderId box_collider{
        EntityId{
            world_namespace,
            200ULL
        },
        1U
    };

    const ColliderId capsule_world_collider{
        EntityId{
            world_namespace,
            300ULL
        },
        1U
    };

    const ColliderId separated_collider{
        EntityId{
            world_namespace,
            400ULL
        },
        1U
    };

    const ColliderId boundary_collider{
        EntityId{
            world_namespace,
            500ULL
        },
        1U
    };

    const ColliderId query_collider{
        EntityId{
            world_namespace,
            900ULL
        },
        1U
    };

    const ColliderId wrong_namespace_query{
        EntityId{
            other_world_namespace,
            900ULL
        },
        1U
    };

    const auto query_capsule_result =
        CapsuleShape::create(
            1.0,
            1.0,
            PhysicsUnitVector3::
                positive_y());

    const auto sphere_shape_result =
        SphereShape::create(
            1.0);

    const auto box_shape_result =
        BoxShape::create(
            PhysicsVector3{
                1.0,
                1.0,
                1.0
            });

    const auto boundary_sphere_result =
        SphereShape::create(
            23.0);

    check(
        state,
        query_capsule_result.has_value(),
        "Query capsule shape is created");

    check(
        state,
        sphere_shape_result.has_value(),
        "World sphere shape is created");

    check(
        state,
        box_shape_result.has_value(),
        "World box shape is created");

    check(
        state,
        boundary_sphere_result.has_value(),
        "Boundary sphere shape is created");

    if (!query_capsule_result.has_value() ||
        !sphere_shape_result.has_value() ||
        !box_shape_result.has_value() ||
        !boundary_sphere_result.has_value())
    {
        return finish(state);
    }

    const auto origin_position_result =
        WorldPosition::create(
            origin_cell.cell,
            LocalPosition{});

    check(
        state,
        origin_position_result.has_value(),
        "Query world origin is created");

    if (!origin_position_result.has_value())
    {
        return finish(state);
    }

    WorldCellColliderRegistry empty_registry{};

    const auto empty_query_result =
        query_world_capsule_contacts(
            empty_registry,
            world_namespace,
            query_collider,
            query_capsule_result.value(),
            origin_position_result.value());

    check(
        state,
        empty_query_result.has_value(),
        "Empty world capsule query succeeds");

    check(
        state,
        empty_query_result.has_value() &&
            empty_query_result.
                value().
                empty(),
        "Empty world capsule query returns no contacts");

    const auto invalid_namespace_result =
        query_world_capsule_contacts(
            empty_registry,
            0ULL,
            query_collider,
            query_capsule_result.value(),
            origin_position_result.value());

    check(
        state,
        !invalid_namespace_result.has_value() &&
            invalid_namespace_result.
                error().
                code ==
            ErrorCode::invalid_argument,
        "World capsule query rejects invalid namespace");

    const auto wrong_namespace_result =
        query_world_capsule_contacts(
            empty_registry,
            world_namespace,
            wrong_namespace_query,
            query_capsule_result.value(),
            origin_position_result.value());

    check(
        state,
        !wrong_namespace_result.has_value() &&
            wrong_namespace_result.
                error().
                code ==
            ErrorCode::invalid_argument,
        "World capsule query rejects collider from another namespace");

    const auto sphere_geometry_result =
        ColliderGeometry::create(
            sphere_collider,
            sphere_shape_result.value(),
            PhysicsVector3{
                1.5,
                0.0,
                0.0
            });

    const auto box_geometry_result =
        ColliderGeometry::create(
            box_collider,
            box_shape_result.value(),
            PhysicsVector3{
                -1.5,
                0.0,
                0.0
            });

    const auto capsule_geometry_result =
        ColliderGeometry::create(
            capsule_world_collider,
            query_capsule_result.value(),
            PhysicsVector3{
                0.0,
                0.0,
                1.5
            });

    const auto separated_geometry_result =
        ColliderGeometry::create(
            separated_collider,
            sphere_shape_result.value(),
            PhysicsVector3{
                10.0,
                0.0,
                0.0
            });

    check(
        state,
        sphere_geometry_result.has_value() &&
            box_geometry_result.has_value() &&
            capsule_geometry_result.has_value() &&
            separated_geometry_result.has_value(),
        "World collider geometry fixtures are created");

    if (!sphere_geometry_result.has_value() ||
        !box_geometry_result.has_value() ||
        !capsule_geometry_result.has_value() ||
        !separated_geometry_result.has_value())
    {
        return finish(state);
    }

    const std::array<
        ColliderGeometry,
        4U>
        origin_colliders{
            separated_geometry_result.value(),
            capsule_geometry_result.value(),
            box_geometry_result.value(),
            sphere_geometry_result.value()
        };

    const auto origin_set_result =
        WorldCellColliderSet::create(
            origin_cell,
            std::span<
                const ColliderGeometry>{
                    origin_colliders
                });

    check(
        state,
        origin_set_result.has_value(),
        "Origin world collider set is created");

    if (!origin_set_result.has_value())
    {
        return finish(state);
    }

    const auto origin_payload_result =
        serialize_world_cell_collider_payload(
            origin_set_result.value());

    check(
        state,
        origin_payload_result.has_value(),
        "Origin world collider payload serializes");

    if (!origin_payload_result.has_value())
    {
        return finish(state);
    }

    const auto origin_snapshot_result =
        WorldCellSnapshot::create(
            origin_cell,
            1ULL,
            std::span<const std::byte>{
                origin_payload_result.value()
            });

    check(
        state,
        origin_snapshot_result.has_value(),
        "Origin world snapshot is created");

    if (!origin_snapshot_result.has_value())
    {
        return finish(state);
    }

    auto origin_residency_result =
        WorldCellResidency::create(
            origin_cell);

    check(
        state,
        origin_residency_result.has_value(),
        "Origin world residency is created");

    if (!origin_residency_result.has_value())
    {
        return finish(state);
    }

    WorldCellResidency origin_residency =
        std::move(
            origin_residency_result.value());

    check(
        state,
        make_resident(
            origin_residency,
            origin_snapshot_result.value(),
            101ULL),
        "Origin world cell becomes resident");

    WorldCellColliderRegistry registry{};

    check(
        state,
        registry.synchronize(
            origin_residency).
            has_value(),
        "Origin world colliders activate");

    check(
        state,
        registry.is_valid() &&
            registry.collider_count() ==
                4U,
        "Origin physical registry contains four valid colliders");

    const auto duplicate_identity_result =
        query_world_capsule_contacts(
            registry,
            world_namespace,
            sphere_collider,
            query_capsule_result.value(),
            origin_position_result.value());

    check(
        state,
        !duplicate_identity_result.has_value() &&
            duplicate_identity_result.
                error().
                code ==
            ErrorCode::invalid_argument,
        "World capsule query rejects active duplicate identity");

    const auto contact_query_result =
        query_world_capsule_contacts(
            registry,
            world_namespace,
            query_collider,
            query_capsule_result.value(),
            origin_position_result.value());

    check(
        state,
        contact_query_result.has_value(),
        "World capsule contact query succeeds");

    check(
        state,
        contact_query_result.has_value() &&
            contact_query_result.
                value().
                size() ==
                3U,
        "World capsule query reports three overlapping colliders");

    if (!contact_query_result.has_value() ||
        contact_query_result.
            value().
            size() !=
            3U)
    {
        return finish(state);
    }

    const std::vector<
        CollisionContact>&
        contacts =
            contact_query_result.value();

    check(
        state,
        contacts[0U].
                pair().
                contains(
                    sphere_collider) &&
            contacts[0U].
                pair().
                contains(
                    query_collider),
        "First contact is the sphere pair");

    check(
        state,
        contacts[1U].
                pair().
                contains(
                    box_collider) &&
            contacts[1U].
                pair().
                contains(
                    query_collider),
        "Second contact is the box pair");

    check(
        state,
        contacts[2U].
                pair().
                contains(
                    capsule_world_collider) &&
            contacts[2U].
                pair().
                contains(
                    query_collider),
        "Third contact is the capsule pair");

    check(
        state,
        !contacts[0U].
            pair().
            contains(
                separated_collider) &&
            !contacts[1U].
                pair().
                contains(
                    separated_collider) &&
            !contacts[2U].
                pair().
                contains(
                    separated_collider),
        "Separated world collider produces no contact");

    check(
        state,
        contacts[0U].pair() <
            contacts[1U].pair() &&
            contacts[1U].pair() <
                contacts[2U].pair(),
        "World capsule contacts are canonically ordered");

    const auto boundary_geometry_result =
        ColliderGeometry::create(
            boundary_collider,
            boundary_sphere_result.value(),
            PhysicsVector3{
                -500.0,
                0.0,
                0.0
            });

    check(
        state,
        boundary_geometry_result.has_value(),
        "Cross-cell boundary collider geometry is created");

    if (!boundary_geometry_result.has_value())
    {
        return finish(state);
    }

    const std::array<
        ColliderGeometry,
        1U>
        boundary_colliders{
            boundary_geometry_result.value()
        };

    const auto boundary_set_result =
        WorldCellColliderSet::create(
            right_cell,
            std::span<
                const ColliderGeometry>{
                    boundary_colliders
                });

    check(
        state,
        boundary_set_result.has_value(),
        "Cross-cell boundary collider set is created");

    if (!boundary_set_result.has_value())
    {
        return finish(state);
    }

    const auto boundary_payload_result =
        serialize_world_cell_collider_payload(
            boundary_set_result.value());

    check(
        state,
        boundary_payload_result.has_value(),
        "Cross-cell boundary payload serializes");

    if (!boundary_payload_result.has_value())
    {
        return finish(state);
    }

    const auto boundary_snapshot_result =
        WorldCellSnapshot::create(
            right_cell,
            2ULL,
            std::span<const std::byte>{
                boundary_payload_result.value()
            });

    check(
        state,
        boundary_snapshot_result.has_value(),
        "Cross-cell boundary snapshot is created");

    if (!boundary_snapshot_result.has_value())
    {
        return finish(state);
    }

    auto boundary_residency_result =
        WorldCellResidency::create(
            right_cell);

    check(
        state,
        boundary_residency_result.has_value(),
        "Cross-cell boundary residency is created");

    if (!boundary_residency_result.has_value())
    {
        return finish(state);
    }

    WorldCellResidency boundary_residency =
        std::move(
            boundary_residency_result.value());

    check(
        state,
        make_resident(
            boundary_residency,
            boundary_snapshot_result.value(),
            102ULL),
        "Cross-cell boundary cell becomes resident");

    check(
        state,
        registry.synchronize(
            boundary_residency).
            has_value(),
        "Cross-cell boundary collider activates");

    check(
        state,
        registry.is_valid() &&
            registry.active_cell_count() ==
                2U &&
            registry.collider_count() ==
                5U,
        "Cross-cell physical registry remains valid");

    const auto boundary_query_position_result =
        WorldPosition::create(
            origin_cell.cell,
            LocalPosition{
                500.0,
                0.0,
                0.0
            });

    check(
        state,
        boundary_query_position_result.has_value(),
        "Cross-cell query position is created");

    if (!boundary_query_position_result.has_value())
    {
        return finish(state);
    }

    const auto boundary_query_result =
        query_world_capsule_contacts(
            registry,
            world_namespace,
            query_collider,
            query_capsule_result.value(),
            boundary_query_position_result.value());

    check(
        state,
        boundary_query_result.has_value(),
        "Cross-cell world capsule query succeeds");

    check(
        state,
        boundary_query_result.has_value() &&
            boundary_query_result.
                value().
                size() ==
                1U,
        "Cross-cell query finds exactly the boundary contact");

    if (!boundary_query_result.has_value() ||
        boundary_query_result.
            value().
            size() !=
            1U)
    {
        return finish(state);
    }

    const CollisionContact&
        boundary_contact =
            boundary_query_result.
                value().
                front();

    check(
        state,
        boundary_contact.
                pair().
                contains(
                    boundary_collider) &&
            boundary_contact.
                pair().
                contains(
                    query_collider),
        "Cross-cell contact preserves both persistent identities");

    check(
        state,
        boundary_contact.is_touching(),
        "Twenty-four-meter cross-cell boundary contact is touching");

    check(
        state,
        boundary_contact.
            penetration_depth() ==
            0.0,
        "Cross-cell touching contact has zero penetration");

    return finish(state);
}