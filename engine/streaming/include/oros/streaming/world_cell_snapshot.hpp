#pragma once

#include "oros/foundation/result.hpp"
#include "oros/streaming/world_cell_key.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace oros::streaming
{
    inline constexpr std::uint32_t
        world_cell_snapshot_schema_version{1U};

    class WorldCellSnapshot final
    {
    public:
        [[nodiscard]]
        static foundation::Result<WorldCellSnapshot>
        create(
            WorldCellKey key,
            std::uint64_t revision,
            std::span<const std::byte> payload);

        WorldCellSnapshot(
            const WorldCellSnapshot&) = default;

        WorldCellSnapshot&
        operator=(
            const WorldCellSnapshot&) = default;

        WorldCellSnapshot(
            WorldCellSnapshot&&) noexcept = default;

        WorldCellSnapshot&
        operator=(
            WorldCellSnapshot&&) noexcept = default;

        [[nodiscard]]
        bool is_valid() const noexcept;

        [[nodiscard]]
        const WorldCellKey&
        key() const noexcept;

        [[nodiscard]]
        std::uint32_t
        schema_version() const noexcept;

        [[nodiscard]]
        std::uint64_t
        revision() const noexcept;

        [[nodiscard]]
        std::span<const std::byte>
        payload() const noexcept;

        [[nodiscard]]
        std::size_t
        byte_count() const noexcept;

        [[nodiscard]]
        bool empty() const noexcept;

        bool operator==(
            const WorldCellSnapshot&) const = default;

    private:
        WorldCellSnapshot(
            WorldCellKey key,
            std::uint64_t revision,
            std::vector<std::byte> payload) noexcept;

        WorldCellKey key_{};

        std::uint32_t schema_version_{
            world_cell_snapshot_schema_version
        };

        std::uint64_t revision_{};

        std::vector<std::byte>
            payload_{};
    };
}