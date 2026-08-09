#include "oros/ai/navigation_portal_record.hpp"

#include <iostream>
#include <string_view>
#include <type_traits>
#include <utility>

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
            NavigationPortalRecord>);

    static_assert(
        std::is_copy_constructible_v<
            NavigationPortalRecord>);

    static_assert(
        std::is_copy_assignable_v<
            NavigationPortalRecord>);

    static_assert(
        std::is_nothrow_move_constructible_v<
            NavigationPortalRecord>);

    static_assert(
        std::is_nothrow_move_assignable_v<
            NavigationPortalRecord>);

    static_assert(
        std::is_same_v<
            decltype(
                NavigationPortalRecord::create(
                    std::declval<
                        NavigationPortalId>(),
                    std::declval<
                        NavigationNodeId>(),
                    std::declval<
                        NavigationNodeId>(),
                    true,
                    true)),
            Result<
                NavigationPortalRecord>>);

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const NavigationPortalRecord&>().
                    id()),
            NavigationPortalId>);

    static_assert(
        noexcept(
            std::declval<
                const NavigationPortalRecord&>().
                id()));

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const NavigationPortalRecord&>().
                    first_node()),
            NavigationNodeId>);

    static_assert(
        noexcept(
            std::declval<
                const NavigationPortalRecord&>().
                first_node()));

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const NavigationPortalRecord&>().
                    second_node()),
            NavigationNodeId>);

    static_assert(
        noexcept(
            std::declval<
                const NavigationPortalRecord&>().
                second_node()));

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const NavigationPortalRecord&>().
                    first_to_second()),
            bool>);

    static_assert(
        noexcept(
            std::declval<
                const NavigationPortalRecord&>().
                first_to_second()));

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const NavigationPortalRecord&>().
                    second_to_first()),
            bool>);

    static_assert(
        noexcept(
            std::declval<
                const NavigationPortalRecord&>().
                second_to_first()));

    TestState state{};

    const NavigationPortalId
        portal_id{
            WorldCell{
                7,
                -3,
                11
            },
            55ULL
        };

    const NavigationNodeId
        node_a{
            WorldCell{
                7,
                -3,
                11
            },
            10ULL
        };

    const NavigationNodeId
        node_b{
            WorldCell{
                7,
                -3,
                11
            },
            20ULL
        };

    const auto invalid_portal_result =
        NavigationPortalRecord::create(
            invalid_navigation_portal_id,
            node_a,
            node_b,
            true,
            true);

    check(
        state,
        !invalid_portal_result.has_value() &&
            invalid_portal_result.error().code ==
                ErrorCode::invalid_argument,
        "Invalid portal identity returns invalid_argument");

    const auto invalid_endpoint_a_result =
        NavigationPortalRecord::create(
            portal_id,
            invalid_navigation_node_id,
            node_b,
            true,
            true);

    check(
        state,
        !invalid_endpoint_a_result.has_value() &&
            invalid_endpoint_a_result.error().code ==
                ErrorCode::invalid_argument,
        "Invalid endpoint A returns invalid_argument");

    const auto invalid_endpoint_b_result =
        NavigationPortalRecord::create(
            portal_id,
            node_a,
            invalid_navigation_node_id,
            true,
            true);

    check(
        state,
        !invalid_endpoint_b_result.has_value() &&
            invalid_endpoint_b_result.error().code ==
                ErrorCode::invalid_argument,
        "Invalid endpoint B returns invalid_argument");

    const auto identical_endpoints_result =
        NavigationPortalRecord::create(
            portal_id,
            node_a,
            node_a,
            true,
            true);

    check(
        state,
        !identical_endpoints_result.has_value() &&
            identical_endpoints_result.error().code ==
                ErrorCode::invalid_argument,
        "Identical portal endpoints return invalid_argument");

    const auto no_direction_result =
        NavigationPortalRecord::create(
            portal_id,
            node_a,
            node_b,
            false,
            false);

    check(
        state,
        !no_direction_result.has_value() &&
            no_direction_result.error().code ==
                ErrorCode::invalid_argument,
        "Portal with no traversal direction returns invalid_argument");

    const auto bidirectional_result =
        NavigationPortalRecord::create(
            portal_id,
            node_a,
            node_b,
            true,
            true);

    check(
        state,
        bidirectional_result.has_value(),
        "Bidirectional same-cell portal is accepted");

    if (!bidirectional_result.has_value())
    {
        return 1;
    }

    const NavigationPortalRecord&
        bidirectional =
            bidirectional_result.value();

    check(
        state,
        bidirectional.id() ==
            portal_id,
        "Portal id accessor returns the exact identity");

    check(
        state,
        bidirectional.first_node() ==
                node_a &&
            bidirectional.second_node() ==
                node_b,
        "Canonical portal endpoints preserve already ordered input");

    check(
        state,
        bidirectional.first_to_second() &&
            bidirectional.second_to_first(),
        "Bidirectional portal preserves both traversal directions");

    const auto first_only_result =
        NavigationPortalRecord::create(
            portal_id,
            node_a,
            node_b,
            true,
            false);

    check(
        state,
        first_only_result.has_value() &&
            first_only_result.value().
                first_to_second() &&
            !first_only_result.value().
                second_to_first(),
        "One-way first-to-second portal is accepted");

    const auto second_only_result =
        NavigationPortalRecord::create(
            portal_id,
            node_a,
            node_b,
            false,
            true);

    check(
        state,
        second_only_result.has_value() &&
            !second_only_result.value().
                first_to_second() &&
            second_only_result.value().
                second_to_first(),
        "One-way second-to-first portal is accepted");

    const auto reversed_semantic_result =
        NavigationPortalRecord::create(
            portal_id,
            node_b,
            node_a,
            false,
            true);

    check(
        state,
        reversed_semantic_result.has_value() &&
            first_only_result.has_value() &&
            reversed_semantic_result.value() ==
                first_only_result.value(),
        "Reversed endpoint input with swapped direction produces equal canonical record");

    const NavigationNodeId
        cross_cell_first{
            WorldCell{
                -100,
                0,
                25
            },
            900ULL
        };

    const NavigationNodeId
        cross_cell_second{
            WorldCell{
                101,
                -2,
                26
            },
            1ULL
        };

    const NavigationPortalId
        independent_namespace_id{
            WorldCell{
                500,
                500,
                500
            },
            2ULL
        };

    const auto cross_cell_result =
        NavigationPortalRecord::create(
            independent_namespace_id,
            cross_cell_first,
            cross_cell_second,
            true,
            true);

    check(
        state,
        cross_cell_result.has_value() &&
            cross_cell_result.value().
                first_node() ==
                cross_cell_first &&
            cross_cell_result.value().
                second_node() ==
                cross_cell_second,
        "Cross-cell endpoints are accepted");

    check(
        state,
        cross_cell_result.has_value() &&
            cross_cell_result.value().
                id().cell !=
                cross_cell_result.value().
                    first_node().cell &&
            cross_cell_result.value().
                id().cell !=
                cross_cell_result.value().
                    second_node().cell,
        "Portal identity namespace cell is independent from endpoint cells");

    const NavigationNodeId
        larger_local{
            WorldCell{
                12,
                4,
                -8
            },
            99ULL
        };

    const NavigationNodeId
        smaller_local{
            WorldCell{
                12,
                4,
                -8
            },
            11ULL
        };

    const auto reversed_order_result =
        NavigationPortalRecord::create(
            portal_id,
            larger_local,
            smaller_local,
            true,
            false);

    check(
        state,
        reversed_order_result.has_value() &&
            reversed_order_result.value().
                first_node() ==
                smaller_local &&
            reversed_order_result.value().
                second_node() ==
                larger_local,
        "Portal factory canonicalizes endpoint ordering");

    check(
        state,
        reversed_order_result.has_value() &&
            !reversed_order_result.value().
                first_to_second() &&
            reversed_order_result.value().
                second_to_first(),
        "Canonical endpoint swap also swaps traversal direction payload");

    const NavigationPortalId
        changed_portal_id{
            portal_id.cell,
            56ULL
        };

    const auto changed_id_result =
        NavigationPortalRecord::create(
            changed_portal_id,
            node_a,
            node_b,
            true,
            true);

    if (!changed_id_result.has_value())
    {
        return 1;
    }

    check(
        state,
        bidirectional !=
            changed_id_result.value(),
        "Changing portal identity changes the structural record");

    const NavigationNodeId
        node_c{
            WorldCell{
                7,
                -3,
                11
            },
            30ULL
        };

    const auto changed_endpoint_result =
        NavigationPortalRecord::create(
            portal_id,
            node_a,
            node_c,
            true,
            true);

    if (!changed_endpoint_result.has_value())
    {
        return 1;
    }

    check(
        state,
        bidirectional !=
            changed_endpoint_result.value(),
        "Changing an endpoint changes the structural record");

    const auto changed_direction_result =
        NavigationPortalRecord::create(
            portal_id,
            node_a,
            node_b,
            true,
            false);

    if (!changed_direction_result.has_value())
    {
        return 1;
    }

    check(
        state,
        bidirectional !=
            changed_direction_result.value(),
        "Changing structural traversal direction changes the record");

    const NavigationPortalRecord
        copied_record{
            bidirectional
        };

    check(
        state,
        copied_record ==
            bidirectional,
        "Navigation portal record supports copy construction");

    NavigationPortalRecord
        assigned_record{
            changed_endpoint_result.value()
        };

    assigned_record =
        bidirectional;

    check(
        state,
        assigned_record ==
            bidirectional,
        "Navigation portal record supports copy assignment");

    std::cout
        << state.checks
        << " checks, "
        << state.failures
        << " failures\n";

    return state.failures == 0
        ? 0
        : 1;
}
