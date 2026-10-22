#include "oros/ai/actor_simulation_position_snapshot_set.hpp"

#include <iostream>
#include <span>
#include <string_view>
#include <type_traits>
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
}

int main()
{
    using namespace oros::ai;
    using namespace oros::foundation;
    using namespace oros::world;

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const ActorSimulationPositionSnapshotSet&>().
                        snapshots_in_entity_order()),
            std::span<
                const ActorSimulationPositionSnapshot>>);

    static_assert(
        std::is_nothrow_move_constructible_v<
            ActorSimulationPositionSnapshot>);

    static_assert(
        std::is_nothrow_move_assignable_v<
            ActorSimulationPositionSnapshot>);

    static_assert(
        std::is_nothrow_move_constructible_v<
            ActorSimulationPositionSnapshotSet>);

    static_assert(
        std::is_nothrow_move_assignable_v<
            ActorSimulationPositionSnapshotSet>);

    TestState state{};

    const Result<WorldPosition>
        origin_result =
            WorldPosition::origin();

    const Result<WorldPosition>
        early_position_result =
            WorldPosition::create(
                WorldCell{},
                LocalPosition{
                    10.0,
                    0.0,
                    -5.0
                });

    const Result<WorldPosition>
        middle_position_result =
            WorldPosition::create(
                WorldCell{
                    1'000'000,
                    0,
                    -1'000'000
                },
                LocalPosition{
                    128.0,
                    64.0,
                    -256.0
                });

    const Result<WorldPosition>
        late_position_result =
            WorldPosition::create(
                WorldCell{
                    -250'000,
                    2,
                    500'000
                },
                LocalPosition{
                    -100.0,
                    12.0,
                    300.0
                });

    check(
        state,
        origin_result.has_value() &&
            early_position_result.has_value() &&
            middle_position_result.has_value() &&
            late_position_result.has_value(),
        "Actor-position snapshot fixtures are valid");

    if (
        !origin_result.has_value() ||
        !early_position_result.has_value() ||
        !middle_position_result.has_value() ||
        !late_position_result.has_value())
    {
        return 1;
    }

    const WorldPosition origin =
        origin_result.value();

    const WorldPosition early_position =
        early_position_result.value();

    const WorldPosition middle_position =
        middle_position_result.value();

    const WorldPosition late_position =
        late_position_result.value();

    ActorSimulationPositionSnapshotSet
        snapshots{};

    check(
        state,
        snapshots.empty(),
        "New actor-position snapshot set is empty");

    check(
        state,
        snapshots.size() == 0U,
        "New actor-position snapshot set has zero entries");

    check(
        state,
        snapshots.
            snapshots_in_entity_order().
            empty(),
        "New snapshot set exposes an empty ordered view");

    check(
        state,
        !snapshots.contains(
            invalid_entity_id),
        "Snapshot set does not contain invalid identity");

    const Result<WorldPosition>
        invalid_lookup =
            snapshots.position(
                invalid_entity_id);

    check(
        state,
        !invalid_lookup.has_value() &&
            invalid_lookup.error().code ==
                ErrorCode::invalid_argument,
        "Invalid snapshot lookup reports invalid_argument");

    const Status invalid_insert =
        snapshots.insert(
            invalid_entity_id,
            origin);

    check(
        state,
        !invalid_insert.has_value() &&
            invalid_insert.error().code ==
                ErrorCode::invalid_argument,
        "Invalid actor snapshot insertion reports invalid_argument");

    check(
        state,
        snapshots.empty(),
        "Rejected invalid snapshot cannot mutate the set");

    const EntityId early_actor{
        0x4F524F53ULL,
        10ULL
    };

    const EntityId middle_actor{
        0x4F524F53ULL,
        20ULL
    };

    const EntityId late_actor{
        0x4F524F53ULL,
        30ULL
    };

    const EntityId missing_actor{
        0x4F524F53ULL,
        40ULL
    };

    const Status insert_late =
        snapshots.insert(
            late_actor,
            late_position);

    const Status insert_early =
        snapshots.insert(
            early_actor,
            early_position);

    const Status insert_middle =
        snapshots.insert(
            middle_actor,
            middle_position);

    check(
        state,
        insert_late.has_value() &&
            insert_early.has_value() &&
            insert_middle.has_value(),
        "Actor-position snapshots insert successfully out of order");

    check(
        state,
        snapshots.size() == 3U &&
            !snapshots.empty(),
        "Snapshot set owns exactly three actor positions");

    const std::span<
        const ActorSimulationPositionSnapshot>
        ordered =
            snapshots.
                snapshots_in_entity_order();

    check(
        state,
        ordered.size() == 3U,
        "Ordered snapshot view contains every actor");

    check(
        state,
        ordered.size() == 3U &&
            ordered[0].actor() ==
                early_actor &&
            ordered[1].actor() ==
                middle_actor &&
            ordered[2].actor() ==
                late_actor,
        "Actor-position snapshots use canonical EntityId order");

    check(
        state,
        ordered.size() == 3U &&
            ordered[0].position() ==
                early_position &&
            ordered[1].position() ==
                middle_position &&
            ordered[2].position() ==
                late_position,
        "Canonical ordering preserves exact WorldPosition snapshots");

    check(
        state,
        snapshots.contains(
            early_actor) &&
            snapshots.contains(
                middle_actor) &&
            snapshots.contains(
                late_actor),
        "Snapshot set resolves every inserted actor");

    check(
        state,
        !snapshots.contains(
            missing_actor),
        "Snapshot set rejects an absent valid actor");

    const Result<WorldPosition>
        middle_lookup =
            snapshots.position(
                middle_actor);

    check(
        state,
        middle_lookup.has_value() &&
            middle_lookup.value() ==
                middle_position,
        "Snapshot lookup preserves large-world actor position");

    const Result<WorldPosition>
        missing_lookup =
            snapshots.position(
                missing_actor);

    check(
        state,
        !missing_lookup.has_value() &&
            missing_lookup.error().code ==
                ErrorCode::not_found,
        "Missing actor snapshot lookup reports not_found");

    const Status duplicate_insert =
        snapshots.insert(
            early_actor,
            origin);

    check(
        state,
        !duplicate_insert.has_value() &&
            duplicate_insert.error().code ==
                ErrorCode::invalid_state,
        "Duplicate actor snapshot insertion reports invalid_state");

    check(
        state,
        snapshots.size() == 3U,
        "Duplicate insertion cannot change snapshot count");

    const Result<WorldPosition>
        early_after_duplicate =
            snapshots.position(
                early_actor);

    check(
        state,
        early_after_duplicate.has_value() &&
            early_after_duplicate.value() ==
                early_position,
        "Duplicate insertion cannot overwrite captured position");

    ActorSimulationPositionSnapshotSet
        alternate{};

    const Status alternate_middle =
        alternate.insert(
            middle_actor,
            middle_position);

    const Status alternate_late =
        alternate.insert(
            late_actor,
            late_position);

    const Status alternate_early =
        alternate.insert(
            early_actor,
            early_position);

    check(
        state,
        alternate_middle.has_value() &&
            alternate_late.has_value() &&
            alternate_early.has_value(),
        "Alternate snapshot insertion history succeeds");

    const std::span<
        const ActorSimulationPositionSnapshot>
        canonical_a =
            snapshots.
                snapshots_in_entity_order();

    const std::span<
        const ActorSimulationPositionSnapshot>
        canonical_b =
            alternate.
                snapshots_in_entity_order();

    bool canonical_match =
        canonical_a.size() ==
            canonical_b.size();

    if (canonical_match)
    {
        for (
            std::size_t index = 0U;
            index < canonical_a.size();
            ++index)
        {
            if (
                canonical_a[index].actor() !=
                    canonical_b[index].actor() ||
                canonical_a[index].position() !=
                    canonical_b[index].position())
            {
                canonical_match = false;
                break;
            }
        }
    }

    check(
        state,
        canonical_match,
        "Canonical snapshot state is independent of insertion history");

    check(
        state,
        snapshots.position(
            middle_actor).
            value() ==
            middle_position,
        "Snapshot set remains unchanged after independent comparison");

    std::cout
        << state.checks
        << " checks, "
        << state.failures
        << " failures\n";

    return state.failures == 0
        ? 0
        : 1;
}