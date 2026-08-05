#pragma once

#include <chrono>

namespace oros::foundation
{
    using MonotonicClock = std::chrono::steady_clock;
    using TimePoint = MonotonicClock::time_point;
    using Nanoseconds = std::chrono::nanoseconds;

    [[nodiscard]] inline TimePoint
    monotonic_now() noexcept
    {
        return MonotonicClock::now();
    }

    [[nodiscard]] inline Nanoseconds elapsed_time(
        const TimePoint start,
        const TimePoint end) noexcept
    {
        return std::chrono::duration_cast<Nanoseconds>(
            end - start);
    }

    [[nodiscard]] inline Nanoseconds elapsed_since(
        const TimePoint start) noexcept
    {
        return elapsed_time(
            start,
            monotonic_now());
    }

    class Stopwatch final
    {
    public:
        Stopwatch() noexcept
            : start_{monotonic_now()}
        {
        }

        void reset() noexcept
        {
            start_ = monotonic_now();
        }

        [[nodiscard]] Nanoseconds elapsed() const noexcept
        {
            return elapsed_since(start_);
        }

    private:
        TimePoint start_;
    };
}