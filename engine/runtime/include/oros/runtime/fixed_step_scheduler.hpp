#pragma once

#include "oros/foundation/clock.hpp"
#include "oros/foundation/result.hpp"

#include <cstdint>

namespace oros::runtime
{
    struct FixedStepConfig final
    {
        foundation::Nanoseconds simulation_step{
            16'666'667
        };

        foundation::Nanoseconds maximum_frame_time{
            250'000'000
        };

        std::uint32_t maximum_updates_per_frame{8U};
    };

    struct FrameSchedule final
    {
        std::uint64_t frame_index{};

        std::uint64_t
            first_simulation_tick_index{};

        std::uint32_t
            simulation_update_count{};

        foundation::Nanoseconds raw_elapsed{};
        foundation::Nanoseconds accepted_elapsed{};
        foundation::Nanoseconds dropped_elapsed{};

        foundation::Nanoseconds
            remaining_accumulator{};

        double interpolation_alpha{};

        bool frame_time_clamped{};
        bool update_limit_reached{};
    };

    class FixedStepScheduler final
    {
    public:
        [[nodiscard]] static
        foundation::Result<FixedStepScheduler>
        create(
            FixedStepConfig config = {});

        FixedStepScheduler(
            const FixedStepScheduler&) = delete;

        FixedStepScheduler& operator=(
            const FixedStepScheduler&) = delete;

        FixedStepScheduler(
            FixedStepScheduler&&) noexcept = default;

        FixedStepScheduler& operator=(
            FixedStepScheduler&&) noexcept = default;

        [[nodiscard]]
        foundation::Result<FrameSchedule>
        advance(
            foundation::Nanoseconds elapsed);

        void reset() noexcept;

        [[nodiscard]] const FixedStepConfig&
        config() const noexcept;

        [[nodiscard]] std::uint64_t
        completed_frame_count() const noexcept;

        [[nodiscard]] std::uint64_t
        completed_simulation_tick_count()
            const noexcept;

        [[nodiscard]] foundation::Nanoseconds
        simulated_time() const noexcept;

        [[nodiscard]] foundation::Nanoseconds
        dropped_time() const noexcept;

        [[nodiscard]] foundation::Nanoseconds
        accumulator() const noexcept;

    private:
        explicit FixedStepScheduler(
            FixedStepConfig config) noexcept;

        FixedStepConfig config_{};

        foundation::Nanoseconds accumulator_{};
        foundation::Nanoseconds simulated_time_{};
        foundation::Nanoseconds dropped_time_{};

        std::uint64_t completed_frame_count_{};
        std::uint64_t
            completed_simulation_tick_count_{};
    };
}