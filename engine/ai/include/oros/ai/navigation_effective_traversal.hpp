#pragma once

#include "oros/ai/navigation_obstacle_overlay.hpp"
#include "oros/ai/navigation_topology.hpp"
#include "oros/ai/navigation_traversal.hpp"
#include "oros/foundation/result.hpp"

#include <vector>

namespace oros::ai
{
    [[nodiscard]]
    foundation::Result<
        std::vector<
            NavigationTraversalCandidate>>
    query_navigation_effective_traversal_candidates(
        const NavigationTopology& topology,
        const NavigationObstacleOverlay& overlay,
        NavigationNodeId source);
}
