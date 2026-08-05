#pragma once

#include "oros/foundation/clock.hpp"
#include "oros/foundation/result.hpp"
#include "oros/runtime/fixed_step_scheduler.hpp"

#include <cstdint>
#include <string_view>

namespace oros::runtime
{
    enum class RuntimeState : std::uint8_t
    {
        ready = 0,
        running,
        stop_requested,
        stopped
    };

    [[nodiscard]] constexpr std::string_view
    to_string(
        const RuntimeState state) noexcept
    {
        switch (state)
        {
        case RuntimeState::ready:
            return "ready";

        case RuntimeState::running:
            return "running";

        case RuntimeState::stop_requested:
            return "stop_requested";

        case RuntimeState::stopped:
            return "stopped";
        }

        return "unknown";
    }

    struct EngineRuntimeConfig final
    {
        FixedStepConfig fixed_step{};
    };

    struct RuntimeFrame final
    {
        foundation::TimePoint frame_time{};
        foundation::Nanoseconds elapsed{};
        FrameSchedule schedule{};
    };

    class EngineRuntime final
    {
    public:
        [[nodiscard]] static
        foundation::Result<EngineRuntime>
        create(
            EngineRuntimeConfig config = {});

        ~EngineRuntime() = default;

        EngineRuntime(
            const EngineRuntime&) = delete;

        EngineRuntime& operator=(
            const EngineRuntime&) = delete;

        EngineRuntime(
            EngineRuntime&&) noexcept = default;

        EngineRuntime& operator=(
            EngineRuntime&&) noexcept = default;

        [[nodiscard]] foundation::Status
        start(
            foundation::TimePoint start_time);

        [[nodiscard]]
        foundation::Result<RuntimeFrame>
        begin_frame(
            foundation::TimePoint frame_time);

        [[nodiscard]] foundation::Status
        request_stop();

        [[nodiscard]] foundation::Status
        stop();

        [[nodiscard]] RuntimeState
        state() const noexcept;

        [[nodiscard]] bool
        is_ready() const noexcept;

        [[nodiscard]] bool
        is_running() const noexcept;

        [[nodiscard]] bool
        is_stop_requested() const noexcept;

        [[nodiscard]] bool
        is_stopped() const noexcept;

        [[nodiscard]] const EngineRuntimeConfig&
        config() const noexcept;

        [[nodiscard]] const FixedStepScheduler&
        scheduler() const noexcept;

    private:
        EngineRuntime(
            EngineRuntimeConfig config,
            FixedStepScheduler scheduler) noexcept;

        EngineRuntimeConfig config_{};
        FixedStepScheduler scheduler_;

        RuntimeState state_{RuntimeState::ready};

        foundation::TimePoint start_time_{};
        foundation::TimePoint last_frame_time_{};
    };
}