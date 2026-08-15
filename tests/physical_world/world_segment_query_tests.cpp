#include "oros/physical_world/world_segment_query.hpp"

#include "oros/foundation/result.hpp"
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
            << '\n'
            << "World segment query test summary: "
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
    bool activate_cell(
        oros::physical_world::
            WorldCellColliderRegistry& registry,
        const oros::streaming::WorldCellKey
            cell_key,
        const std::span<
            const oros::physics::
                ColliderGeometry>
            colliders,
        const std::uint64_t revision,
        const std::uint64_t request_id)
    {
        using namespace oros::physical_world;
        using namespace oros::streaming;

        const auto collider_set_result =
            WorldCellColliderSet::create(
                cell_key,
                colliders);

        if (!collider_set_result.has_value())
        {
            return false;
        }

        const auto payload_result =
            serialize_world_cell_collider_payload(
                collider_set_result.value());

        if (!payload_result.has_value())
        {
            return false;
        }

        const auto snapshot_result =
            WorldCellSnapshot::create(
                cell_key,
                revision,
                std::span<const std::byte>{
                    payload_result.value()
                });

        if (!snapshot_result.has_value())
        {
            return false;
        }

        auto residency_result =
            WorldCellResidency::create(
                cell_key);

        if (!residency_result.has_value())
        {
            return false;
        }

        WorldCellResidency residency =
            std::move(
                residency_result.value());

        const WorldCellRevisionId
            revision_id{
                cell_key,
                revision
            };

        if (!residency.queue_load(
                revision_id,
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

        if (!residency.complete_load(
                request_id,
                snapshot_result.value()).
                has_value())
        {
            return false;
        }

        return
            registry.synchronize(
                residency).
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
            0x4F524F5300000009ULL
        };

    const WorldCellKey cell_zero{
        world_namespace,
        WorldCell{
            0,
            0,
            0
        }
    };

    const WorldCellKey cell_one{
        world_namespace,
        WorldCell{
            1,
            0,
            0
        }
    };

    const WorldCellKey cell_two{
        world_namespace,
        WorldCell{
            2,
            0,
            0
        }
    };

    const WorldCellKey diagonal_cell{
        world_namespace,
        WorldCell{
            1,
            1,
            0
        }
    };

    const auto start_result =
        WorldPosition::create(
            WorldCell{
                0,
                0,
                0
            },
            LocalPosition{
                0.0,
                0.0,
                0.0
            });

    const auto same_cell_end_result =
        WorldPosition::create(
            WorldCell{
                0,
                0,
                0
            },
            LocalPosition{
                400.0,
                0.0,
                0.0
            });

    const auto two_cell_end_result =
        WorldPosition::create(
            WorldCell{
                2,
                0,
                0
            },
            LocalPosition{
                0.0,
                0.0,
                0.0
            });

    const auto diagonal_end_result =
        WorldPosition::create(
            WorldCell{
                1,
                1,
                0
            },
            LocalPosition{
                0.0,
                0.0,
                0.0
            });

    check(
        state,
        start_result.has_value() &&
            same_cell_end_result.has_value() &&
            two_cell_end_result.has_value() &&
            diagonal_end_result.has_value(),
        "World segment test positions are created");

    if (
        !start_result.has_value() ||
        !same_cell_end_result.has_value() ||
        !two_cell_end_result.has_value() ||
        !diagonal_end_result.has_value())
    {
        return finish(state);
    }

    const auto invalid_unavailable_result =
        WorldSegmentQueryResult::
            create_unavailable(
                invalid_world_cell_key);

    check(
        state,
        !invalid_unavailable_result.
                has_value() &&
            invalid_unavailable_result.
                    error().code ==
                ErrorCode::invalid_argument,
        "Unavailable result rejects invalid cell identity");

    const auto clear_factory_result =
        WorldSegmentQueryResult::
            create_clear();

    check(
        state,
        clear_factory_result.has_value() &&
            clear_factory_result.
                value().
                is_valid() &&
            clear_factory_result.
                    value().
                    state() ==
                WorldSegmentQueryState::clear &&
            !clear_factory_result.
                value().
                hit().
                has_value() &&
            !clear_factory_result.
                value().
                unavailable_cell().
                has_value(),
        "Clear result preserves exact invariant");

    WorldCellColliderRegistry
        unavailable_registry{};

    const auto invalid_namespace_result =
        query_world_segment(
            unavailable_registry,
            0ULL,
            start_result.value(),
            same_cell_end_result.value());

    check(
        state,
        !invalid_namespace_result.has_value() &&
            invalid_namespace_result.
                    error().code ==
                ErrorCode::invalid_argument,
        "World segment query rejects invalid namespace");

    const auto missing_start_result =
        query_world_segment(
            unavailable_registry,
            world_namespace,
            start_result.value(),
            same_cell_end_result.value());

    check(
        state,
        missing_start_result.has_value() &&
            missing_start_result.
                value().
                is_valid() &&
            missing_start_result.
                    value().
                    state() ==
                WorldSegmentQueryState::
                    unavailable &&
            missing_start_result.
                value().
                unavailable_cell().
                has_value() &&
            missing_start_result.
                    value().
                    unavailable_cell().
                    value() ==
                cell_zero &&
            !missing_start_result.
                value().
                hit().
                has_value(),
        "Missing start cell returns unavailable");

    WorldCellColliderRegistry
        clear_registry{};

    check(
        state,
        activate_cell(
            clear_registry,
            cell_zero,
            std::span<
                const ColliderGeometry>{},
            1ULL,
            101ULL),
        "Empty resident start cell activates");

    const auto same_cell_clear_result =
        query_world_segment(
            clear_registry,
            world_namespace,
            start_result.value(),
            same_cell_end_result.value());

    check(
        state,
        same_cell_clear_result.has_value() &&
            same_cell_clear_result.
                value().
                is_valid() &&
            same_cell_clear_result.
                    value().
                    state() ==
                WorldSegmentQueryState::clear &&
            !same_cell_clear_result.
                value().
                hit().
                has_value(),
        "Resident empty same-cell segment is clear");

    const auto degenerate_clear_result =
        query_world_segment(
            clear_registry,
            world_namespace,
            start_result.value(),
            start_result.value());

    check(
        state,
        degenerate_clear_result.has_value() &&
            degenerate_clear_result.
                    value().
                    state() ==
                WorldSegmentQueryState::clear,
        "Resident degenerate world segment is clear");

    const ColliderId near_collider{
        EntityId{
            world_namespace,
            100ULL
        },
        1U
    };

    const ColliderId far_collider{
        EntityId{
            world_namespace,
            200ULL
        },
        1U
    };

    const auto near_shape_result =
        SphereShape::create(
            50.0);

    const auto far_shape_result =
        SphereShape::create(
            50.0);

    check(
        state,
        near_shape_result.has_value() &&
            far_shape_result.has_value(),
        "World segment blocker shapes are created");

    if (
        !near_shape_result.has_value() ||
        !far_shape_result.has_value())
    {
        return finish(state);
    }

    const auto near_geometry_result =
        ColliderGeometry::create(
            near_collider,
            CollisionShape{
                near_shape_result.value()
            },
            PhysicsVector3{
                200.0,
                0.0,
                0.0
            });

    const auto far_geometry_result =
        ColliderGeometry::create(
            far_collider,
            CollisionShape{
                far_shape_result.value()
            },
            PhysicsVector3{
                0.0,
                0.0,
                0.0
            });

    check(
        state,
        near_geometry_result.has_value() &&
            far_geometry_result.has_value(),
        "World segment blocker geometry is created");

    if (
        !near_geometry_result.has_value() ||
        !far_geometry_result.has_value())
    {
        return finish(state);
    }

    const std::array<
        ColliderGeometry,
        1U>
        near_colliders{
            near_geometry_result.value()
        };

    const std::array<
        ColliderGeometry,
        1U>
        far_colliders{
            far_geometry_result.value()
        };

    WorldCellColliderRegistry
        blocked_same_cell_registry{};

    check(
        state,
        activate_cell(
            blocked_same_cell_registry,
            cell_zero,
            std::span<
                const ColliderGeometry>{
                    near_colliders
                },
            2ULL,
            102ULL),
        "Blocked same-cell registry activates");

    const auto same_cell_blocked_result =
        query_world_segment(
            blocked_same_cell_registry,
            world_namespace,
            start_result.value(),
            same_cell_end_result.value());

    check(
        state,
        same_cell_blocked_result.
                has_value() &&
            same_cell_blocked_result.
                    value().
                    state() ==
                WorldSegmentQueryState::blocked &&
            same_cell_blocked_result.
                value().
                hit().
                has_value() &&
            same_cell_blocked_result.
                    value().
                    hit()->
                    collider() ==
                near_collider &&
            nearly_equal(
                same_cell_blocked_result.
                    value().
                    hit()->
                    segment_fraction(),
                0.375),
        "Same-cell blocker returns earliest physical hit");

    WorldCellColliderRegistry
        cross_clear_registry{};

    check(
        state,
        activate_cell(
            cross_clear_registry,
            cell_zero,
            std::span<
                const ColliderGeometry>{},
            3ULL,
            103ULL) &&
            activate_cell(
                cross_clear_registry,
                cell_one,
                std::span<
                    const ColliderGeometry>{},
                4ULL,
                104ULL) &&
            activate_cell(
                cross_clear_registry,
                cell_two,
                std::span<
                    const ColliderGeometry>{},
                5ULL,
                105ULL),
        "All centerline cells activate as empty residents");

    const auto cross_clear_result =
        query_world_segment(
            cross_clear_registry,
            world_namespace,
            start_result.value(),
            two_cell_end_result.value());

    check(
        state,
        cross_clear_result.has_value() &&
            cross_clear_result.
                    value().
                    state() ==
                WorldSegmentQueryState::clear,
        "Fully resident cross-cell segment is clear");

    WorldCellColliderRegistry
        missing_destination_registry{};

    check(
        state,
        activate_cell(
            missing_destination_registry,
            cell_zero,
            std::span<
                const ColliderGeometry>{},
            6ULL,
            106ULL) &&
            activate_cell(
                missing_destination_registry,
                cell_one,
                std::span<
                    const ColliderGeometry>{},
                7ULL,
                107ULL),
        "Pre-destination centerline cells activate");

    const auto missing_destination_result =
        query_world_segment(
            missing_destination_registry,
            world_namespace,
            start_result.value(),
            two_cell_end_result.value());

    check(
        state,
        missing_destination_result.
                has_value() &&
            missing_destination_result.
                    value().
                    state() ==
                WorldSegmentQueryState::
                    unavailable &&
            missing_destination_result.
                value().
                unavailable_cell().
                has_value() &&
            missing_destination_result.
                    value().
                    unavailable_cell().
                    value() ==
                cell_two,
        "Missing destination cell returns unavailable");

    WorldCellColliderRegistry
        blocker_before_missing_registry{};

    check(
        state,
        activate_cell(
            blocker_before_missing_registry,
            cell_zero,
            std::span<
                const ColliderGeometry>{
                    near_colliders
                },
            8ULL,
            108ULL) &&
            activate_cell(
                blocker_before_missing_registry,
                cell_one,
                std::span<
                    const ColliderGeometry>{},
                9ULL,
                109ULL),
        "Known blocker and intermediate resident cell activate");

    const auto blocker_before_missing_result =
        query_world_segment(
            blocker_before_missing_registry,
            world_namespace,
            start_result.value(),
            two_cell_end_result.value());

    check(
        state,
        blocker_before_missing_result.
                has_value() &&
            blocker_before_missing_result.
                    value().
                    state() ==
                WorldSegmentQueryState::blocked &&
            blocker_before_missing_result.
                value().
                hit().
                has_value() &&
            blocker_before_missing_result.
                    value().
                    hit()->
                    collider() ==
                near_collider &&
            blocker_before_missing_result.
                    value().
                    hit()->
                    segment_fraction() <
                0.75,
        "Known blocker before unavailable cell remains conclusively blocked");

    WorldCellColliderRegistry
        missing_before_blocker_registry{};

    check(
        state,
        activate_cell(
            missing_before_blocker_registry,
            cell_zero,
            std::span<
                const ColliderGeometry>{},
            10ULL,
            110ULL) &&
            activate_cell(
                missing_before_blocker_registry,
                cell_two,
                std::span<
                    const ColliderGeometry>{
                        far_colliders
                    },
                11ULL,
                111ULL),
        "Known distant blocker activates beyond missing cell");

    const auto missing_before_blocker_result =
        query_world_segment(
            missing_before_blocker_registry,
            world_namespace,
            start_result.value(),
            two_cell_end_result.value());

    check(
        state,
        missing_before_blocker_result.
                has_value() &&
            missing_before_blocker_result.
                    value().
                    state() ==
                WorldSegmentQueryState::
                    unavailable &&
            missing_before_blocker_result.
                value().
                unavailable_cell().
                has_value() &&
            missing_before_blocker_result.
                    value().
                    unavailable_cell().
                    value() ==
                cell_one &&
            !missing_before_blocker_result.
                value().
                hit().
                has_value(),
        "Unavailable cell before known blocker prevents false blocked answer");

    WorldCellColliderRegistry
        fully_known_blocker_registry{};

    check(
        state,
        activate_cell(
            fully_known_blocker_registry,
            cell_zero,
            std::span<
                const ColliderGeometry>{},
            12ULL,
            112ULL) &&
            activate_cell(
                fully_known_blocker_registry,
                cell_one,
                std::span<
                    const ColliderGeometry>{},
                13ULL,
                113ULL) &&
            activate_cell(
                fully_known_blocker_registry,
                cell_two,
                std::span<
                    const ColliderGeometry>{
                        far_colliders
                    },
                14ULL,
                114ULL),
        "Fully known distant blocker registry activates");

    const auto fully_known_blocker_result =
        query_world_segment(
            fully_known_blocker_registry,
            world_namespace,
            start_result.value(),
            two_cell_end_result.value());

    check(
        state,
        fully_known_blocker_result.
                has_value() &&
            fully_known_blocker_result.
                    value().
                    state() ==
                WorldSegmentQueryState::blocked &&
            fully_known_blocker_result.
                value().
                hit().
                has_value() &&
            fully_known_blocker_result.
                    value().
                    hit()->
                    collider() ==
                far_collider,
        "Fully resident distant blocker returns blocked");

    WorldCellColliderRegistry
        diagonal_registry{};

    check(
        state,
        activate_cell(
            diagonal_registry,
            cell_zero,
            std::span<
                const ColliderGeometry>{},
            15ULL,
            115ULL) &&
            activate_cell(
                diagonal_registry,
                diagonal_cell,
                std::span<
                    const ColliderGeometry>{},
                16ULL,
                116ULL),
        "Diagonal corner cells activate");

    const auto diagonal_result =
        query_world_segment(
            diagonal_registry,
            world_namespace,
            start_result.value(),
            diagonal_end_result.value());

    check(
        state,
        diagonal_result.has_value() &&
            diagonal_result.
                    value().
                    state() ==
                WorldSegmentQueryState::clear,
        "Exact corner crossing advances tied cell axes deterministically");

    return finish(state);
}
