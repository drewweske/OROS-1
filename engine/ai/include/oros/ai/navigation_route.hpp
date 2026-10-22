#pragma once

#include "oros/ai/navigation_node_id.hpp"
#include "oros/ai/navigation_portal_id.hpp"
#include "oros/ai/navigation_topology.hpp"
#include "oros/foundation/result.hpp"

#include <optional>
#include <span>
#include <vector>

namespace oros::ai
{
    enum class NavigationRouteKind
    {
        complete,
        partial
    };

    class NavigationRoute final
    {
    public:
        [[nodiscard]]
        static foundation::Result<
            NavigationRoute>
        create_complete(
            const NavigationTopology&
                topology,
            NavigationNodeId
                requested_destination,
            std::span<
                const NavigationNodeId>
                nodes,
            std::span<
                const NavigationPortalId>
                portals);

        [[nodiscard]]
        static foundation::Result<
            NavigationRoute>
        create_partial(
            const NavigationTopology&
                topology,
            NavigationNodeId
                requested_destination,
            std::span<
                const NavigationNodeId>
                nodes,
            std::span<
                const NavigationPortalId>
                portals,
            NavigationPortalId
                continuation_portal);

        NavigationRoute(
            const NavigationRoute&) =
                default;

        NavigationRoute&
        operator=(
            const NavigationRoute&) =
                default;

        NavigationRoute(
            NavigationRoute&&)
            noexcept = default;

        NavigationRoute&
        operator=(
            NavigationRoute&&)
            noexcept = default;

        [[nodiscard]]
        NavigationRouteKind
        kind() const noexcept;

        [[nodiscard]]
        NavigationNodeId
        requested_destination()
            const noexcept;

        [[nodiscard]]
        NavigationNodeId
        start_node() const noexcept;

        [[nodiscard]]
        NavigationNodeId
        terminal_node() const noexcept;

        [[nodiscard]]
        std::span<
            const NavigationNodeId>
        nodes_in_traversal_order()
            const noexcept;

        [[nodiscard]]
        std::span<
            const NavigationPortalId>
        portals_in_traversal_order()
            const noexcept;

        [[nodiscard]]
        std::optional<
            NavigationPortalId>
        continuation_portal()
            const noexcept;

        [[nodiscard]]
        std::optional<
            NavigationNodeId>
        continuation_node()
            const noexcept;

        bool operator==(
            const NavigationRoute&)
            const = default;

    private:
        struct Continuation final
        {
            NavigationPortalId portal{};
            NavigationNodeId node{};

            bool operator==(
                const Continuation&)
                const = default;
        };

        NavigationRoute(
            NavigationNodeId
                requested_destination,
            std::vector<
                NavigationNodeId>&& nodes,
            std::vector<
                NavigationPortalId>&& portals,
            std::optional<
                Continuation> continuation)
            noexcept;

        NavigationNodeId
            requested_destination_;

        std::vector<
            NavigationNodeId>
            nodes_;

        std::vector<
            NavigationPortalId>
            portals_;

        std::optional<
            Continuation>
            continuation_;
    };
}
