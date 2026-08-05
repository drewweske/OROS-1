#include "oros/runtime/fixed_step_scheduler.hpp"

#include <algorithm>
#include <limits>

namespace oros::runtime
{
    namespace
    {
        using DurationRepresentation =
            foundation::Nanoseconds::rep;

        [[nodiscard]] constexpr bool
        can_add_durations(
            const DurationRepresentation left,
            const DurationRepresentation right) noexcept
        {
            return right <=
                (std::numeric_limits<
                    DurationRepresentation>::max)() -
                    left;
        }
    }

    foundation::Result<FixedStepScheduler>
    FixedStepScheduler::create(
        const FixedStepConfig config)
    {
        if (config.simulation_step <=
            foundation::Nanoseconds::zero())
        {
            return foundation::fail(
                foundation::ErrorCode::invalid_argument,
                "Fixed simulation step must be greater "
                "than zero.");
        }

        if (config.maximum_frame_time <=
            foundation::Nanoseconds::zero())
        {
            return foundation::fail(
                foundation::ErrorCode::invalid_argument,
                "Maximum frame time must be greater "
                "than zero.");
        }

        if (config.maximum_updates_per_frame == 0U)
        {
            return foundation::fail(
                foundation::ErrorCode::invalid_argument,
                "Maximum updates per frame must be "
                "greater than zero.");
        }

        return FixedStepScheduler{config};
    }

    FixedStepScheduler::FixedStepScheduler(
        const FixedStepConfig config) noexcept
        : config_{config}
    {
    }

    foundation::Result<FrameSchedule>
    FixedStepScheduler::advance(
        const foundation::Nanoseconds elapsed)
    {
        if (elapsed < foundation::Nanoseconds::zero())
        {
            return foundation::fail(
                foundation::ErrorCode::invalid_argument,
                "Elapsed frame time cannot be negative.");
        }

        if (completed_frame_count_ ==
            (std::numeric_limits<
                std::uint64_t>::max)())
        {
            return foundation::fail(
                foundation::ErrorCode::invalid_state,
                "The scheduler frame counter has "
                "reached its maximum value.");
        }

        const bool frame_time_clamped =
            elapsed > config_.maximum_frame_time;

        const foundation::Nanoseconds
            accepted_elapsed =
                std::min(
                    elapsed,
                    config_.maximum_frame_time);

        const foundation::Nanoseconds
            frame_clamp_drop =
                elapsed - accepted_elapsed;

        if (!can_add_durations(
                accumulator_.count(),
                accepted_elapsed.count()))
        {
            return foundation::fail(
                foundation::ErrorCode::invalid_state,
                "Advancing the scheduler would overflow "
                "its accumulated time.");
        }

        const DurationRepresentation pending_count =
            accumulator_.count() +
            accepted_elapsed.count();

        const DurationRepresentation
            simulation_step_count =
                config_.simulation_step.count();

        const DurationRepresentation
            available_update_count =
                pending_count /
                simulation_step_count;

        const DurationRepresentation
            maximum_update_count =
                static_cast<DurationRepresentation>(
                    config_.
                        maximum_updates_per_frame);

        const DurationRepresentation
            scheduled_update_count =
                std::min(
                    available_update_count,
                    maximum_update_count);

        const bool update_limit_reached =
            available_update_count >
            maximum_update_count;

        const DurationRepresentation
            processed_time_count =
                scheduled_update_count *
                simulation_step_count;

        const DurationRepresentation
            discarded_update_count =
                available_update_count -
                scheduled_update_count;

        const DurationRepresentation
            update_limit_drop_count =
                discarded_update_count *
                simulation_step_count;

        const DurationRepresentation
            remaining_accumulator_count =
                pending_count -
                processed_time_count -
                update_limit_drop_count;

        if (!can_add_durations(
                frame_clamp_drop.count(),
                update_limit_drop_count))
        {
            return foundation::fail(
                foundation::ErrorCode::invalid_state,
                "The scheduler's dropped frame time "
                "would overflow.");
        }

        const DurationRepresentation
            dropped_elapsed_count =
                frame_clamp_drop.count() +
                update_limit_drop_count;

        if (!can_add_durations(
                simulated_time_.count(),
                processed_time_count))
        {
            return foundation::fail(
                foundation::ErrorCode::invalid_state,
                "The scheduler's simulated time would "
                "overflow.");
        }

        if (!can_add_durations(
                dropped_time_.count(),
                dropped_elapsed_count))
        {
            return foundation::fail(
                foundation::ErrorCode::invalid_state,
                "The scheduler's cumulative dropped "
                "time would overflow.");
        }

        const auto simulation_update_count =
            static_cast<std::uint32_t>(
                scheduled_update_count);

        if (completed_simulation_tick_count_ >
            (std::numeric_limits<
                std::uint64_t>::max)() -
                static_cast<std::uint64_t>(
                    simulation_update_count))
        {
            return foundation::fail(
                foundation::ErrorCode::invalid_state,
                "The scheduler simulation tick counter "
                "would overflow.");
        }

        FrameSchedule schedule{};

        schedule.frame_index =
            completed_frame_count_;

        schedule.first_simulation_tick_index =
            completed_simulation_tick_count_;

        schedule.simulation_update_count =
            simulation_update_count;

        schedule.raw_elapsed = elapsed;
        schedule.accepted_elapsed =
            accepted_elapsed;

        schedule.dropped_elapsed =
            foundation::Nanoseconds{
                dropped_elapsed_count
            };

        schedule.remaining_accumulator =
            foundation::Nanoseconds{
                remaining_accumulator_count
            };

        schedule.interpolation_alpha =
            static_cast<double>(
                remaining_accumulator_count) /
            static_cast<double>(
                simulation_step_count);

        schedule.frame_time_clamped =
            frame_time_clamped;

        schedule.update_limit_reached =
            update_limit_reached;

        accumulator_ =
            schedule.remaining_accumulator;

        simulated_time_ =
            foundation::Nanoseconds{
                simulated_time_.count() +
                processed_time_count
            };

        dropped_time_ =
            foundation::Nanoseconds{
                dropped_time_.count() +
                dropped_elapsed_count
            };

        completed_simulation_tick_count_ +=
            static_cast<std::uint64_t>(
                simulation_update_count);

        ++completed_frame_count_;

        return schedule;
    }

    void FixedStepScheduler::reset() noexcept
    {
        accumulator_ =
            foundation::Nanoseconds::zero();

        simulated_time_ =
            foundation::Nanoseconds::zero();

        dropped_time_ =
            foundation::Nanoseconds::zero();

        completed_frame_count_ = 0U;

        completed_simulation_tick_count_ = 0U;
    }

    const FixedStepConfig&
    FixedStepScheduler::config() const noexcept
    {
        return config_;
    }

    std::uint64_t
    FixedStepScheduler::completed_frame_count()
        const noexcept
    {
        return completed_frame_count_;
    }

    std::uint64_t
    FixedStepScheduler::
        completed_simulation_tick_count()
            const noexcept
    {
        return completed_simulation_tick_count_;
    }

    foundation::Nanoseconds
    FixedStepScheduler::simulated_time()
        const noexcept
    {
        return simulated_time_;
    }

    foundation::Nanoseconds
    FixedStepScheduler::dropped_time()
        const noexcept
    {
        return dropped_time_;
    }

    foundation::Nanoseconds
    FixedStepScheduler::accumulator()
        const noexcept
    {
        return accumulator_;
    }
}