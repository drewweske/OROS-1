#include "oros/ai/navigation_cell_topology.hpp"

#include <algorithm>
#include <cstddef>
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
        node_record_less(
            const NavigationNodeRecord& left,
            const NavigationNodeRecord& right)
            noexcept
        {
            return left.id() < right.id();
        }

        [[nodiscard]]
        bool
        portal_record_less(
            const NavigationPortalRecord& left,
            const NavigationPortalRecord& right)
            noexcept
        {
            return left.id() < right.id();
        }

        [[nodiscard]]
        bool
        contains_node_id(
            const std::span<
                const NavigationNodeRecord>
                nodes,
            const NavigationNodeId id)
            noexcept
        {
            const auto iterator =
                std::lower_bound(
                    nodes.begin(),
                    nodes.end(),
                    id,
                    [](
                        const NavigationNodeRecord& node,
                        const NavigationNodeId candidate)
                        noexcept
                    {
                        return
                            node.id() <
                            candidate;
                    });

            return
                iterator != nodes.end() &&
                iterator->id() == id;
        }
    }

    foundation::Result<
        NavigationCellTopology>
    NavigationCellTopology::create(
        const world::WorldCell cell,
        const std::span<
            const NavigationNodeRecord>
            nodes,
        const std::span<
            const NavigationPortalRecord>
            portals)
    {
        try
        {
            std::vector<
                NavigationNodeRecord>
                owned_nodes{
                    nodes.begin(),
                    nodes.end()
                };

            std::vector<
                NavigationPortalRecord>
                owned_portals{
                    portals.begin(),
                    portals.end()
                };

            std::sort(
                owned_nodes.begin(),
                owned_nodes.end(),
                node_record_less);

            std::sort(
                owned_portals.begin(),
                owned_portals.end(),
                portal_record_less);

            for (
                std::size_t index = 0;
                index < owned_nodes.size();
                ++index)
            {
                if (
                    index > 0 &&
                    owned_nodes[index - 1].id() ==
                        owned_nodes[index].id())
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "Navigation cell topology "
                        "contains a duplicate node "
                        "identity.");
                }

                if (
                    owned_nodes[index].
                        id().cell !=
                    cell)
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "Navigation cell topology "
                        "contains a node owned by "
                        "another world cell.");
                }
            }

            const std::span<
                const NavigationNodeRecord>
                canonical_nodes{
                    owned_nodes
                };

            for (
                std::size_t index = 0;
                index < owned_portals.size();
                ++index)
            {
                const NavigationPortalRecord&
                    portal =
                        owned_portals[index];

                if (
                    index > 0 &&
                    owned_portals[index - 1].id() ==
                        portal.id())
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "Navigation cell topology "
                        "contains a duplicate portal "
                        "identity.");
                }

                const bool first_local =
                    portal.first_node().cell ==
                    cell;

                const bool second_local =
                    portal.second_node().cell ==
                    cell;

                if (
                    !first_local &&
                    !second_local)
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "Navigation cell topology "
                        "contains a portal that is "
                        "not incident to the cell.");
                }

                if (
                    first_local &&
                    !contains_node_id(
                        canonical_nodes,
                        portal.first_node()))
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "Navigation cell topology "
                        "is missing a local first "
                        "portal endpoint.");
                }

                if (
                    second_local &&
                    !contains_node_id(
                        canonical_nodes,
                        portal.second_node()))
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "Navigation cell topology "
                        "is missing a local second "
                        "portal endpoint.");
                }
            }

            return NavigationCellTopology{
                cell,
                std::move(
                    owned_nodes),
                std::move(
                    owned_portals)
            };
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Navigation cell topology could "
                "not allocate owned structural "
                "records.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "Navigation cell topology failed "
                "while constructing its canonical "
                "structural projection.");
        }
    }

    const world::WorldCell&
    NavigationCellTopology::cell()
        const noexcept
    {
        return cell_;
    }

    std::span<
        const NavigationNodeRecord>
    NavigationCellTopology::
        nodes_in_canonical_order()
        const noexcept
    {
        return
            std::span<
                const NavigationNodeRecord>{
                    nodes_
                };
    }

    std::span<
        const NavigationPortalRecord>
    NavigationCellTopology::
        portals_in_canonical_order()
        const noexcept
    {
        return
            std::span<
                const NavigationPortalRecord>{
                    portals_
                };
    }

    bool
    NavigationCellTopology::empty()
        const noexcept
    {
        return
            nodes_.empty() &&
            portals_.empty();
    }

    NavigationCellTopology::
        NavigationCellTopology(
            const world::WorldCell cell,
            std::vector<
                NavigationNodeRecord>&& nodes,
            std::vector<
                NavigationPortalRecord>&& portals)
            noexcept
        : cell_{cell},
          nodes_{
              std::move(
                  nodes)},
          portals_{
              std::move(
                  portals)}
    {
    }
}
