#include "oros/ai/navigation_cell_topology.hpp"

#include <iostream>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace
{
    struct TestState final
    {
        int checks{};
        int failures{};
    };

    void check(
        TestState& state,
        const bool condition,
        const std::string_view name)
    {
        ++state.checks;

        if (condition)
        {
            std::cout
                << "[pass] "
                << name
                << '\n';

            return;
        }

        ++state.failures;

        std::cerr
            << "[fail] "
            << name
            << '\n';
    }
}

int main()
{
    using namespace oros::ai;
    using namespace oros::foundation;
    using namespace oros::world;

    static_assert(
        !std::is_default_constructible_v<
            NavigationCellTopology>);

    static_assert(
        std::is_copy_constructible_v<
            NavigationCellTopology>);

    static_assert(
        std::is_copy_assignable_v<
            NavigationCellTopology>);

    static_assert(
        std::is_nothrow_move_constructible_v<
            NavigationCellTopology>);

    static_assert(
        std::is_nothrow_move_assignable_v<
            NavigationCellTopology>);

    static_assert(
        std::is_same_v<
            decltype(
                NavigationCellTopology::create(
                    std::declval<
                        WorldCell>(),
                    std::declval<
                        std::span<
                            const NavigationNodeRecord>>(),
                    std::declval<
                        std::span<
                            const NavigationPortalRecord>>())),
            Result<
                NavigationCellTopology>>);

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const NavigationCellTopology&>().
                    cell()),
            const WorldCell&>);

    static_assert(
        noexcept(
            std::declval<
                const NavigationCellTopology&>().
                cell()));

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const NavigationCellTopology&>().
                    nodes_in_canonical_order()),
            std::span<
                const NavigationNodeRecord>>);

    static_assert(
        noexcept(
            std::declval<
                const NavigationCellTopology&>().
                nodes_in_canonical_order()));

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const NavigationCellTopology&>().
                    portals_in_canonical_order()),
            std::span<
                const NavigationPortalRecord>>);

    static_assert(
        noexcept(
            std::declval<
                const NavigationCellTopology&>().
                portals_in_canonical_order()));

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const NavigationCellTopology&>().
                    empty()),
            bool>);

    static_assert(
        noexcept(
            std::declval<
                const NavigationCellTopology&>().
                empty()));

    TestState state{};

    const WorldCell
        cell{
            10,
            -20,
            30
        };

    const WorldCell
        remote_cell{
            11,
            -20,
            30
        };

    const WorldCell
        other_cell{
            -5,
            6,
            7
        };

    const auto empty_result =
        NavigationCellTopology::create(
            cell,
            std::span<
                const NavigationNodeRecord>{},
            std::span<
                const NavigationPortalRecord>{});

    check(
        state,
        empty_result.has_value(),
        "Empty supplied cell succeeds");

    if (!empty_result.has_value())
    {
        return 1;
    }

    check(
        state,
        empty_result.value().empty(),
        "empty() is true for zero-node zero-portal supplied topology");

    check(
        state,
        empty_result.value().cell() ==
            cell,
        "Cell accessor returns the exact coverage cell");

    const auto position_10_result =
        WorldPosition::create(
            cell,
            LocalPosition{
                10.25,
                1.0,
                -2.0
            });

    const auto position_20_result =
        WorldPosition::create(
            cell,
            LocalPosition{
                20.5,
                2.0,
                -3.0
            });

    const auto position_30_result =
        WorldPosition::create(
            cell,
            LocalPosition{
                30.75,
                3.0,
                -4.0
            });

    if (
        !position_10_result.has_value() ||
        !position_20_result.has_value() ||
        !position_30_result.has_value())
    {
        return 1;
    }

    const NavigationNodeId
        node_10_id{
            cell,
            10ULL
        };

    const NavigationNodeId
        node_20_id{
            cell,
            20ULL
        };

    const NavigationNodeId
        node_30_id{
            cell,
            30ULL
        };

    const auto node_10_result =
        NavigationNodeRecord::create(
            node_10_id,
            position_10_result.value());

    const auto node_20_result =
        NavigationNodeRecord::create(
            node_20_id,
            position_20_result.value());

    const auto node_30_result =
        NavigationNodeRecord::create(
            node_30_id,
            position_30_result.value());

    if (
        !node_10_result.has_value() ||
        !node_20_result.has_value() ||
        !node_30_result.has_value())
    {
        return 1;
    }

    const NavigationPortalId
        low_portal_id{
            WorldCell{
                -500,
                0,
                0
            },
            7ULL
        };

    const NavigationPortalId
        high_portal_id{
            WorldCell{
                500,
                0,
                0
            },
            8ULL
        };

    const auto low_portal_result =
        NavigationPortalRecord::create(
            low_portal_id,
            node_10_id,
            node_20_id,
            true,
            true);

    const auto high_portal_result =
        NavigationPortalRecord::create(
            high_portal_id,
            node_20_id,
            node_30_id,
            true,
            false);

    if (
        !low_portal_result.has_value() ||
        !high_portal_result.has_value())
    {
        return 1;
    }

    std::vector<
        NavigationNodeRecord>
        unsorted_nodes{
            node_30_result.value(),
            node_10_result.value(),
            node_20_result.value()
        };

    std::vector<
        NavigationPortalRecord>
        unsorted_portals{
            high_portal_result.value(),
            low_portal_result.value()
        };

    const auto canonical_result =
        NavigationCellTopology::create(
            cell,
            unsorted_nodes,
            unsorted_portals);

    check(
        state,
        canonical_result.has_value(),
        "Valid supplied topology succeeds");

    if (!canonical_result.has_value())
    {
        return 1;
    }

    check(
        state,
        !canonical_result.value().empty(),
        "empty() is false when supplied topology contains structural records");

    const auto canonical_nodes =
        canonical_result.value().
            nodes_in_canonical_order();

    check(
        state,
        canonical_nodes.size() == 3 &&
            canonical_nodes[0].id() ==
                node_10_id &&
            canonical_nodes[1].id() ==
                node_20_id &&
            canonical_nodes[2].id() ==
                node_30_id,
        "Unsorted nodes canonicalize by NavigationNodeId");

    const auto canonical_portals =
        canonical_result.value().
            portals_in_canonical_order();

    check(
        state,
        canonical_portals.size() == 2 &&
            canonical_portals[0].id() ==
                low_portal_id &&
            canonical_portals[1].id() ==
                high_portal_id,
        "Unsorted portals canonicalize by NavigationPortalId");

    check(
        state,
        canonical_nodes[0] ==
                node_10_result.value() &&
            canonical_nodes[1] ==
                node_20_result.value() &&
            canonical_nodes[2] ==
                node_30_result.value(),
        "Canonical node span preserves the exact structural records");

    check(
        state,
        canonical_portals[0] ==
                low_portal_result.value() &&
            canonical_portals[1] ==
                high_portal_result.value(),
        "Canonical portal span preserves the exact structural records");

    std::vector<
        NavigationNodeRecord>
        differently_ordered_nodes{
            node_20_result.value(),
            node_30_result.value(),
            node_10_result.value()
        };

    std::vector<
        NavigationPortalRecord>
        differently_ordered_portals{
            low_portal_result.value(),
            high_portal_result.value()
        };

    const auto equivalent_result =
        NavigationCellTopology::create(
            cell,
            differently_ordered_nodes,
            differently_ordered_portals);

    check(
        state,
        equivalent_result.has_value() &&
            equivalent_result.value() ==
                canonical_result.value(),
        "Differently ordered equivalent input produces equal value");

    std::vector<
        NavigationNodeRecord>
        duplicate_nodes{
            node_10_result.value(),
            node_10_result.value()
        };

    const auto duplicate_node_result =
        NavigationCellTopology::create(
            cell,
            duplicate_nodes,
            std::span<
                const NavigationPortalRecord>{});

    check(
        state,
        !duplicate_node_result.has_value() &&
            duplicate_node_result.error().code ==
                ErrorCode::invalid_argument,
        "Duplicate node identity is rejected");

    const auto wrong_position_result =
        WorldPosition::create(
            other_cell,
            LocalPosition{
                1.0,
                2.0,
                3.0
            });

    if (!wrong_position_result.has_value())
    {
        return 1;
    }

    const NavigationNodeId
        wrong_node_id{
            other_cell,
            5ULL
        };

    const auto wrong_node_result =
        NavigationNodeRecord::create(
            wrong_node_id,
            wrong_position_result.value());

    if (!wrong_node_result.has_value())
    {
        return 1;
    }

    std::vector<
        NavigationNodeRecord>
        wrong_cell_nodes{
            node_10_result.value(),
            wrong_node_result.value()
        };

    const auto wrong_cell_result =
        NavigationCellTopology::create(
            cell,
            wrong_cell_nodes,
            std::span<
                const NavigationPortalRecord>{});

    check(
        state,
        !wrong_cell_result.has_value() &&
            wrong_cell_result.error().code ==
                ErrorCode::invalid_argument,
        "Node owned by another cell is rejected");

    const auto duplicate_portal_payload_result =
        NavigationPortalRecord::create(
            low_portal_id,
            node_10_id,
            node_30_id,
            true,
            false);

    if (!duplicate_portal_payload_result.has_value())
    {
        return 1;
    }

    std::vector<
        NavigationPortalRecord>
        duplicate_portals{
            low_portal_result.value(),
            duplicate_portal_payload_result.value()
        };

    const auto duplicate_portal_result =
        NavigationCellTopology::create(
            cell,
            unsorted_nodes,
            duplicate_portals);

    check(
        state,
        !duplicate_portal_result.has_value() &&
            duplicate_portal_result.error().code ==
                ErrorCode::invalid_argument,
        "Duplicate portal identity is rejected");

    const NavigationNodeId
        remote_node_1{
            remote_cell,
            1ULL
        };

    const NavigationNodeId
        remote_node_2{
            other_cell,
            2ULL
        };

    const NavigationPortalId
        outside_portal_id{
            WorldCell{
                900,
                900,
                900
            },
            3ULL
        };

    const auto outside_portal_result =
        NavigationPortalRecord::create(
            outside_portal_id,
            remote_node_1,
            remote_node_2,
            true,
            true);

    if (!outside_portal_result.has_value())
    {
        return 1;
    }

    std::vector<
        NavigationPortalRecord>
        outside_portals{
            outside_portal_result.value()
        };

    const auto outside_result =
        NavigationCellTopology::create(
            cell,
            unsorted_nodes,
            outside_portals);

    check(
        state,
        !outside_result.has_value() &&
            outside_result.error().code ==
                ErrorCode::invalid_argument,
        "Portal touching neither endpoint cell is rejected");

    std::vector<
        NavigationNodeRecord>
        both_same_cell_nodes{
            node_10_result.value(),
            node_20_result.value()
        };

    std::vector<
        NavigationPortalRecord>
        same_cell_portals{
            low_portal_result.value()
        };

    const auto same_cell_result =
        NavigationCellTopology::create(
            cell,
            both_same_cell_nodes,
            same_cell_portals);

    check(
        state,
        same_cell_result.has_value(),
        "Same-cell portal is accepted when both local nodes exist");

    std::vector<
        NavigationNodeRecord>
        missing_first_nodes{
            node_20_result.value()
        };

    const auto missing_first_result =
        NavigationCellTopology::create(
            cell,
            missing_first_nodes,
            same_cell_portals);

    check(
        state,
        !missing_first_result.has_value() &&
            missing_first_result.error().code ==
                ErrorCode::invalid_argument,
        "Same-cell portal missing first local node is rejected");

    std::vector<
        NavigationNodeRecord>
        missing_second_nodes{
            node_10_result.value()
        };

    const auto missing_second_result =
        NavigationCellTopology::create(
            cell,
            missing_second_nodes,
            same_cell_portals);

    check(
        state,
        !missing_second_result.has_value() &&
            missing_second_result.error().code ==
                ErrorCode::invalid_argument,
        "Same-cell portal missing second local node is rejected");

    const NavigationPortalId
        cross_portal_id{
            WorldCell{
                1000,
                -1000,
                500
            },
            99ULL
        };

    const auto cross_portal_result =
        NavigationPortalRecord::create(
            cross_portal_id,
            node_30_id,
            remote_node_1,
            true,
            true);

    if (!cross_portal_result.has_value())
    {
        return 1;
    }

    std::vector<
        NavigationNodeRecord>
        cross_local_nodes{
            node_30_result.value()
        };

    std::vector<
        NavigationPortalRecord>
        cross_portals{
            cross_portal_result.value()
        };

    const auto cross_result =
        NavigationCellTopology::create(
            cell,
            cross_local_nodes,
            cross_portals);

    check(
        state,
        cross_result.has_value(),
        "Cross-cell portal is accepted with local endpoint only");

    check(
        state,
        cross_result.has_value() &&
            cross_result.value().
                nodes_in_canonical_order().
                size() == 1 &&
            cross_result.value().
                portals_in_canonical_order().
                size() == 1,
        "Remote endpoint record is not required");

    check(
        state,
        cross_result.has_value() &&
            cross_result.value().
                portals_in_canonical_order()[0].
                id().cell !=
                cell,
        "Portal id namespace cell may differ from coverage cell");

    std::vector<
        NavigationNodeRecord>
        wrong_local_endpoint_nodes{
            node_10_result.value()
        };

    const auto missing_cross_local_result =
        NavigationCellTopology::create(
            cell,
            wrong_local_endpoint_nodes,
            cross_portals);

    check(
        state,
        !missing_cross_local_result.has_value() &&
            missing_cross_local_result.error().code ==
                ErrorCode::invalid_argument,
        "Cross-cell portal missing local endpoint is rejected");

    const NavigationCellTopology
        copied_topology{
            canonical_result.value()
        };

    check(
        state,
        copied_topology ==
            canonical_result.value(),
        "Navigation cell topology supports copy construction");

    NavigationCellTopology
        assigned_topology{
            empty_result.value()
        };

    assigned_topology =
        canonical_result.value();

    check(
        state,
        assigned_topology ==
            canonical_result.value(),
        "Navigation cell topology supports copy assignment");

    std::cout
        << state.checks
        << " checks, "
        << state.failures
        << " failures\n";

    return state.failures == 0
        ? 0
        : 1;
}
