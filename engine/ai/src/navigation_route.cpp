#include "oros/ai/navigation_route.hpp"

#include "oros/ai/navigation_cell_topology.hpp"
#include "oros/ai/navigation_node_record.hpp"
#include "oros/ai/navigation_portal_record.hpp"

#include <algorithm>
#include <expected>
#include <new>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace oros::ai
{
    namespace
    {
        [[nodiscard]]
        const NavigationNodeRecord*
        find_node_record(
            const NavigationCellTopology&
                topology,
            const NavigationNodeId id)
            noexcept
        {
            const std::span<
                const NavigationNodeRecord>
                nodes =
                    topology.
                        nodes_in_canonical_order();

            const auto iterator =
                std::lower_bound(
                    nodes.begin(),
                    nodes.end(),
                    id,
                    [](
                        const NavigationNodeRecord&
                            record,
                        const NavigationNodeId
                            candidate)
                        noexcept
                    {
                        return
                            record.id() <
                            candidate;
                    });

            if (
                iterator == nodes.end() ||
                iterator->id() != id)
            {
                return nullptr;
            }

            return &*iterator;
        }

        [[nodiscard]]
        const NavigationPortalRecord*
        find_portal_record(
            const NavigationCellTopology&
                topology,
            const NavigationPortalId id)
            noexcept
        {
            const std::span<
                const NavigationPortalRecord>
                portals =
                    topology.
                        portals_in_canonical_order();

            const auto iterator =
                std::lower_bound(
                    portals.begin(),
                    portals.end(),
                    id,
                    [](
                        const NavigationPortalRecord&
                            record,
                        const NavigationPortalId
                            candidate)
                        noexcept
                    {
                        return
                            record.id() <
                            candidate;
                    });

            if (
                iterator == portals.end() ||
                iterator->id() != id)
            {
                return nullptr;
            }

            return &*iterator;
        }

        [[nodiscard]]
        bool permits_traversal(
            const NavigationPortalRecord&
                portal,
            const NavigationNodeId from,
            const NavigationNodeId to)
            noexcept
        {
            if (
                portal.first_node() == from &&
                portal.second_node() == to)
            {
                return
                    portal.
                        first_to_second();
            }

            if (
                portal.second_node() == from &&
                portal.first_node() == to)
            {
                return
                    portal.
                        second_to_first();
            }

            return false;
        }

        [[nodiscard]]
        foundation::Status
        validate_route_prefix(
            const NavigationTopology&
                topology,
            const NavigationNodeId
                requested_destination,
            const std::span<
                const NavigationNodeId>
                nodes,
            const std::span<
                const NavigationPortalId>
                portals)
        {
            if (
                !requested_destination.
                    is_valid())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Navigation route requires "
                    "a valid requested destination.");
            }

            if (nodes.empty())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Navigation route requires "
                    "at least one known node.");
            }

            if (
                portals.size() !=
                nodes.size() - 1)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Navigation route portal count "
                    "must be exactly one fewer "
                    "than node count.");
            }

            for (
                const NavigationNodeId node :
                nodes)
            {
                if (!node.is_valid())
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "Navigation route contains "
                        "an invalid node identity.");
                }

                const NavigationCellTopology*
                    cell_topology =
                        topology.
                            find_cell_topology(
                                node.cell);

                if (cell_topology == nullptr)
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "Navigation route node "
                        "belongs to unavailable "
                        "topology.");
                }

                if (
                    find_node_record(
                        *cell_topology,
                        node) == nullptr)
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "Navigation route node is "
                        "not present in supplied "
                        "cell topology.");
                }
            }

            for (
                const NavigationPortalId portal :
                portals)
            {
                if (!portal.is_valid())
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "Navigation route contains "
                        "an invalid portal identity.");
                }
            }

            for (
                std::size_t index = 0;
                index < portals.size();
                ++index)
            {
                const NavigationNodeId from =
                    nodes[index];

                const NavigationNodeId to =
                    nodes[index + 1];

                const NavigationCellTopology*
                    from_topology =
                        topology.
                            find_cell_topology(
                                from.cell);

                if (from_topology == nullptr)
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "Navigation route traversal "
                        "origin topology is "
                        "unavailable.");
                }

                const NavigationPortalRecord*
                    portal_record =
                        find_portal_record(
                            *from_topology,
                            portals[index]);

                if (portal_record == nullptr)
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "Navigation route portal is "
                        "not present in traversal "
                        "origin topology.");
                }

                if (
                    !permits_traversal(
                        *portal_record,
                        from,
                        to))
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "Navigation route portal "
                        "does not permit the "
                        "requested node traversal.");
                }
            }

            return {};
        }
    }

    foundation::Result<
        NavigationRoute>
    NavigationRoute::create_complete(
        const NavigationTopology& topology,
        const NavigationNodeId
            requested_destination,
        const std::span<
            const NavigationNodeId> nodes,
        const std::span<
            const NavigationPortalId> portals)
    {
        try
        {
            foundation::Status validation =
                validate_route_prefix(
                    topology,
                    requested_destination,
                    nodes,
                    portals);

            if (!validation.has_value())
            {
                return
                    std::unexpected<
                        foundation::Error>{
                            std::move(
                                validation.error())
                        };
            }

            if (
                nodes.back() !=
                requested_destination)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Complete navigation route "
                    "must terminate at the "
                    "requested destination.");
            }

            std::vector<
                NavigationNodeId>
                owned_nodes{
                    nodes.begin(),
                    nodes.end()
                };

            std::vector<
                NavigationPortalId>
                owned_portals{
                    portals.begin(),
                    portals.end()
                };

            return NavigationRoute{
                requested_destination,
                std::move(
                    owned_nodes),
                std::move(
                    owned_portals),
                std::optional<
                    Continuation>{}
            };
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Navigation route could not "
                "allocate complete route data.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "Navigation route failed while "
                "creating a complete route.");
        }
    }

    foundation::Result<
        NavigationRoute>
    NavigationRoute::create_partial(
        const NavigationTopology& topology,
        const NavigationNodeId
            requested_destination,
        const std::span<
            const NavigationNodeId> nodes,
        const std::span<
            const NavigationPortalId> portals,
        const NavigationPortalId
            continuation_portal)
    {
        try
        {
            foundation::Status validation =
                validate_route_prefix(
                    topology,
                    requested_destination,
                    nodes,
                    portals);

            if (!validation.has_value())
            {
                return
                    std::unexpected<
                        foundation::Error>{
                            std::move(
                                validation.error())
                        };
            }

            if (
                !continuation_portal.
                    is_valid())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Partial navigation route "
                    "requires a valid continuation "
                    "portal.");
            }

            if (
                std::find(
                    nodes.begin(),
                    nodes.end(),
                    requested_destination) !=
                nodes.end())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Partial navigation route "
                    "cannot already contain the "
                    "requested destination.");
            }

            const NavigationNodeId terminal =
                nodes.back();

            const NavigationCellTopology*
                terminal_topology =
                    topology.
                        find_cell_topology(
                            terminal.cell);

            if (terminal_topology == nullptr)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Partial navigation route "
                    "terminal topology is "
                    "unavailable.");
            }

            const NavigationPortalRecord*
                continuation_record =
                    find_portal_record(
                        *terminal_topology,
                        continuation_portal);

            if (
                continuation_record ==
                nullptr)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Partial navigation route "
                    "continuation portal is not "
                    "present in terminal topology.");
            }

            NavigationNodeId
                continuation_node{};

            if (
                continuation_record->
                    first_node() ==
                terminal)
            {
                if (
                    !continuation_record->
                        first_to_second())
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "Partial navigation route "
                        "continuation direction is "
                        "not permitted.");
                }

                continuation_node =
                    continuation_record->
                        second_node();
            }
            else if (
                continuation_record->
                    second_node() ==
                terminal)
            {
                if (
                    !continuation_record->
                        second_to_first())
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "Partial navigation route "
                        "continuation direction is "
                        "not permitted.");
                }

                continuation_node =
                    continuation_record->
                        first_node();
            }
            else
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Partial navigation route "
                    "continuation portal does not "
                    "touch the terminal node.");
            }

            if (
                topology.
                    find_cell_topology(
                        continuation_node.cell) !=
                nullptr)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Partial navigation route "
                    "continuation must enter "
                    "unavailable topology.");
            }

            std::vector<
                NavigationNodeId>
                owned_nodes{
                    nodes.begin(),
                    nodes.end()
                };

            std::vector<
                NavigationPortalId>
                owned_portals{
                    portals.begin(),
                    portals.end()
                };

            return NavigationRoute{
                requested_destination,
                std::move(
                    owned_nodes),
                std::move(
                    owned_portals),
                Continuation{
                    continuation_portal,
                    continuation_node
                }
            };
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Navigation route could not "
                "allocate partial route data.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "Navigation route failed while "
                "creating a partial route.");
        }
    }

    NavigationRouteKind
    NavigationRoute::kind()
        const noexcept
    {
        return continuation_.has_value()
            ? NavigationRouteKind::partial
            : NavigationRouteKind::complete;
    }

    NavigationNodeId
    NavigationRoute::
        requested_destination()
        const noexcept
    {
        return requested_destination_;
    }

    NavigationNodeId
    NavigationRoute::start_node()
        const noexcept
    {
        return nodes_.front();
    }

    NavigationNodeId
    NavigationRoute::terminal_node()
        const noexcept
    {
        return nodes_.back();
    }

    std::span<
        const NavigationNodeId>
    NavigationRoute::
        nodes_in_traversal_order()
        const noexcept
    {
        return
            std::span<
                const NavigationNodeId>{
                    nodes_
                };
    }

    std::span<
        const NavigationPortalId>
    NavigationRoute::
        portals_in_traversal_order()
        const noexcept
    {
        return
            std::span<
                const NavigationPortalId>{
                    portals_
                };
    }

    std::optional<
        NavigationPortalId>
    NavigationRoute::
        continuation_portal()
        const noexcept
    {
        if (!continuation_.has_value())
        {
            return std::nullopt;
        }

        return continuation_->portal;
    }

    std::optional<
        NavigationNodeId>
    NavigationRoute::
        continuation_node()
        const noexcept
    {
        if (!continuation_.has_value())
        {
            return std::nullopt;
        }

        return continuation_->node;
    }

    NavigationRoute::
        NavigationRoute(
            const NavigationNodeId
                requested_destination,
            std::vector<
                NavigationNodeId>&& nodes,
            std::vector<
                NavigationPortalId>&& portals,
            std::optional<
                Continuation> continuation)
        noexcept
        : requested_destination_{
              requested_destination
          },
          nodes_{
              std::move(nodes)
          },
          portals_{
              std::move(portals)
          },
          continuation_{
              std::move(continuation)
          }
    {
    }
}
