#include "oros/streaming/world_cell_residency.hpp"

#include "oros/foundation/error.hpp"
#include "oros/world/world_position.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
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

    template <typename T>
    void check_failure(
        TestState& state,
        const oros::foundation::Result<T>& result,
        const oros::foundation::ErrorCode
            expected_code,
        const std::string_view name)
    {
        check(
            state,
            !result.has_value() &&
                result.error().code ==
                    expected_code,
            name);
    }

    int finish(
        const TestState& state)
    {
        std::cout
            << "\nWorld cell residency test "
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
}

int main()
{
    using namespace oros::streaming;
    using namespace oros::world;
    using oros::foundation::Error;
    using oros::foundation::ErrorCode;

    static_assert(
        std::is_copy_constructible_v<
            WorldCellResidency>);

    static_assert(
        std::is_copy_assignable_v<
            WorldCellResidency>);

    static_assert(
        std::is_move_constructible_v<
            WorldCellResidency>);

    static_assert(
        std::is_move_assignable_v<
            WorldCellResidency>);

    TestState state{};

    check(
        state,
        !is_residency_transitioning(
            WorldCellResidencyState::
                invalid),
        "Invalid residency is not transitioning");

    check(
        state,
        !is_residency_transitioning(
            WorldCellResidencyState::
                unloaded),
        "Unloaded residency is not transitioning");

    check(
        state,
        is_residency_transitioning(
            WorldCellResidencyState::
                load_queued),
        "Load-queued residency is transitioning");

    check(
        state,
        is_residency_transitioning(
            WorldCellResidencyState::
                loading),
        "Loading residency is transitioning");

    check(
        state,
        !is_residency_transitioning(
            WorldCellResidencyState::
                resident),
        "Resident residency is not transitioning");

    check(
        state,
        is_residency_transitioning(
            WorldCellResidencyState::
                unload_queued),
        "Unload-queued residency is transitioning");

    check(
        state,
        is_residency_transitioning(
            WorldCellResidencyState::
                unloading),
        "Unloading residency is transitioning");

    check(
        state,
        !is_residency_transitioning(
            WorldCellResidencyState::
                failed),
        "Failed residency is not transitioning");

    check_failure(
        state,
        WorldCellResidency::create(
            invalid_world_cell_key),
        ErrorCode::invalid_argument,
        "Residency creation rejects an invalid cell key");

    constexpr std::uint64_t
        world_namespace{
            0x4F524F5300000007ULL
        };

    const WorldCellKey cell_key{
        world_namespace,
        WorldCell{
            -12,
            34,
            -56
        }
    };

    const WorldCellKey other_cell_key{
        world_namespace,
        WorldCell{
            10,
            20,
            30
        }
    };

    const std::array<std::byte, 8U>
        payload{
            std::byte{0x10},
            std::byte{0x20},
            std::byte{0x30},
            std::byte{0x40},
            std::byte{0x50},
            std::byte{0x60},
            std::byte{0x70},
            std::byte{0x80}
        };

    const auto snapshot_result =
        WorldCellSnapshot::create(
            cell_key,
            7ULL,
            payload);

    check(
        state,
        snapshot_result.has_value(),
        "Snapshot fixture is created");

    if (!snapshot_result.has_value())
    {
        return finish(state);
    }

    const WorldCellSnapshot snapshot =
        snapshot_result.value();

    const WorldCellRevisionId revision_id{
        snapshot.key(),
        snapshot.revision()
    };

    const auto wrong_revision_snapshot_result =
        WorldCellSnapshot::create(
            cell_key,
            8ULL,
            payload);

    check(
        state,
        wrong_revision_snapshot_result.
            has_value(),
        "Wrong-revision snapshot fixture is created");

    if (!wrong_revision_snapshot_result.
        has_value())
    {
        return finish(state);
    }

    const WorldCellSnapshot
        wrong_revision_snapshot =
            wrong_revision_snapshot_result.
                value();

    const auto wrong_cell_snapshot_result =
        WorldCellSnapshot::create(
            other_cell_key,
            7ULL,
            payload);

    check(
        state,
        wrong_cell_snapshot_result.
            has_value(),
        "Wrong-cell snapshot fixture is created");

    if (!wrong_cell_snapshot_result.
        has_value())
    {
        return finish(state);
    }

    const WorldCellSnapshot
        wrong_cell_snapshot =
            wrong_cell_snapshot_result.value();

    const WorldCellRevisionId
        wrong_cell_revision{
            other_cell_key,
            7ULL
        };

    auto residency_result =
        WorldCellResidency::create(
            cell_key);

    check(
        state,
        residency_result.has_value(),
        "Residency record is created");

    if (!residency_result.has_value())
    {
        return finish(state);
    }

    WorldCellResidency residency =
        std::move(
            residency_result.value());

    check(
        state,
        residency.is_valid(),
        "New residency record is valid");

    check(
        state,
        residency.cell_key() ==
            cell_key,
        "Residency preserves its cell key");

    check(
        state,
        residency.state() ==
            WorldCellResidencyState::
                unloaded,
        "New residency begins unloaded");

    check(
        state,
        !residency.target_revision().
            is_valid(),
        "Unloaded residency has no target revision");

    check(
        state,
        !residency.resident_revision().
            is_valid(),
        "Unloaded residency has no resident revision");

    check(
        state,
        residency.active_request_id() ==
            0ULL,
        "Unloaded residency has no active request");

    check(
        state,
        residency.failure().code ==
            ErrorCode::none,
        "Unloaded residency has no failure");

    check(
        state,
        residency.snapshot() == nullptr,
        "Unloaded residency has no snapshot");

    check(
        state,
        residency.resident_byte_count() ==
            0U,
        "Unloaded residency consumes no resident bytes");

    check(
        state,
        !residency.is_resident(),
        "Unloaded residency is not resident");

    check_failure(
        state,
        residency.begin_load(
            1ULL),
        ErrorCode::invalid_state,
        "Load cannot begin from unloaded state");

    check_failure(
        state,
        residency.complete_load(
            1ULL,
            snapshot),
        ErrorCode::invalid_state,
        "Load cannot complete from unloaded state");

    check_failure(
        state,
        residency.fail_load(
            1ULL,
            Error{
                ErrorCode::not_found,
                "Fixture failure."
            }),
        ErrorCode::invalid_state,
        "Load cannot fail from unloaded state");

    check_failure(
        state,
        residency.queue_unload(),
        ErrorCode::invalid_state,
        "Unload cannot queue from unloaded state");

    check_failure(
        state,
        residency.begin_unload(),
        ErrorCode::invalid_state,
        "Unload cannot begin from unloaded state");

    check_failure(
        state,
        residency.complete_unload(),
        ErrorCode::invalid_state,
        "Unload cannot complete from unloaded state");

    check_failure(
        state,
        residency.clear_failure(),
        ErrorCode::invalid_state,
        "Failure cannot clear from unloaded state");

    check_failure(
        state,
        residency.queue_load(
            invalid_world_cell_revision_id,
            41ULL),
        ErrorCode::invalid_argument,
        "Load queue rejects invalid revision identity");

    check_failure(
        state,
        residency.queue_load(
            wrong_cell_revision,
            41ULL),
        ErrorCode::invalid_argument,
        "Load queue rejects a different cell");

    check_failure(
        state,
        residency.queue_load(
            revision_id,
            0ULL),
        ErrorCode::invalid_argument,
        "Load queue rejects request identifier zero");

    check(
        state,
        residency.queue_load(
            revision_id,
            41ULL).
            has_value(),
        "Valid load is queued");

    check(
        state,
        residency.is_valid(),
        "Load-queued residency remains valid");

    check(
        state,
        residency.state() ==
            WorldCellResidencyState::
                load_queued,
        "Queued load enters load-queued state");

    check(
        state,
        residency.target_revision() ==
            revision_id,
        "Queued load preserves target revision");

    check(
        state,
        residency.active_request_id() ==
            41ULL,
        "Queued load preserves request identifier");

    check(
        state,
        residency.snapshot() == nullptr &&
            !residency.is_resident(),
        "Queued load has no resident snapshot");

    check_failure(
        state,
        residency.queue_load(
            revision_id,
            42ULL),
        ErrorCode::invalid_state,
        "Second load cannot queue while one is queued");

    check_failure(
        state,
        residency.begin_load(
            42ULL),
        ErrorCode::invalid_argument,
        "Load begin rejects a different request identifier");

    check(
        state,
        residency.begin_load(
            41ULL).
            has_value(),
        "Queued load begins");

    check(
        state,
        residency.state() ==
            WorldCellResidencyState::
                loading,
        "Load begin enters loading state");

    check(
        state,
        residency.is_valid(),
        "Loading residency remains valid");

    check_failure(
        state,
        residency.complete_load(
            42ULL,
            snapshot),
        ErrorCode::invalid_argument,
        "Load completion rejects a different request identifier");

    check_failure(
        state,
        residency.complete_load(
            41ULL,
            wrong_revision_snapshot),
        ErrorCode::invalid_argument,
        "Load completion rejects a different revision");

    check_failure(
        state,
        residency.complete_load(
            41ULL,
            wrong_cell_snapshot),
        ErrorCode::invalid_argument,
        "Load completion rejects a different cell");

    check_failure(
        state,
        residency.fail_load(
            42ULL,
            Error{
                ErrorCode::not_found,
                "Wrong request."
            }),
        ErrorCode::invalid_argument,
        "Load failure rejects a different request identifier");

    check(
        state,
        residency.complete_load(
            41ULL,
            snapshot).
            has_value(),
        "Matching snapshot completes the load");

    check(
        state,
        residency.is_valid(),
        "Resident record satisfies its invariants");

    check(
        state,
        residency.state() ==
            WorldCellResidencyState::
                resident,
        "Completed load enters resident state");

    check(
        state,
        !residency.target_revision().
            is_valid(),
        "Resident state clears target revision");

    check(
        state,
        residency.resident_revision() ==
            revision_id,
        "Resident state preserves loaded revision");

    check(
        state,
        residency.active_request_id() ==
            0ULL,
        "Resident state clears active request");

    check(
        state,
        residency.failure().code ==
            ErrorCode::none,
        "Resident state has no failure");

    check(
        state,
        residency.snapshot() != nullptr,
        "Resident state exposes its snapshot");

    check(
        state,
        residency.snapshot() != nullptr &&
            *residency.snapshot() ==
                snapshot,
        "Resident state preserves snapshot data");

    check(
        state,
        residency.resident_byte_count() ==
            snapshot.byte_count(),
        "Resident byte count matches snapshot payload");

    check(
        state,
        residency.is_resident(),
        "Completed load is resident");

    check_failure(
        state,
        residency.queue_load(
            revision_id,
            50ULL),
        ErrorCode::invalid_state,
        "Load cannot queue while already resident");

    check_failure(
        state,
        residency.begin_load(
            50ULL),
        ErrorCode::invalid_state,
        "Load cannot begin while resident");

    check_failure(
        state,
        residency.fail_load(
            50ULL,
            Error{
                ErrorCode::not_found,
                "Invalid resident failure."
            }),
        ErrorCode::invalid_state,
        "Load cannot fail while resident");

    check_failure(
        state,
        residency.begin_unload(),
        ErrorCode::invalid_state,
        "Unload cannot begin before being queued");

    check_failure(
        state,
        residency.complete_unload(),
        ErrorCode::invalid_state,
        "Unload cannot complete before beginning");

    check_failure(
        state,
        residency.clear_failure(),
        ErrorCode::invalid_state,
        "Failure cannot clear while resident");

    check(
        state,
        residency.queue_unload().
            has_value(),
        "Resident cell queues an unload");

    check(
        state,
        residency.state() ==
            WorldCellResidencyState::
                unload_queued,
        "Queued unload enters unload-queued state");

    check(
        state,
        residency.is_resident(),
        "Queued unload retains resident data");

    check(
        state,
        residency.snapshot() != nullptr &&
            residency.resident_byte_count() ==
                snapshot.byte_count(),
        "Queued unload retains snapshot ownership");

    check_failure(
        state,
        residency.queue_unload(),
        ErrorCode::invalid_state,
        "Second unload cannot queue");

    check(
        state,
        residency.begin_unload().
            has_value(),
        "Queued unload begins");

    check(
        state,
        residency.state() ==
            WorldCellResidencyState::
                unloading,
        "Unload begin enters unloading state");

    check(
        state,
        residency.is_resident(),
        "Unloading retains resident data until completion");

    check(
        state,
        residency.complete_unload().
            has_value(),
        "Unload completes");

    check(
        state,
        residency.is_valid(),
        "Unloaded record satisfies its invariants");

    check(
        state,
        residency.state() ==
            WorldCellResidencyState::
                unloaded,
        "Completed unload returns to unloaded state");

    check(
        state,
        !residency.is_resident() &&
            residency.snapshot() == nullptr,
        "Completed unload releases resident data");

    check(
        state,
        residency.resident_byte_count() ==
            0U,
        "Completed unload releases resident bytes");

    check(
        state,
        residency.queue_load(
            revision_id,
            51ULL).
            has_value(),
        "Second load can queue after unload");

    check(
        state,
        residency.fail_load(
            51ULL,
            Error{}).
            has_value(),
        "Queued load can fail before beginning");

    check(
        state,
        residency.is_valid(),
        "Failed residency satisfies its invariants");

    check(
        state,
        residency.state() ==
            WorldCellResidencyState::
                failed,
        "Load failure enters failed state");

    check(
        state,
        residency.target_revision() ==
            revision_id,
        "Failed state preserves attempted revision");

    check(
        state,
        residency.active_request_id() ==
            0ULL,
        "Failed state clears active request");

    check(
        state,
        residency.failure().code ==
            ErrorCode::internal_failure,
        "Missing failure code is normalized");

    check(
        state,
        residency.snapshot() == nullptr &&
            !residency.is_resident(),
        "Failed load owns no resident snapshot");

    check_failure(
        state,
        residency.queue_load(
            revision_id,
            52ULL),
        ErrorCode::invalid_state,
        "Load cannot queue until failure is cleared");

    check(
        state,
        residency.clear_failure().
            has_value(),
        "Failed state can be cleared");

    check(
        state,
        residency.state() ==
            WorldCellResidencyState::
                unloaded,
        "Cleared failure returns to unloaded state");

    check(
        state,
        residency.failure().code ==
            ErrorCode::none &&
            !residency.target_revision().
                is_valid(),
        "Cleared failure removes failure metadata");

    check(
        state,
        residency.queue_load(
            revision_id,
            53ULL).
            has_value(),
        "Load can queue after clearing failure");

    check(
        state,
        residency.begin_load(
            53ULL).
            has_value(),
        "Failure fixture begins loading");

    check(
        state,
        residency.fail_load(
            53ULL,
            Error{
                ErrorCode::not_found,
                "Snapshot fixture was not found."
            }).
            has_value(),
        "Running load can fail");

    check(
        state,
        residency.state() ==
            WorldCellResidencyState::
                failed,
        "Running failure enters failed state");

    check(
        state,
        residency.failure().code ==
            ErrorCode::not_found,
        "Running failure preserves its error code");

    check(
        state,
        residency.failure().message ==
            "Snapshot fixture was not found.",
        "Running failure preserves its error message");

    check(
        state,
        residency.clear_failure().
            has_value(),
        "Running failure can be cleared");

    check(
        state,
        residency.state() ==
            WorldCellResidencyState::
                unloaded &&
            residency.is_valid(),
        "Cleared running failure restores valid unloaded state");

    return finish(state);
}