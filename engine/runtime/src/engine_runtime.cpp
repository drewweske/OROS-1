#include "oros/runtime/engine_runtime.hpp"

#include <utility>

namespace oros::runtime
{
    foundation::Result<EngineRuntime>
    EngineRuntime::create(
        EngineRuntimeConfig config)
    {
        foundation::Result<FixedStepScheduler>
            scheduler_result =
                FixedStepScheduler::create(
                    config.fixed_step);

        if (!scheduler_result.has_value())
        {
            return std::unexpected{
                scheduler_result.error()
            };
        }

        return EngineRuntime{
            std::move(config),
            std::move(scheduler_result.value())
        };
    }

    EngineRuntime::EngineRuntime(
        EngineRuntimeConfig config,
        FixedStepScheduler scheduler) noexcept
        : config_{std::move(config)},
          scheduler_{std::move(scheduler)}
    {
    }

    foundation::Status
    EngineRuntime::start(
        const foundation::TimePoint start_time)
    {
        if (state_ != RuntimeState::ready)
        {
            return foundation::fail(
                foundation::ErrorCode::invalid_state,
                "The engine runtime can only start "
                "from the ready state.");
        }

        scheduler_.reset();

        start_time_ = start_time;
        last_frame_time_ = start_time;

        state_ = RuntimeState::running;

        return {};
    }

    foundation::Result<RuntimeFrame>
    EngineRuntime::begin_frame(
        const foundation::TimePoint frame_time)
    {
        if (state_ != RuntimeState::running)
        {
            return foundation::fail(
                foundation::ErrorCode::invalid_state,
                "A runtime frame can only begin while "
                "the engine runtime is running.");
        }

        if (frame_time < last_frame_time_)
        {
            return foundation::fail(
                foundation::ErrorCode::invalid_argument,
                "Runtime frame time cannot move "
                "backward.");
        }

        const foundation::Nanoseconds elapsed =
            foundation::elapsed_time(
                last_frame_time_,
                frame_time);

        foundation::Result<FrameSchedule>
            schedule_result =
                scheduler_.advance(elapsed);

        if (!schedule_result.has_value())
        {
            return std::unexpected{
                schedule_result.error()
            };
        }

        RuntimeFrame frame{};
        frame.frame_time = frame_time;
        frame.elapsed = elapsed;
        frame.schedule =
            std::move(schedule_result.value());

        last_frame_time_ = frame_time;

        return frame;
    }

    foundation::Status
    EngineRuntime::request_stop()
    {
        if (state_ != RuntimeState::running)
        {
            return foundation::fail(
                foundation::ErrorCode::invalid_state,
                "A stop request can only be made while "
                "the engine runtime is running.");
        }

        state_ = RuntimeState::stop_requested;

        return {};
    }

    foundation::Status
    EngineRuntime::stop()
    {
        if (state_ != RuntimeState::stop_requested)
        {
            return foundation::fail(
                foundation::ErrorCode::invalid_state,
                "The engine runtime can only stop after "
                "a stop has been requested.");
        }

        state_ = RuntimeState::stopped;

        return {};
    }

    RuntimeState
    EngineRuntime::state() const noexcept
    {
        return state_;
    }

    bool
    EngineRuntime::is_ready() const noexcept
    {
        return state_ == RuntimeState::ready;
    }

    bool
    EngineRuntime::is_running() const noexcept
    {
        return state_ == RuntimeState::running;
    }

    bool
    EngineRuntime::is_stop_requested()
        const noexcept
    {
        return state_ ==
            RuntimeState::stop_requested;
    }

    bool
    EngineRuntime::is_stopped() const noexcept
    {
        return state_ == RuntimeState::stopped;
    }

    const EngineRuntimeConfig&
    EngineRuntime::config() const noexcept
    {
        return config_;
    }

    const FixedStepScheduler&
    EngineRuntime::scheduler() const noexcept
    {
        return scheduler_;
    }
}