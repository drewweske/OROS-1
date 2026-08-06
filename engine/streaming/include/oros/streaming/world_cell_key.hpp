#pragma once

#include "oros/foundation/result.hpp"
#include "oros/world/world_position.hpp"

#include <compare>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace oros::streaming
{
    struct WorldCellKey final
    {
        std::uint64_t world_namespace{};
        world::WorldCell cell{};

        [[nodiscard]]
        constexpr bool is_valid() const noexcept
        {
            return world_namespace != 0ULL;
        }

        [[nodiscard]]
        constexpr explicit operator bool() const noexcept
        {
            return is_valid();
        }

        auto operator<=>(
            const WorldCellKey&) const noexcept = default;
    };

    inline constexpr WorldCellKey
        invalid_world_cell_key{};

    struct WorldCellKeyHash final
    {
        [[nodiscard]]
        std::size_t operator()(
            WorldCellKey key) const noexcept;
    };

    [[nodiscard]]
    std::string to_string(
        WorldCellKey key);

    [[nodiscard]]
    foundation::Result<WorldCellKey>
    parse_world_cell_key(
        std::string_view text);
}