#include "oros/ai/navigation_effective_traversal.hpp"

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
                "NavigationEffectiveTraversal "
                "test could not create node "
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

    bool same_error_code(
        const oros::foundation::Result<
            std::vector<
                oros::ai::
                    NavigationTraversalCandidate>>&
            left,
        const oros::foundation::Result<
            std::vector<
                oros::ai::
                    NavigationTraversalCandidate>>&
            right)
    {
        return
            !left.has_value() &&
            !right.has_value() &&
            left.error().code ==
                right.error().code;
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

    bool lacks_candidate(
        const std::vector<
            oros::ai::
                NavigationTraversalCandidate>&
            candidates,
        const oros::ai::
            NavigationPortalId portal)
    {
        return
            find_candidate(
                candidates,
                portal) ==
            nullptr;
    }
}

int main()
{
    using namespace oros::ai;
    using namespace oros::foundation;
    using namespace oros::world;

    static_assert(
        std::is_same_v<
            decltype(
                query_navigation_effective_traversal_candidates(
                    std::declval<
                        const NavigationTopology&>(),
                    std::declval<
                        const NavigationObstacleOverlay&>(),
                    std::declval<
                        NavigationNodeId>())),
            Result<
                std::vector<
                    NavigationTraversalCandidate>>>);

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
        portal_reverse_only_id{
            portal_namespace,
            3ULL
        };

    const NavigationPortalId
        portal_parallel_id{
            portal_namespace,
            4ULL
        };

    const NavigationPortalId
        portal_cross_supplied_id{
            portal_namespace,
            5ULL
        };

    const NavigationPortalId
        portal_cross_unavailable_id{
            portal_namespace,
            6ULL
        };

    const NavigationPortalId
        unknown_portal_id{
            WorldCell{
                500,
                0,
                0
            },
            77ULL
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

    const auto portal_reverse_only_result =
        NavigationPortalRecord::create(
            portal_reverse_only_id,
            a1,
            a2,
            false,
            true);

    const auto portal_parallel_result =
        NavigationPortalRecord::create(
            portal_parallel_id,
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

    if (
        !portal_to_a3_result.has_value() ||
        !portal_to_a2_result.has_value() ||
        !portal_reverse_only_result.
            has_value() ||
        !portal_parallel_result.has_value() ||
        !portal_cross_supplied_result.
            has_value() ||
        !portal_cross_unavailable_result.
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
            portal_cross_unavailable_result.
                value(),
            portal_cross_supplied_result.
                value(),
            portal_parallel_result.value(),
            portal_reverse_only_result.value(),
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

    const NavigationObstacleOverlay
        empty_overlay{};

    const auto invalid_structural =
        query_navigation_traversal_candidates(
            topology_ab,
            invalid_navigation_node_id);

    const auto invalid_effective =
        query_navigation_effective_traversal_candidates(
            topology_ab,
            empty_overlay,
            invalid_navigation_node_id);

    check(
        state,
        same_error_code(
            invalid_structural,
            invalid_effective),
        "Invalid source structural failure propagates");

    const auto unavailable_structural =
        query_navigation_traversal_candidates(
            topology_ab,
            unavailable_source);

    const auto unavailable_effective =
        query_navigation_effective_traversal_candidates(
            topology_ab,
            empty_overlay,
            unavailable_source);

    check(
        state,
        same_error_code(
            unavailable_structural,
            unavailable_effective),
        "Unavailable source structural failure propagates");

    const auto missing_structural =
        query_navigation_traversal_candidates(
            topology_ab,
            missing_a);

    const auto missing_effective =
        query_navigation_effective_traversal_candidates(
            topology_ab,
            empty_overlay,
            missing_a);

    check(
        state,
        same_error_code(
            missing_structural,
            missing_effective),
        "Missing source-record failure propagates");

    const auto empty_effective =
        query_navigation_effective_traversal_candidates(
            topology_c_only,
            empty_overlay,
            c1);

    check(
        state,
        empty_effective.has_value() &&
            empty_effective.value().empty(),
        "Empty structural outgoing set succeeds empty");

    const auto structural_a1 =
        query_navigation_traversal_candidates(
            topology_ab,
            a1);

    const auto effective_a1_empty =
        query_navigation_effective_traversal_candidates(
            topology_ab,
            empty_overlay,
            a1);

    check(
        state,
        structural_a1.has_value() &&
            effective_a1_empty.has_value() &&
            structural_a1.value() ==
                effective_a1_empty.value(),
        "Empty overlay equals structural output exactly");

    if (
        !structural_a1.has_value() ||
        !effective_a1_empty.has_value())
    {
        return 1;
    }

    const auto first =
        NavigationPortalTraversalDirection::
            first_to_second;

    const auto second =
        NavigationPortalTraversalDirection::
            second_to_first;

    const auto same_cell_block_result =
        NavigationObstacleOverlay::create(
            std::vector<
                NavigationTraversalBlock>{
                    NavigationTraversalBlock{
                        portal_to_a3_id,
                        first
                    }
                });

    if (!same_cell_block_result.has_value())
    {
        return 1;
    }

    const auto same_cell_blocked =
        query_navigation_effective_traversal_candidates(
            topology_ab,
            same_cell_block_result.value(),
            a1);

    check(
        state,
        same_cell_blocked.has_value() &&
            lacks_candidate(
                same_cell_blocked.value(),
                portal_to_a3_id),
        "Blocked same-cell supplied edge is removed");

    const auto cross_supplied_block_result =
        NavigationObstacleOverlay::create(
            std::vector<
                NavigationTraversalBlock>{
                    NavigationTraversalBlock{
                        portal_cross_supplied_id,
                        first
                    }
                });

    if (!cross_supplied_block_result.has_value())
    {
        return 1;
    }

    const auto cross_supplied_blocked =
        query_navigation_effective_traversal_candidates(
            topology_ab,
            cross_supplied_block_result.value(),
            a1);

    check(
        state,
        cross_supplied_blocked.has_value() &&
            lacks_candidate(
                cross_supplied_blocked.value(),
                portal_cross_supplied_id),
        "Blocked cross-cell supplied edge is removed");

    const auto unavailable_block_result =
        NavigationObstacleOverlay::create(
            std::vector<
                NavigationTraversalBlock>{
                    NavigationTraversalBlock{
                        portal_cross_unavailable_id,
                        first
                    }
                });

    if (!unavailable_block_result.has_value())
    {
        return 1;
    }

    const auto unavailable_blocked =
        query_navigation_effective_traversal_candidates(
            topology_ab,
            unavailable_block_result.value(),
            a1);

    check(
        state,
        unavailable_blocked.has_value() &&
            lacks_candidate(
                unavailable_blocked.value(),
                portal_cross_unavailable_id),
        "Blocked unavailable frontier is removed");

    const NavigationTraversalCandidate*
        unblocked_unavailable =
            find_candidate(
                effective_a1_empty.value(),
                portal_cross_unavailable_id);

    check(
        state,
        unblocked_unavailable != nullptr &&
            unblocked_unavailable->target ==
                u1 &&
            unblocked_unavailable->target_kind ==
                NavigationTraversalTargetKind::
                    unavailable,
        "Unblocked unavailable frontier is retained");

    const NavigationTraversalCandidate*
        retained_supplied =
            find_candidate(
                effective_a1_empty.value(),
                portal_cross_supplied_id);

    check(
        state,
        retained_supplied != nullptr &&
            retained_supplied->target ==
                b1 &&
            retained_supplied->target_kind ==
                NavigationTraversalTargetKind::
                    supplied,
        "Retained supplied classification is unchanged");

    check(
        state,
        unblocked_unavailable != nullptr &&
            unblocked_unavailable->target_kind ==
                NavigationTraversalTargetKind::
                    unavailable,
        "Retained unavailable classification is unchanged");

    const auto opposite_direction_result =
        NavigationObstacleOverlay::create(
            std::vector<
                NavigationTraversalBlock>{
                    NavigationTraversalBlock{
                        portal_to_a2_id,
                        second
                    }
                });

    if (!opposite_direction_result.has_value())
    {
        return 1;
    }

    const auto opposite_direction_a1 =
        query_navigation_effective_traversal_candidates(
            topology_ab,
            opposite_direction_result.value(),
            a1);

    check(
        state,
        opposite_direction_a1.has_value() &&
            find_candidate(
                opposite_direction_a1.value(),
                portal_to_a2_id) !=
                nullptr,
        "Opposite-direction block does not suppress candidate");

    const auto matching_direction_result =
        NavigationObstacleOverlay::create(
            std::vector<
                NavigationTraversalBlock>{
                    NavigationTraversalBlock{
                        portal_to_a2_id,
                        first
                    }
                });

    if (!matching_direction_result.has_value())
    {
        return 1;
    }

    const auto matching_direction_a1 =
        query_navigation_effective_traversal_candidates(
            topology_ab,
            matching_direction_result.value(),
            a1);

    check(
        state,
        matching_direction_a1.has_value() &&
            lacks_candidate(
                matching_direction_a1.value(),
                portal_to_a2_id),
        "Directional block suppresses only matching direction");

    const auto both_directions_result =
        NavigationObstacleOverlay::create(
            std::vector<
                NavigationTraversalBlock>{
                    NavigationTraversalBlock{
                        portal_to_a2_id,
                        first
                    },
                    NavigationTraversalBlock{
                        portal_to_a2_id,
                        second
                    }
                });

    if (!both_directions_result.has_value())
    {
        return 1;
    }

    const auto both_from_a1 =
        query_navigation_effective_traversal_candidates(
            topology_ab,
            both_directions_result.value(),
            a1);

    const auto both_from_a2 =
        query_navigation_effective_traversal_candidates(
            topology_ab,
            both_directions_result.value(),
            a2);

    check(
        state,
        both_from_a1.has_value() &&
            both_from_a2.has_value() &&
            lacks_candidate(
                both_from_a1.value(),
                portal_to_a2_id) &&
            lacks_candidate(
                both_from_a2.value(),
                portal_to_a2_id),
        "Both directions may be independently blocked");

    const auto unknown_block_result =
        NavigationObstacleOverlay::create(
            std::vector<
                NavigationTraversalBlock>{
                    NavigationTraversalBlock{
                        unknown_portal_id,
                        first
                    }
                });

    if (!unknown_block_result.has_value())
    {
        return 1;
    }

    const auto unknown_block_effective =
        query_navigation_effective_traversal_candidates(
            topology_ab,
            unknown_block_result.value(),
            a1);

    check(
        state,
        unknown_block_effective.has_value() &&
            unknown_block_effective.value() ==
                structural_a1.value(),
        "Unknown portal block is inert");

    const auto forbidden_direction_result =
        NavigationObstacleOverlay::create(
            std::vector<
                NavigationTraversalBlock>{
                    NavigationTraversalBlock{
                        portal_reverse_only_id,
                        first
                    }
                });

    if (!forbidden_direction_result.has_value())
    {
        return 1;
    }

    const auto forbidden_direction_effective =
        query_navigation_effective_traversal_candidates(
            topology_ab,
            forbidden_direction_result.value(),
            a1);

    check(
        state,
        forbidden_direction_effective.
                has_value() &&
            forbidden_direction_effective.
                value() ==
                structural_a1.value(),
        "Structurally forbidden-direction block is inert");

    const auto parallel_block_result =
        NavigationObstacleOverlay::create(
            std::vector<
                NavigationTraversalBlock>{
                    NavigationTraversalBlock{
                        portal_to_a2_id,
                        first
                    }
                });

    if (!parallel_block_result.has_value())
    {
        return 1;
    }

    const auto parallel_effective =
        query_navigation_effective_traversal_candidates(
            topology_ab,
            parallel_block_result.value(),
            a1);

    check(
        state,
        parallel_effective.has_value() &&
            lacks_candidate(
                parallel_effective.value(),
                portal_to_a2_id) &&
            find_candidate(
                parallel_effective.value(),
                portal_parallel_id) !=
                nullptr,
        "Parallel portal P1 may be blocked while P2 remains");

    const auto ordering_overlay_result =
        NavigationObstacleOverlay::create(
            std::vector<
                NavigationTraversalBlock>{
                    NavigationTraversalBlock{
                        portal_parallel_id,
                        first
                    },
                    NavigationTraversalBlock{
                        portal_cross_supplied_id,
                        first
                    }
                });

    if (!ordering_overlay_result.has_value())
    {
        return 1;
    }

    const auto ordering_effective =
        query_navigation_effective_traversal_candidates(
            topology_ab,
            ordering_overlay_result.value(),
            a1);

    std::vector<
        NavigationTraversalCandidate>
        expected_filtered_order{};

    for (
        const NavigationTraversalCandidate&
            candidate :
        structural_a1.value())
    {
        if (
            candidate.portal ==
                portal_parallel_id ||
            candidate.portal ==
                portal_cross_supplied_id)
        {
            continue;
        }

        expected_filtered_order.push_back(
            candidate);
    }

    check(
        state,
        ordering_effective.has_value() &&
            ordering_effective.value() ==
                expected_filtered_order,
        "Effective ordering equals structural ordering minus removals");

    check(
        state,
        ordering_effective.has_value() &&
            ordering_effective.value().
                size() >= 2 &&
            ordering_effective.value()[0].
                portal ==
                portal_to_a3_id &&
            ordering_effective.value()[0].
                target ==
                a3 &&
            ordering_effective.value()[1].
                portal ==
                portal_to_a2_id &&
            ordering_effective.value()[1].
                target ==
                a2 &&
            ordering_effective.value()[0].
                portal <
                ordering_effective.value()[1].
                    portal &&
            ordering_effective.value()[1].
                target <
                ordering_effective.value()[0].
                    target,
        "Target-node ordering never replaces portal ordering");

    const auto all_blocked_result =
        NavigationObstacleOverlay::create(
            std::vector<
                NavigationTraversalBlock>{
                    NavigationTraversalBlock{
                        portal_to_a3_id,
                        first
                    },
                    NavigationTraversalBlock{
                        portal_to_a2_id,
                        first
                    },
                    NavigationTraversalBlock{
                        portal_parallel_id,
                        first
                    },
                    NavigationTraversalBlock{
                        portal_cross_supplied_id,
                        first
                    },
                    NavigationTraversalBlock{
                        portal_cross_unavailable_id,
                        first
                    }
                });

    if (!all_blocked_result.has_value())
    {
        return 1;
    }

    const auto all_blocked_effective =
        query_navigation_effective_traversal_candidates(
            topology_ab,
            all_blocked_result.value(),
            a1);

    check(
        state,
        all_blocked_effective.has_value() &&
            all_blocked_effective.value().
                empty(),
        "All candidates blocked returns successful empty vector");

    const auto repeated_first =
        query_navigation_effective_traversal_candidates(
            topology_ab,
            ordering_overlay_result.value(),
            a1);

    const auto repeated_second =
        query_navigation_effective_traversal_candidates(
            topology_ab,
            ordering_overlay_result.value(),
            a1);

    check(
        state,
        repeated_first.has_value() &&
            repeated_second.has_value() &&
            repeated_first.value() ==
                repeated_second.value(),
        "Repeated identical query produces equal output");

    std::cout
        << state.checks
        << " checks, "
        << state.failures
        << " failures\n";

    return state.failures == 0
        ? 0
        : 1;
}
