#include "oros/ai/navigation_traversal.hpp"

#include "oros/ai/navigation_cell_topology.hpp"
#include "oros/ai/navigation_node_record.hpp"
#include "oros/ai/navigation_portal_record.hpp"

#include <algorithm>
#include <cstdint>
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
                "NavigationTraversal test "
                "could not create node "
                "position.");
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
            std::vector<
                oros::ai::
                    NavigationTraversalCandidate>>&
            result)
    {
        return
            !result.has_value() &&
            result.error().code ==
                oros::foundation::
                    ErrorCode::invalid_argument;
    }

    const oros::ai::
        NavigationTraversalCandidate*
    find_candidate(
        const std::vector<
            oros::ai::
                NavigationTraversalCandidate>&
            candidates,
        const oros::ai::
            NavigationPortalId portal)
    {
        const auto iterator =
            std::find_if(
                candidates.begin(),
                candidates.end(),
                [portal](
                    const oros::ai::
                        NavigationTraversalCandidate&
                        candidate)
                {
                    return
                        candidate.portal ==
                        portal;
                });

        if (iterator == candidates.end())
        {
            return nullptr;
        }

        return &*iterator;
    }
}

int main()
{
    using namespace oros::ai;
    using namespace oros::foundation;
    using namespace oros::world;

    static_assert(
        std::is_copy_constructible_v<
            NavigationTraversalCandidate>);

    static_assert(
        std::is_copy_assignable_v<
            NavigationTraversalCandidate>);

    static_assert(
        std::is_nothrow_move_constructible_v<
            NavigationTraversalCandidate>);

    static_assert(
        std::is_nothrow_move_assignable_v<
            NavigationTraversalCandidate>);

    static_assert(
        std::is_same_v<
            decltype(
                query_navigation_traversal_candidates(
                    std::declval<
                        const NavigationTopology&>(),
                    std::declval<
                        NavigationNodeId>())),
            Result<
                std::vector<
                    NavigationTraversalCandidate>>>);

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    NavigationTraversalCandidate>().
                    portal),
            NavigationPortalId>);

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    NavigationTraversalCandidate>().
                    target),
            NavigationNodeId>);

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    NavigationTraversalCandidate>().
                    target_kind),
            NavigationTraversalTargetKind>);

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

    const WorldCell cell_c{
        3,
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

    const auto c1_result =
        make_node(
            cell_c,
            10ULL,
            10.0);

    if (
        !a1_result.has_value() ||
        !a2_result.has_value() ||
        !a3_result.has_value() ||
        !b1_result.has_value() ||
        !c1_result.has_value())
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

    const NavigationNodeId c1 =
        c1_result.value().id();

    const NavigationNodeId u1{
        cell_u,
        10ULL
    };

    const NavigationNodeId
        missing_a{
            cell_a,
            999ULL
        };

    const NavigationNodeId
        unavailable_source{
            cell_x,
            1ULL
        };

    const WorldCell portal_namespace{
        100,
        0,
        0
    };

    const NavigationPortalId
        portal_to_a3_id{
            portal_namespace,
            1ULL
        };

    const NavigationPortalId
        portal_to_a2_id{
            portal_namespace,
            2ULL
        };

    const NavigationPortalId
        portal_not_touching_a1_id{
            portal_namespace,
            3ULL
        };

    const NavigationPortalId
        portal_reverse_only_id{
            portal_namespace,
            4ULL
        };

    const NavigationPortalId
        portal_bidirectional_id{
            portal_namespace,
            5ULL
        };

    const NavigationPortalId
        portal_cross_supplied_id{
            portal_namespace,
            6ULL
        };

    const NavigationPortalId
        portal_cross_unavailable_id{
            portal_namespace,
            7ULL
        };

    const NavigationPortalId
        portal_duplicate_target_id{
            portal_namespace,
            8ULL
        };

    const auto portal_to_a3_result =
        NavigationPortalRecord::create(
            portal_to_a3_id,
            a1,
            a3,
            true,
            true);

    const auto portal_to_a2_result =
        NavigationPortalRecord::create(
            portal_to_a2_id,
            a1,
            a2,
            true,
            true);

    const auto
        portal_not_touching_a1_result =
            NavigationPortalRecord::create(
                portal_not_touching_a1_id,
                a2,
                a3,
                true,
                true);

    const auto portal_reverse_only_result =
        NavigationPortalRecord::create(
            portal_reverse_only_id,
            a1,
            a2,
            false,
            true);

    const auto portal_bidirectional_result =
        NavigationPortalRecord::create(
            portal_bidirectional_id,
            a1,
            a2,
            true,
            true);

    const auto portal_cross_supplied_result =
        NavigationPortalRecord::create(
            portal_cross_supplied_id,
            a1,
            b1,
            true,
            true);

    const auto
        portal_cross_unavailable_result =
            NavigationPortalRecord::create(
                portal_cross_unavailable_id,
                a1,
                u1,
                true,
                false);

    const auto portal_duplicate_target_result =
        NavigationPortalRecord::create(
            portal_duplicate_target_id,
            a1,
            a2,
            true,
            true);

    if (
        !portal_to_a3_result.has_value() ||
        !portal_to_a2_result.has_value() ||
        !portal_not_touching_a1_result.
            has_value() ||
        !portal_reverse_only_result.
            has_value() ||
        !portal_bidirectional_result.
            has_value() ||
        !portal_cross_supplied_result.
            has_value() ||
        !portal_cross_unavailable_result.
            has_value() ||
        !portal_duplicate_target_result.
            has_value())
    {
        return 1;
    }

    const std::vector<
        NavigationNodeRecord>
        a_nodes{
            a3_result.value(),
            a1_result.value(),
            a2_result.value()
        };

    const std::vector<
        NavigationPortalRecord>
        a_portals{
            portal_duplicate_target_result.
                value(),
            portal_cross_unavailable_result.
                value(),
            portal_cross_supplied_result.
                value(),
            portal_bidirectional_result.
                value(),
            portal_reverse_only_result.
                value(),
            portal_not_touching_a1_result.
                value(),
            portal_to_a2_result.value(),
            portal_to_a3_result.value()
        };

    const auto topology_a_result =
        NavigationCellTopology::create(
            cell_a,
            a_nodes,
            a_portals);

    const std::vector<
        NavigationNodeRecord>
        b_nodes{
            b1_result.value()
        };

    const std::vector<
        NavigationPortalRecord>
        b_portals{
            portal_cross_supplied_result.
                value()
        };

    const auto topology_b_result =
        NavigationCellTopology::create(
            cell_b,
            b_nodes,
            b_portals);

    const std::vector<
        NavigationNodeRecord>
        c_nodes{
            c1_result.value()
        };

    const auto topology_c_result =
        NavigationCellTopology::create(
            cell_c,
            c_nodes,
            std::span<
                const NavigationPortalRecord>{});

    if (
        !topology_a_result.has_value() ||
        !topology_b_result.has_value() ||
        !topology_c_result.has_value())
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

    NavigationTopology topology_c_only{};

    if (
        !topology_c_only.
            set_cell_topology(
                topology_c_result.value()).
            has_value())
    {
        return 1;
    }

    const auto invalid_source_result =
        query_navigation_traversal_candidates(
            topology_a_only,
            invalid_navigation_node_id);

    check(
        state,
        is_invalid_argument(
            invalid_source_result),
        "Invalid source is rejected");

    const auto unavailable_source_result =
        query_navigation_traversal_candidates(
            topology_a_only,
            unavailable_source);

    check(
        state,
        is_invalid_argument(
            unavailable_source_result),
        "Unavailable source cell is rejected");

    const auto missing_source_result =
        query_navigation_traversal_candidates(
            topology_a_only,
            missing_a);

    check(
        state,
        is_invalid_argument(
            missing_source_result),
        "Missing source record is rejected");

    const auto no_portal_result =
        query_navigation_traversal_candidates(
            topology_c_only,
            c1);

    check(
        state,
        no_portal_result.has_value() &&
            no_portal_result.value().empty(),
        "Node with no portals returns empty candidate vector");

    const auto a1_only_result =
        query_navigation_traversal_candidates(
            topology_a_only,
            a1);

    if (!a1_only_result.has_value())
    {
        return 1;
    }

    const auto&
        a1_only =
            a1_only_result.value();

    check(
        state,
        find_candidate(
            a1_only,
            portal_not_touching_a1_id) ==
            nullptr,
        "Portal not touching source is ignored");

    check(
        state,
        find_candidate(
            a1_only,
            portal_reverse_only_id) ==
            nullptr,
        "Forbidden outbound direction is ignored");

    const NavigationTraversalCandidate*
        forward_candidate =
            find_candidate(
                a1_only,
                portal_to_a3_id);

    check(
        state,
        forward_candidate != nullptr &&
            forward_candidate->target ==
                a3 &&
            forward_candidate->target_kind ==
                NavigationTraversalTargetKind::
                    supplied,
        "Permitted forward direction is included");

    check(
        state,
        forward_candidate != nullptr &&
            forward_candidate->portal ==
                portal_to_a3_id,
        "Portal identity is preserved exactly");

    const NavigationTraversalCandidate*
        same_cell_candidate =
            find_candidate(
                a1_only,
                portal_to_a2_id);

    check(
        state,
        same_cell_candidate != nullptr &&
            same_cell_candidate->target ==
                a2 &&
            same_cell_candidate->target_kind ==
                NavigationTraversalTargetKind::
                    supplied,
        "Same-cell target is classified supplied");

    const auto a2_result_candidates =
        query_navigation_traversal_candidates(
            topology_a_only,
            a2);

    if (!a2_result_candidates.has_value())
    {
        return 1;
    }

    const auto&
        from_a2 =
            a2_result_candidates.value();

    const NavigationTraversalCandidate*
        reverse_candidate =
            find_candidate(
                from_a2,
                portal_reverse_only_id);

    check(
        state,
        reverse_candidate != nullptr &&
            reverse_candidate->target ==
                a1 &&
            reverse_candidate->target_kind ==
                NavigationTraversalTargetKind::
                    supplied,
        "Permitted reverse direction is included");

    const NavigationTraversalCandidate*
        bidirectional_from_a1 =
            find_candidate(
                a1_only,
                portal_bidirectional_id);

    const NavigationTraversalCandidate*
        bidirectional_from_a2 =
            find_candidate(
                from_a2,
                portal_bidirectional_id);

    check(
        state,
        bidirectional_from_a1 != nullptr &&
            bidirectional_from_a1->target ==
                a2 &&
            bidirectional_from_a2 != nullptr &&
            bidirectional_from_a2->target ==
                a1,
        "Bidirectional portal works from either endpoint");

    const auto a1_ab_result =
        query_navigation_traversal_candidates(
            topology_ab,
            a1);

    if (!a1_ab_result.has_value())
    {
        return 1;
    }

    const auto&
        a1_ab =
            a1_ab_result.value();

    const NavigationTraversalCandidate*
        cross_supplied =
            find_candidate(
                a1_ab,
                portal_cross_supplied_id);

    check(
        state,
        cross_supplied != nullptr &&
            cross_supplied->target ==
                b1 &&
            cross_supplied->target_kind ==
                NavigationTraversalTargetKind::
                    supplied,
        "Cross-cell supplied target is classified supplied");

    const NavigationTraversalCandidate*
        cross_unavailable =
            find_candidate(
                a1_ab,
                portal_cross_unavailable_id);

    check(
        state,
        cross_unavailable != nullptr &&
            cross_unavailable->target ==
                u1 &&
            cross_unavailable->target_kind ==
                NavigationTraversalTargetKind::
                    unavailable,
        "Cross-cell absent target is classified unavailable");

    check(
        state,
        cross_unavailable != nullptr &&
            cross_unavailable->target ==
                u1,
        "Unavailable target identity is preserved");

    const NavigationTraversalCandidate*
        duplicate_target =
            find_candidate(
                a1_ab,
                portal_duplicate_target_id);

    check(
        state,
        duplicate_target != nullptr &&
            same_cell_candidate != nullptr &&
            duplicate_target->target ==
                same_cell_candidate->target &&
            duplicate_target->portal !=
                same_cell_candidate->portal,
        "Multiple portals to same target remain distinct");

    const std::vector<
        NavigationPortalId>
        expected_a1_portal_order{
            portal_to_a3_id,
            portal_to_a2_id,
            portal_bidirectional_id,
            portal_cross_supplied_id,
            portal_cross_unavailable_id,
            portal_duplicate_target_id
        };

    bool canonical_order_matches =
        a1_ab.size() ==
        expected_a1_portal_order.size();

    if (canonical_order_matches)
    {
        for (
            std::size_t index = 0;
            index < a1_ab.size();
            ++index)
        {
            if (
                a1_ab[index].portal !=
                expected_a1_portal_order[index])
            {
                canonical_order_matches =
                    false;

                break;
            }
        }
    }

    check(
        state,
        canonical_order_matches,
        "Output follows canonical portal-id order after filtering");

    check(
        state,
        a1_ab.size() >= 2 &&
            a1_ab[0].target == a3 &&
            a1_ab[1].target == a2 &&
            a1_ab[0].portal <
                a1_ab[1].portal &&
            a1_ab[1].target <
                a1_ab[0].target,
        "Target-node order does not override portal order");

    check(
        state,
        a1_ab.size() == 6,
        "Filtering preserves exactly the six permitted source edges");

    const std::size_t
        a2_target_count =
            static_cast<std::size_t>(
                std::count_if(
                    a1_ab.begin(),
                    a1_ab.end(),
                    [a2](
                        const NavigationTraversalCandidate&
                            candidate)
                    {
                        return
                            candidate.target ==
                            a2;
                    }));

    check(
        state,
        a2_target_count == 3,
        "Parallel structural edges to one target remain distinct");

    std::cout
        << state.checks
        << " checks, "
        << state.failures
        << " failures\n";

    return state.failures == 0
        ? 0
        : 1;
}
