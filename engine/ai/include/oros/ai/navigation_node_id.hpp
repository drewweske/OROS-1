#pragma once

#include "oros/world/world_position.hpp"

#include <compare>
#include <cstdint>

namespace oros::ai
{
    struct NavigationNodeId final
    {
        world::WorldCell cell{};
        std::uint64_t local_id{};

        [[nodiscard]]
        constexpr bool
        is_valid() const noexcept
        {
            return local_id != 0ULL;
        }

        [[nodiscard]]
        constexpr explicit
        operator bool() const noexcept
        {
            return is_valid();
        }

        auto operator<=>(
            const NavigationNodeId&)
            const noexcept = default;
    };

    inline constexpr NavigationNodeId
        invalid_navigation_node_id{};
}
