#include "oros/ai/navigation_route.hpp"

#include "oros/ai/navigation_cell_topology.hpp"
#include "oros/ai/navigation_node_record.hpp"
#include "oros/ai/navigation_portal_record.hpp"

#include <cstdint>
#include <iostream>
#include <optional>
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

    oros::foundation::Result<
        oros::ai::NavigationNodeRecord>
    make_node(
        const oros::world::WorldCell cell,
        const std::uint64_t local_id,
        const double local_x)
    {
        using namespace oros;

        const auto position =
            world::WorldPosition::create(
                cell,
                world::LocalPosition{
                    local_x,
                    0.0,
                    0.0
                });

        if (!position.has_value())
        {
            return foundation::fail(
                position.error().code,
                "NavigationRoute test could "
                "not create node position.");
        }

        return
            ai::NavigationNodeRecord::create(
                ai::NavigationNodeId{
                    cell,
                    local_id
                },
                position.value());
    }

    bool is_invalid_argument(
        const oros::foundation::Result<
            oros::ai::NavigationRoute>&
            result)
    {
        return
            !result.has_value() &&
            result.error().code ==
                oros::foundation::
                    ErrorCode::invalid_argument;
    }

    bool is_invalid_argument(
        const oros::foundation::Status&
            status)
    {
        return
            !status.has_value() &&
            status.error().code ==
                oros::foundation::
                    ErrorCode::invalid_argument;
    }
}

int main()
{
    using namespace oros::ai;
    using namespace oros::foundation;
    using namespace oros::world;

    static_assert(
        !std::is_default_constructible_v<
            NavigationRoute>);

    static_assert(
        std::is_copy_constructible_v<
            NavigationRoute>);

    static_assert(
        std::is_copy_assignable_v<
            NavigationRoute>);

    static_assert(
        std::is_nothrow_move_constructible_v<
            NavigationRoute>);

    static_assert(
        std::is_nothrow_move_assignable_v<
            NavigationRoute>);

    static_assert(
        std::is_same_v<
            decltype(
                NavigationRoute::
                    create_complete(
                        std::declval<
                            const NavigationTopology&>(),
                        std::declval<
                            NavigationNodeId>(),
                        std::declval<
                            std::span<
                                const NavigationNodeId>>(),
                        std::declval<
                            std::span<
                                const NavigationPortalId>>())),
            Result<
                NavigationRoute>>);

    static_assert(
        std::is_same_v<
            decltype(
                NavigationRoute::
                    create_partial(
                        std::declval<
                            const NavigationTopology&>(),
                        std::declval<
                            NavigationNodeId>(),
                        std::declval<
                            std::span<
                                const NavigationNodeId>>(),
                        std::declval<
                            std::span<
                                const NavigationPortalId>>(),
                        std::declval<
                            NavigationPortalId>())),
            Result<
                NavigationRoute>>);

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const NavigationRoute&>().
                    kind()),
            NavigationRouteKind>);

    static_assert(
        noexcept(
            std::declval<
                const NavigationRoute&>().
                kind()));

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const NavigationRoute&>().
                    requested_destination()),
            NavigationNodeId>);

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const NavigationRoute&>().
                    nodes_in_traversal_order()),
            std::span<
                const NavigationNodeId>>);

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const NavigationRoute&>().
                    portals_in_traversal_order()),
            std::span<
                const NavigationPortalId>>);

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const NavigationRoute&>().
                    continuation_portal()),
            std::optional<
                NavigationPortalId>>);

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const NavigationRoute&>().
                    continuation_node()),
            std::optional<
                NavigationNodeId>>);

    TestState state{};

    const WorldCell cell_a{
        0,
        0,
        0
    };

    const WorldCell cell_b{
        1,
        0,
        0
    };

    const WorldCell cell_u{
        2,
        0,
        0
    };

    const WorldCell cell_x{
        9,
        0,
        0
    };

    const auto a1_result =
        make_node(
            cell_a,
            10ULL,
            10.0);

    const auto a2_result =
        make_node(
            cell_a,
            20ULL,
            20.0);

    const auto a3_result =
        make_node(
            cell_a,
            30ULL,
            30.0);

    const auto b1_result =
        make_node(
            cell_b,
            10ULL,
            10.0);

    if (
        !a1_result.has_value() ||
        !a2_result.has_value() ||
        !a3_result.has_value() ||
        !b1_result.has_value())
    {
        return 1;
    }

    const NavigationNodeId a1 =
        a1_result.value().id();

    const NavigationNodeId a2 =
        a2_result.value().id();

    const NavigationNodeId a3 =
        a3_result.value().id();

    const NavigationNodeId b1 =
        b1_result.value().id();

    const NavigationNodeId u1{
        cell_u,
        10ULL
    };

    const NavigationNodeId
        unavailable_destination{
            cell_u,
            99ULL
        };

    const NavigationNodeId
        unsupplied_node{
            cell_x,
            1ULL
        };

    const NavigationNodeId
        missing_local_node{
            cell_a,
            40ULL
        };

    const NavigationPortalId
        portal_a1_a2_id{
            WorldCell{
                100,
                0,
                0
            },
            1ULL
        };

    const NavigationPortalId
        portal_a1_a3_id{
            WorldCell{
                100,
                0,
                0
            },
            2ULL
        };

    const NavigationPortalId
        portal_reverse_id{
            WorldCell{
                100,
                0,
                0
            },
            3ULL
        };

    const NavigationPortalId
        portal_ab_id{
            WorldCell{
                100,
                0,
                0
            },
            4ULL
        };

    const NavigationPortalId
        portal_au_id{
            WorldCell{
                100,
                0,
                0
            },
            5ULL
        };

    const NavigationPortalId
        portal_a2u_id{
            WorldCell{
                100,
                0,
                0
            },
            6ULL
        };

    const NavigationPortalId
        portal_au_reverse_id{
            WorldCell{
                100,
                0,
                0
            },
            7ULL
        };

    const NavigationPortalId
        missing_portal_id{
            WorldCell{
                100,
                0,
                0
            },
            999ULL
        };

    const auto portal_a1_a2_result =
        NavigationPortalRecord::create(
            portal_a1_a2_id,
            a1,
            a2,
            true,
            true);

    const auto portal_a1_a3_result =
        NavigationPortalRecord::create(
            portal_a1_a3_id,
            a1,
            a3,
            true,
            true);

    const auto portal_reverse_result =
        NavigationPortalRecord::create(
            portal_reverse_id,
            a1,
            a2,
            false,
            true);

    const auto portal_ab_result =
        NavigationPortalRecord::create(
            portal_ab_id,
            a1,
            b1,
            true,
            true);

    const auto portal_au_result =
        NavigationPortalRecord::create(
            portal_au_id,
            a1,
            u1,
            true,
            false);

    const auto portal_a2u_result =
        NavigationPortalRecord::create(
            portal_a2u_id,
            a2,
            u1,
            true,
            false);

    const auto portal_au_reverse_result =
        NavigationPortalRecord::create(
            portal_au_reverse_id,
            a1,
            u1,
            false,
            true);

    if (
        !portal_a1_a2_result.has_value() ||
        !portal_a1_a3_result.has_value() ||
        !portal_reverse_result.has_value() ||
        !portal_ab_result.has_value() ||
        !portal_au_result.has_value() ||
        !portal_a2u_result.has_value() ||
        !portal_au_reverse_result.has_value())
    {
        return 1;
    }

    const std::vector<
        NavigationNodeRecord>
        a_nodes{
            a1_result.value(),
            a2_result.value(),
            a3_result.value()
        };

    const std::vector<
        NavigationPortalRecord>
        a_portals{
            portal_a1_a2_result.value(),
            portal_a1_a3_result.value(),
            portal_reverse_result.value(),
            portal_ab_result.value(),
            portal_au_result.value(),
            portal_a2u_result.value(),
            portal_au_reverse_result.value()
        };

    const std::vector<
        NavigationNodeRecord>
        b_nodes{
            b1_result.value()
        };

    const std::vector<
        NavigationPortalRecord>
        b_portals{
            portal_ab_result.value()
        };

    const auto topology_a_result =
        NavigationCellTopology::create(
            cell_a,
            a_nodes,
            a_portals);

    const auto topology_b_result =
        NavigationCellTopology::create(
            cell_b,
            b_nodes,
            b_portals);

    const auto empty_b_result =
        NavigationCellTopology::create(
            cell_b,
            std::span<
                const NavigationNodeRecord>{},
            std::span<
                const NavigationPortalRecord>{});

    if (
        !topology_a_result.has_value() ||
        !topology_b_result.has_value() ||
        !empty_b_result.has_value())
    {
        return 1;
    }

    NavigationTopology topology_a_only{};

    if (
        !topology_a_only.
            set_cell_topology(
                topology_a_result.value()).
            has_value())
    {
        return 1;
    }

    NavigationTopology topology_ab{};

    if (
        !topology_ab.
            set_cell_topology(
                topology_a_result.value()).
            has_value() ||
        !topology_ab.
            set_cell_topology(
                topology_b_result.value()).
            has_value())
    {
        return 1;
    }

    const auto invalid_destination =
        NavigationRoute::create_complete(
            topology_a_only,
            invalid_navigation_node_id,
            std::vector<
                NavigationNodeId>{
                    a1
                },
            std::span<
                const NavigationPortalId>{});

    check(
        state,
        is_invalid_argument(
            invalid_destination),
        "Invalid requested destination is rejected");

    const auto empty_nodes =
        NavigationRoute::create_complete(
            topology_a_only,
            a1,
            std::span<
                const NavigationNodeId>{},
            std::span<
                const NavigationPortalId>{});

    check(
        state,
        is_invalid_argument(
            empty_nodes),
        "Empty node sequence is rejected");

    const auto count_mismatch =
        NavigationRoute::create_complete(
            topology_a_only,
            a2,
            std::vector<
                NavigationNodeId>{
                    a1,
                    a2
                },
            std::span<
                const NavigationPortalId>{});

    check(
        state,
        is_invalid_argument(
            count_mismatch),
        "Portal and node count mismatch is rejected");

    const auto invalid_node =
        NavigationRoute::create_complete(
            topology_a_only,
            a1,
            std::vector<
                NavigationNodeId>{
                    invalid_navigation_node_id
                },
            std::span<
                const NavigationPortalId>{});

    check(
        state,
        is_invalid_argument(
            invalid_node),
        "Invalid stored node identity is rejected");

    const auto invalid_portal =
        NavigationRoute::create_complete(
            topology_a_only,
            a2,
            std::vector<
                NavigationNodeId>{
                    a1,
                    a2
                },
            std::vector<
                NavigationPortalId>{
                    invalid_navigation_portal_id
                });

    check(
        state,
        is_invalid_argument(
            invalid_portal),
        "Invalid stored portal identity is rejected");

    const auto unavailable_node =
        NavigationRoute::create_complete(
            topology_a_only,
            unsupplied_node,
            std::vector<
                NavigationNodeId>{
                    unsupplied_node
                },
            std::span<
                const NavigationPortalId>{});

    check(
        state,
        is_invalid_argument(
            unavailable_node),
        "Unsupplied stored route node is rejected");

    const auto missing_node =
        NavigationRoute::create_complete(
            topology_a_only,
            missing_local_node,
            std::vector<
                NavigationNodeId>{
                    missing_local_node
                },
            std::span<
                const NavigationPortalId>{});

    check(
        state,
        is_invalid_argument(
            missing_node),
        "Missing stored node record is rejected");

    const auto missing_portal =
        NavigationRoute::create_complete(
            topology_a_only,
            a2,
            std::vector<
                NavigationNodeId>{
                    a1,
                    a2
                },
            std::vector<
                NavigationPortalId>{
                    missing_portal_id
                });

    check(
        state,
        is_invalid_argument(
            missing_portal),
        "Portal absent from traversal origin cell is rejected");

    const auto wrong_endpoints =
        NavigationRoute::create_complete(
            topology_a_only,
            a2,
            std::vector<
                NavigationNodeId>{
                    a1,
                    a2
                },
            std::vector<
                NavigationPortalId>{
                    portal_a1_a3_id
                });

    check(
        state,
        is_invalid_argument(
            wrong_endpoints),
        "Portal not connecting consecutive nodes is rejected");

    const auto forbidden_direction =
        NavigationRoute::create_complete(
            topology_a_only,
            a2,
            std::vector<
                NavigationNodeId>{
                    a1,
                    a2
                },
            std::vector<
                NavigationPortalId>{
                    portal_reverse_id
                });

    check(
        state,
        is_invalid_argument(
            forbidden_direction),
        "Forbidden traversal direction is rejected");

    const auto same_cell_route =
        NavigationRoute::create_complete(
            topology_a_only,
            a2,
            std::vector<
                NavigationNodeId>{
                    a1,
                    a2
                },
            std::vector<
                NavigationPortalId>{
                    portal_a1_a2_id
                });

    check(
        state,
        same_cell_route.has_value(),
        "Same-cell traversal is accepted");

    const auto cross_cell_route =
        NavigationRoute::create_complete(
            topology_ab,
            b1,
            std::vector<
                NavigationNodeId>{
                    a1,
                    b1
                },
            std::vector<
                NavigationPortalId>{
                    portal_ab_id
                });

    check(
        state,
        cross_cell_route.has_value(),
        "Cross-cell supplied traversal is accepted");

    const auto zero_edge_complete =
        NavigationRoute::create_complete(
            topology_a_only,
            a1,
            std::vector<
                NavigationNodeId>{
                    a1
                },
            std::span<
                const NavigationPortalId>{});

    check(
        state,
        zero_edge_complete.has_value() &&
            zero_edge_complete.value().
                kind() ==
                NavigationRouteKind::complete &&
            zero_edge_complete.value().
                start_node() == a1 &&
            zero_edge_complete.value().
                terminal_node() == a1 &&
            !zero_edge_complete.value().
                continuation_portal().
                has_value() &&
            !zero_edge_complete.value().
                continuation_node().
                has_value(),
        "Complete zero-edge route is accepted without continuation");

    const auto wrong_terminal =
        NavigationRoute::create_complete(
            topology_a_only,
            a2,
            std::vector<
                NavigationNodeId>{
                    a1
                },
            std::span<
                const NavigationPortalId>{});

    check(
        state,
        is_invalid_argument(
            wrong_terminal),
        "Complete route terminal must equal requested destination");

    const auto partial_zero_edge =
        NavigationRoute::create_partial(
            topology_a_only,
            unavailable_destination,
            std::vector<
                NavigationNodeId>{
                    a1
                },
            std::span<
                const NavigationPortalId>{},
            portal_au_id);

    check(
        state,
        partial_zero_edge.has_value(),
        "Partial zero-local-edge boundary is accepted");

    if (!partial_zero_edge.has_value())
    {
        return 1;
    }

    check(
        state,
        partial_zero_edge.value().
                kind() ==
                NavigationRouteKind::partial &&
            partial_zero_edge.value().
                requested_destination() ==
                unavailable_destination &&
            partial_zero_edge.value().
                terminal_node() == a1 &&
            partial_zero_edge.value().
                portals_in_traversal_order().
                empty() &&
            partial_zero_edge.value().
                continuation_portal() ==
                std::optional<
                    NavigationPortalId>{
                        portal_au_id
                    } &&
            partial_zero_edge.value().
                continuation_node() ==
                std::optional<
                    NavigationNodeId>{
                        u1
                    },
        "Partial route exposes boundary continuation exactly");

    const auto missing_continuation =
        NavigationRoute::create_partial(
            topology_a_only,
            unavailable_destination,
            std::vector<
                NavigationNodeId>{
                    a1
                },
            std::span<
                const NavigationPortalId>{},
            missing_portal_id);

    check(
        state,
        is_invalid_argument(
            missing_continuation),
        "Missing partial continuation portal is rejected");

    const auto wrong_continuation_endpoint =
        NavigationRoute::create_partial(
            topology_a_only,
            unavailable_destination,
            std::vector<
                NavigationNodeId>{
                    a1
                },
            std::span<
                const NavigationPortalId>{},
            portal_a2u_id);

    check(
        state,
        is_invalid_argument(
            wrong_continuation_endpoint),
        "Partial continuation portal must touch terminal node");

    const auto wrong_continuation_direction =
        NavigationRoute::create_partial(
            topology_a_only,
            unavailable_destination,
            std::vector<
                NavigationNodeId>{
                    a1
                },
            std::span<
                const NavigationPortalId>{},
            portal_au_reverse_id);

    check(
        state,
        is_invalid_argument(
            wrong_continuation_direction),
        "Partial continuation direction must be permitted");

    const auto supplied_continuation =
        NavigationRoute::create_partial(
            topology_ab,
            b1,
            std::vector<
                NavigationNodeId>{
                    a1
                },
            std::span<
                const NavigationPortalId>{},
            portal_ab_id);

    check(
        state,
        is_invalid_argument(
            supplied_continuation),
        "Partial continuation into supplied topology is rejected");

    NavigationTopology known_empty_boundary{};

    const Status set_empty_b =
        known_empty_boundary.
            set_cell_topology(
                empty_b_result.value());

    const Status set_a_against_empty_b =
        known_empty_boundary.
            set_cell_topology(
                topology_a_result.value());

    check(
        state,
        set_empty_b.has_value() &&
            is_invalid_argument(
                set_a_against_empty_b),
        "Known-empty continuation target cannot coexist with boundary projection");

    const auto destination_is_continuation =
        NavigationRoute::create_partial(
            topology_a_only,
            u1,
            std::vector<
                NavigationNodeId>{
                    a1
                },
            std::span<
                const NavigationPortalId>{},
            portal_au_id);

    check(
        state,
        destination_is_continuation.
            has_value() &&
            destination_is_continuation.
                value().kind() ==
                NavigationRouteKind::partial &&
            destination_is_continuation.
                value().
                continuation_node() ==
                std::optional<
                    NavigationNodeId>{
                        u1
                    },
        "Destination equal continuation node remains partial while unavailable");

    const auto destination_in_prefix =
        NavigationRoute::create_partial(
            topology_a_only,
            a1,
            std::vector<
                NavigationNodeId>{
                    a1
                },
            std::span<
                const NavigationPortalId>{},
            portal_au_id);

    check(
        state,
        is_invalid_argument(
            destination_in_prefix),
        "Partial route rejects requested destination already in supplied prefix");

    const auto descending_route =
        NavigationRoute::create_complete(
            topology_a_only,
            a1,
            std::vector<
                NavigationNodeId>{
                    a2,
                    a1
                },
            std::vector<
                NavigationPortalId>{
                    portal_a1_a2_id
                });

    check(
        state,
        descending_route.has_value() &&
            descending_route.value().
                nodes_in_traversal_order().
                size() == 2 &&
            descending_route.value().
                nodes_in_traversal_order()[0] ==
                a2 &&
            descending_route.value().
                nodes_in_traversal_order()[1] ==
                a1 &&
            descending_route.value().
                portals_in_traversal_order()[0] ==
                portal_a1_a2_id,
        "Traversal order is preserved and never canonical-sorted");

    if (
        !same_cell_route.has_value() ||
        !cross_cell_route.has_value())
    {
        return 1;
    }

    check(
        state,
        same_cell_route.value().
                requested_destination() ==
                a2 &&
            same_cell_route.value().
                start_node() ==
                a1 &&
            same_cell_route.value().
                terminal_node() ==
                a2 &&
            same_cell_route.value().
                nodes_in_traversal_order().
                size() == 2 &&
            same_cell_route.value().
                portals_in_traversal_order().
                size() == 1 &&
            same_cell_route.value().
                continuation_portal() ==
                std::nullopt &&
            same_cell_route.value().
                continuation_node() ==
                std::nullopt,
        "Complete route accessors expose exact structural route");

    const NavigationRoute copied{
        cross_cell_route.value()
    };

    check(
        state,
        copied ==
            cross_cell_route.value(),
        "NavigationRoute supports copy construction");

    NavigationRoute assigned{
        same_cell_route.value()
    };

    assigned =
        cross_cell_route.value();

    check(
        state,
        assigned ==
            cross_cell_route.value(),
        "NavigationRoute supports copy assignment");

    std::cout
        << state.checks
        << " checks, "
        << state.failures
        << " failures\n";

    return state.failures == 0
        ? 0
        : 1;
}
