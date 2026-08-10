#include "oros/ai/navigation_traversal.hpp"

#include "oros/ai/navigation_cell_topology.hpp"
#include "oros/ai/navigation_node_record.hpp"
#include "oros/ai/navigation_portal_record.hpp"

#include <algorithm>
#include <new>
#include <span>
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
    }

    foundation::Result<
        std::vector<
            NavigationTraversalCandidate>>
    query_navigation_traversal_candidates(
        const NavigationTopology& topology,
        const NavigationNodeId source)
    {
        try
        {
            if (!source.is_valid())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Navigation traversal requires "
                    "a valid source node.");
            }

            const NavigationCellTopology*
                source_topology =
                    topology.
                        find_cell_topology(
                            source.cell);

            if (source_topology == nullptr)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Navigation traversal source "
                    "topology is unavailable.");
            }

            if (
                find_node_record(
                    *source_topology,
                    source) == nullptr)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Navigation traversal source "
                    "node is not present in "
                    "supplied topology.");
            }

            const std::span<
                const NavigationPortalRecord>
                portals =
                    source_topology->
                        portals_in_canonical_order();

            std::vector<
                NavigationTraversalCandidate>
                candidates{};

            candidates.reserve(
                portals.size());

            for (
                const NavigationPortalRecord&
                    portal :
                portals)
            {
                NavigationNodeId target{};

                if (
                    portal.first_node() ==
                    source)
                {
                    if (
                        !portal.
                            first_to_second())
                    {
                        continue;
                    }

                    target =
                        portal.second_node();
                }
                else if (
                    portal.second_node() ==
                    source)
                {
                    if (
                        !portal.
                            second_to_first())
                    {
                        continue;
                    }

                    target =
                        portal.first_node();
                }
                else
                {
                    continue;
                }

                const bool supplied =
                    topology.
                        find_cell_topology(
                            target.cell) !=
                    nullptr;

                candidates.push_back(
                    NavigationTraversalCandidate{
                        portal.id(),
                        target,
                        supplied
                            ? NavigationTraversalTargetKind::
                                supplied
                            : NavigationTraversalTargetKind::
                                unavailable
                    });
            }

            return candidates;
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Navigation traversal could not "
                "allocate candidate output.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "Navigation traversal failed "
                "while enumerating structural "
                "candidates.");
        }
    }
}
