#pragma once

#include "oros/ai/navigation_effective_traversal.hpp"
#include "oros/ai/navigation_route.hpp"
#include "oros/foundation/result.hpp"

#include <optional>

namespace oros::ai
{
    enum class NavigationSearchResultKind
    {
        complete,
        partial,
        known_unreachable
    };

    class NavigationSearchResult final
    {
    public:
        NavigationSearchResult(
            const NavigationSearchResult&) =
                default;

        NavigationSearchResult&
        operator=(
            const NavigationSearchResult&) =
                default;

        NavigationSearchResult(
            NavigationSearchResult&&)
            noexcept = default;

        NavigationSearchResult&
        operator=(
            NavigationSearchResult&&)
            noexcept = default;

        [[nodiscard]]
        NavigationSearchResultKind
        kind() const noexcept;

        [[nodiscard]]
        NavigationNodeId
        start_node() const noexcept;

        [[nodiscard]]
        NavigationNodeId
        requested_destination()
            const noexcept;

        [[nodiscard]]
        const NavigationRoute*
        route() const noexcept;

        bool operator==(
            const NavigationSearchResult&)
            const = default;

    private:
        friend
        foundation::Result<
            NavigationSearchResult>
        search_navigation_route(
            const NavigationTopology&
                topology,
            const NavigationObstacleOverlay&
                overlay,
            NavigationNodeId start,
            NavigationNodeId
                requested_destination);

        NavigationSearchResult(
            NavigationNodeId start,
            NavigationNodeId
                requested_destination,
            std::optional<
                NavigationRoute>&& route)
            noexcept;

        NavigationNodeId
            start_{};

        NavigationNodeId
            requested_destination_{};

        std::optional<
            NavigationRoute>
            route_{};
    };

    [[nodiscard]]
    foundation::Result<
        NavigationSearchResult>
    search_navigation_route(
        const NavigationTopology& topology,
        const NavigationObstacleOverlay& overlay,
        NavigationNodeId start,
        NavigationNodeId requested_destination);
}
