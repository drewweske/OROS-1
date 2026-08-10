#pragma once

#include "oros/ai/navigation_portal_id.hpp"
#include "oros/foundation/result.hpp"

#include <cstddef>
#include <span>
#include <vector>

namespace oros::ai
{
    enum class NavigationPortalTraversalDirection
    {
        first_to_second,
        second_to_first
    };

    struct NavigationTraversalBlock final
    {
        NavigationPortalId portal{};

        NavigationPortalTraversalDirection
            direction{
                NavigationPortalTraversalDirection::
                    first_to_second
            };

        bool operator==(
            const NavigationTraversalBlock&)
            const = default;
    };

    class NavigationObstacleOverlay final
    {
    public:
        NavigationObstacleOverlay() =
            default;

        NavigationObstacleOverlay(
            const NavigationObstacleOverlay&) =
                default;

        NavigationObstacleOverlay&
        operator=(
            const NavigationObstacleOverlay&) =
                default;

        NavigationObstacleOverlay(
            NavigationObstacleOverlay&&)
            noexcept = default;

        NavigationObstacleOverlay&
        operator=(
            NavigationObstacleOverlay&&)
            noexcept = default;

        [[nodiscard]]
        static foundation::Result<
            NavigationObstacleOverlay>
        create(
            std::span<
                const NavigationTraversalBlock>
                blocks);

        [[nodiscard]]
        bool
        blocks(
            NavigationPortalId portal,
            NavigationPortalTraversalDirection
                direction)
            const noexcept;

        [[nodiscard]]
        std::size_t
        size() const noexcept;

        [[nodiscard]]
        bool
        empty() const noexcept;

        [[nodiscard]]
        std::span<
            const NavigationTraversalBlock>
        blocks_in_canonical_order()
            const noexcept;

        bool operator==(
            const NavigationObstacleOverlay&)
            const = default;

    private:
        explicit
        NavigationObstacleOverlay(
            std::vector<
                NavigationTraversalBlock>&&
                blocks)
            noexcept;

        std::vector<
            NavigationTraversalBlock>
            blocks_{};
    };
}
