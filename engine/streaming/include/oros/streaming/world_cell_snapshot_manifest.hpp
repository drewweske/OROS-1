#pragma once

#include "oros/foundation/result.hpp"
#include "oros/streaming/world_cell_revision_id.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace oros::streaming
{
    inline constexpr std::uint32_t
        world_cell_snapshot_manifest_schema_version{1U};

    class WorldCellSnapshotManifest final
    {
    public:
        [[nodiscard]]
        static foundation::Result<
            WorldCellSnapshotManifest>
        create(
            std::uint64_t world_namespace,
            std::uint64_t manifest_revision,
            std::span<
                const WorldCellRevisionId>
                entries);

        WorldCellSnapshotManifest(
            const WorldCellSnapshotManifest&) =
                default;

        WorldCellSnapshotManifest&
        operator=(
            const WorldCellSnapshotManifest&) =
                default;

        WorldCellSnapshotManifest(
            WorldCellSnapshotManifest&&)
            noexcept = default;

        WorldCellSnapshotManifest&
        operator=(
            WorldCellSnapshotManifest&&)
            noexcept = default;

        [[nodiscard]]
        bool is_valid() const noexcept;

        [[nodiscard]]
        std::uint64_t
        world_namespace() const noexcept;

        [[nodiscard]]
        std::uint32_t
        schema_version() const noexcept;

        [[nodiscard]]
        std::uint64_t
        manifest_revision() const noexcept;

        [[nodiscard]]
        std::span<
            const WorldCellRevisionId>
        entries() const noexcept;

        [[nodiscard]]
        const WorldCellRevisionId*
        find(
            WorldCellKey cell_key)
            const noexcept;

        [[nodiscard]]
        bool contains(
            WorldCellKey cell_key)
            const noexcept;

        [[nodiscard]]
        std::size_t size() const noexcept;

        [[nodiscard]]
        bool empty() const noexcept;

        bool operator==(
            const WorldCellSnapshotManifest&)
            const = default;

    private:
        WorldCellSnapshotManifest(
            std::uint64_t world_namespace,
            std::uint64_t manifest_revision,
            std::vector<
                WorldCellRevisionId>
                entries) noexcept;

        std::uint64_t world_namespace_{};

        std::uint32_t schema_version_{
            world_cell_snapshot_manifest_schema_version
        };

        std::uint64_t manifest_revision_{};

        std::vector<
            WorldCellRevisionId>
            entries_{};
    };
}