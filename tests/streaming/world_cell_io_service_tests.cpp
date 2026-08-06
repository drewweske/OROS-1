#include "oros/streaming/world_cell_io_service.hpp"

#include "world_cell_io_shared_state.hpp"

#include "oros/foundation/error.hpp"
#include "oros/world/world_position.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <variant>

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
            << "\nWorld cell asynchronous I/O "
            << "service test summary: "
            << state.checks - state.failures
            << '/'
            << state.checks
            << " passed.\n";

        return
            state.failures == 0
                ? 0
                : 1;
    }

    class TemporaryDirectory final
    {
    public:
        TemporaryDirectory()
        {
            static std::atomic<std::uint64_t>
                sequence{
                    0ULL
                };

            const auto timestamp =
                std::chrono::steady_clock::now().
                    time_since_epoch().
                    count();

            const std::uint64_t suffix =
                sequence.fetch_add(
                    1ULL,
                    std::memory_order_relaxed);

            path_ =
                std::filesystem::
                    temp_directory_path() /
                (
                    "oros-world-cell-io-tests-" +
                    std::to_string(
                        timestamp) +
                    "-" +
                    std::to_string(
                        suffix)
                );

            std::error_code
                create_error{};

            std::filesystem::create_directories(
                path_,
                create_error);

            valid_ =
                !create_error;
        }

        ~TemporaryDirectory()
        {
            std::error_code
                remove_error{};

            std::filesystem::remove_all(
                path_,
                remove_error);
        }

        TemporaryDirectory(
            const TemporaryDirectory&) =
                delete;

        TemporaryDirectory&
        operator=(
            const TemporaryDirectory&) =
                delete;

        TemporaryDirectory(
            TemporaryDirectory&&) =
                delete;

        TemporaryDirectory&
        operator=(
            TemporaryDirectory&&) =
                delete;

        [[nodiscard]]
        bool is_valid() const noexcept
        {
            return valid_;
        }

        [[nodiscard]]
        const std::filesystem::path&
        path() const noexcept
        {
            return path_;
        }

    private:
        std::filesystem::path
            path_{};

        bool valid_{};
    };
}

int main()
{
    using namespace oros::streaming;
    using namespace oros::world;
    using oros::foundation::Error;
    using oros::foundation::ErrorCode;

    static_assert(
        !std::is_copy_constructible_v<
            WorldCellIoRequest>);

    static_assert(
        !std::is_copy_assignable_v<
            WorldCellIoRequest>);

    static_assert(
        std::is_move_constructible_v<
            WorldCellIoRequest>);

    static_assert(
        std::is_move_assignable_v<
            WorldCellIoRequest>);

    static_assert(
        !std::is_copy_constructible_v<
            WorldCellIoService>);

    static_assert(
        !std::is_move_constructible_v<
            WorldCellIoService>);

    TestState state{};

    check(
        state,
        !is_terminal(
            WorldCellIoRequestState::
                invalid),
        "Invalid request state is not terminal");

    check(
        state,
        !is_terminal(
            WorldCellIoRequestState::
                queued),
        "Queued request state is not terminal");

    check(
        state,
        !is_terminal(
            WorldCellIoRequestState::
                running),
        "Running request state is not terminal");

    check(
        state,
        is_terminal(
            WorldCellIoRequestState::
                succeeded),
        "Succeeded request state is terminal");

    check(
        state,
        is_terminal(
            WorldCellIoRequestState::
                failed),
        "Failed request state is terminal");

    check(
        state,
        is_terminal(
            WorldCellIoRequestState::
                cancelled),
        "Cancelled request state is terminal");

    WorldCellIoRequest
        invalid_request{};

    check(
        state,
        !invalid_request.is_valid(),
        "Default request handle is invalid");

    check(
        state,
        !static_cast<bool>(
            invalid_request),
        "Invalid request converts to false");

    check(
        state,
        invalid_request.id() == 0ULL,
        "Invalid request has identifier zero");

    check(
        state,
        invalid_request.kind() ==
            WorldCellIoRequestKind::invalid,
        "Invalid request has invalid kind");

    check(
        state,
        invalid_request.state() ==
            WorldCellIoRequestState::invalid,
        "Invalid request has invalid state");

    check(
        state,
        !invalid_request.is_terminal(),
        "Invalid request is not terminal");

    check(
        state,
        !invalid_request.
            cancellation_requested(),
        "Invalid request has no cancellation");

    check(
        state,
        !invalid_request.cancel(),
        "Invalid request cannot be cancelled");

    check_failure(
        state,
        invalid_request.wait(),
        ErrorCode::invalid_state,
        "Invalid request cannot be waited");

    check_failure(
        state,
        invalid_request.take_result(),
        ErrorCode::invalid_state,
        "Invalid request has no result");

    const WorldCellIoServiceLimits
        default_limits{};

    check(
        state,
        default_limits.is_valid(),
        "Default I/O service limits are valid");

    check(
        state,
        default_limits.
            maximum_queued_request_count ==
            256U,
        "Default queue capacity is 256");

    const WorldCellIoServiceLimits
        zero_queue_limits{
            0U
        };

    check(
        state,
        !zero_queue_limits.is_valid(),
        "Zero queue capacity is invalid");

    constexpr std::uint64_t
        world_namespace{
            0x4F524F5300000007ULL
        };

    constexpr std::uint64_t
        other_world_namespace{
            0x4F524F5300000008ULL
        };

    const WorldCellKey cell_key{
        world_namespace,
        WorldCell{
            -12,
            34,
            -56
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

    const std::array<
        WorldCellRevisionId,
        1U>
        manifest_entries{
            revision_id
        };

    const auto manifest_result =
        WorldCellSnapshotManifest::create(
            world_namespace,
            11ULL,
            manifest_entries);

    check(
        state,
        manifest_result.has_value(),
        "Manifest fixture is created");

    if (!manifest_result.has_value())
    {
        return finish(state);
    }

    const WorldCellSnapshotManifest manifest =
        manifest_result.value();

    const detail::WorldCellIoOperation
        snapshot_operation{
            detail::
                WorldCellSnapshotLoadOperation{
                    revision_id
                }
        };

    detail::WorldCellIoSharedState
        zero_identifier_state{
            0ULL,
            WorldCellIoRequestKind::
                load_snapshot,
            snapshot_operation
        };

    check(
        state,
        !zero_identifier_state.is_valid(),
        "Shared state rejects identifier zero");

    check(
        state,
        zero_identifier_state.state() ==
            WorldCellIoRequestState::invalid,
        "Invalid shared state remains invalid");

    detail::WorldCellIoSharedState
        mismatched_operation_state{
            1ULL,
            WorldCellIoRequestKind::
                load_snapshot,
            detail::WorldCellIoOperation{
                detail::
                    WorldCellManifestLoadOperation{
                        world_namespace,
                        manifest.
                            manifest_revision()
                    }
            }
        };

    check(
        state,
        !mismatched_operation_state.is_valid(),
        "Shared state rejects a mismatched operation");

    detail::WorldCellIoSharedState
        queued_cancellation_state{
            2ULL,
            WorldCellIoRequestKind::
                load_snapshot,
            snapshot_operation
        };

    check(
        state,
        queued_cancellation_state.is_valid(),
        "Valid shared state is accepted");

    check(
        state,
        queued_cancellation_state.state() ==
            WorldCellIoRequestState::queued,
        "Valid shared state begins queued");

    check(
        state,
        queued_cancellation_state.
            request_cancellation(),
        "Queued request accepts cancellation");

    check(
        state,
        queued_cancellation_state.
            cancellation_requested(),
        "Queued cancellation is recorded");

    check(
        state,
        queued_cancellation_state.state() ==
            WorldCellIoRequestState::
                cancelled,
        "Queued cancellation becomes terminal");

    check(
        state,
        queued_cancellation_state.
            is_terminal(),
        "Cancelled queued request is terminal");

    check(
        state,
        !queued_cancellation_state.
            request_cancellation(),
        "Terminal request rejects cancellation");

    check_failure(
        state,
        queued_cancellation_state.
            wait_until_terminal(),
        ErrorCode::invalid_state,
        "Waiting reports queued cancellation");

    check_failure(
        state,
        queued_cancellation_state.
            take_result(),
        ErrorCode::invalid_state,
        "Cancelled queued request has no result");

    detail::WorldCellIoSharedState
        running_cancellation_state{
            3ULL,
            WorldCellIoRequestKind::
                load_snapshot,
            snapshot_operation
        };

    check(
        state,
        running_cancellation_state.
            begin_running(),
        "Queued request begins running");

    check(
        state,
        running_cancellation_state.state() ==
            WorldCellIoRequestState::running,
        "Running transition is observable");

    check(
        state,
        running_cancellation_state.
            request_cancellation(),
        "Running request accepts cancellation");

    check(
        state,
        running_cancellation_state.state() ==
            WorldCellIoRequestState::running,
        "Running cancellation remains cooperative");

    running_cancellation_state.
        complete_success(
            WorldCellIoResultValue{
                std::in_place_type<
                    WorldCellSnapshot>,
                snapshot
            });

    check(
        state,
        running_cancellation_state.state() ==
            WorldCellIoRequestState::
                cancelled,
        "Cancelled running request discards success");

    check_failure(
        state,
        running_cancellation_state.
            wait_until_terminal(),
        ErrorCode::invalid_state,
        "Waiting reports running cancellation");

    detail::WorldCellIoSharedState
        successful_state{
            4ULL,
            WorldCellIoRequestKind::
                load_snapshot,
            snapshot_operation
        };

    check(
        state,
        successful_state.begin_running(),
        "Successful request begins running");

    successful_state.complete_success(
        WorldCellIoResultValue{
            std::in_place_type<
                WorldCellSnapshot>,
            snapshot
        });

    check(
        state,
        successful_state.state() ==
            WorldCellIoRequestState::
                succeeded,
        "Successful request reaches success");

    check(
        state,
        successful_state.
            wait_until_terminal().
            has_value(),
        "Waiting succeeds for successful request");

    auto successful_result =
        successful_state.take_result();

    check(
        state,
        successful_result.has_value(),
        "Successful shared state returns a result");

    check(
        state,
        successful_result.has_value() &&
            std::holds_alternative<
                WorldCellSnapshot>(
                    successful_result.value()),
        "Successful shared state returns snapshot type");

    check(
        state,
        successful_result.has_value() &&
            std::get<
                WorldCellSnapshot>(
                    successful_result.value()) ==
                snapshot,
        "Successful shared state preserves snapshot");

    check_failure(
        state,
        successful_state.take_result(),
        ErrorCode::invalid_state,
        "Shared-state result is consumed once");

    detail::WorldCellIoSharedState
        failed_state{
            5ULL,
            WorldCellIoRequestKind::
                load_snapshot,
            snapshot_operation
        };

    check(
        state,
        failed_state.begin_running(),
        "Failed request begins running");

    failed_state.complete_failure(
        Error{
            ErrorCode::not_found,
            "Fixture was not found."
        });

    check(
        state,
        failed_state.state() ==
            WorldCellIoRequestState::failed,
        "Failed request reaches failed state");

    check_failure(
        state,
        failed_state.wait_until_terminal(),
        ErrorCode::not_found,
        "Waiting preserves operation failure");

    check_failure(
        state,
        failed_state.take_result(),
        ErrorCode::not_found,
        "Result extraction preserves operation failure");

    detail::WorldCellIoSharedState
        normalized_failure_state{
            6ULL,
            WorldCellIoRequestKind::
                load_snapshot,
            snapshot_operation
        };

    check(
        state,
        normalized_failure_state.
            begin_running(),
        "Normalized failure request begins running");

    normalized_failure_state.
        complete_failure(
            Error{});

    check_failure(
        state,
        normalized_failure_state.
            wait_until_terminal(),
        ErrorCode::internal_failure,
        "Missing failure code is normalized");

    detail::WorldCellIoSharedState
        shutdown_queued_state{
            7ULL,
            WorldCellIoRequestKind::
                load_snapshot,
            snapshot_operation
        };

    shutdown_queued_state.
        cancel_for_shutdown();

    check(
        state,
        shutdown_queued_state.state() ==
            WorldCellIoRequestState::
                cancelled,
        "Shutdown cancels queued shared state");

    detail::WorldCellIoSharedState
        shutdown_running_state{
            8ULL,
            WorldCellIoRequestKind::
                load_snapshot,
            snapshot_operation
        };

    check(
        state,
        shutdown_running_state.
            begin_running(),
        "Shutdown fixture begins running");

    shutdown_running_state.
        cancel_for_shutdown();

    check(
        state,
        shutdown_running_state.
            cancellation_requested(),
        "Shutdown requests running cancellation");

    shutdown_running_state.
        complete_failure(
            Error{
                ErrorCode::
                    input_output_failure,
                "Fixture operation stopped."
            });

    check(
        state,
        shutdown_running_state.state() ==
            WorldCellIoRequestState::
                cancelled,
        "Running shutdown cancellation wins over failure");

    TemporaryDirectory
        temporary_directory{};

    check(
        state,
        temporary_directory.is_valid(),
        "Temporary I/O directory is created");

    if (!temporary_directory.is_valid())
    {
        return finish(state);
    }

    WorldCellIoService
        invalid_store_service{
            WorldCellPersistenceStore{
                std::filesystem::path{}
            }
        };

    check(
        state,
        !invalid_store_service.is_valid(),
        "Service with invalid store is invalid");

    check(
        state,
        !invalid_store_service.
            is_accepting_requests(),
        "Invalid service rejects requests");

    check(
        state,
        invalid_store_service.
            queued_request_count() == 0U,
        "Invalid service has no queued requests");

    check_failure(
        state,
        invalid_store_service.
            request_snapshot_load(
                revision_id),
        ErrorCode::invalid_state,
        "Invalid service rejects snapshot request");

    WorldCellIoService
        invalid_limits_service{
            WorldCellPersistenceStore{
                temporary_directory.path() /
                    "invalid-limits"
            },
            zero_queue_limits
        };

    check(
        state,
        !invalid_limits_service.is_valid(),
        "Service with zero queue capacity is invalid");

    WorldCellPersistenceStore store{
        temporary_directory.path() /
            "store"
    };

    check(
        state,
        store.is_valid(),
        "Persistence store fixture is valid");

    const auto stored_revision_result =
        store.store_snapshot(
            snapshot);

    check(
        state,
        stored_revision_result.has_value(),
        "Snapshot fixture is stored");

    check(
        state,
        stored_revision_result.has_value() &&
            stored_revision_result.value() ==
                revision_id,
        "Stored snapshot identity is preserved");

    const auto publish_status =
        store.publish_manifest(
            manifest);

    check(
        state,
        publish_status.has_value(),
        "Manifest fixture is published");

    if (!stored_revision_result.has_value() ||
        !publish_status.has_value())
    {
        return finish(state);
    }

    const WorldCellIoServiceLimits
        service_limits{
            8U
        };

    WorldCellIoService service{
        std::move(
            store),
        service_limits
    };

    check(
        state,
        service.is_valid(),
        "Service with valid store is valid");

    check(
        state,
        service.is_accepting_requests(),
        "Valid service accepts requests");

    check(
        state,
        service.limits().
            maximum_queued_request_count ==
            8U,
        "Service preserves queue capacity");

    check_failure(
        state,
        service.request_snapshot_load(
            invalid_world_cell_revision_id),
        ErrorCode::invalid_argument,
        "Snapshot request rejects invalid identity");

    check_failure(
        state,
        service.request_manifest_load(
            0ULL,
            manifest.manifest_revision()),
        ErrorCode::invalid_argument,
        "Manifest request rejects zero namespace");

    check_failure(
        state,
        service.request_manifest_load(
            world_namespace,
            0ULL),
        ErrorCode::invalid_argument,
        "Manifest request rejects zero revision");

    check_failure(
        state,
        service.
            request_current_manifest_load(
                0ULL),
        ErrorCode::invalid_argument,
        "Current manifest request rejects zero namespace");

    auto snapshot_request_result =
        service.request_snapshot_load(
            revision_id);

    auto manifest_request_result =
        service.request_manifest_load(
            world_namespace,
            manifest.manifest_revision());

    auto current_request_result =
        service.
            request_current_manifest_load(
                world_namespace);

    check(
        state,
        snapshot_request_result.has_value(),
        "Snapshot load request is accepted");

    check(
        state,
        manifest_request_result.has_value(),
        "Manifest load request is accepted");

    check(
        state,
        current_request_result.has_value(),
        "Current manifest request is accepted");

    if (!snapshot_request_result.has_value() ||
        !manifest_request_result.has_value() ||
        !current_request_result.has_value())
    {
        return finish(state);
    }

    WorldCellIoRequest snapshot_request{
        std::move(
            snapshot_request_result.value())
    };

    WorldCellIoRequest manifest_request{
        std::move(
            manifest_request_result.value())
    };

    WorldCellIoRequest current_request{
        std::move(
            current_request_result.value())
    };

    check(
        state,
        snapshot_request.is_valid() &&
            manifest_request.is_valid() &&
            current_request.is_valid(),
        "Accepted request handles are valid");

    check(
        state,
        snapshot_request.kind() ==
            WorldCellIoRequestKind::
                load_snapshot,
        "Snapshot request preserves its kind");

    check(
        state,
        manifest_request.kind() ==
            WorldCellIoRequestKind::
                load_manifest,
        "Manifest request preserves its kind");

    check(
        state,
        current_request.kind() ==
            WorldCellIoRequestKind::
                load_current_manifest,
        "Current request preserves its kind");

    check(
        state,
        snapshot_request.id() != 0ULL &&
            manifest_request.id() != 0ULL &&
            current_request.id() != 0ULL,
        "Accepted requests receive identifiers");

    check(
        state,
        snapshot_request.id() !=
                manifest_request.id() &&
            snapshot_request.id() !=
                current_request.id() &&
            manifest_request.id() !=
                current_request.id(),
        "Accepted request identifiers are unique");

    WorldCellIoRequest moved_snapshot_request{
        std::move(
            snapshot_request)
    };

    check(
        state,
        moved_snapshot_request.is_valid(),
        "Move construction preserves request handle");

    check(
        state,
        !snapshot_request.is_valid(),
        "Move construction invalidates source handle");

    moved_snapshot_request =
        std::move(
            moved_snapshot_request);

    check(
        state,
        moved_snapshot_request.is_valid(),
        "Self move assignment preserves request handle");

    check(
        state,
        moved_snapshot_request.wait().
            has_value(),
        "Snapshot request completes successfully");

    check(
        state,
        manifest_request.wait().
            has_value(),
        "Manifest request completes successfully");

    check(
        state,
        current_request.wait().
            has_value(),
        "Current manifest request completes successfully");

    check(
        state,
        moved_snapshot_request.state() ==
            WorldCellIoRequestState::
                succeeded,
        "Snapshot request reports success");

    check(
        state,
        manifest_request.state() ==
            WorldCellIoRequestState::
                succeeded,
        "Manifest request reports success");

    check(
        state,
        current_request.state() ==
            WorldCellIoRequestState::
                succeeded,
        "Current request reports success");

    check(
        state,
        moved_snapshot_request.is_terminal() &&
            manifest_request.is_terminal() &&
            current_request.is_terminal(),
        "Completed requests are terminal");

    auto loaded_snapshot_result =
        moved_snapshot_request.take_result();

    check(
        state,
        loaded_snapshot_result.has_value() &&
            std::holds_alternative<
                WorldCellSnapshot>(
                    loaded_snapshot_result.value()),
        "Snapshot request returns snapshot variant");

    check(
        state,
        loaded_snapshot_result.has_value() &&
            std::get<
                WorldCellSnapshot>(
                    loaded_snapshot_result.value()) ==
                snapshot,
        "Asynchronous snapshot load preserves data");

    check_failure(
        state,
        moved_snapshot_request.take_result(),
        ErrorCode::invalid_state,
        "Snapshot result can be consumed once");

    auto loaded_manifest_result =
        manifest_request.take_result();

    check(
        state,
        loaded_manifest_result.has_value() &&
            std::holds_alternative<
                WorldCellSnapshotManifest>(
                    loaded_manifest_result.value()),
        "Manifest request returns manifest variant");

    check(
        state,
        loaded_manifest_result.has_value() &&
            std::get<
                WorldCellSnapshotManifest>(
                    loaded_manifest_result.value()) ==
                manifest,
        "Asynchronous manifest load preserves data");

    auto loaded_current_result =
        current_request.take_result();

    check(
        state,
        loaded_current_result.has_value() &&
            std::holds_alternative<
                WorldCellSnapshotManifest>(
                    loaded_current_result.value()),
        "Current request returns manifest variant");

    check(
        state,
        loaded_current_result.has_value() &&
            std::get<
                WorldCellSnapshotManifest>(
                    loaded_current_result.value()) ==
                manifest,
        "Current manifest load preserves publication");

    const WorldCellRevisionId
        missing_revision{
            cell_key,
            999ULL
        };

    auto missing_snapshot_request_result =
        service.request_snapshot_load(
            missing_revision);

    check(
        state,
        missing_snapshot_request_result.
            has_value(),
        "Missing snapshot request is accepted");

    if (missing_snapshot_request_result.
        has_value())
    {
        WorldCellIoRequest missing_request{
            std::move(
                missing_snapshot_request_result.
                    value())
        };

        check_failure(
            state,
            missing_request.wait(),
            ErrorCode::not_found,
            "Missing snapshot failure is propagated");

        check(
            state,
            missing_request.state() ==
                WorldCellIoRequestState::
                    failed,
            "Missing snapshot request reports failure");

        check_failure(
            state,
            missing_request.take_result(),
            ErrorCode::not_found,
            "Missing snapshot result preserves failure");
    }

    auto missing_manifest_request_result =
        service.request_manifest_load(
            world_namespace,
            999ULL);

    check(
        state,
        missing_manifest_request_result.
            has_value(),
        "Missing manifest request is accepted");

    if (missing_manifest_request_result.
        has_value())
    {
        WorldCellIoRequest missing_request{
            std::move(
                missing_manifest_request_result.
                    value())
        };

        check_failure(
            state,
            missing_request.wait(),
            ErrorCode::not_found,
            "Missing manifest failure is propagated");

        check_failure(
            state,
            missing_request.take_result(),
            ErrorCode::not_found,
            "Missing manifest result preserves failure");
    }

    auto missing_current_request_result =
        service.
            request_current_manifest_load(
                other_world_namespace);

    check(
        state,
        missing_current_request_result.
            has_value(),
        "Missing current manifest request is accepted");

    if (missing_current_request_result.
        has_value())
    {
        WorldCellIoRequest missing_request{
            std::move(
                missing_current_request_result.
                    value())
        };

        check_failure(
            state,
            missing_request.wait(),
            ErrorCode::not_found,
            "Missing current manifest failure is propagated");

        check_failure(
            state,
            missing_request.take_result(),
            ErrorCode::not_found,
            "Missing current result preserves failure");
    }

    service.shutdown();

    check(
        state,
        !service.is_accepting_requests(),
        "Shutdown stops request acceptance");

    check(
        state,
        service.queued_request_count() ==
            0U,
        "Shutdown leaves no queued requests");

    check_failure(
        state,
        service.request_snapshot_load(
            revision_id),
        ErrorCode::invalid_state,
        "Shutdown service rejects new requests");

    service.shutdown();

    check(
        state,
        !service.is_accepting_requests(),
        "Repeated shutdown is safe");

    return finish(state);
}