#include "navigation_demo.hpp"

#include "oros/ai/navigation_cell_topology.hpp"
#include "oros/ai/navigation_node_record.hpp"
#include "oros/ai/navigation_portal_record.hpp"
#include "oros/world/world_position.hpp"

#include <array>
#include <span>
#include <utility>

namespace oros::bootstrap
{
    foundation::Result<NavigationDemo>
    create_navigation_demo()
    {
        const world::WorldCell cell_a{
            0,
            0,
            0
        };

        const world::WorldCell cell_b{
            1,
            0,
            0
        };

        const world::WorldCell cell_u{
            2,
            0,
            0
        };

        const world::WorldCell
            portal_namespace{
                100,
                0,
                0
            };

        const ai::NavigationNodeId a1{
            cell_a,
            1ULL
        };

        const ai::NavigationNodeId a2{
            cell_a,
            2ULL
        };

        const ai::NavigationNodeId b1{
            cell_b,
            1ULL
        };

        const ai::NavigationNodeId u1{
            cell_u,
            1ULL
        };

        const ai::NavigationPortalId p1{
            portal_namespace,
            1ULL
        };

        const ai::NavigationPortalId p2{
            portal_namespace,
            2ULL
        };

        const ai::NavigationPortalId p3{
            portal_namespace,
            3ULL
        };

        const auto a1_position_result =
            world::WorldPosition::create(
                cell_a,
                world::LocalPosition{
                    -128.0,
                    0.0,
                    0.0
                });

        if (!a1_position_result.has_value())
        {
            return foundation::fail(
                a1_position_result.error().code,
                a1_position_result.error().message);
        }

        const auto a2_position_result =
            world::WorldPosition::create(
                cell_a,
                world::LocalPosition{
                    128.0,
                    0.0,
                    0.0
                });

        if (!a2_position_result.has_value())
        {
            return foundation::fail(
                a2_position_result.error().code,
                a2_position_result.error().message);
        }

        const auto b1_position_result =
            world::WorldPosition::create(
                cell_b,
                world::LocalPosition{
                    -128.0,
                    0.0,
                    0.0
                });

        if (!b1_position_result.has_value())
        {
            return foundation::fail(
                b1_position_result.error().code,
                b1_position_result.error().message);
        }

        const auto a1_record_result =
            ai::NavigationNodeRecord::create(
                a1,
                a1_position_result.value());

        if (!a1_record_result.has_value())
        {
            return foundation::fail(
                a1_record_result.error().code,
                a1_record_result.error().message);
        }

        const auto a2_record_result =
            ai::NavigationNodeRecord::create(
                a2,
                a2_position_result.value());

        if (!a2_record_result.has_value())
        {
            return foundation::fail(
                a2_record_result.error().code,
                a2_record_result.error().message);
        }

        const auto b1_record_result =
            ai::NavigationNodeRecord::create(
                b1,
                b1_position_result.value());

        if (!b1_record_result.has_value())
        {
            return foundation::fail(
                b1_record_result.error().code,
                b1_record_result.error().message);
        }

        const auto p1_record_result =
            ai::NavigationPortalRecord::create(
                p1,
                a1,
                a2,
                true,
                true);

        if (!p1_record_result.has_value())
        {
            return foundation::fail(
                p1_record_result.error().code,
                p1_record_result.error().message);
        }

        const auto p2_record_result =
            ai::NavigationPortalRecord::create(
                p2,
                a2,
                b1,
                true,
                true);

        if (!p2_record_result.has_value())
        {
            return foundation::fail(
                p2_record_result.error().code,
                p2_record_result.error().message);
        }

        const auto p3_record_result =
            ai::NavigationPortalRecord::create(
                p3,
                b1,
                u1,
                true,
                false);

        if (!p3_record_result.has_value())
        {
            return foundation::fail(
                p3_record_result.error().code,
                p3_record_result.error().message);
        }

        const std::array<
            ai::NavigationNodeRecord,
            2U>
            cell_a_nodes{
                a1_record_result.value(),
                a2_record_result.value()
            };

        const std::array<
            ai::NavigationPortalRecord,
            2U>
            cell_a_portals{
                p1_record_result.value(),
                p2_record_result.value()
            };

        const std::array<
            ai::NavigationNodeRecord,
            1U>
            cell_b_nodes{
                b1_record_result.value()
            };

        const std::array<
            ai::NavigationPortalRecord,
            2U>
            cell_b_portals{
                p2_record_result.value(),
                p3_record_result.value()
            };

        const auto cell_a_result =
            ai::NavigationCellTopology::create(
                cell_a,
                std::span<
                    const ai::NavigationNodeRecord>{
                        cell_a_nodes
                    },
                std::span<
                    const ai::NavigationPortalRecord>{
                        cell_a_portals
                    });

        if (!cell_a_result.has_value())
        {
            return foundation::fail(
                cell_a_result.error().code,
                cell_a_result.error().message);
        }

        const auto cell_b_result =
            ai::NavigationCellTopology::create(
                cell_b,
                std::span<
                    const ai::NavigationNodeRecord>{
                        cell_b_nodes
                    },
                std::span<
                    const ai::NavigationPortalRecord>{
                        cell_b_portals
                    });

        if (!cell_b_result.has_value())
        {
            return foundation::fail(
                cell_b_result.error().code,
                cell_b_result.error().message);
        }

        ai::NavigationTopology topology{};

        foundation::Status
            cell_a_status =
                topology.set_cell_topology(
                    cell_a_result.value());

        if (!cell_a_status.has_value())
        {
            return foundation::fail(
                cell_a_status.error().code,
                cell_a_status.error().message);
        }

        foundation::Status
            cell_b_status =
                topology.set_cell_topology(
                    cell_b_result.value());

        if (!cell_b_status.has_value())
        {
            return foundation::fail(
                cell_b_status.error().code,
                cell_b_status.error().message);
        }

        return NavigationDemo{
            std::move(topology),
            a1,
            b1,
            u1
        };
    }
}
