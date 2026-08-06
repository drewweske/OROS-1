#include "oros/streaming/world_cell_io_service.hpp"
#include "oros/streaming/world_cell_persistence_store.hpp"
#include "oros/streaming/world_cell_residency.hpp"
#include "oros/streaming/world_cell_snapshot.hpp"
#include "oros/streaming/world_cell_snapshot_manifest.hpp"
#include "oros/streaming/world_cell_streaming_policy.hpp"

#include "oros/world/world_position.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <system_error>
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

    int finish(
        const TestState& state)
    {
        std::cout
            << "\nWorld cell streaming lifecycle "
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

    class TemporaryDirectoryCleanup final
    {
    public:
        explicit TemporaryDirectoryCleanup(
            std::filesystem::path directory)
            : directory_{
                  std::move(
                      directory)
              }
        {
        }

        ~TemporaryDirectoryCleanup()
        {
            std::error_code error{};

            std::filesystem::remove_all(
                directory_,
                error);
        }

        TemporaryDirectoryCleanup(
            const TemporaryDirectoryCleanup&) =
                delete;

        TemporaryDirectoryCleanup&
        operator=(
            const TemporaryDirectoryCleanup&) =
                delete;

        TemporaryDirectoryCleanup(
            TemporaryDirectoryCleanup&&) =
                delete;

        TemporaryDirectoryCleanup&
        operator=(
            TemporaryDirectoryCleanup&&) =
                delete;

    private:
        std::filesystem::path
            directory_{};
    };

    [[nodiscard]]
    std::filesystem::path
    create_temporary_directory()
    {
        std::error_code error{};

        const std::filesystem::path
            temporary_root =
                std::filesystem::
                    temp_directory_path(
                        error);

        if (error)
        {
            return {};
        }

        const auto nonce =
            std::chrono::steady_clock::now().
                time_since_epoch().
                count();

        const std::filesystem::path
            directory =
                temporary_root /
                (
                    "oros-world-cell-streaming-"
                    "lifecycle-" +
                    std::to_string(
                        nonce)
                );

        std::filesystem::create_directories(
            directory,
            error);

        if (error)
        {
            return {};
        }

        return directory;
    }
}

int main()
{
    using namespace oros::streaming;
    using namespace oros::world;

    TestState state{};

    constexpr WorldCellStreamingBudgets
        budgets{
            64U,
            2U,
            1U
        };

    WorldCellStreamingUsage usage{};

    check(
        state,
        budgets.is_valid(),
        "Lifecycle streaming budgets are valid");

    check(
        state,
        usage.is_within(
            budgets),
        "Initial streaming usage is within budgets");

    check(
        state,
        has_higher_streaming_priority(
            WorldCellStreamingPriority::
                critical,
            WorldCellStreamingPriority::
                normal),
        "Critical lifecycle work outranks normal work");

    constexpr std::uint64_t
        world_namespace{
            0x4F524F5300000007ULL
        };

    const WorldCellKey cell_key{
        world_namespace,
        WorldCell{
            1024,
            -2048,
            4096
        }
    };

    const std::array<std::byte, 16U>
        payload{
            std::byte{0x4F},
            std::byte{0x52},
            std::byte{0x4F},
            std::byte{0x53},
            std::byte{0x00},
            std::byte{0x07},
            std::byte{0x10},
            std::byte{0x20},
            std::byte{0x30},
            std::byte{0x40},
            std::byte{0x50},
            std::byte{0x60},
            std::byte{0x70},
            std::byte{0x80},
            std::byte{0x90},
            std::byte{0xA0}
        };

    const auto snapshot_result =
        WorldCellSnapshot::create(
            cell_key,
            17ULL,
            payload);

    check(
        state,
        snapshot_result.has_value(),
        "Lifecycle snapshot is created");

    if (!snapshot_result.has_value())
    {
        return finish(state);
    }

    const WorldCellSnapshot snapshot =
        snapshot_result.value();

    const WorldCellRevisionId
        revision_id{
            snapshot.key(),
            snapshot.revision()
        };

    check(
        state,
        revision_id.is_valid(),
        "Lifecycle snapshot has a stable revision identity");

    const std::array<
        WorldCellRevisionId,
        1U>
        manifest_entries{
            revision_id
        };

    const auto manifest_result =
        WorldCellSnapshotManifest::create(
            world_namespace,
            23ULL,
            manifest_entries);

    check(
        state,
        manifest_result.has_value(),
        "Lifecycle manifest is created");

    if (!manifest_result.has_value())
    {
        return finish(state);
    }

    const WorldCellSnapshotManifest
        manifest =
            manifest_result.value();

    check(
        state,
        manifest.contains(
            cell_key),
        "Lifecycle manifest contains the target cell");

    const std::filesystem::path
        temporary_directory =
            create_temporary_directory();

    check(
        state,
        !temporary_directory.empty(),
        "Lifecycle temporary directory is created");

    if (temporary_directory.empty())
    {
        return finish(state);
    }

    const TemporaryDirectoryCleanup
        cleanup{
            temporary_directory
        };

    WorldCellPersistenceStore store{
        temporary_directory
    };

    check(
        state,
        store.is_valid(),
        "Lifecycle persistence store is valid");

    const auto stored_revision_result =
        store.store_snapshot(
            snapshot);

    check(
        state,
        stored_revision_result.has_value(),
        "Lifecycle snapshot is stored");

    check(
        state,
        stored_revision_result.has_value() &&
            stored_revision_result.value() ==
                revision_id,
        "Stored snapshot preserves revision identity");

    const auto publish_result =
        store.publish_manifest(
            manifest);

    check(
        state,
        publish_result.has_value(),
        "Lifecycle manifest is published atomically");

    const auto contains_snapshot_result =
        store.contains_snapshot(
            revision_id);

    check(
        state,
        contains_snapshot_result.has_value() &&
            contains_snapshot_result.value(),
        "Persistence store contains the lifecycle snapshot");

    const auto contains_manifest_result =
        store.contains_manifest(
            world_namespace,
            manifest.manifest_revision());

    check(
        state,
        contains_manifest_result.has_value() &&
            contains_manifest_result.value(),
        "Persistence store contains the lifecycle manifest");

    WorldCellIoService io_service{
        std::move(
            store),
        WorldCellIoServiceLimits{
            budgets.
                queued_io_request_limit
        }
    };

    check(
        state,
        io_service.is_valid(),
        "Lifecycle asynchronous I/O service is valid");

    check(
        state,
        io_service.is_accepting_requests(),
        "Lifecycle I/O service accepts requests");

    check(
        state,
        usage.can_queue_io_request(
            budgets),
        "Manifest request fits the queued-I/O budget");

    auto manifest_request_result =
        io_service.
            request_current_manifest_load(
                world_namespace);

    check(
        state,
        manifest_request_result.has_value(),
        "Current manifest load is requested asynchronously");

    if (!manifest_request_result.has_value())
    {
        io_service.shutdown();

        return finish(state);
    }

    WorldCellIoRequest manifest_request =
        std::move(
            manifest_request_result.value());

    check(
        state,
        manifest_request.is_valid(),
        "Manifest request handle is valid");

    check(
        state,
        manifest_request.kind() ==
            WorldCellIoRequestKind::
                load_current_manifest,
        "Manifest request preserves its operation kind");

    usage.queued_io_request_count =
        1U;

    check(
        state,
        usage.is_within(
            budgets),
        "Queued manifest request remains within budgets");

    check(
        state,
        usage.can_activate_io_request(
            budgets),
        "Manifest request fits the active-I/O budget");

    usage.queued_io_request_count =
        0U;

    usage.active_io_request_count =
        1U;

    const auto manifest_wait_result =
        manifest_request.wait();

    check(
        state,
        manifest_wait_result.has_value(),
        "Asynchronous current manifest load completes");

    usage.active_io_request_count =
        0U;

    check(
        state,
        manifest_request.state() ==
            WorldCellIoRequestState::
                succeeded,
        "Current manifest request reports success");

    const auto loaded_manifest_result =
        manifest_request.take_result();

    check(
        state,
        loaded_manifest_result.has_value(),
        "Current manifest request returns a result");

    if (!loaded_manifest_result.has_value())
    {
        io_service.shutdown();

        return finish(state);
    }

    const WorldCellSnapshotManifest*
        loaded_manifest =
            std::get_if<
                WorldCellSnapshotManifest>(
                    &loaded_manifest_result.
                        value());

    check(
        state,
        loaded_manifest != nullptr,
        "Current manifest result has manifest type");

    if (loaded_manifest == nullptr)
    {
        io_service.shutdown();

        return finish(state);
    }

    check(
        state,
        *loaded_manifest ==
            manifest,
        "Asynchronous manifest load preserves publication");

    const WorldCellRevisionId*
        resolved_revision =
            loaded_manifest->find(
                cell_key);

    check(
        state,
        resolved_revision != nullptr,
        "Published manifest resolves the target cell");

    check(
        state,
        resolved_revision != nullptr &&
            *resolved_revision ==
                revision_id,
        "Manifest resolves the expected snapshot revision");

    if (resolved_revision == nullptr)
    {
        io_service.shutdown();

        return finish(state);
    }

    auto residency_result =
        WorldCellResidency::create(
            cell_key);

    check(
        state,
        residency_result.has_value(),
        "Lifecycle residency record is created");

    if (!residency_result.has_value())
    {
        io_service.shutdown();

        return finish(state);
    }

    WorldCellResidency residency =
        std::move(
            residency_result.value());

    check(
        state,
        residency.state() ==
            WorldCellResidencyState::
                unloaded,
        "Lifecycle cell begins unloaded");

    check(
        state,
        usage.can_queue_io_request(
            budgets),
        "Snapshot request fits the queued-I/O budget");

    auto snapshot_request_result =
        io_service.request_snapshot_load(
            *resolved_revision);

    check(
        state,
        snapshot_request_result.has_value(),
        "Snapshot load is requested asynchronously");

    if (!snapshot_request_result.has_value())
    {
        io_service.shutdown();

        return finish(state);
    }

    WorldCellIoRequest snapshot_request =
        std::move(
            snapshot_request_result.value());

    const std::uint64_t
        snapshot_request_id =
            snapshot_request.id();

    check(
        state,
        snapshot_request_id !=
            0ULL,
        "Snapshot request receives a stable identifier");

    check(
        state,
        residency.queue_load(
            *resolved_revision,
            snapshot_request_id).
            has_value(),
        "Residency records the queued snapshot load");

    check(
        state,
        residency.state() ==
            WorldCellResidencyState::
                load_queued,
        "Residency enters load-queued state");

    usage.queued_io_request_count =
        1U;

    check(
        state,
        usage.is_within(
            budgets),
        "Queued snapshot request remains within budgets");

    check(
        state,
        usage.can_activate_io_request(
            budgets),
        "Snapshot request fits the active-I/O budget");

    check(
        state,
        residency.begin_load(
            snapshot_request_id).
            has_value(),
        "Residency begins the snapshot load");

    usage.queued_io_request_count =
        0U;

    usage.active_io_request_count =
        1U;

    check(
        state,
        residency.state() ==
            WorldCellResidencyState::
                loading,
        "Residency enters loading state");

    const auto snapshot_wait_result =
        snapshot_request.wait();

    check(
        state,
        snapshot_wait_result.has_value(),
        "Asynchronous snapshot load completes");

    usage.active_io_request_count =
        0U;

    check(
        state,
        snapshot_request.state() ==
            WorldCellIoRequestState::
                succeeded,
        "Snapshot request reports success");

    auto loaded_snapshot_result =
        snapshot_request.take_result();

    check(
        state,
        loaded_snapshot_result.has_value(),
        "Snapshot request returns a result");

    if (!loaded_snapshot_result.has_value())
    {
        io_service.shutdown();

        return finish(state);
    }

    WorldCellSnapshot*
        loaded_snapshot =
            std::get_if<
                WorldCellSnapshot>(
                    &loaded_snapshot_result.
                        value());

    check(
        state,
        loaded_snapshot != nullptr,
        "Snapshot request result has snapshot type");

    if (loaded_snapshot == nullptr)
    {
        io_service.shutdown();

        return finish(state);
    }

    check(
        state,
        *loaded_snapshot ==
            snapshot,
        "Asynchronous snapshot load preserves cell data");

    check(
        state,
        usage.can_reserve_resident_bytes(
            loaded_snapshot->
                byte_count(),
            budgets),
        "Loaded snapshot fits the resident-memory budget");

    const std::size_t
        loaded_byte_count =
            loaded_snapshot->
                byte_count();

    check(
        state,
        residency.complete_load(
            snapshot_request_id,
            std::move(
                *loaded_snapshot)).
            has_value(),
        "Residency accepts the loaded snapshot");

    usage.resident_byte_count =
        loaded_byte_count;

    check(
        state,
        usage.is_within(
            budgets),
        "Resident snapshot remains within all budgets");

    check(
        state,
        residency.state() ==
            WorldCellResidencyState::
                resident,
        "Lifecycle cell becomes resident");

    check(
        state,
        residency.is_resident(),
        "Lifecycle cell exposes resident ownership");

    check(
        state,
        residency.resident_revision() ==
            revision_id,
        "Resident cell preserves snapshot identity");

    check(
        state,
        residency.snapshot() != nullptr &&
            *residency.snapshot() ==
                snapshot,
        "Resident cell preserves snapshot contents");

    check(
        state,
        residency.resident_byte_count() ==
            payload.size(),
        "Resident byte accounting matches the payload");

    check(
        state,
        residency.queue_unload().
            has_value(),
        "Lifecycle cell queues an unload");

    check(
        state,
        residency.state() ==
            WorldCellResidencyState::
                unload_queued,
        "Lifecycle cell enters unload-queued state");

    check(
        state,
        residency.is_resident() &&
            usage.resident_byte_count ==
                payload.size(),
        "Queued unload retains resident ownership and accounting");

    check(
        state,
        residency.begin_unload().
            has_value(),
        "Lifecycle cell begins unloading");

    check(
        state,
        residency.state() ==
            WorldCellResidencyState::
                unloading,
        "Lifecycle cell enters unloading state");

    check(
        state,
        residency.is_resident(),
        "Unloading retains data until completion");

    check(
        state,
        residency.complete_unload().
            has_value(),
        "Lifecycle cell completes unloading");

    usage.resident_byte_count =
        0U;

    check(
        state,
        residency.state() ==
            WorldCellResidencyState::
                unloaded,
        "Lifecycle cell returns to unloaded state");

    check(
        state,
        !residency.is_resident() &&
            residency.snapshot() ==
                nullptr,
        "Completed unload releases resident ownership");

    check(
        state,
        residency.resident_byte_count() ==
            0U,
        "Completed unload releases resident bytes");

    check(
        state,
        usage.is_within(
            budgets),
        "Final streaming usage is within budgets");

    check(
        state,
        usage.resident_byte_count ==
                0U &&
            usage.queued_io_request_count ==
                0U &&
            usage.active_io_request_count ==
                0U,
        "Full lifecycle returns all tracked usage to zero");

    io_service.shutdown();

    check(
        state,
        !io_service.
            is_accepting_requests(),
        "Lifecycle I/O service shuts down cleanly");

    check(
        state,
        io_service.
            queued_request_count() ==
            0U,
        "Lifecycle shutdown leaves no queued requests");

    return finish(state);
}