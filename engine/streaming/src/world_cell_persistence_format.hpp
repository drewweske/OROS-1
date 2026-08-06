#pragma once

#include "oros/foundation/result.hpp"
#include "oros/streaming/world_cell_persistence_store.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <vector>

namespace oros::streaming::detail
{
    inline constexpr std::uint64_t
        world_cell_snapshot_file_fixed_byte_count{
            72ULL
        };

    inline constexpr std::uint64_t
        world_cell_manifest_file_fixed_byte_count{
            48ULL
        };

    inline constexpr std::uint64_t
        world_cell_manifest_entry_file_byte_count{
            32ULL
        };

    inline constexpr std::uint64_t
        world_cell_manifest_pointer_file_byte_count{
            40ULL
        };

    [[nodiscard]]
    constexpr std::uint64_t
    maximum_snapshot_file_byte_count(
        const WorldCellPersistenceLimits&
            limits) noexcept
    {
        const std::uint64_t maximum =
            (std::numeric_limits<
                std::uint64_t>::max)();

        if (limits.
                maximum_snapshot_payload_byte_count >
            maximum -
                world_cell_snapshot_file_fixed_byte_count)
        {
            return maximum;
        }

        return
            world_cell_snapshot_file_fixed_byte_count +
            limits.
                maximum_snapshot_payload_byte_count;
    }

    [[nodiscard]]
    constexpr std::uint64_t
    maximum_manifest_file_byte_count(
        const WorldCellPersistenceLimits&
            limits) noexcept
    {
        const std::uint64_t maximum =
            (std::numeric_limits<
                std::uint64_t>::max)();

        if (limits.maximum_manifest_entry_count >
            (
                maximum -
                world_cell_manifest_file_fixed_byte_count
            ) /
            world_cell_manifest_entry_file_byte_count)
        {
            return maximum;
        }

        return
            world_cell_manifest_file_fixed_byte_count +
            limits.maximum_manifest_entry_count *
                world_cell_manifest_entry_file_byte_count;
    }

    [[nodiscard]]
    foundation::Result<std::vector<std::byte>>
    serialize_world_cell_snapshot(
        const WorldCellSnapshot& snapshot,
        const WorldCellPersistenceLimits&
            limits) noexcept;

    [[nodiscard]]
    foundation::Result<WorldCellSnapshot>
    deserialize_world_cell_snapshot(
        std::span<const std::byte> bytes,
        const WorldCellPersistenceLimits&
            limits) noexcept;

    [[nodiscard]]
    foundation::Result<std::vector<std::byte>>
    serialize_world_cell_snapshot_manifest(
        const WorldCellSnapshotManifest&
            manifest,
        const WorldCellPersistenceLimits&
            limits) noexcept;

    [[nodiscard]]
    foundation::Result<
        WorldCellSnapshotManifest>
    deserialize_world_cell_snapshot_manifest(
        std::span<const std::byte> bytes,
        const WorldCellPersistenceLimits&
            limits) noexcept;

    [[nodiscard]]
    foundation::Result<std::vector<std::byte>>
    serialize_world_cell_manifest_pointer(
        std::uint64_t world_namespace,
        std::uint64_t manifest_revision)
        noexcept;

    [[nodiscard]]
    foundation::Result<std::uint64_t>
    deserialize_world_cell_manifest_pointer(
        std::span<const std::byte> bytes,
        std::uint64_t expected_world_namespace)
        noexcept;
}