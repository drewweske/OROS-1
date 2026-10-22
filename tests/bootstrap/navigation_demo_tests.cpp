#include "navigation_demo.hpp"

#include "oros/ai/navigation_obstacle_overlay.hpp"
#include "oros/ai/navigation_search.hpp"

#include <algorithm>
#include <initializer_list>
#include <iostream>
#include <span>
#include <string_view>

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
    using namespace oros::bootstrap;
    using namespace oros::world;

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

    const WorldCell portal_namespace{
        100,
        0,
        0
    };

    const NavigationNodeId a1{
        cell_a,
        1ULL
    };

    const NavigationNodeId a2{
        cell_a,
        2ULL
    };

    const NavigationNodeId b1{
        cell_b,
        1ULL
    };

    const NavigationNodeId u1{
        cell_u,
        1ULL
    };

    const NavigationPortalId p1{
        portal_namespace,
        1ULL
    };

    const NavigationPortalId p2{
        portal_namespace,
        2ULL
    };

    const NavigationPortalId p3{
        portal_namespace,
        3ULL
    };

    const auto demo_result =
        create_navigation_demo();

    check(
        state,
        demo_result.has_value(),
        "create_navigation_demo succeeds");

    if (!demo_result.has_value())
    {
        return 1;
    }

    const NavigationDemo& demo =
        demo_result.value();

    check(
        state,
        demo.topology.size() == 2U,
        "Exactly two topology cells are supplied");

    const NavigationCellTopology*
        supplied_a =
            demo.topology.
                find_cell_topology(
                    cell_a);

    const NavigationCellTopology*
        supplied_b =
            demo.topology.
                find_cell_topology(
                    cell_b);

    const NavigationCellTopology*
        supplied_u =
            demo.topology.
                find_cell_topology(
                    cell_u);

    check(
        state,
        supplied_a != nullptr,
        "Cell A is supplied");

    check(
        state,
        supplied_b != nullptr,
        "Cell B is supplied");

    check(
        state,
        supplied_u == nullptr,
        "Cell U is absent");

    check(
        state,
        demo.start_node == a1 &&
            demo.complete_destination == b1 &&
            demo.partial_destination == u1,
        "NavigationDemo endpoint identities are exact");

    if (
        supplied_a == nullptr ||
        supplied_b == nullptr)
    {
        return 1;
    }

    const std::span<
        const NavigationNodeRecord>
        nodes_a =
            supplied_a->
                nodes_in_canonical_order();

    const std::span<
        const NavigationNodeRecord>
        nodes_b =
            supplied_b->
                nodes_in_canonical_order();

    check(
        state,
        nodes_a.size() == 2U &&
            nodes_a[0].id() == a1 &&
            nodes_a[1].id() == a2 &&
            nodes_b.size() == 1U &&
            nodes_b[0].id() == b1,
        "A1 A2 and B1 identities are exact");

    check(
        state,
        nodes_a.size() == 2U &&
            nodes_a[0].position().cell() ==
                a1.cell &&
            nodes_a[1].position().cell() ==
                a2.cell &&
            nodes_b.size() == 1U &&
            nodes_b[0].position().cell() ==
                b1.cell,
        "A1 A2 and B1 positions belong to their ID cells");

    const std::span<
        const NavigationPortalRecord>
        portals_a =
            supplied_a->
                portals_in_canonical_order();

    const std::span<
        const NavigationPortalRecord>
        portals_b =
            supplied_b->
                portals_in_canonical_order();

    check(
        state,
        portals_a.size() == 2U &&
            portals_a[0].id() == p1 &&
            portals_a[0].first_node() ==
                a1 &&
            portals_a[0].second_node() ==
                a2 &&
            portals_a[0].
                first_to_second() &&
            portals_a[0].
                second_to_first(),
        "P1 is same-cell bidirectional");

    check(
        state,
        portals_a.size() == 2U &&
            portals_a[1].id() == p2 &&
            portals_a[1].first_node() ==
                a2 &&
            portals_a[1].second_node() ==
                b1 &&
            portals_a[1].
                first_to_second() &&
            portals_a[1].
                second_to_first(),
        "P2 is cross-cell bidirectional");

    check(
        state,
        portals_a.size() == 2U &&
            portals_b.size() == 2U &&
            portals_a[1] ==
                portals_b[0],
        "P2 projection is identical in A and B");

    check(
        state,
        portals_b.size() == 2U &&
            portals_b[1].id() == p3 &&
            portals_b[1].first_node() ==
                b1 &&
            portals_b[1].second_node() ==
                u1 &&
            portals_b[1].
                first_to_second() &&
            !portals_b[1].
                second_to_first(),
        "P3 is boundary forward-only");

    const NavigationObstacleOverlay
        empty_overlay{};

    const auto complete_result =
        search_navigation_route(
            demo.topology,
            empty_overlay,
            demo.start_node,
            demo.complete_destination);

    check(
        state,
        complete_result.has_value() &&
            complete_result.value().kind() ==
                NavigationSearchResultKind::
                    complete &&
            complete_result.value().route() !=
                nullptr,
        "Complete search A1 to B1 succeeds");

    if (
        !complete_result.has_value() ||
        complete_result.value().route() ==
            nullptr)
    {
        return 1;
    }

    const NavigationRoute&
        complete_route =
            *complete_result.value().route();

    check(
        state,
        span_equals<
            NavigationNodeId>(
                complete_route.
                    nodes_in_traversal_order(),
                {
                    a1,
                    a2,
                    b1
                }),
        "Complete nodes are exactly A1 A2 B1");

    check(
        state,
        span_equals<
            NavigationPortalId>(
                complete_route.
                    portals_in_traversal_order(),
                {
                    p1,
                    p2
                }),
        "Complete portals are exactly P1 P2");

    const auto partial_result =
        search_navigation_route(
            demo.topology,
            empty_overlay,
            demo.start_node,
            demo.partial_destination);

    check(
        state,
        partial_result.has_value() &&
            partial_result.value().kind() ==
                NavigationSearchResultKind::
                    partial &&
            partial_result.value().route() !=
                nullptr,
        "Partial search A1 to U1 succeeds");

    if (
        !partial_result.has_value() ||
        partial_result.value().route() ==
            nullptr)
    {
        return 1;
    }

    const NavigationRoute&
        partial_route =
            *partial_result.value().route();

    check(
        state,
        span_equals<
            NavigationNodeId>(
                partial_route.
                    nodes_in_traversal_order(),
                {
                    a1,
                    a2,
                    b1
                }),
        "Partial supplied prefix is exactly A1 A2 B1");

    check(
        state,
        span_equals<
            NavigationPortalId>(
                partial_route.
                    portals_in_traversal_order(),
                {
                    p1,
                    p2
                }),
        "Partial traversed portals are exactly P1 P2");

    check(
        state,
        partial_route.
                continuation_portal().
                has_value() &&
            *partial_route.
                continuation_portal() ==
                p3,
        "Partial continuation portal is exactly P3");

    check(
        state,
        partial_route.
                continuation_node().
                has_value() &&
            *partial_route.
                continuation_node() ==
                u1,
        "Partial continuation node is exactly U1");

    check(
        state,
        std::find(
            partial_route.
                nodes_in_traversal_order().
                begin(),
            partial_route.
                nodes_in_traversal_order().
                end(),
            u1) ==
                partial_route.
                    nodes_in_traversal_order().
                    end(),
        "U1 is absent from traversed node span");

    check(
        state,
        std::find(
            partial_route.
                portals_in_traversal_order().
                begin(),
            partial_route.
                portals_in_traversal_order().
                end(),
            p3) ==
                partial_route.
                    portals_in_traversal_order().
                    end(),
        "P3 is absent from traversed portal span");

    const auto repeated_result =
        create_navigation_demo();

    check(
        state,
        repeated_result.has_value() &&
            repeated_result.value().topology ==
                demo.topology &&
            repeated_result.value().
                    start_node ==
                demo.start_node &&
            repeated_result.value().
                    complete_destination ==
                demo.complete_destination &&
            repeated_result.value().
                    partial_destination ==
                demo.partial_destination,
        "Repeated demo creation is deterministic");

    std::cout
        << state.checks
        << " checks, "
        << state.failures
        << " failures\n";

    return state.failures == 0
        ? 0
        : 1;
}
