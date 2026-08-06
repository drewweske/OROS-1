#pragma once

#include "oros/foundation/result.hpp"
#include "oros/streaming/world_cell_revision_id.hpp"
#include "oros/streaming/world_cell_snapshot.hpp"
#include "oros/streaming/world_cell_snapshot_manifest.hpp"

#include <cstdint>
#include <filesystem>

namespace oros::streaming
{
    inline constexpr std::uint32_t
        world_cell_snapshot_file_format_version{1U};

    inline constexpr std::uint32_t
        world_cell_manifest_file_format_version{1U};

    inline constexpr std::uint32_t
        world_cell_manifest_pointer_format_version{1U};

    struct WorldCellPersistenceLimits final
    {
        std::uint64_t
            maximum_snapshot_payload_byte_count{
                64ULL *
                1024ULL *
                1024ULL
            };

        std::uint64_t
            maximum_manifest_entry_count{
                1'000'000ULL
            };

        [[nodiscard]]
        constexpr bool is_valid() const noexcept
        {
            return
                maximum_snapshot_payload_byte_count >
                    0ULL &&
                maximum_manifest_entry_count >
                    0ULL;
        }
    };

    class WorldCellPersistenceStore final
    {
    public:
        explicit WorldCellPersistenceStore(
            std::filesystem::path root_directory,
            WorldCellPersistenceLimits limits = {});

        ~WorldCellPersistenceStore() = default;

        WorldCellPersistenceStore(
            const WorldCellPersistenceStore&) =
                delete;

        WorldCellPersistenceStore&
        operator=(
            const WorldCellPersistenceStore&) =
                delete;

        WorldCellPersistenceStore(
            WorldCellPersistenceStore&& other)
            noexcept;

        WorldCellPersistenceStore&
        operator=(
            WorldCellPersistenceStore&& other)
            noexcept;

        [[nodiscard]]
        bool is_valid() const noexcept;

        [[nodiscard]]
        const std::filesystem::path&
        root_directory() const noexcept;

        [[nodiscard]]
        const WorldCellPersistenceLimits&
        limits() const noexcept;

        [[nodiscard]]
        foundation::Result<
            std::filesystem::path>
        snapshot_path_for(
            const WorldCellRevisionId&
                revision_id) const noexcept;

        [[nodiscard]]
        foundation::Result<
            std::filesystem::path>
        manifest_path_for(
            std::uint64_t world_namespace,
            std::uint64_t manifest_revision)
            const noexcept;

        [[nodiscard]]
        foundation::Result<
            std::filesystem::path>
        current_manifest_pointer_path_for(
            std::uint64_t world_namespace)
            const noexcept;

        [[nodiscard]]
        foundation::Result<
            WorldCellRevisionId>
        store_snapshot(
            const WorldCellSnapshot& snapshot)
            const noexcept;

        [[nodiscard]]
        foundation::Result<
            WorldCellSnapshot>
        load_snapshot(
            const WorldCellRevisionId&
                revision_id) const noexcept;

        [[nodiscard]]
        foundation::Result<bool>
        contains_snapshot(
            const WorldCellRevisionId&
                revision_id) const noexcept;

        [[nodiscard]]
        foundation::Status
        store_manifest(
            const WorldCellSnapshotManifest&
                manifest) const noexcept;

        [[nodiscard]]
        foundation::Result<
            WorldCellSnapshotManifest>
        load_manifest(
            std::uint64_t world_namespace,
            std::uint64_t manifest_revision)
            const noexcept;

        [[nodiscard]]
        foundation::Result<bool>
        contains_manifest(
            std::uint64_t world_namespace,
            std::uint64_t manifest_revision)
            const noexcept;

        [[nodiscard]]
        foundation::Status
        publish_manifest(
            const WorldCellSnapshotManifest&
                manifest) const noexcept;

        [[nodiscard]]
        foundation::Result<
            WorldCellSnapshotManifest>
        load_current_manifest(
            std::uint64_t world_namespace)
            const noexcept;

    private:
        std::filesystem::path
            root_directory_{};

        WorldCellPersistenceLimits
            limits_{};
    };
}