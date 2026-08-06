#include "oros/streaming/world_cell_streaming_policy.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string_view>
#include <type_traits>

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

    int finish(
        const TestState& state)
    {
        std::cout
            << "\nWorld cell streaming policy test "
            << "summary: "
            << state.checks - state.failures
            << '/'
            << state.checks
            << " passed.\n";

        return
            state.failures == 0
                ? 0
                : 1;
    }
}

int main()
{
    using namespace oros::streaming;

    static_assert(
        std::is_trivially_copyable_v<
            WorldCellStreamingBudgets>);

    static_assert(
        std::is_trivially_copyable_v<
            WorldCellStreamingUsage>);

    static_assert(
        !is_valid_streaming_priority(
            WorldCellStreamingPriority::
                invalid));

    static_assert(
        is_valid_streaming_priority(
            WorldCellStreamingPriority::
                background));

    static_assert(
        is_valid_streaming_priority(
            WorldCellStreamingPriority::
                critical));

    static_assert(
        streaming_priority_rank(
            WorldCellStreamingPriority::
                invalid) ==
            0U);

    static_assert(
        streaming_priority_rank(
            WorldCellStreamingPriority::
                background) <
            streaming_priority_rank(
                WorldCellStreamingPriority::
                    low));

    static_assert(
        streaming_priority_rank(
            WorldCellStreamingPriority::
                low) <
            streaming_priority_rank(
                WorldCellStreamingPriority::
                    normal));

    static_assert(
        streaming_priority_rank(
            WorldCellStreamingPriority::
                normal) <
            streaming_priority_rank(
                WorldCellStreamingPriority::
                    high));

    static_assert(
        streaming_priority_rank(
            WorldCellStreamingPriority::
                high) <
            streaming_priority_rank(
                WorldCellStreamingPriority::
                    critical));

    static_assert(
        has_higher_streaming_priority(
            WorldCellStreamingPriority::
                critical,
            WorldCellStreamingPriority::
                high));

    static_assert(
        !has_higher_streaming_priority(
            WorldCellStreamingPriority::
                high,
            WorldCellStreamingPriority::
                critical));

    TestState state{};

    check(
        state,
        !is_valid_streaming_priority(
            WorldCellStreamingPriority::
                invalid),
        "Invalid streaming priority is rejected");

    check(
        state,
        is_valid_streaming_priority(
            WorldCellStreamingPriority::
                background),
        "Background streaming priority is valid");

    check(
        state,
        is_valid_streaming_priority(
            WorldCellStreamingPriority::
                low),
        "Low streaming priority is valid");

    check(
        state,
        is_valid_streaming_priority(
            WorldCellStreamingPriority::
                normal),
        "Normal streaming priority is valid");

    check(
        state,
        is_valid_streaming_priority(
            WorldCellStreamingPriority::
                high),
        "High streaming priority is valid");

    check(
        state,
        is_valid_streaming_priority(
            WorldCellStreamingPriority::
                critical),
        "Critical streaming priority is valid");

    check(
        state,
        streaming_priority_rank(
            WorldCellStreamingPriority::
                invalid) ==
            0U,
        "Invalid priority has rank zero");

    check(
        state,
        streaming_priority_rank(
            WorldCellStreamingPriority::
                background) ==
            1U,
        "Background priority has rank one");

    check(
        state,
        streaming_priority_rank(
            WorldCellStreamingPriority::
                low) ==
            2U,
        "Low priority has rank two");

    check(
        state,
        streaming_priority_rank(
            WorldCellStreamingPriority::
                normal) ==
            3U,
        "Normal priority has rank three");

    check(
        state,
        streaming_priority_rank(
            WorldCellStreamingPriority::
                high) ==
            4U,
        "High priority has rank four");

    check(
        state,
        streaming_priority_rank(
            WorldCellStreamingPriority::
                critical) ==
            5U,
        "Critical priority has rank five");

    check(
        state,
        has_higher_streaming_priority(
            WorldCellStreamingPriority::
                critical,
            WorldCellStreamingPriority::
                high),
        "Critical priority outranks high priority");

    check(
        state,
        has_higher_streaming_priority(
            WorldCellStreamingPriority::
                high,
            WorldCellStreamingPriority::
                normal),
        "High priority outranks normal priority");

    check(
        state,
        has_higher_streaming_priority(
            WorldCellStreamingPriority::
                normal,
            WorldCellStreamingPriority::
                low),
        "Normal priority outranks low priority");

    check(
        state,
        has_higher_streaming_priority(
            WorldCellStreamingPriority::
                low,
            WorldCellStreamingPriority::
                background),
        "Low priority outranks background priority");

    check(
        state,
        !has_higher_streaming_priority(
            WorldCellStreamingPriority::
                background,
            WorldCellStreamingPriority::
                low),
        "Background priority does not outrank low priority");

    check(
        state,
        !has_higher_streaming_priority(
            WorldCellStreamingPriority::
                normal,
            WorldCellStreamingPriority::
                normal),
        "Equal priorities do not outrank each other");

    check(
        state,
        !has_higher_streaming_priority(
            WorldCellStreamingPriority::
                invalid,
            WorldCellStreamingPriority::
                background),
        "Invalid priority cannot outrank a valid priority");

    check(
        state,
        has_higher_streaming_priority(
            WorldCellStreamingPriority::
                background,
            WorldCellStreamingPriority::
                invalid),
        "Valid background priority outranks invalid priority");

    constexpr WorldCellStreamingBudgets
        invalid_budgets{};

    check(
        state,
        !invalid_budgets.is_valid(),
        "Zero-valued streaming budgets are invalid");

    constexpr WorldCellStreamingBudgets
        zero_resident_budget{
            0U,
            8U,
            4U
        };

    check(
        state,
        !zero_resident_budget.is_valid(),
        "Zero resident-byte budget is invalid");

    constexpr WorldCellStreamingBudgets
        zero_queue_budget{
            1024U,
            0U,
            4U
        };

    check(
        state,
        !zero_queue_budget.is_valid(),
        "Zero queued-request budget is invalid");

    constexpr WorldCellStreamingBudgets
        zero_active_budget{
            1024U,
            8U,
            0U
        };

    check(
        state,
        !zero_active_budget.is_valid(),
        "Zero active-request budget is invalid");

    constexpr WorldCellStreamingBudgets
        budgets{
            1024U,
            8U,
            4U
        };

    check(
        state,
        budgets.is_valid(),
        "Non-zero streaming budgets are valid");

    constexpr WorldCellStreamingUsage
        empty_usage{};

    check(
        state,
        empty_usage.is_within(
            budgets),
        "Empty usage is within valid budgets");

    check(
        state,
        !empty_usage.is_within(
            invalid_budgets),
        "Usage cannot be within invalid budgets");

    constexpr WorldCellStreamingUsage
        exact_usage{
            1024U,
            8U,
            4U
        };

    check(
        state,
        exact_usage.is_within(
            budgets),
        "Usage exactly at every limit is within budgets");

    constexpr WorldCellStreamingUsage
        resident_over_budget{
            1025U,
            8U,
            4U
        };

    check(
        state,
        !resident_over_budget.is_within(
            budgets),
        "Resident bytes above the limit exceed budgets");

    constexpr WorldCellStreamingUsage
        queue_over_budget{
            1024U,
            9U,
            4U
        };

    check(
        state,
        !queue_over_budget.is_within(
            budgets),
        "Queued requests above the limit exceed budgets");

    constexpr WorldCellStreamingUsage
        active_over_budget{
            1024U,
            8U,
            5U
        };

    check(
        state,
        !active_over_budget.is_within(
            budgets),
        "Active requests above the limit exceed budgets");

    constexpr WorldCellStreamingUsage
        partial_usage{
            768U,
            6U,
            3U
        };

    check(
        state,
        partial_usage.can_reserve_resident_bytes(
            0U,
            budgets),
        "Zero resident bytes can be reserved");

    check(
        state,
        partial_usage.can_reserve_resident_bytes(
            256U,
            budgets),
        "Remaining resident-byte capacity can be reserved");

    check(
        state,
        !partial_usage.can_reserve_resident_bytes(
            257U,
            budgets),
        "Resident reservation above remaining capacity is rejected");

    check(
        state,
        !partial_usage.can_reserve_resident_bytes(
            1U,
            invalid_budgets),
        "Resident reservation rejects invalid budgets");

    check(
        state,
        !resident_over_budget.
            can_reserve_resident_bytes(
                0U,
                budgets),
        "Resident reservation rejects usage already over budget");

    constexpr WorldCellStreamingUsage
        overflow_probe_usage{
            std::numeric_limits<
                std::size_t>::max(),
            0U,
            0U
        };

    check(
        state,
        !overflow_probe_usage.
            can_reserve_resident_bytes(
                1U,
                budgets),
        "Resident reservation remains safe at maximum size");

    check(
        state,
        partial_usage.can_queue_io_request(
            budgets),
        "Queued I/O request can be added below its limit");

    constexpr WorldCellStreamingUsage
        queue_at_limit{
            0U,
            8U,
            0U
        };

    check(
        state,
        !queue_at_limit.can_queue_io_request(
            budgets),
        "Queued I/O request cannot be added at its limit");

    check(
        state,
        !partial_usage.can_queue_io_request(
            invalid_budgets),
        "Queued I/O request rejects invalid budgets");

    check(
        state,
        partial_usage.can_activate_io_request(
            budgets),
        "Active I/O request can be added below its limit");

    constexpr WorldCellStreamingUsage
        active_at_limit{
            0U,
            0U,
            4U
        };

    check(
        state,
        !active_at_limit.
            can_activate_io_request(
                budgets),
        "Active I/O request cannot be added at its limit");

    check(
        state,
        !partial_usage.
            can_activate_io_request(
                invalid_budgets),
        "Active I/O request rejects invalid budgets");

    return finish(state);
}