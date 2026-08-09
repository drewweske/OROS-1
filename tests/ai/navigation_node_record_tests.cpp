#include "oros/ai/navigation_node_record.hpp"

#include <cstdint>
#include <iostream>
#include <limits>
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
            NavigationNodeRecord>);

    static_assert(
        std::is_copy_constructible_v<
            NavigationNodeRecord>);

    static_assert(
        std::is_copy_assignable_v<
            NavigationNodeRecord>);

    static_assert(
        std::is_nothrow_move_constructible_v<
            NavigationNodeRecord>);

    static_assert(
        std::is_nothrow_move_assignable_v<
            NavigationNodeRecord>);

    static_assert(
        std::is_same_v<
            decltype(
                NavigationNodeRecord::create(
                    std::declval<
                        NavigationNodeId>(),
                    std::declval<
                        const WorldPosition&>())),
            Result<
                NavigationNodeRecord>>);

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const NavigationNodeRecord&>().
                    id()),
            NavigationNodeId>);

    static_assert(
        noexcept(
            std::declval<
                const NavigationNodeRecord&>().
                id()));

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const NavigationNodeRecord&>().
                    position()),
            const WorldPosition&>);

    static_assert(
        noexcept(
            std::declval<
                const NavigationNodeRecord&>().
                position()));

    TestState state{};

    auto origin_position_result =
        WorldPosition::origin();

    if (!origin_position_result.has_value())
    {
        return 1;
    }

    const WorldPosition&
        origin_position =
            origin_position_result.value();

    const NavigationNodeId
        invalid_id{
            WorldCell{
                0,
                0,
                0
            },
            0ULL
        };

    const auto invalid_id_result =
        NavigationNodeRecord::create(
            invalid_id,
            origin_position);

    check(
        state,
        !invalid_id_result.has_value() &&
            invalid_id_result.error().code ==
                ErrorCode::invalid_argument,
        "Invalid navigation node id returns invalid_argument");

    const NavigationNodeId
        origin_id{
            WorldCell{
                0,
                0,
                0
            },
            1ULL
        };

    const auto origin_record_result =
        NavigationNodeRecord::create(
            origin_id,
            origin_position);

    check(
        state,
        origin_record_result.has_value(),
        "Valid origin node and origin position are accepted");

    if (!origin_record_result.has_value())
    {
        return 1;
    }

    const NavigationNodeRecord&
        origin_record =
            origin_record_result.value();

    const WorldCell
        negative_cell{
            -17,
            -29,
            -43
        };

    const LocalPosition
        negative_local{
            -111.25,
            72.5,
            255.75
        };

    const auto negative_position_result =
        WorldPosition::create(
            negative_cell,
            negative_local);

    if (!negative_position_result.has_value())
    {
        return 1;
    }

    const NavigationNodeId
        negative_id{
            negative_cell,
            91ULL
        };

    const auto negative_record_result =
        NavigationNodeRecord::create(
            negative_id,
            negative_position_result.value());

    check(
        state,
        negative_record_result.has_value() &&
            negative_record_result.value().
                id() ==
                negative_id &&
            negative_record_result.value().
                position().cell() ==
                negative_cell,
        "Matching negative world cell is accepted");

    const WorldCell
        extreme_cell{
            (std::numeric_limits<
                std::int64_t>::min)(),
            (std::numeric_limits<
                std::int64_t>::max)(),
            (std::numeric_limits<
                std::int64_t>::min)()
        };

    const auto extreme_position_result =
        WorldPosition::create(
            extreme_cell,
            LocalPosition{});

    if (!extreme_position_result.has_value())
    {
        return 1;
    }

    const NavigationNodeId
        extreme_id{
            extreme_cell,
            (std::numeric_limits<
                std::uint64_t>::max)()
        };

    const auto extreme_record_result =
        NavigationNodeRecord::create(
            extreme_id,
            extreme_position_result.value());

    check(
        state,
        extreme_record_result.has_value() &&
            extreme_record_result.value().
                id() ==
                extreme_id &&
            extreme_record_result.value().
                position().cell() ==
                extreme_cell,
        "Matching extreme legal WorldCell is accepted");

    const auto mismatched_x_position_result =
        WorldPosition::create(
            WorldCell{
                1,
                0,
                0
            },
            LocalPosition{});

    const auto mismatched_y_position_result =
        WorldPosition::create(
            WorldCell{
                0,
                1,
                0
            },
            LocalPosition{});

    const auto mismatched_z_position_result =
        WorldPosition::create(
            WorldCell{
                0,
                0,
                1
            },
            LocalPosition{});

    if (
        !mismatched_x_position_result.has_value() ||
        !mismatched_y_position_result.has_value() ||
        !mismatched_z_position_result.has_value())
    {
        return 1;
    }

    const auto mismatched_x_result =
        NavigationNodeRecord::create(
            origin_id,
            mismatched_x_position_result.value());

    check(
        state,
        !mismatched_x_result.has_value() &&
            mismatched_x_result.error().code ==
                ErrorCode::invalid_argument,
        "Mismatched world-cell x is rejected");

    const auto mismatched_y_result =
        NavigationNodeRecord::create(
            origin_id,
            mismatched_y_position_result.value());

    check(
        state,
        !mismatched_y_result.has_value() &&
            mismatched_y_result.error().code ==
                ErrorCode::invalid_argument,
        "Mismatched world-cell y is rejected");

    const auto mismatched_z_result =
        NavigationNodeRecord::create(
            origin_id,
            mismatched_z_position_result.value());

    check(
        state,
        !mismatched_z_result.has_value() &&
            mismatched_z_result.error().code ==
                ErrorCode::invalid_argument,
        "Mismatched world-cell z is rejected");

    check(
        state,
        origin_record.id() ==
            origin_id,
        "Navigation node id accessor returns the exact identity");

    check(
        state,
        origin_record.position() ==
            origin_position,
        "Navigation node position accessor returns the exact position");

    const WorldCell
        payload_cell{
            125,
            -77,
            901
        };

    const LocalPosition
        payload_local{
            123.125,
            -456.5,
            0.03125
        };

    const auto payload_position_result =
        WorldPosition::create(
            payload_cell,
            payload_local);

    if (!payload_position_result.has_value())
    {
        return 1;
    }

    const NavigationNodeId
        payload_id{
            payload_cell,
            875ULL
        };

    const auto payload_record_result =
        NavigationNodeRecord::create(
            payload_id,
            payload_position_result.value());

    if (!payload_record_result.has_value())
    {
        return 1;
    }

    const NavigationNodeRecord&
        payload_record =
            payload_record_result.value();

    check(
        state,
        payload_record.position().local().x ==
                payload_local.x &&
            payload_record.position().local().y ==
                payload_local.y &&
            payload_record.position().local().z ==
                payload_local.z,
        "Navigation node record preserves local double position exactly");

    const auto equal_record_result =
        NavigationNodeRecord::create(
            payload_id,
            payload_position_result.value());

    if (!equal_record_result.has_value())
    {
        return 1;
    }

    check(
        state,
        payload_record ==
            equal_record_result.value(),
        "Equal navigation node records compare equal");

    const NavigationNodeId
        changed_id{
            payload_cell,
            876ULL
        };

    const auto changed_id_record_result =
        NavigationNodeRecord::create(
            changed_id,
            payload_position_result.value());

    if (!changed_id_record_result.has_value())
    {
        return 1;
    }

    check(
        state,
        payload_record !=
            changed_id_record_result.value(),
        "Changing navigation node identity changes the record");

    const LocalPosition
        changed_local{
            124.125,
            -456.5,
            0.03125
        };

    const auto changed_position_result =
        WorldPosition::create(
            payload_cell,
            changed_local);

    if (!changed_position_result.has_value())
    {
        return 1;
    }

    const auto changed_position_record_result =
        NavigationNodeRecord::create(
            payload_id,
            changed_position_result.value());

    if (!changed_position_record_result.has_value())
    {
        return 1;
    }

    check(
        state,
        payload_record !=
            changed_position_record_result.value(),
        "Changing navigation node position changes the record");

    const NavigationNodeRecord
        copied_record{
            payload_record
        };

    check(
        state,
        copied_record ==
            payload_record,
        "Navigation node record supports copy construction");

    NavigationNodeRecord
        assigned_record{
            changed_id_record_result.value()
        };

    assigned_record =
        payload_record;

    check(
        state,
        assigned_record ==
            payload_record,
        "Navigation node record supports copy assignment");

    std::cout
        << state.checks
        << " checks, "
        << state.failures
        << " failures\n";

    return state.failures == 0
        ? 0
        : 1;
}
