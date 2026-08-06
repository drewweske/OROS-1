#pragma once

#include <cstddef>
#include <cstdint>

namespace oros::streaming
{
    enum class WorldCellStreamingPriority :
        std::uint8_t
    {
        invalid = 0,
        background,
        low,
        normal,
        high,
        critical
    };

    [[nodiscard]]
    constexpr bool
    is_valid_streaming_priority(
        const WorldCellStreamingPriority
            priority) noexcept
    {
        return
            priority !=
                WorldCellStreamingPriority::
                    invalid;
    }

    [[nodiscard]]
    constexpr std::uint8_t
    streaming_priority_rank(
        const WorldCellStreamingPriority
            priority) noexcept
    {
        return
            static_cast<std::uint8_t>(
                priority);
    }

    [[nodiscard]]
    constexpr bool
    has_higher_streaming_priority(
        const WorldCellStreamingPriority
            left,
        const WorldCellStreamingPriority
            right) noexcept
    {
        return
            is_valid_streaming_priority(
                left) &&
            streaming_priority_rank(
                left) >
                streaming_priority_rank(
                    right);
    }

    struct WorldCellStreamingBudgets final
    {
        std::size_t
            resident_byte_limit{};

        std::size_t
            queued_io_request_limit{};

        std::size_t
            active_io_request_limit{};

        [[nodiscard]]
        constexpr bool
        is_valid() const noexcept
        {
            return
                resident_byte_limit != 0U &&
                queued_io_request_limit != 0U &&
                active_io_request_limit != 0U;
        }
    };

    struct WorldCellStreamingUsage final
    {
        std::size_t
            resident_byte_count{};

        std::size_t
            queued_io_request_count{};

        std::size_t
            active_io_request_count{};

        [[nodiscard]]
        constexpr bool
        is_within(
            const WorldCellStreamingBudgets&
                budgets) const noexcept
        {
            return
                budgets.is_valid() &&
                resident_byte_count <=
                    budgets.
                        resident_byte_limit &&
                queued_io_request_count <=
                    budgets.
                        queued_io_request_limit &&
                active_io_request_count <=
                    budgets.
                        active_io_request_limit;
        }

        [[nodiscard]]
        constexpr bool
        can_reserve_resident_bytes(
            const std::size_t byte_count,
            const WorldCellStreamingBudgets&
                budgets) const noexcept
        {
            if (!budgets.is_valid() ||
                resident_byte_count >
                    budgets.
                        resident_byte_limit)
            {
                return false;
            }

            return
                byte_count <=
                    budgets.
                        resident_byte_limit -
                    resident_byte_count;
        }

        [[nodiscard]]
        constexpr bool
        can_queue_io_request(
            const WorldCellStreamingBudgets&
                budgets) const noexcept
        {
            return
                budgets.is_valid() &&
                queued_io_request_count <
                    budgets.
                        queued_io_request_limit;
        }

        [[nodiscard]]
        constexpr bool
        can_activate_io_request(
            const WorldCellStreamingBudgets&
                budgets) const noexcept
        {
            return
                budgets.is_valid() &&
                active_io_request_count <
                    budgets.
                        active_io_request_limit;
        }
    };
}