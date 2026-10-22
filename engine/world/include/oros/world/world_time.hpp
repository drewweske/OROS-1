#pragma once

#include <compare>
#include <cstdint>

namespace oros::world
{
    // Absolute deterministic in-world time coordinate.
    //
    // WorldTime does not read a clock, advance itself, or define the
    // mapping from simulation ticks to in-world time. Composition owns
    // that mapping and supplies explicit WorldTime values.
    class WorldTime final
    {
    public:
        constexpr WorldTime() noexcept =
            default;

        [[nodiscard]]
        static constexpr WorldTime
        epoch() noexcept
        {
            return {};
        }

        [[nodiscard]]
        static constexpr WorldTime
        from_microseconds_since_epoch(
            const std::uint64_t microseconds)
            noexcept
        {
            WorldTime time{};
            time.microseconds_since_epoch_ =
                microseconds;

            return time;
        }

        [[nodiscard]]
        constexpr std::uint64_t
        microseconds_since_epoch()
            const noexcept
        {
            return microseconds_since_epoch_;
        }

        [[nodiscard]]
        constexpr bool
        is_epoch() const noexcept
        {
            return
                microseconds_since_epoch_ ==
                0ULL;
        }

        auto operator<=>(
            const WorldTime&)
            const noexcept = default;

    private:
        std::uint64_t
            microseconds_since_epoch_{};
    };
}