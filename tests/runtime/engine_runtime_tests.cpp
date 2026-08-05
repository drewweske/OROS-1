#include "oros/runtime/engine_runtime.hpp"

#include <chrono>
#include <iostream>
#include <string_view>
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
    using namespace oros::foundation;
    using namespace oros::runtime;
    using namespace std::chrono_literals;

    TestState state{};

    check(
        state,
        to_string(RuntimeState::ready) == "ready",
        "Ready runtime state converts to text");

    check(
        state,
        to_string(RuntimeState::running) == "running",
        "Running runtime state converts to text");

    check(
        state,
        to_string(RuntimeState::stop_requested) ==
            "stop_requested",
        "Stop-requested runtime state converts to text");

    check(
        state,
        to_string(RuntimeState::stopped) == "stopped",
        "Stopped runtime state converts to text");

    check(
        state,
        to_string(
            static_cast<RuntimeState>(255U)) ==
            "unknown",
        "Unknown runtime state converts to unknown");

    EngineRuntimeConfig invalid_config{};
    invalid_config.fixed_step.simulation_step =
        Nanoseconds::zero();

    const Result<EngineRuntime> invalid_result =
        EngineRuntime::create(invalid_config);

    check(
        state,
        !invalid_result.has_value(),
        "Runtime rejects an invalid scheduler "
        "configuration");

    check(
        state,
        !invalid_result.has_value() &&
            invalid_result.error().code ==
                ErrorCode::invalid_argument,
        "Runtime preserves scheduler configuration "
        "errors");

    EngineRuntimeConfig runtime_config{};
    runtime_config.fixed_step.simulation_step = 10ms;
    runtime_config.fixed_step.maximum_frame_time =
        100ms;
    runtime_config.fixed_step.
        maximum_updates_per_frame = 4U;

    Result<EngineRuntime> runtime_result =
        EngineRuntime::create(runtime_config);

    check(
        state,
        runtime_result.has_value(),
        "Runtime accepts a valid configuration");

    if (runtime_result.has_value())
    {
        EngineRuntime runtime{
            std::move(runtime_result.value())
        };

        check(
            state,
            runtime.state() == RuntimeState::ready,
            "New runtime begins in the ready state");

        check(
            state,
            runtime.is_ready(),
            "New runtime reports that it is ready");

        check(
            state,
            !runtime.is_running(),
            "New runtime does not report running");

        check(
            state,
            !runtime.is_stop_requested(),
            "New runtime has no stop request");

        check(
            state,
            !runtime.is_stopped(),
            "New runtime does not report stopped");

        check(
            state,
            runtime.config().
                fixed_step.simulation_step == 10ms,
            "Runtime preserves its simulation step");

        check(
            state,
            runtime.config().
                fixed_step.maximum_frame_time == 100ms,
            "Runtime preserves its maximum frame time");

        check(
            state,
            runtime.config().
                fixed_step.
                    maximum_updates_per_frame == 4U,
            "Runtime preserves its frame update limit");

        check(
            state,
            runtime.scheduler().
                completed_frame_count() == 0ULL,
            "New runtime scheduler has no completed "
            "frames");

        check(
            state,
            runtime.scheduler().
                completed_simulation_tick_count() ==
                0ULL,
            "New runtime scheduler has no completed "
            "ticks");

        const TimePoint start_time =
            TimePoint{} + 100ms;

        const Result<RuntimeFrame>
            premature_frame_result =
                runtime.begin_frame(start_time);

        check(
            state,
            !premature_frame_result.has_value(),
            "Runtime rejects frames before startup");

        check(
            state,
            !premature_frame_result.has_value() &&
                premature_frame_result.error().code ==
                    ErrorCode::invalid_state,
            "Frame before startup reports invalid_state");

        check(
            state,
            runtime.state() == RuntimeState::ready,
            "Rejected premature frame preserves ready "
            "state");

        const Status premature_request_status =
            runtime.request_stop();

        check(
            state,
            !premature_request_status.has_value(),
            "Runtime rejects a stop request before "
            "startup");

        check(
            state,
            !premature_request_status.has_value() &&
                premature_request_status.error().code ==
                    ErrorCode::invalid_state,
            "Premature stop request reports "
            "invalid_state");

        const Status premature_stop_status =
            runtime.stop();

        check(
            state,
            !premature_stop_status.has_value(),
            "Runtime rejects stopping before a stop "
            "request");

        check(
            state,
            !premature_stop_status.has_value() &&
                premature_stop_status.error().code ==
                    ErrorCode::invalid_state,
            "Premature stop reports invalid_state");

        const Status start_status =
            runtime.start(start_time);

        check(
            state,
            start_status.has_value(),
            "Runtime starts from the ready state");

        check(
            state,
            runtime.state() == RuntimeState::running,
            "Started runtime enters the running state");

        check(
            state,
            !runtime.is_ready(),
            "Started runtime no longer reports ready");

        check(
            state,
            runtime.is_running(),
            "Started runtime reports running");

        check(
            state,
            !runtime.is_stop_requested(),
            "Started runtime has no stop request");

        check(
            state,
            !runtime.is_stopped(),
            "Started runtime does not report stopped");

        check(
            state,
            runtime.scheduler().
                completed_frame_count() == 0ULL,
            "Starting resets scheduler frame count");

        check(
            state,
            runtime.scheduler().
                completed_simulation_tick_count() ==
                0ULL,
            "Starting resets scheduler tick count");

        const Status duplicate_start_status =
            runtime.start(start_time);

        check(
            state,
            !duplicate_start_status.has_value(),
            "Runtime rejects duplicate startup");

        check(
            state,
            !duplicate_start_status.has_value() &&
                duplicate_start_status.error().code ==
                    ErrorCode::invalid_state,
            "Duplicate startup reports invalid_state");

        check(
            state,
            runtime.state() == RuntimeState::running,
            "Rejected duplicate startup preserves "
            "running state");

        const Result<RuntimeFrame>
            first_frame_result =
                runtime.begin_frame(start_time);

        check(
            state,
            first_frame_result.has_value(),
            "Runtime accepts its first frame");

        if (first_frame_result.has_value())
        {
            const RuntimeFrame& frame =
                first_frame_result.value();

            check(
                state,
                frame.frame_time == start_time,
                "First frame preserves its time point");

            check(
                state,
                frame.elapsed ==
                    Nanoseconds::zero(),
                "First frame at startup time has zero "
                "elapsed time");

            check(
                state,
                frame.schedule.frame_index == 0ULL,
                "First runtime frame uses frame index "
                "zero");

            check(
                state,
                frame.schedule.
                    first_simulation_tick_index ==
                    0ULL,
                "First runtime frame begins at tick zero");

            check(
                state,
                frame.schedule.
                    simulation_update_count == 0U,
                "Zero-duration first frame schedules no "
                "updates");
        }

        const TimePoint second_frame_time =
            start_time + 25ms;

        const Result<RuntimeFrame>
            second_frame_result =
                runtime.begin_frame(
                    second_frame_time);

        check(
            state,
            second_frame_result.has_value(),
            "Runtime accepts a later frame");

        if (second_frame_result.has_value())
        {
            const RuntimeFrame& frame =
                second_frame_result.value();

            check(
                state,
                frame.frame_time ==
                    second_frame_time,
                "Runtime frame preserves its supplied "
                "time point");

            check(
                state,
                frame.elapsed == 25ms,
                "Runtime calculates elapsed frame time");

            check(
                state,
                frame.schedule.frame_index == 1ULL,
                "Second runtime frame uses frame index "
                "one");

            check(
                state,
                frame.schedule.
                    first_simulation_tick_index ==
                    0ULL,
                "Second runtime frame begins at tick zero");

            check(
                state,
                frame.schedule.
                    simulation_update_count == 2U,
                "Twenty-five millisecond runtime frame "
                "schedules two updates");

            check(
                state,
                frame.schedule.
                    remaining_accumulator == 5ms,
                "Runtime frame preserves a five "
                "millisecond remainder");
        }

        check(
            state,
            runtime.scheduler().
                completed_frame_count() == 2ULL,
            "Two accepted runtime frames are recorded");

        check(
            state,
            runtime.scheduler().
                completed_simulation_tick_count() ==
                2ULL,
            "Runtime scheduler records two fixed ticks");

        check(
            state,
            runtime.scheduler().simulated_time() ==
                20ms,
            "Runtime scheduler records processed "
            "simulation time");

        check(
            state,
            runtime.scheduler().accumulator() == 5ms,
            "Runtime scheduler stores its fractional "
            "remainder");

        const std::uint64_t
            frame_count_before_backward_time =
                runtime.scheduler().
                    completed_frame_count();

        const std::uint64_t
            tick_count_before_backward_time =
                runtime.scheduler().
                    completed_simulation_tick_count();

        const Result<RuntimeFrame>
            backward_frame_result =
                runtime.begin_frame(
                    start_time + 20ms);

        check(
            state,
            !backward_frame_result.has_value(),
            "Runtime rejects backward-moving frame time");

        check(
            state,
            !backward_frame_result.has_value() &&
                backward_frame_result.error().code ==
                    ErrorCode::invalid_argument,
            "Backward frame time reports "
            "invalid_argument");

        check(
            state,
            runtime.scheduler().
                completed_frame_count() ==
                frame_count_before_backward_time,
            "Rejected backward frame preserves frame "
            "count");

        check(
            state,
            runtime.scheduler().
                completed_simulation_tick_count() ==
                tick_count_before_backward_time,
            "Rejected backward frame preserves tick "
            "count");

        check(
            state,
            runtime.scheduler().accumulator() == 5ms,
            "Rejected backward frame preserves the "
            "accumulator");

        const Result<RuntimeFrame>
            third_frame_result =
                runtime.begin_frame(
                    start_time + 30ms);

        check(
            state,
            third_frame_result.has_value(),
            "Runtime accepts a valid frame after a "
            "rejected frame");

        if (third_frame_result.has_value())
        {
            const RuntimeFrame& frame =
                third_frame_result.value();

            check(
                state,
                frame.elapsed == 5ms,
                "Rejected backward frame does not change "
                "the previous accepted frame time");

            check(
                state,
                frame.schedule.frame_index == 2ULL,
                "Third accepted frame uses frame index "
                "two");

            check(
                state,
                frame.schedule.
                    first_simulation_tick_index ==
                    2ULL,
                "Third accepted frame begins at tick two");

            check(
                state,
                frame.schedule.
                    simulation_update_count == 1U,
                "Stored and new partial time produce one "
                "fixed update");

            check(
                state,
                frame.schedule.
                    remaining_accumulator ==
                    Nanoseconds::zero(),
                "Combined partial runtime frames consume "
                "the accumulator");
        }

        check(
            state,
            runtime.scheduler().
                completed_frame_count() == 3ULL,
            "Runtime records three accepted frames");

        check(
            state,
            runtime.scheduler().
                completed_simulation_tick_count() ==
                3ULL,
            "Runtime records three completed ticks");

        check(
            state,
            runtime.scheduler().simulated_time() ==
                30ms,
            "Runtime records thirty milliseconds of "
            "simulation");

        const Status request_status =
            runtime.request_stop();

        check(
            state,
            request_status.has_value(),
            "Running runtime accepts a stop request");

        check(
            state,
            runtime.state() ==
                RuntimeState::stop_requested,
            "Stop request changes the runtime state");

        check(
            state,
            !runtime.is_ready(),
            "Stop-requested runtime is not ready");

        check(
            state,
            !runtime.is_running(),
            "Stop-requested runtime is not running");

        check(
            state,
            runtime.is_stop_requested(),
            "Runtime reports its pending stop request");

        check(
            state,
            !runtime.is_stopped(),
            "Stop-requested runtime is not yet stopped");

        const Result<RuntimeFrame>
            frame_after_request_result =
                runtime.begin_frame(
                    start_time + 40ms);

        check(
            state,
            !frame_after_request_result.has_value(),
            "Runtime rejects frames after a stop request");

        check(
            state,
            !frame_after_request_result.has_value() &&
                frame_after_request_result.error().code ==
                    ErrorCode::invalid_state,
            "Frame after stop request reports "
            "invalid_state");

        check(
            state,
            runtime.scheduler().
                completed_frame_count() == 3ULL,
            "Rejected post-request frame preserves frame "
            "count");

        const Status duplicate_request_status =
            runtime.request_stop();

        check(
            state,
            !duplicate_request_status.has_value(),
            "Runtime rejects a duplicate stop request");

        check(
            state,
            !duplicate_request_status.has_value() &&
                duplicate_request_status.error().code ==
                    ErrorCode::invalid_state,
            "Duplicate stop request reports "
            "invalid_state");

        const Status restart_during_request_status =
            runtime.start(start_time + 40ms);

        check(
            state,
            !restart_during_request_status.has_value(),
            "Runtime rejects startup while a stop is "
            "requested");

        const Status stop_status =
            runtime.stop();

        check(
            state,
            stop_status.has_value(),
            "Runtime stops after a stop request");

        check(
            state,
            runtime.state() == RuntimeState::stopped,
            "Stopped runtime enters the stopped state");

        check(
            state,
            !runtime.is_ready(),
            "Stopped runtime is not ready");

        check(
            state,
            !runtime.is_running(),
            "Stopped runtime is not running");

        check(
            state,
            !runtime.is_stop_requested(),
            "Stopped runtime has no pending stop request");

        check(
            state,
            runtime.is_stopped(),
            "Stopped runtime reports stopped");

        const Status duplicate_stop_status =
            runtime.stop();

        check(
            state,
            !duplicate_stop_status.has_value(),
            "Runtime rejects duplicate shutdown");

        check(
            state,
            !duplicate_stop_status.has_value() &&
                duplicate_stop_status.error().code ==
                    ErrorCode::invalid_state,
            "Duplicate shutdown reports invalid_state");

        const Status request_after_stop_status =
            runtime.request_stop();

        check(
            state,
            !request_after_stop_status.has_value(),
            "Stopped runtime rejects a new stop request");

        const Status restart_after_stop_status =
            runtime.start(start_time + 50ms);

        check(
            state,
            !restart_after_stop_status.has_value(),
            "Stopped runtime cannot restart");

        const Result<RuntimeFrame>
            frame_after_stop_result =
                runtime.begin_frame(
                    start_time + 50ms);

        check(
            state,
            !frame_after_stop_result.has_value(),
            "Stopped runtime rejects new frames");

        check(
            state,
            runtime.state() == RuntimeState::stopped,
            "Rejected stopped-state operations preserve "
            "the stopped state");

        check(
            state,
            runtime.scheduler().
                completed_frame_count() == 3ULL,
            "Stopped runtime preserves completed frame "
            "count");

        check(
            state,
            runtime.scheduler().
                completed_simulation_tick_count() ==
                3ULL,
            "Stopped runtime preserves completed tick "
            "count");
    }

    EngineRuntimeConfig clamp_config{};
    clamp_config.fixed_step.simulation_step = 10ms;
    clamp_config.fixed_step.maximum_frame_time = 25ms;
    clamp_config.fixed_step.
        maximum_updates_per_frame = 4U;

    Result<EngineRuntime> clamp_runtime_result =
        EngineRuntime::create(clamp_config);

    check(
        state,
        clamp_runtime_result.has_value(),
        "Runtime clamp test instance initializes");

    if (clamp_runtime_result.has_value())
    {
        EngineRuntime runtime{
            std::move(clamp_runtime_result.value())
        };

        const TimePoint start_time{};

        const Status start_status =
            runtime.start(start_time);

        check(
            state,
            start_status.has_value(),
            "Runtime clamp test instance starts");

        const Result<RuntimeFrame> frame_result =
            runtime.begin_frame(start_time + 40ms);

        check(
            state,
            frame_result.has_value(),
            "Runtime advances a frame exceeding its "
            "maximum frame time");

        if (frame_result.has_value())
        {
            const RuntimeFrame& frame =
                frame_result.value();

            check(
                state,
                frame.elapsed == 40ms,
                "Runtime preserves unclamped elapsed "
                "frame time");

            check(
                state,
                frame.schedule.raw_elapsed == 40ms,
                "Runtime schedule preserves raw frame "
                "time");

            check(
                state,
                frame.schedule.accepted_elapsed == 25ms,
                "Runtime scheduler clamps accepted frame "
                "time");

            check(
                state,
                frame.schedule.dropped_elapsed == 15ms,
                "Runtime scheduler reports clamped time");

            check(
                state,
                frame.schedule.
                    simulation_update_count == 2U,
                "Clamped runtime frame schedules two "
                "updates");

            check(
                state,
                frame.schedule.
                    remaining_accumulator == 5ms,
                "Clamped runtime frame preserves its "
                "remainder");

            check(
                state,
                frame.schedule.frame_time_clamped,
                "Runtime exposes the frame-time clamp");
        }

        check(
            state,
            runtime.scheduler().simulated_time() ==
                20ms,
            "Runtime exposes clamped simulated time");

        check(
            state,
            runtime.scheduler().dropped_time() == 15ms,
            "Runtime exposes cumulative dropped time");

        const Status request_status =
            runtime.request_stop();

        const Status stop_status =
            runtime.stop();

        check(
            state,
            request_status.has_value() &&
                stop_status.has_value(),
            "Runtime clamp test instance shuts down "
            "legally");
    }

    std::cout
        << "\nEngine runtime test summary: "
        << state.checks - state.failures
        << '/'
        << state.checks
        << " passed.\n";

    return state.failures == 0 ? 0 : 1;
}