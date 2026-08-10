#pragma once

#include "oros/ai/navigation_node_id.hpp"
#include "oros/ai/navigation_portal_id.hpp"
#include "oros/ai/navigation_topology.hpp"
#include "oros/foundation/result.hpp"

#include <vector>

namespace oros::ai
{
    enum class NavigationTraversalTargetKind
    {
        supplied,
        unavailable
    };

    struct NavigationTraversalCandidate final
    {
        NavigationPortalId portal{};
        NavigationNodeId target{};
        NavigationTraversalTargetKind
            target_kind{
                NavigationTraversalTargetKind::
                    unavailable
            };

        bool operator==(
            const NavigationTraversalCandidate&)
            const = default;
    };

    [[nodiscard]]
    foundation::Result<
        std::vector<
            NavigationTraversalCandidate>>
    query_navigation_traversal_candidates(
        const NavigationTopology& topology,
        NavigationNodeId source);
}
