#include "oros/ai/navigation_search.hpp"

#include "oros/ai/navigation_cell_topology.hpp"
#include "oros/ai/navigation_node_record.hpp"
#include "oros/ai/navigation_portal_record.hpp"
#include "oros/world/world_position.hpp"

#include <algorithm>
#include <cstdint>
#include <initializer_list>
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
                "NavigationSearch test could "
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

    oros::foundation::Result<
        oros::ai::NavigationPortalRecord>
    make_portal(
        const oros::world::WorldCell
            portal_namespace,
        const std::uint64_t local_id,
        const oros::ai::NavigationNodeId first,
        const oros::ai::NavigationNodeId second,
        const bool first_to_second,
        const bool second_to_first)
    {
        return
            oros::ai::
                NavigationPortalRecord::create(
                    oros::ai::
                        NavigationPortalId{
                            portal_namespace,
                            local_id
                        },
                    first,
                    second,
                    first_to_second,
                    second_to_first);
    }

    oros::foundation::Result<
        oros::ai::NavigationCellTopology>
    make_cell(
        const oros::world::WorldCell cell,
        const std::initializer_list<
            oros::ai::NavigationNodeRecord>
            nodes,
        const std::initializer_list<
            oros::ai::NavigationPortalRecord>
            portals)
    {
        return
            oros::ai::
                NavigationCellTopology::create(
                    cell,
                    std::span<
                        const oros::ai::
                            NavigationNodeRecord>{
                                nodes.begin(),
                                nodes.size()
                            },
                    std::span<
                        const oros::ai::
                            NavigationPortalRecord>{
                                portals.begin(),
                                portals.size()
                            });
    }

    std::optional<
        oros::ai::NavigationTopology>
    make_topology(
        const std::initializer_list<
            oros::ai::NavigationCellTopology>
            cells)
    {
        oros::ai::NavigationTopology
            topology{};

        for (
            const oros::ai::
                NavigationCellTopology& cell :
            cells)
        {
            const auto status =
                topology.
                    set_cell_topology(
                        cell);

            if (!status.has_value())
            {
                return std::nullopt;
            }
        }

        return topology;
    }

    bool is_invalid_argument(
        const oros::foundation::Result<
            oros::ai::
                NavigationSearchResult>&
            result)
    {
        return
            !result.has_value() &&
            result.error().code ==
                oros::foundation::
                    ErrorCode::invalid_argument;
    }

    template <typename T>
    bool span_equals(
        const std::span<const T> actual,
        const std::initializer_list<T>
            expected)
    {
        return
            actual.size() ==
                expected.size() &&
            std::equal(
                actual.begin(),
                actual.end(),
                expected.begin(),
                expected.end());
    }
}

int main()
{
    using namespace oros::ai;
    using namespace oros::foundation;
    using namespace oros::world;

    static_assert(
        !std::is_default_constructible_v<
            NavigationSearchResult>);

    static_assert(
        std::is_copy_constructible_v<
            NavigationSearchResult>);

    static_assert(
        std::is_copy_assignable_v<
            NavigationSearchResult>);

    static_assert(
        std::is_nothrow_move_constructible_v<
            NavigationSearchResult>);

    static_assert(
        std::is_nothrow_move_assignable_v<
            NavigationSearchResult>);

    static_assert(
        std::is_same_v<
            decltype(
                search_navigation_route(
                    std::declval<
                        const NavigationTopology&>(),
                    std::declval<
                        const NavigationObstacleOverlay&>(),
                    std::declval<
                        NavigationNodeId>(),
                    std::declval<
                        NavigationNodeId>())),
            Result<
                NavigationSearchResult>>);

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const NavigationSearchResult&>().
                    route()),
            const NavigationRoute*>);

    TestState state{};

    const WorldCell cell_a{
        0,
        0,
        0
    };

    const WorldCell cell_d{
        4,
        0,
        0
    };

    const WorldCell cell_e{
        5,
        0,
        0
    };

    const WorldCell cell_u{
        10,
        0,
        0
    };

    const WorldCell cell_v{
        11,
        0,
        0
    };

    const WorldCell cell_w{
        12,
        0,
        0
    };

    const WorldCell cell_x{
        99,
        0,
        0
    };

    const WorldCell portal_namespace{
        100,
        0,
        0
    };

    const auto a1_result =
        make_node(
            cell_a,
            1ULL,
            1.0);

    const auto a2_result =
        make_node(
            cell_a,
            2ULL,
            2.0);

    const auto a3_result =
        make_node(
            cell_a,
            3ULL,
            3.0);

    const auto a4_result =
        make_node(
            cell_a,
            4ULL,
            4.0);

    const auto a5_result =
        make_node(
            cell_a,
            5ULL,
            5.0);

    const auto d1_result =
        make_node(
            cell_d,
            1ULL,
            1.0);

    if (
        !a1_result.has_value() ||
        !a2_result.has_value() ||
        !a3_result.has_value() ||
        !a4_result.has_value() ||
        !a5_result.has_value() ||
        !d1_result.has_value())
    {
        return 1;
    }

    const NavigationNodeId a1 =
        a1_result.value().id();

    const NavigationNodeId a2 =
        a2_result.value().id();

    const NavigationNodeId a3 =
        a3_result.value().id();

    const NavigationNodeId a4 =
        a4_result.value().id();

    const NavigationNodeId a5 =
        a5_result.value().id();

    const NavigationNodeId d1 =
        d1_result.value().id();

    const NavigationNodeId u1{
        cell_u,
        1ULL
    };

    const NavigationNodeId u9{
        cell_u,
        9ULL
    };

    const NavigationNodeId v1{
        cell_v,
        1ULL
    };

    const NavigationNodeId w1{
        cell_w,
        1ULL
    };

    const NavigationNodeId
        missing_a{
            cell_a,
            999ULL
        };

    const NavigationNodeId
        empty_e_destination{
            cell_e,
            1ULL
        };

    const NavigationNodeId
        unavailable_start{
            cell_x,
            1ULL
        };

    const auto direct_12_result =
        make_portal(
            portal_namespace,
            10ULL,
            a1,
            a2,
            true,
            true);

    const auto multi_12_result =
        make_portal(
            portal_namespace,
            20ULL,
            a1,
            a2,
            true,
            true);

    const auto multi_23_result =
        make_portal(
            portal_namespace,
            21ULL,
            a2,
            a3,
            true,
            true);

    const auto short_13_result =
        make_portal(
            portal_namespace,
            30ULL,
            a1,
            a3,
            true,
            true);

    const auto short_12_result =
        make_portal(
            portal_namespace,
            31ULL,
            a1,
            a2,
            true,
            true);

    const auto short_35_result =
        make_portal(
            portal_namespace,
            32ULL,
            a3,
            a5,
            true,
            true);

    const auto short_24_result =
        make_portal(
            portal_namespace,
            33ULL,
            a2,
            a4,
            true,
            true);

    const auto short_54_result =
        make_portal(
            portal_namespace,
            34ULL,
            a5,
            a4,
            true,
            true);

    const auto tie_13_result =
        make_portal(
            portal_namespace,
            40ULL,
            a1,
            a3,
            true,
            true);

    const auto tie_12_result =
        make_portal(
            portal_namespace,
            41ULL,
            a1,
            a2,
            true,
            true);

    const auto tie_34_result =
        make_portal(
            portal_namespace,
            42ULL,
            a3,
            a4,
            true,
            true);

    const auto tie_24_result =
        make_portal(
            portal_namespace,
            43ULL,
            a2,
            a4,
            true,
            true);

    const auto parallel_1_result =
        make_portal(
            portal_namespace,
            50ULL,
            a1,
            a2,
            true,
            true);

    const auto parallel_2_result =
        make_portal(
            portal_namespace,
            51ULL,
            a1,
            a2,
            true,
            true);

    const auto cycle_12_result =
        make_portal(
            portal_namespace,
            60ULL,
            a1,
            a2,
            true,
            true);

    const auto cycle_23_result =
        make_portal(
            portal_namespace,
            61ULL,
            a2,
            a3,
            true,
            true);

    const auto cycle_31_result =
        make_portal(
            portal_namespace,
            62ULL,
            a3,
            a1,
            true,
            true);

    const auto exact_v_result =
        make_portal(
            portal_namespace,
            70ULL,
            a1,
            v1,
            true,
            false);

    const auto exact_12_result =
        make_portal(
            portal_namespace,
            71ULL,
            a1,
            a2,
            true,
            true);

    const auto exact_u1_result =
        make_portal(
            portal_namespace,
            72ULL,
            a2,
            u1,
            true,
            false);

    const auto same_v_result =
        make_portal(
            portal_namespace,
            80ULL,
            a1,
            v1,
            true,
            false);

    const auto same_12_result =
        make_portal(
            portal_namespace,
            81ULL,
            a1,
            a2,
            true,
            true);

    const auto same_u9_result =
        make_portal(
            portal_namespace,
            82ULL,
            a2,
            u9,
            true,
            false);

    const auto tier_v_result =
        make_portal(
            portal_namespace,
            90ULL,
            a1,
            v1,
            true,
            false);

    const auto tier_w_result =
        make_portal(
            portal_namespace,
            91ULL,
            a1,
            w1,
            true,
            false);

    const auto unrelated_u_result =
        make_portal(
            portal_namespace,
            100ULL,
            a1,
            u1,
            true,
            false);

    const auto blocked_12_result =
        make_portal(
            portal_namespace,
            110ULL,
            a1,
            a2,
            true,
            true);

    if (
        !direct_12_result.has_value() ||
        !multi_12_result.has_value() ||
        !multi_23_result.has_value() ||
        !short_13_result.has_value() ||
        !short_12_result.has_value() ||
        !short_35_result.has_value() ||
        !short_24_result.has_value() ||
        !short_54_result.has_value() ||
        !tie_13_result.has_value() ||
        !tie_12_result.has_value() ||
        !tie_34_result.has_value() ||
        !tie_24_result.has_value() ||
        !parallel_1_result.has_value() ||
        !parallel_2_result.has_value() ||
        !cycle_12_result.has_value() ||
        !cycle_23_result.has_value() ||
        !cycle_31_result.has_value() ||
        !exact_v_result.has_value() ||
        !exact_12_result.has_value() ||
        !exact_u1_result.has_value() ||
        !same_v_result.has_value() ||
        !same_12_result.has_value() ||
        !same_u9_result.has_value() ||
        !tier_v_result.has_value() ||
        !tier_w_result.has_value() ||
        !unrelated_u_result.has_value() ||
        !blocked_12_result.has_value())
    {
        return 1;
    }

    const NavigationPortalId direct_12 =
        direct_12_result.value().id();

    const NavigationPortalId multi_12 =
        multi_12_result.value().id();

    const NavigationPortalId multi_23 =
        multi_23_result.value().id();

    const NavigationPortalId short_13 =
        short_13_result.value().id();

    const NavigationPortalId short_12 =
        short_12_result.value().id();

    const NavigationPortalId short_24 =
        short_24_result.value().id();

    const NavigationPortalId tie_13 =
        tie_13_result.value().id();

    const NavigationPortalId tie_34 =
        tie_34_result.value().id();

    const NavigationPortalId parallel_1 =
        parallel_1_result.value().id();

    const NavigationPortalId exact_12 =
        exact_12_result.value().id();

    const NavigationPortalId exact_u1 =
        exact_u1_result.value().id();

    const NavigationPortalId same_u9 =
        same_u9_result.value().id();

    const NavigationPortalId tier_v =
        tier_v_result.value().id();

    const NavigationPortalId unrelated_u =
        unrelated_u_result.value().id();

    const NavigationPortalId blocked_12 =
        blocked_12_result.value().id();

    const auto start_cell_result =
        make_cell(
            cell_a,
            {
                a1_result.value()
            },
            {});

    const auto empty_e_cell_result =
        make_cell(
            cell_e,
            {},
            {});

    const auto d_cell_result =
        make_cell(
            cell_d,
            {
                d1_result.value()
            },
            {});

    const auto direct_cell_result =
        make_cell(
            cell_a,
            {
                a1_result.value(),
                a2_result.value()
            },
            {
                direct_12_result.value()
            });

    const auto multi_cell_result =
        make_cell(
            cell_a,
            {
                a1_result.value(),
                a2_result.value(),
                a3_result.value()
            },
            {
                multi_12_result.value(),
                multi_23_result.value()
            });

    const auto shortest_cell_result =
        make_cell(
            cell_a,
            {
                a1_result.value(),
                a2_result.value(),
                a3_result.value(),
                a4_result.value(),
                a5_result.value()
            },
            {
                short_54_result.value(),
                short_24_result.value(),
                short_35_result.value(),
                short_12_result.value(),
                short_13_result.value()
            });

    const auto tie_cell_result =
        make_cell(
            cell_a,
            {
                a1_result.value(),
                a2_result.value(),
                a3_result.value(),
                a4_result.value()
            },
            {
                tie_24_result.value(),
                tie_34_result.value(),
                tie_12_result.value(),
                tie_13_result.value()
            });

    const auto parallel_cell_result =
        make_cell(
            cell_a,
            {
                a1_result.value(),
                a2_result.value()
            },
            {
                parallel_2_result.value(),
                parallel_1_result.value()
            });

    const auto cycle_cell_result =
        make_cell(
            cell_a,
            {
                a1_result.value(),
                a2_result.value(),
                a3_result.value()
            },
            {
                cycle_31_result.value(),
                cycle_23_result.value(),
                cycle_12_result.value()
            });

    const auto exact_cell_result =
        make_cell(
            cell_a,
            {
                a1_result.value(),
                a2_result.value()
            },
            {
                exact_u1_result.value(),
                exact_12_result.value(),
                exact_v_result.value()
            });

    const auto same_cell_result =
        make_cell(
            cell_a,
            {
                a1_result.value(),
                a2_result.value()
            },
            {
                same_u9_result.value(),
                same_12_result.value(),
                same_v_result.value()
            });

    const auto tier_cell_result =
        make_cell(
            cell_a,
            {
                a1_result.value()
            },
            {
                tier_w_result.value(),
                tier_v_result.value()
            });

    const auto unrelated_cell_result =
        make_cell(
            cell_a,
            {
                a1_result.value()
            },
            {
                unrelated_u_result.value()
            });

    const auto blocked_cell_result =
        make_cell(
            cell_a,
            {
                a1_result.value(),
                a2_result.value()
            },
            {
                blocked_12_result.value()
            });

    if (
        !start_cell_result.has_value() ||
        !empty_e_cell_result.has_value() ||
        !d_cell_result.has_value() ||
        !direct_cell_result.has_value() ||
        !multi_cell_result.has_value() ||
        !shortest_cell_result.has_value() ||
        !tie_cell_result.has_value() ||
        !parallel_cell_result.has_value() ||
        !cycle_cell_result.has_value() ||
        !exact_cell_result.has_value() ||
        !same_cell_result.has_value() ||
        !tier_cell_result.has_value() ||
        !unrelated_cell_result.has_value() ||
        !blocked_cell_result.has_value())
    {
        return 1;
    }

    const auto start_topology =
        make_topology(
            {
                start_cell_result.value()
            });

    const auto start_empty_topology =
        make_topology(
            {
                start_cell_result.value(),
                empty_e_cell_result.value()
            });

    const auto direct_topology =
        make_topology(
            {
                direct_cell_result.value()
            });

    const auto multi_topology =
        make_topology(
            {
                multi_cell_result.value()
            });

    const auto shortest_topology =
        make_topology(
            {
                shortest_cell_result.value()
            });

    const auto tie_topology =
        make_topology(
            {
                tie_cell_result.value()
            });

    const auto parallel_topology =
        make_topology(
            {
                parallel_cell_result.value()
            });

    const auto cycle_topology =
        make_topology(
            {
                cycle_cell_result.value(),
                d_cell_result.value()
            });

    const auto exact_topology =
        make_topology(
            {
                exact_cell_result.value()
            });

    const auto same_frontier_topology =
        make_topology(
            {
                same_cell_result.value()
            });

    const auto tier_topology =
        make_topology(
            {
                tier_cell_result.value()
            });

    const auto unrelated_topology =
        make_topology(
            {
                unrelated_cell_result.value(),
                d_cell_result.value()
            });

    const auto blocked_topology =
        make_topology(
            {
                blocked_cell_result.value()
            });

    if (
        !start_topology.has_value() ||
        !start_empty_topology.has_value() ||
        !direct_topology.has_value() ||
        !multi_topology.has_value() ||
        !shortest_topology.has_value() ||
        !tie_topology.has_value() ||
        !parallel_topology.has_value() ||
        !cycle_topology.has_value() ||
        !exact_topology.has_value() ||
        !same_frontier_topology.has_value() ||
        !tier_topology.has_value() ||
        !unrelated_topology.has_value() ||
        !blocked_topology.has_value())
    {
        return 1;
    }

    const NavigationObstacleOverlay
        empty_overlay{};

    const auto invalid_start =
        search_navigation_route(
            start_topology.value(),
            empty_overlay,
            invalid_navigation_node_id,
            u1);

    check(
        state,
        is_invalid_argument(
            invalid_start),
        "Invalid start identity is rejected");

    const auto unavailable_start_result =
        search_navigation_route(
            start_topology.value(),
            empty_overlay,
            unavailable_start,
            u1);

    check(
        state,
        is_invalid_argument(
            unavailable_start_result),
        "Unavailable start cell is rejected");

    const auto missing_start =
        search_navigation_route(
            start_topology.value(),
            empty_overlay,
            missing_a,
            u1);

    check(
        state,
        is_invalid_argument(
            missing_start),
        "Missing start node is rejected");

    const auto invalid_destination =
        search_navigation_route(
            start_topology.value(),
            empty_overlay,
            a1,
            invalid_navigation_node_id);

    check(
        state,
        is_invalid_argument(
            invalid_destination),
        "Invalid destination identity is rejected");

    const auto missing_destination =
        search_navigation_route(
            start_topology.value(),
            empty_overlay,
            a1,
            missing_a);

    check(
        state,
        is_invalid_argument(
            missing_destination),
        "Supplied missing destination node is rejected");

    const auto empty_destination =
        search_navigation_route(
            start_empty_topology.value(),
            empty_overlay,
            a1,
            empty_e_destination);

    check(
        state,
        is_invalid_argument(
            empty_destination),
        "Supplied-empty destination node is rejected");

    const auto unavailable_destination =
        search_navigation_route(
            start_topology.value(),
            empty_overlay,
            a1,
            u1);

    check(
        state,
        unavailable_destination.has_value(),
        "Unavailable destination identity is accepted");

    const auto zero_edge =
        search_navigation_route(
            start_topology.value(),
            empty_overlay,
            a1,
            a1);

    check(
        state,
        zero_edge.has_value() &&
            zero_edge.value().kind() ==
                NavigationSearchResultKind::
                    complete &&
            zero_edge.value().route() !=
                nullptr &&
            zero_edge.value().route()->
                kind() ==
                NavigationRouteKind::complete &&
            span_equals<
                NavigationNodeId>(
                    zero_edge.value().
                        route()->
                        nodes_in_traversal_order(),
                    {
                        a1
                    }) &&
            zero_edge.value().route()->
                portals_in_traversal_order().
                empty(),
        "Start equals destination creates zero-edge complete route");

    const auto direct =
        search_navigation_route(
            direct_topology.value(),
            empty_overlay,
            a1,
            a2);

    check(
        state,
        direct.has_value() &&
            direct.value().kind() ==
                NavigationSearchResultKind::
                    complete &&
            direct.value().route() !=
                nullptr &&
            span_equals<
                NavigationNodeId>(
                    direct.value().
                        route()->
                        nodes_in_traversal_order(),
                    {
                        a1,
                        a2
                    }) &&
            span_equals<
                NavigationPortalId>(
                    direct.value().
                        route()->
                        portals_in_traversal_order(),
                    {
                        direct_12
                    }),
        "Direct complete route is produced");

    const auto multi =
        search_navigation_route(
            multi_topology.value(),
            empty_overlay,
            a1,
            a3);

    check(
        state,
        multi.has_value() &&
            multi.value().route() !=
                nullptr &&
            span_equals<
                NavigationNodeId>(
                    multi.value().
                        route()->
                        nodes_in_traversal_order(),
                    {
                        a1,
                        a2,
                        a3
                    }) &&
            span_equals<
                NavigationPortalId>(
                    multi.value().
                        route()->
                        portals_in_traversal_order(),
                    {
                        multi_12,
                        multi_23
                    }),
        "Multi-hop complete route is produced");

    const auto shortest =
        search_navigation_route(
            shortest_topology.value(),
            empty_overlay,
            a1,
            a4);

    check(
        state,
        shortest.has_value() &&
            shortest.value().route() !=
                nullptr &&
            span_equals<
                NavigationNodeId>(
                    shortest.value().
                        route()->
                        nodes_in_traversal_order(),
                    {
                        a1,
                        a2,
                        a4
                    }) &&
            span_equals<
                NavigationPortalId>(
                    shortest.value().
                        route()->
                        portals_in_traversal_order(),
                    {
                        short_12,
                        short_24
                    }),
        "Shortest-hop complete route wins");

    const auto equal_hop =
        search_navigation_route(
            tie_topology.value(),
            empty_overlay,
            a1,
            a4);

    check(
        state,
        equal_hop.has_value() &&
            equal_hop.value().route() !=
                nullptr &&
            span_equals<
                NavigationNodeId>(
                    equal_hop.value().
                        route()->
                        nodes_in_traversal_order(),
                    {
                        a1,
                        a3,
                        a4
                    }) &&
            span_equals<
                NavigationPortalId>(
                    equal_hop.value().
                        route()->
                        portals_in_traversal_order(),
                    {
                        tie_13,
                        tie_34
                    }),
        "Equal-hop first-discovery tie is deterministic");

    const auto parallel =
        search_navigation_route(
            parallel_topology.value(),
            empty_overlay,
            a1,
            a2);

    check(
        state,
        parallel.has_value() &&
            parallel.value().route() !=
                nullptr &&
            span_equals<
                NavigationPortalId>(
                    parallel.value().
                        route()->
                        portals_in_traversal_order(),
                    {
                        parallel_1
                    }),
        "Parallel portal first discovery is deterministic");

    const auto cycle =
        search_navigation_route(
            cycle_topology.value(),
            empty_overlay,
            a1,
            d1);

    check(
        state,
        cycle.has_value() &&
            cycle.value().kind() ==
                NavigationSearchResultKind::
                    known_unreachable &&
            cycle.value().route() ==
                nullptr,
        "Cycles terminate without duplicate discovery");

    const auto exact_partial =
        search_navigation_route(
            exact_topology.value(),
            empty_overlay,
            a1,
            u1);

    check(
        state,
        exact_partial.has_value() &&
            exact_partial.value().kind() ==
                NavigationSearchResultKind::
                    partial &&
            exact_partial.value().route() !=
                nullptr &&
            exact_partial.value().route()->
                continuation_portal().
                has_value() &&
            *exact_partial.value().route()->
                continuation_portal() ==
                exact_u1,
        "Exact-destination frontier is preferred over other frontiers");

    const auto same_frontier =
        search_navigation_route(
            same_frontier_topology.value(),
            empty_overlay,
            a1,
            u1);

    check(
        state,
        same_frontier.has_value() &&
            same_frontier.value().route() !=
                nullptr &&
            same_frontier.value().route()->
                continuation_portal().
                has_value() &&
            *same_frontier.value().route()->
                continuation_portal() ==
                same_u9,
        "Destination-cell frontier is preferred over unrelated frontier");

    const auto tier_partial =
        search_navigation_route(
            tier_topology.value(),
            empty_overlay,
            a1,
            u1);

    check(
        state,
        tier_partial.has_value() &&
            tier_partial.value().route() !=
                nullptr &&
            tier_partial.value().route()->
                continuation_portal().
                has_value() &&
            *tier_partial.value().route()->
                continuation_portal() ==
                tier_v,
        "First BFS frontier wins within one preference tier");

    check(
        state,
        exact_partial.has_value() &&
            exact_partial.value().route() !=
                nullptr &&
            span_equals<
                NavigationNodeId>(
                    exact_partial.value().
                        route()->
                        nodes_in_traversal_order(),
                    {
                        a1,
                        a2
                    }) &&
            exact_partial.value().route()->
                terminal_node() ==
                a2,
        "Partial prefix ends at supplied terminal");

    check(
        state,
        exact_partial.has_value() &&
            exact_partial.value().route() !=
                nullptr &&
            span_equals<
                NavigationPortalId>(
                    exact_partial.value().
                        route()->
                        portals_in_traversal_order(),
                    {
                        exact_12
                    }) &&
            exact_partial.value().route()->
                continuation_portal().
                has_value() &&
            *exact_partial.value().route()->
                continuation_portal() ==
                exact_u1,
        "Continuation portal is not in traversed portal span");

    check(
        state,
        exact_partial.has_value() &&
            exact_partial.value().route() !=
                nullptr &&
            exact_partial.value().route()->
                continuation_node().
                has_value() &&
            *exact_partial.value().route()->
                continuation_node() ==
                u1 &&
            exact_topology.value().
                find_cell_topology(
                    u1.cell) ==
                nullptr,
        "Continuation node remains unavailable");

    check(
        state,
        exact_partial.has_value() &&
            exact_partial.value().
                requested_destination() ==
                u1 &&
            exact_partial.value().route() !=
                nullptr &&
            exact_partial.value().route()->
                requested_destination() ==
                u1,
        "Original requested destination is preserved");

    check(
        state,
        exact_partial.has_value() &&
            exact_partial.value().route() !=
                nullptr &&
            std::find(
                exact_partial.value().
                    route()->
                    nodes_in_traversal_order().
                    begin(),
                exact_partial.value().
                    route()->
                    nodes_in_traversal_order().
                    end(),
                u1) ==
                exact_partial.value().
                    route()->
                    nodes_in_traversal_order().
                    end(),
        "Unavailable candidate is not enqueued");

    const auto unrelated_partial =
        search_navigation_route(
            unrelated_topology.value(),
            empty_overlay,
            a1,
            d1);

    check(
        state,
        unrelated_partial.has_value() &&
            unrelated_partial.value().kind() ==
                NavigationSearchResultKind::
                    partial &&
            unrelated_partial.value().route() !=
                nullptr &&
            unrelated_partial.value().route()->
                continuation_portal().
                has_value() &&
            *unrelated_partial.value().
                route()->
                continuation_portal() ==
                unrelated_u,
        "Unrelated frontier still yields partial if no better frontier");

    check(
        state,
        unrelated_partial.has_value() &&
            unrelated_partial.value().kind() ==
                NavigationSearchResultKind::
                    partial,
        "Reachable frontier prevents known_unreachable");

    check(
        state,
        unrelated_partial.has_value() &&
            unrelated_partial.value().
                requested_destination() ==
                d1 &&
            unrelated_partial.value().kind() ==
                NavigationSearchResultKind::
                    partial,
        "Supplied destination plus unrelated frontier yields partial");

    const auto first_direction =
        NavigationPortalTraversalDirection::
            first_to_second;

    const auto blocked_frontier_overlay =
        NavigationObstacleOverlay::create(
            std::vector<
                NavigationTraversalBlock>{
                    NavigationTraversalBlock{
                        unrelated_u,
                        first_direction
                    }
                });

    if (
        !blocked_frontier_overlay.
            has_value())
    {
        return 1;
    }

    const auto blocked_frontier =
        search_navigation_route(
            unrelated_topology.value(),
            blocked_frontier_overlay.value(),
            a1,
            d1);

    check(
        state,
        blocked_frontier.has_value() &&
            blocked_frontier.value().kind() ==
                NavigationSearchResultKind::
                    known_unreachable &&
            blocked_frontier.value().route() ==
                nullptr,
        "Blocked unavailable frontier does not prevent unreachable proof");

    check(
        state,
        cycle.has_value() &&
            cycle.value().kind() ==
                NavigationSearchResultKind::
                    known_unreachable,
        "No frontier plus exhausted supplied graph yields known_unreachable");

    check(
        state,
        unavailable_destination.
                has_value() &&
            unavailable_destination.
                value().kind() ==
                NavigationSearchResultKind::
                    known_unreachable &&
            unavailable_destination.
                value().route() ==
                nullptr,
        "Unavailable destination plus no reachable frontier may be known_unreachable");

    const auto all_blocked_overlay =
        NavigationObstacleOverlay::create(
            std::vector<
                NavigationTraversalBlock>{
                    NavigationTraversalBlock{
                        blocked_12,
                        first_direction
                    }
                });

    if (!all_blocked_overlay.has_value())
    {
        return 1;
    }

    const auto all_blocked =
        search_navigation_route(
            blocked_topology.value(),
            all_blocked_overlay.value(),
            a1,
            a2);

    check(
        state,
        all_blocked.has_value() &&
            all_blocked.value().kind() ==
                NavigationSearchResultKind::
                    known_unreachable &&
            all_blocked.value().route() ==
                nullptr,
        "All outgoing edges dynamically blocked may yield known_unreachable");

    check(
        state,
        direct.has_value() &&
            direct.value().kind() ==
                NavigationSearchResultKind::
                    complete &&
            direct.value().route() !=
                nullptr &&
            direct.value().route()->
                kind() ==
                NavigationRouteKind::complete,
        "Complete kind is derived from complete route");

    check(
        state,
        exact_partial.has_value() &&
            exact_partial.value().kind() ==
                NavigationSearchResultKind::
                    partial &&
            exact_partial.value().route() !=
                nullptr &&
            exact_partial.value().route()->
                kind() ==
                NavigationRouteKind::partial,
        "Partial kind is derived from partial route");

    check(
        state,
        cycle.has_value() &&
            cycle.value().kind() ==
                NavigationSearchResultKind::
                    known_unreachable &&
            cycle.value().route() ==
                nullptr,
        "Known-unreachable has null route");

    check(
        state,
        direct.has_value() &&
            direct.value().start_node() ==
                a1 &&
            direct.value().
                requested_destination() ==
                a2,
        "Search endpoints are preserved");

    if (
        !direct.has_value() ||
        !exact_partial.has_value() ||
        !cycle.has_value())
    {
        return 1;
    }

    const NavigationSearchResult
        copied{
            direct.value()
        };

    check(
        state,
        copied ==
            direct.value(),
        "NavigationSearchResult supports copy construction");

    NavigationSearchResult
        assigned{
            exact_partial.value()
        };

    assigned =
        direct.value();

    check(
        state,
        assigned ==
            direct.value(),
        "NavigationSearchResult supports copy assignment");

    const auto repeated_first =
        search_navigation_route(
            exact_topology.value(),
            empty_overlay,
            a1,
            u1);

    const auto repeated_second =
        search_navigation_route(
            exact_topology.value(),
            empty_overlay,
            a1,
            u1);

    check(
        state,
        repeated_first.has_value() &&
            repeated_second.has_value() &&
            repeated_first.value() ==
                repeated_second.value(),
        "Repeated identical search returns equal result");

    std::cout
        << state.checks
        << " checks, "
        << state.failures
        << " failures\n";

    return state.failures == 0
        ? 0
        : 1;
}
