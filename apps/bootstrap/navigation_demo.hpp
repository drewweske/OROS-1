#pragma once

#include "oros/ai/navigation_node_id.hpp"
#include "oros/ai/navigation_topology.hpp"
#include "oros/foundation/result.hpp"

namespace oros::bootstrap
{
    struct NavigationDemo final
    {
        ai::NavigationTopology topology;

        ai::NavigationNodeId
            start_node{};

        ai::NavigationNodeId
            complete_destination{};

        ai::NavigationNodeId
            partial_destination{};
    };

    [[nodiscard]]
    foundation::Result<NavigationDemo>
    create_navigation_demo();
}
