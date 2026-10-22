#include "oros/ai/navigation_topology.hpp"

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

        const auto position_result =
            world::WorldPosition::create(
                cell,
                world::LocalPosition{
                    local_x,
                    0.0,
                    0.0
                });

        if (!position_result.has_value())
        {
            return foundation::fail(
                position_result.error().code,
                "NavigationTopology test could "
                "not construct node position.");
        }

        return
            ai::NavigationNodeRecord::create(
                ai::NavigationNodeId{
                    cell,
                    local_id
                },
                position_result.value());
    }

    oros::foundation::Result<
        oros::ai::NavigationCellTopology>
    make_empty_cell(
        const oros::world::WorldCell cell)
    {
        return
            oros::ai::
                NavigationCellTopology::create(
                    cell,
                    std::span<
                        const oros::ai::
                            NavigationNodeRecord>{},
                    std::span<
                        const oros::ai::
                            NavigationPortalRecord>{});
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
        std::is_default_constructible_v<
            NavigationTopology>);

    static_assert(
        std::is_copy_constructible_v<
            NavigationTopology>);

    static_assert(
        std::is_copy_assignable_v<
            NavigationTopology>);

    static_assert(
        std::is_nothrow_move_constructible_v<
            NavigationTopology>);

    static_assert(
        std::is_nothrow_move_assignable_v<
            NavigationTopology>);

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    NavigationTopology&>().
                    set_cell_topology(
                        std::declval<
                            const NavigationCellTopology&>())),
            Status>);

    static_assert(
        noexcept(
            std::declval<
                NavigationTopology&>().
                remove_cell_topology(
                    std::declval<
                        WorldCell>())));

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const NavigationTopology&>().
                    find_cell_topology(
                        std::declval<
                            WorldCell>())),
            const NavigationCellTopology*>);

    static_assert(
        noexcept(
            std::declval<
                const NavigationTopology&>().
                find_cell_topology(
                    std::declval<
                        WorldCell>())));

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const NavigationTopology&>().
                    cells_in_canonical_order()),
            std::span<
                const NavigationCellTopology>>);

    TestState state{};

    const WorldCell cell_a{0, 0, 0};
    const WorldCell cell_b{1, 0, 0};
    const WorldCell cell_c{-1, 0, 0};
    const WorldCell cell_d{3, 0, 0};
    const WorldCell cell_e{4, 0, 0};

    NavigationTopology empty_topology{};

    check(
        state,
        empty_topology.empty() &&
            empty_topology.size() == 0,
        "Default aggregate means zero supplied cells");

    check(
        state,
        empty_topology.
            find_cell_topology(
                cell_a) == nullptr,
        "Absent find returns nullptr");

    const auto empty_a_result =
        make_empty_cell(cell_a);

    const auto empty_b_result =
        make_empty_cell(cell_b);

    const auto empty_c_result =
        make_empty_cell(cell_c);

    if (
        !empty_a_result.has_value() ||
        !empty_b_result.has_value() ||
        !empty_c_result.has_value())
    {
        return 1;
    }

    NavigationTopology explicit_empty{};

    const Status explicit_empty_status =
        explicit_empty.
            set_cell_topology(
                empty_a_result.value());

    check(
        state,
        explicit_empty_status.has_value() &&
            explicit_empty.
                find_cell_topology(
                    cell_a) != nullptr &&
            explicit_empty.
                find_cell_topology(
                    cell_a)->empty(),
        "Explicit empty cell remains findable and non-null");

    NavigationTopology canonical_order{};

    if (
        !canonical_order.
            set_cell_topology(
                empty_b_result.value()).
            has_value() ||
        !canonical_order.
            set_cell_topology(
                empty_c_result.value()).
            has_value() ||
        !canonical_order.
            set_cell_topology(
                empty_a_result.value()).
            has_value())
    {
        return 1;
    }

    const auto ordered_cells =
        canonical_order.
            cells_in_canonical_order();

    check(
        state,
        ordered_cells.size() == 3 &&
            ordered_cells[0].cell() ==
                cell_c &&
            ordered_cells[1].cell() ==
                cell_a &&
            ordered_cells[2].cell() ==
                cell_b,
        "Cells canonicalize by WorldCell");

    check(
        state,
        ordered_cells[0] ==
                empty_c_result.value() &&
            ordered_cells[1] ==
                empty_a_result.value() &&
            ordered_cells[2] ==
                empty_b_result.value(),
        "Canonical span preserves exact cell topology values");

    NavigationTopology identical_set{};

    const Status identical_first =
        identical_set.
            set_cell_topology(
                empty_a_result.value());

    const Status identical_second =
        identical_set.
            set_cell_topology(
                empty_a_result.value());

    check(
        state,
        identical_first.has_value() &&
            identical_second.has_value() &&
            identical_set.size() == 1,
        "Identical set succeeds as no-op");

    const auto node_c_result =
        make_node(
            cell_c,
            1ULL,
            10.0);

    if (!node_c_result.has_value())
    {
        return 1;
    }

    const std::vector<
        NavigationNodeRecord>
        cell_c_nodes{
            node_c_result.value()
        };

    const auto populated_c_result =
        NavigationCellTopology::create(
            cell_c,
            cell_c_nodes,
            std::span<
                const NavigationPortalRecord>{});

    if (!populated_c_result.has_value())
    {
        return 1;
    }

    NavigationTopology replacement{};

    if (
        !replacement.
            set_cell_topology(
                empty_c_result.value()).
            has_value())
    {
        return 1;
    }

    const Status replacement_status =
        replacement.
            set_cell_topology(
                populated_c_result.value());

    check(
        state,
        replacement_status.has_value() &&
            replacement.size() == 1 &&
            replacement.
                find_cell_topology(
                    cell_c) != nullptr &&
            *replacement.
                find_cell_topology(
                    cell_c) ==
                populated_c_result.value(),
        "Changed same-cell value replaces whole value");

    const auto node_a_result =
        make_node(
            cell_a,
            10ULL,
            10.0);

    const auto node_b_result =
        make_node(
            cell_b,
            20ULL,
            20.0);

    const auto node_d_result =
        make_node(
            cell_d,
            30ULL,
            30.0);

    if (
        !node_a_result.has_value() ||
        !node_b_result.has_value() ||
        !node_d_result.has_value())
    {
        return 1;
    }

    const NavigationNodeId node_a_id =
        node_a_result.value().id();

    const NavigationNodeId node_b_id =
        node_b_result.value().id();

    const NavigationNodeId
        node_a_second_id{
            cell_a,
            11ULL
        };

    const NavigationNodeId
        node_e_id{
            cell_e,
            40ULL
        };

    const NavigationPortalId
        shared_portal_id{
            WorldCell{
                99,
                -99,
                7
            },
            100ULL
        };

    const auto shared_portal_result =
        NavigationPortalRecord::create(
            shared_portal_id,
            node_a_id,
            node_b_id,
            true,
            true);

    const auto direction_mismatch_result =
        NavigationPortalRecord::create(
            shared_portal_id,
            node_a_id,
            node_b_id,
            true,
            false);

    const auto endpoint_mismatch_result =
        NavigationPortalRecord::create(
            shared_portal_id,
            node_a_second_id,
            node_b_id,
            true,
            true);

    const auto unrelated_portal_result =
        NavigationPortalRecord::create(
            shared_portal_id,
            node_d_result.value().id(),
            node_e_id,
            true,
            true);

    if (
        !shared_portal_result.has_value() ||
        !direction_mismatch_result.has_value() ||
        !endpoint_mismatch_result.has_value() ||
        !unrelated_portal_result.has_value())
    {
        return 1;
    }

    const std::vector<
        NavigationNodeRecord>
        a_nodes{
            node_a_result.value()
        };

    const std::vector<
        NavigationNodeRecord>
        b_nodes{
            node_b_result.value()
        };

    const std::vector<
        NavigationNodeRecord>
        d_nodes{
            node_d_result.value()
        };

    const std::vector<
        NavigationPortalRecord>
        shared_portals{
            shared_portal_result.value()
        };

    const std::vector<
        NavigationPortalRecord>
        direction_mismatch_portals{
            direction_mismatch_result.value()
        };

    const std::vector<
        NavigationPortalRecord>
        endpoint_mismatch_portals{
            endpoint_mismatch_result.value()
        };

    const std::vector<
        NavigationPortalRecord>
        unrelated_portals{
            unrelated_portal_result.value()
        };

    const auto topology_a_result =
        NavigationCellTopology::create(
            cell_a,
            a_nodes,
            shared_portals);

    const auto topology_b_result =
        NavigationCellTopology::create(
            cell_b,
            b_nodes,
            shared_portals);

    const auto topology_b_direction_result =
        NavigationCellTopology::create(
            cell_b,
            b_nodes,
            direction_mismatch_portals);

    const auto topology_b_endpoint_result =
        NavigationCellTopology::create(
            cell_b,
            b_nodes,
            endpoint_mismatch_portals);

    const auto topology_d_result =
        NavigationCellTopology::create(
            cell_d,
            d_nodes,
            unrelated_portals);

    const auto topology_a_without_portal_result =
        NavigationCellTopology::create(
            cell_a,
            a_nodes,
            std::span<
                const NavigationPortalRecord>{});

    if (
        !topology_a_result.has_value() ||
        !topology_b_result.has_value() ||
        !topology_b_direction_result.has_value() ||
        !topology_b_endpoint_result.has_value() ||
        !topology_d_result.has_value() ||
        !topology_a_without_portal_result.has_value())
    {
        return 1;
    }

    NavigationTopology unavailable_neighbor{};

    const Status a_only_status =
        unavailable_neighbor.
            set_cell_topology(
                topology_a_result.value());

    check(
        state,
        a_only_status.has_value() &&
            unavailable_neighbor.
                find_cell_topology(
                    cell_b) == nullptr,
        "A may reference unavailable B");

    NavigationTopology matching_boundary{};

    const Status matching_a =
        matching_boundary.
            set_cell_topology(
                topology_a_result.value());

    const Status matching_b =
        matching_boundary.
            set_cell_topology(
                topology_b_result.value());

    check(
        state,
        matching_a.has_value() &&
            matching_b.has_value() &&
            matching_boundary.size() == 2,
        "Matching A/B cross-cell projections succeed");

    NavigationTopology reverse_matching_boundary{};

    const Status reverse_b =
        reverse_matching_boundary.
            set_cell_topology(
                topology_b_result.value());

    const Status reverse_a =
        reverse_matching_boundary.
            set_cell_topology(
                topology_a_result.value());

    check(
        state,
        reverse_b.has_value() &&
            reverse_a.has_value() &&
            reverse_matching_boundary ==
                matching_boundary,
        "Matching boundary succeeds independent of insertion order");

    NavigationTopology known_empty_first{};

    if (
        !known_empty_first.
            set_cell_topology(
                empty_b_result.value()).
            has_value())
    {
        return 1;
    }

    const Status known_empty_rejection =
        known_empty_first.
            set_cell_topology(
                topology_a_result.value());

    check(
        state,
        is_invalid_argument(
            known_empty_rejection) &&
            known_empty_first.size() == 1 &&
            known_empty_first.
                find_cell_topology(
                    cell_a) == nullptr,
        "Supplied known-empty B rejects incoming A portal");

    NavigationTopology existing_boundary_first{};

    if (
        !existing_boundary_first.
            set_cell_topology(
                topology_a_result.value()).
            has_value())
    {
        return 1;
    }

    const Status missing_incoming_projection =
        existing_boundary_first.
            set_cell_topology(
                empty_b_result.value());

    check(
        state,
        is_invalid_argument(
            missing_incoming_projection) &&
            existing_boundary_first.
                find_cell_topology(
                    cell_b) == nullptr,
        "Existing neighbor portal missing from incoming cell is rejected");

    NavigationTopology direction_conflict{};

    if (
        !direction_conflict.
            set_cell_topology(
                topology_a_result.value()).
            has_value())
    {
        return 1;
    }

    const Status direction_conflict_status =
        direction_conflict.
            set_cell_topology(
                topology_b_direction_result.value());

    check(
        state,
        is_invalid_argument(
            direction_conflict_status),
        "Shared portal direction mismatch is rejected");

    NavigationTopology payload_conflict{};

    if (
        !payload_conflict.
            set_cell_topology(
                topology_a_result.value()).
            has_value())
    {
        return 1;
    }

    const Status payload_conflict_status =
        payload_conflict.
            set_cell_topology(
                topology_b_endpoint_result.value());

    check(
        state,
        is_invalid_argument(
            payload_conflict_status),
        "Shared portal payload mismatch is rejected");

    NavigationTopology unrelated_identity_conflict{};

    if (
        !unrelated_identity_conflict.
            set_cell_topology(
                topology_d_result.value()).
            has_value())
    {
        return 1;
    }

    const Status unrelated_conflict_status =
        unrelated_identity_conflict.
            set_cell_topology(
                topology_a_result.value());

    check(
        state,
        is_invalid_argument(
            unrelated_conflict_status),
        "Same portal id with conflicting unrelated record is rejected");

    NavigationTopology conflicting_replacement{
        matching_boundary
    };

    const Status conflicting_replacement_status =
        conflicting_replacement.
            set_cell_topology(
                topology_a_without_portal_result.
                    value());

    const NavigationCellTopology*
        preserved_a =
            conflicting_replacement.
                find_cell_topology(
                    cell_a);

    const NavigationCellTopology*
        preserved_b =
            conflicting_replacement.
                find_cell_topology(
                    cell_b);

    check(
        state,
        is_invalid_argument(
            conflicting_replacement_status) &&
            preserved_a != nullptr &&
            preserved_b != nullptr &&
            *preserved_a ==
                topology_a_result.value() &&
            *preserved_b ==
                topology_b_result.value(),
        "Conflicting replacement preserves old aggregate");

    NavigationTopology removal_case{
        matching_boundary
    };

    const bool removed_b =
        removal_case.
            remove_cell_topology(
                cell_b);

    const NavigationCellTopology*
        remaining_a =
            removal_case.
                find_cell_topology(
                    cell_a);

    check(
        state,
        removed_b &&
            removal_case.
                find_cell_topology(
                    cell_b) == nullptr &&
            remaining_a != nullptr &&
            remaining_a->
                portals_in_canonical_order().
                size() == 1 &&
            remaining_a->
                portals_in_canonical_order()[0] ==
                shared_portal_result.value(),
        "Removal preserves neighboring boundary portal");

    check(
        state,
        !removal_case.
            remove_cell_topology(
                cell_b),
        "Remove absent cell returns false");

    check(
        state,
        removal_case.
            remove_cell_topology(
                cell_a) &&
            removal_case.empty(),
        "Remove present cell returns true");

    const NavigationTopology
        copied_topology{
            matching_boundary
        };

    check(
        state,
        copied_topology ==
            matching_boundary,
        "Navigation topology supports copy construction");

    NavigationTopology assigned_topology{
        canonical_order
    };

    assigned_topology =
        matching_boundary;

    check(
        state,
        assigned_topology ==
            matching_boundary,
        "Navigation topology supports copy assignment");

    std::cout
        << state.checks
        << " checks, "
        << state.failures
        << " failures\n";

    return state.failures == 0
        ? 0
        : 1;
}
