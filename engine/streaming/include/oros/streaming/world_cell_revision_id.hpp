#pragma once

#include "oros/foundation/result.hpp"
#include "oros/streaming/world_cell_key.hpp"

#include <compare>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace oros::streaming
{
    struct WorldCellRevisionId final
    {
        WorldCellKey cell_key{};
        std::uint64_t revision{};

        [[nodiscard]]
        constexpr bool is_valid() const noexcept
        {
            return
                cell_key.is_valid() &&
                revision != 0ULL;
        }

        [[nodiscard]]
        constexpr explicit operator bool()
            const noexcept
        {
            return is_valid();
        }

        auto operator<=>(
            const WorldCellRevisionId&)
            const noexcept = default;
    };

    inline constexpr WorldCellRevisionId
        invalid_world_cell_revision_id{};

    struct WorldCellRevisionIdHash final
    {
        [[nodiscard]]
        std::size_t operator()(
            const WorldCellRevisionId&
                revision_id) const noexcept;
    };

    [[nodiscard]]
    std::string to_string(
        const WorldCellRevisionId&
            revision_id);

    [[nodiscard]]
    foundation::Result<WorldCellRevisionId>
    parse_world_cell_revision_id(
        std::string_view text);
}