#include "oros/ai/navigation_obstacle_overlay.hpp"

#include <algorithm>
#include <new>
#include <span>
#include <utility>
#include <vector>

namespace oros::ai
{
    namespace
    {
        [[nodiscard]]
        bool
        is_valid_direction(
            const NavigationPortalTraversalDirection
                direction)
            noexcept
        {
            switch (direction)
            {
            case NavigationPortalTraversalDirection::
                first_to_second:

            case NavigationPortalTraversalDirection::
                second_to_first:
                return true;
            }

            return false;
        }

        [[nodiscard]]
        int
        direction_rank(
            const NavigationPortalTraversalDirection
                direction)
            noexcept
        {
            switch (direction)
            {
            case NavigationPortalTraversalDirection::
                first_to_second:
                return 0;

            case NavigationPortalTraversalDirection::
                second_to_first:
                return 1;
            }

            return 2;
        }

        [[nodiscard]]
        bool
        block_less(
            const NavigationTraversalBlock&
                left,
            const NavigationTraversalBlock&
                right)
            noexcept
        {
            if (left.portal != right.portal)
            {
                return
                    left.portal <
                    right.portal;
            }

            return
                direction_rank(
                    left.direction) <
                direction_rank(
                    right.direction);
        }
    }

    foundation::Result<
        NavigationObstacleOverlay>
    NavigationObstacleOverlay::create(
        const std::span<
            const NavigationTraversalBlock>
            blocks)
    {
        try
        {
            for (
                const NavigationTraversalBlock&
                    block :
                blocks)
            {
                if (!block.portal.is_valid())
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "Navigation obstacle overlay "
                        "requires valid portal "
                        "identities.");
                }

                if (
                    !is_valid_direction(
                        block.direction))
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "Navigation obstacle overlay "
                        "contains an invalid portal "
                        "traversal direction.");
                }
            }

            std::vector<
                NavigationTraversalBlock>
                owned_blocks{
                    blocks.begin(),
                    blocks.end()
                };

            std::sort(
                owned_blocks.begin(),
                owned_blocks.end(),
                block_less);

            owned_blocks.erase(
                std::unique(
                    owned_blocks.begin(),
                    owned_blocks.end()),
                owned_blocks.end());

            return NavigationObstacleOverlay{
                std::move(
                    owned_blocks)
            };
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Navigation obstacle overlay "
                "could not allocate block data.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "Navigation obstacle overlay "
                "failed during construction.");
        }
    }

    bool
    NavigationObstacleOverlay::blocks(
        const NavigationPortalId portal,
        const NavigationPortalTraversalDirection
            direction)
        const noexcept
    {
        if (
            !portal.is_valid() ||
            !is_valid_direction(
                direction))
        {
            return false;
        }

        const NavigationTraversalBlock key{
            portal,
            direction
        };

        const auto iterator =
            std::lower_bound(
                blocks_.begin(),
                blocks_.end(),
                key,
                block_less);

        return
            iterator != blocks_.end() &&
            *iterator == key;
    }

    std::size_t
    NavigationObstacleOverlay::size()
        const noexcept
    {
        return blocks_.size();
    }

    bool
    NavigationObstacleOverlay::empty()
        const noexcept
    {
        return blocks_.empty();
    }

    std::span<
        const NavigationTraversalBlock>
    NavigationObstacleOverlay::
        blocks_in_canonical_order()
        const noexcept
    {
        return
            std::span<
                const NavigationTraversalBlock>{
                    blocks_
                };
    }

    NavigationObstacleOverlay::
        NavigationObstacleOverlay(
            std::vector<
                NavigationTraversalBlock>&&
                blocks)
        noexcept
        : blocks_{
              std::move(blocks)
          }
    {
    }
}
