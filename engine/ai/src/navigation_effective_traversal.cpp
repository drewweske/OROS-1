#include "oros/ai/navigation_effective_traversal.hpp"

#include "oros/ai/navigation_cell_topology.hpp"
#include "oros/ai/navigation_portal_record.hpp"

#include <algorithm>
#include <cstddef>
#include <expected>
#include <new>
#include <span>
#include <utility>
#include <vector>

namespace oros::ai
{
    namespace
    {
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
        bool
        try_derive_direction(
            const NavigationPortalRecord&
                portal,
            const NavigationNodeId source,
            const NavigationNodeId target,
            NavigationPortalTraversalDirection&
                direction)
            noexcept
        {
            if (
                portal.first_node() == source &&
                portal.second_node() == target)
            {
                direction =
                    NavigationPortalTraversalDirection::
                        first_to_second;

                return true;
            }

            if (
                portal.second_node() == source &&
                portal.first_node() == target)
            {
                direction =
                    NavigationPortalTraversalDirection::
                        second_to_first;

                return true;
            }

            return false;
        }
    }

    foundation::Result<
        std::vector<
            NavigationTraversalCandidate>>
    query_navigation_effective_traversal_candidates(
        const NavigationTopology& topology,
        const NavigationObstacleOverlay& overlay,
        const NavigationNodeId source)
    {
        try
        {
            auto structural_result =
                query_navigation_traversal_candidates(
                    topology,
                    source);

            if (!structural_result.has_value())
            {
                return
                    std::unexpected<
                        foundation::Error>{
                            std::move(
                                structural_result.error())
                        };
            }

            std::vector<
                NavigationTraversalCandidate>
                candidates =
                    std::move(
                        structural_result.value());

            const NavigationCellTopology*
                source_topology =
                    topology.
                        find_cell_topology(
                            source.cell);

            if (source_topology == nullptr)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "Effective navigation traversal "
                    "lost structurally validated "
                    "source topology.");
            }

            std::size_t write_index = 0;

            for (
                std::size_t read_index = 0;
                read_index < candidates.size();
                ++read_index)
            {
                NavigationTraversalCandidate&
                    candidate =
                        candidates[read_index];

                const NavigationPortalRecord*
                    portal =
                        find_portal_record(
                            *source_topology,
                            candidate.portal);

                if (portal == nullptr)
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            internal_failure,
                        "Effective navigation traversal "
                        "could not recover a "
                        "structurally validated portal.");
                }

                NavigationPortalTraversalDirection
                    direction =
                        NavigationPortalTraversalDirection::
                            first_to_second;

                if (
                    !try_derive_direction(
                        *portal,
                        source,
                        candidate.target,
                        direction))
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            internal_failure,
                        "Effective navigation traversal "
                        "found structural candidate "
                        "endpoints inconsistent with "
                        "their portal.");
                }

                if (
                    overlay.blocks(
                        candidate.portal,
                        direction))
                {
                    continue;
                }

                if (write_index != read_index)
                {
                    candidates[write_index] =
                        std::move(candidate);
                }

                ++write_index;
            }

            candidates.resize(
                write_index);

            return candidates;
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Effective navigation traversal "
                "encountered an allocation failure.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "Effective navigation traversal "
                "failed while composing traversal "
                "constraints.");
        }
    }
}
