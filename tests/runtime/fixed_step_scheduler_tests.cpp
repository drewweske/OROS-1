#include "oros/runtime/fixed_step_scheduler.hpp"

#include <chrono>
#include <cmath>
#include <cstdint>
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

    [[nodiscard]] bool nearly_equal(
        const double left,
        const double right,
        const double tolerance = 0.000'000'001)
        noexcept
    {
        return std::abs(left - right) <= tolerance;
    }
}

int main()
{
    using namespace oros::foundation;
    using namespace oros::runtime;
    using namespace std::chrono_literals;

    TestState state{};

    const FixedStepConfig default_config{};

    check(
        state,
        default_config.simulation_step ==
            Nanoseconds{16'666'667},
        "Default simulation step is approximately "
        "60 updates per second");

    check(
        state,
        default_config.maximum_frame_time ==
            250ms,
        "Default maximum frame time is 250 milliseconds");

    check(
        state,
        default_config.maximum_updates_per_frame == 8U,
        "Default frame update limit is eight");

    Result<FixedStepScheduler> default_result =
        FixedStepScheduler::create();

    check(
        state,
        default_result.has_value(),
        "Scheduler accepts its default configuration");

    if (default_result.has_value())
    {
        const FixedStepScheduler& scheduler =
            default_result.value();

        check(
            state,
            scheduler.config().simulation_step ==
                default_config.simulation_step,
            "Scheduler preserves its simulation step");

        check(
            state,
            scheduler.config().maximum_frame_time ==
                default_config.maximum_frame_time,
            "Scheduler preserves its maximum frame time");

        check(
            state,
            scheduler.config().
                maximum_updates_per_frame ==
                default_config.
                    maximum_updates_per_frame,
            "Scheduler preserves its frame update limit");
    }

    FixedStepConfig invalid_step_config{};
    invalid_step_config.simulation_step =
        Nanoseconds::zero();

    const Result<FixedStepScheduler>
        invalid_step_result =
            FixedStepScheduler::create(
                invalid_step_config);

    check(
        state,
        !invalid_step_result.has_value(),
        "Scheduler rejects a zero simulation step");

    check(
        state,
        invalid_step_result.error().code ==
            ErrorCode::invalid_argument,
        "Zero simulation step reports invalid_argument");

    invalid_step_config.simulation_step =
        Nanoseconds{-1};

    const Result<FixedStepScheduler>
        negative_step_result =
            FixedStepScheduler::create(
                invalid_step_config);

    check(
        state,
        !negative_step_result.has_value(),
        "Scheduler rejects a negative simulation step");

    check(
        state,
        negative_step_result.error().code ==
            ErrorCode::invalid_argument,
        "Negative simulation step reports "
        "invalid_argument");

    FixedStepConfig invalid_frame_config{};
    invalid_frame_config.maximum_frame_time =
        Nanoseconds::zero();

    const Result<FixedStepScheduler>
        invalid_frame_result =
            FixedStepScheduler::create(
                invalid_frame_config);

    check(
        state,
        !invalid_frame_result.has_value(),
        "Scheduler rejects a zero maximum frame time");

    check(
        state,
        invalid_frame_result.error().code ==
            ErrorCode::invalid_argument,
        "Zero maximum frame time reports "
        "invalid_argument");

    invalid_frame_config.maximum_frame_time =
        Nanoseconds{-1};

    const Result<FixedStepScheduler>
        negative_frame_result =
            FixedStepScheduler::create(
                invalid_frame_config);

    check(
        state,
        !negative_frame_result.has_value(),
        "Scheduler rejects a negative maximum frame time");

    check(
        state,
        negative_frame_result.error().code ==
            ErrorCode::invalid_argument,
        "Negative maximum frame time reports "
        "invalid_argument");

    FixedStepConfig invalid_limit_config{};
    invalid_limit_config.maximum_updates_per_frame = 0U;

    const Result<FixedStepScheduler>
        invalid_limit_result =
            FixedStepScheduler::create(
                invalid_limit_config);

    check(
        state,
        !invalid_limit_result.has_value(),
        "Scheduler rejects a zero frame update limit");

    check(
        state,
        invalid_limit_result.error().code ==
            ErrorCode::invalid_argument,
        "Zero frame update limit reports "
        "invalid_argument");

    FixedStepConfig test_config{};
    test_config.simulation_step = 10ms;
    test_config.maximum_frame_time = 100ms;
    test_config.maximum_updates_per_frame = 4U;

    Result<FixedStepScheduler> zero_result =
        FixedStepScheduler::create(test_config);

    check(
        state,
        zero_result.has_value(),
        "Scheduler accepts the deterministic test "
        "configuration");

    if (zero_result.has_value())
    {
        FixedStepScheduler scheduler{
            std::move(zero_result.value())
        };

        const Result<FrameSchedule> zero_schedule_result =
            scheduler.advance(Nanoseconds::zero());

        check(
            state,
            zero_schedule_result.has_value(),
            "Scheduler accepts a zero-duration frame");

        if (zero_schedule_result.has_value())
        {
            const FrameSchedule& schedule =
                zero_schedule_result.value();

            check(
                state,
                schedule.frame_index == 0ULL,
                "First frame uses frame index zero");

            check(
                state,
                schedule.first_simulation_tick_index ==
                    0ULL,
                "First frame begins at simulation tick zero");

            check(
                state,
                schedule.simulation_update_count == 0U,
                "Zero-duration frame schedules no updates");

            check(
                state,
                schedule.raw_elapsed ==
                    Nanoseconds::zero(),
                "Zero-duration frame preserves raw time");

            check(
                state,
                schedule.accepted_elapsed ==
                    Nanoseconds::zero(),
                "Zero-duration frame accepts zero time");

            check(
                state,
                schedule.dropped_elapsed ==
                    Nanoseconds::zero(),
                "Zero-duration frame drops no time");

            check(
                state,
                schedule.remaining_accumulator ==
                    Nanoseconds::zero(),
                "Zero-duration frame leaves no remainder");

            check(
                state,
                nearly_equal(
                    schedule.interpolation_alpha,
                    0.0),
                "Zero-duration frame has zero "
                "interpolation");

            check(
                state,
                !schedule.frame_time_clamped,
                "Zero-duration frame is not clamped");

            check(
                state,
                !schedule.update_limit_reached,
                "Zero-duration frame does not reach the "
                "update limit");
        }

        check(
            state,
            scheduler.completed_frame_count() == 1ULL,
            "Accepted zero-duration frame advances the "
            "frame counter");

        check(
            state,
            scheduler.
                completed_simulation_tick_count() ==
                0ULL,
            "Zero-duration frame does not advance the "
            "tick counter");

        const std::uint64_t frame_count_before_error =
            scheduler.completed_frame_count();

        const Result<FrameSchedule>
            negative_elapsed_result =
                scheduler.advance(Nanoseconds{-1});

        check(
            state,
            !negative_elapsed_result.has_value(),
            "Scheduler rejects negative elapsed time");

        check(
            state,
            negative_elapsed_result.error().code ==
                ErrorCode::invalid_argument,
            "Negative elapsed time reports "
            "invalid_argument");

        check(
            state,
            scheduler.completed_frame_count() ==
                frame_count_before_error,
            "Rejected elapsed time does not mutate the "
            "frame counter");

        check(
            state,
            scheduler.
                completed_simulation_tick_count() ==
                0ULL,
            "Rejected elapsed time does not mutate the "
            "tick counter");

        check(
            state,
            scheduler.accumulator() ==
                Nanoseconds::zero(),
            "Rejected elapsed time does not mutate the "
            "accumulator");
    }

    Result<FixedStepScheduler> accumulation_result =
        FixedStepScheduler::create(test_config);

    check(
        state,
        accumulation_result.has_value(),
        "Accumulator test scheduler initializes");

    if (accumulation_result.has_value())
    {
        FixedStepScheduler scheduler{
            std::move(accumulation_result.value())
        };

        const Result<FrameSchedule> first_result =
            scheduler.advance(25ms);

        check(
            state,
            first_result.has_value(),
            "Scheduler advances a 25 millisecond frame");

        if (first_result.has_value())
        {
            const FrameSchedule& schedule =
                first_result.value();

            check(
                state,
                schedule.frame_index == 0ULL,
                "First accumulated frame uses index zero");

            check(
                state,
                schedule.first_simulation_tick_index ==
                    0ULL,
                "First accumulated frame begins at tick "
                "zero");

            check(
                state,
                schedule.simulation_update_count == 2U,
                "Twenty-five milliseconds schedules two "
                "fixed updates");

            check(
                state,
                schedule.raw_elapsed == 25ms,
                "Frame schedule preserves raw elapsed "
                "time");

            check(
                state,
                schedule.accepted_elapsed == 25ms,
                "Unclamped frame accepts all elapsed time");

            check(
                state,
                schedule.dropped_elapsed ==
                    Nanoseconds::zero(),
                "Unclamped frame drops no time");

            check(
                state,
                schedule.remaining_accumulator == 5ms,
                "Twenty-five milliseconds preserves a "
                "five millisecond remainder");

            check(
                state,
                nearly_equal(
                    schedule.interpolation_alpha,
                    0.5),
                "Five millisecond remainder produces "
                "one-half interpolation");

            check(
                state,
                !schedule.frame_time_clamped,
                "Normal frame is not clamped");

            check(
                state,
                !schedule.update_limit_reached,
                "Normal frame does not reach the update "
                "limit");
        }

        check(
            state,
            scheduler.completed_frame_count() == 1ULL,
            "First advancement increments frame count");

        check(
            state,
            scheduler.
                completed_simulation_tick_count() ==
                2ULL,
            "First advancement completes two ticks");

        check(
            state,
            scheduler.simulated_time() == 20ms,
            "First advancement records twenty "
            "milliseconds of simulation");

        check(
            state,
            scheduler.dropped_time() ==
                Nanoseconds::zero(),
            "First advancement records no dropped time");

        check(
            state,
            scheduler.accumulator() == 5ms,
            "Scheduler stores the fractional remainder");

        const Result<FrameSchedule> second_result =
            scheduler.advance(5ms);

        check(
            state,
            second_result.has_value(),
            "Scheduler advances a second partial frame");

        if (second_result.has_value())
        {
            const FrameSchedule& schedule =
                second_result.value();

            check(
                state,
                schedule.frame_index == 1ULL,
                "Second frame uses frame index one");

            check(
                state,
                schedule.first_simulation_tick_index ==
                    2ULL,
                "Second frame begins at simulation tick "
                "two");

            check(
                state,
                schedule.simulation_update_count == 1U,
                "Two partial frames combine into one "
                "fixed update");

            check(
                state,
                schedule.remaining_accumulator ==
                    Nanoseconds::zero(),
                "Combined partial frames consume the "
                "remainder");

            check(
                state,
                nearly_equal(
                    schedule.interpolation_alpha,
                    0.0),
                "Consumed remainder produces zero "
                "interpolation");
        }

        check(
            state,
            scheduler.completed_frame_count() == 2ULL,
            "Second advancement increments frame count");

        check(
            state,
            scheduler.
                completed_simulation_tick_count() ==
                3ULL,
            "Second advancement completes the third tick");

        check(
            state,
            scheduler.simulated_time() == 30ms,
            "Two frames record thirty milliseconds of "
            "simulation");

        check(
            state,
            scheduler.accumulator() ==
                Nanoseconds::zero(),
            "Two frames leave no accumulated remainder");
    }

    FixedStepConfig clamp_config{};
    clamp_config.simulation_step = 10ms;
    clamp_config.maximum_frame_time = 25ms;
    clamp_config.maximum_updates_per_frame = 4U;

    Result<FixedStepScheduler> clamp_result =
        FixedStepScheduler::create(clamp_config);

    check(
        state,
        clamp_result.has_value(),
        "Clamp test scheduler initializes");

    if (clamp_result.has_value())
    {
        FixedStepScheduler scheduler{
            std::move(clamp_result.value())
        };

        const Result<FrameSchedule> schedule_result =
            scheduler.advance(40ms);

        check(
            state,
            schedule_result.has_value(),
            "Scheduler advances a frame exceeding the "
            "time clamp");

        if (schedule_result.has_value())
        {
            const FrameSchedule& schedule =
                schedule_result.value();

            check(
                state,
                schedule.raw_elapsed == 40ms,
                "Clamped frame preserves raw elapsed time");

            check(
                state,
                schedule.accepted_elapsed == 25ms,
                "Clamped frame accepts only the "
                "configured maximum");

            check(
                state,
                schedule.dropped_elapsed == 15ms,
                "Clamped frame reports discarded time");

            check(
                state,
                schedule.simulation_update_count == 2U,
                "Clamped frame schedules two updates");

            check(
                state,
                schedule.remaining_accumulator == 5ms,
                "Clamped frame preserves its fractional "
                "remainder");

            check(
                state,
                schedule.frame_time_clamped,
                "Frame time clamp is reported");

            check(
                state,
                !schedule.update_limit_reached,
                "Frame time clamp does not falsely report "
                "an update limit");
        }

        check(
            state,
            scheduler.simulated_time() == 20ms,
            "Clamped frame records processed simulation "
            "time");

        check(
            state,
            scheduler.dropped_time() == 15ms,
            "Clamped frame records cumulative dropped "
            "time");
    }

    FixedStepConfig limit_config{};
    limit_config.simulation_step = 10ms;
    limit_config.maximum_frame_time = 100ms;
    limit_config.maximum_updates_per_frame = 2U;

    Result<FixedStepScheduler> limit_result =
        FixedStepScheduler::create(limit_config);

    check(
        state,
        limit_result.has_value(),
        "Update-limit test scheduler initializes");

    if (limit_result.has_value())
    {
        FixedStepScheduler scheduler{
            std::move(limit_result.value())
        };

        const Result<FrameSchedule> boundary_result =
            scheduler.advance(20ms);

        check(
            state,
            boundary_result.has_value(),
            "Scheduler advances a frame exactly at the "
            "update limit");

        if (boundary_result.has_value())
        {
            const FrameSchedule& schedule =
                boundary_result.value();

            check(
                state,
                schedule.simulation_update_count == 2U,
                "Exact update-limit frame schedules both "
                "updates");

            check(
                state,
                !schedule.update_limit_reached,
                "Exact update-limit frame is not reported "
                "as exceeding the limit");

            check(
                state,
                schedule.dropped_elapsed ==
                    Nanoseconds::zero(),
                "Exact update-limit frame drops no time");
        }

        scheduler.reset();

        check(
            state,
            scheduler.completed_frame_count() == 0ULL,
            "Reset clears completed frame count");

        check(
            state,
            scheduler.
                completed_simulation_tick_count() ==
                0ULL,
            "Reset clears completed tick count");

        check(
            state,
            scheduler.simulated_time() ==
                Nanoseconds::zero(),
            "Reset clears simulated time");

        check(
            state,
            scheduler.dropped_time() ==
                Nanoseconds::zero(),
            "Reset clears dropped time");

        check(
            state,
            scheduler.accumulator() ==
                Nanoseconds::zero(),
            "Reset clears accumulated time");

        check(
            state,
            scheduler.config().
                maximum_updates_per_frame == 2U,
            "Reset preserves scheduler configuration");

        const Result<FrameSchedule> overload_result =
            scheduler.advance(55ms);

        check(
            state,
            overload_result.has_value(),
            "Scheduler advances an overloaded frame");

        if (overload_result.has_value())
        {
            const FrameSchedule& schedule =
                overload_result.value();

            check(
                state,
                schedule.simulation_update_count == 2U,
                "Overloaded frame obeys the update limit");

            check(
                state,
                schedule.update_limit_reached,
                "Overloaded frame reports the update "
                "limit");

            check(
                state,
                !schedule.frame_time_clamped,
                "Update-limited frame is not falsely "
                "reported as time-clamped");

            check(
                state,
                schedule.dropped_elapsed == 30ms,
                "Overloaded frame drops three complete "
                "simulation steps");

            check(
                state,
                schedule.remaining_accumulator == 5ms,
                "Overloaded frame preserves only its "
                "fractional remainder");

            check(
                state,
                nearly_equal(
                    schedule.interpolation_alpha,
                    0.5),
                "Overloaded frame reports remainder "
                "interpolation");
        }

        check(
            state,
            scheduler.
                completed_simulation_tick_count() ==
                2ULL,
            "Overloaded frame completes only permitted "
            "ticks");

        check(
            state,
            scheduler.simulated_time() == 20ms,
            "Overloaded frame records only processed "
            "simulation time");

        check(
            state,
            scheduler.dropped_time() == 30ms,
            "Overloaded frame records discarded complete "
            "steps");

        check(
            state,
            scheduler.accumulator() == 5ms,
            "Overloaded frame retains its remainder");
    }

    FixedStepConfig deterministic_config{};
    deterministic_config.simulation_step = 10ms;
    deterministic_config.maximum_frame_time = 1s;
    deterministic_config.maximum_updates_per_frame =
        100U;

    Result<FixedStepScheduler> single_result =
        FixedStepScheduler::create(
            deterministic_config);

    Result<FixedStepScheduler> partitioned_result =
        FixedStepScheduler::create(
            deterministic_config);

    check(
        state,
        single_result.has_value() &&
            partitioned_result.has_value(),
        "Determinism comparison schedulers initialize");

    if (single_result.has_value() &&
        partitioned_result.has_value())
    {
        FixedStepScheduler single_scheduler{
            std::move(single_result.value())
        };

        FixedStepScheduler partitioned_scheduler{
            std::move(partitioned_result.value())
        };

        const Result<FrameSchedule>
            single_advance_result =
                single_scheduler.advance(95ms);

        const Result<FrameSchedule>
            partition_one_result =
                partitioned_scheduler.advance(30ms);

        const Result<FrameSchedule>
            partition_two_result =
                partitioned_scheduler.advance(20ms);

        const Result<FrameSchedule>
            partition_three_result =
                partitioned_scheduler.advance(45ms);

        check(
            state,
            single_advance_result.has_value() &&
                partition_one_result.has_value() &&
                partition_two_result.has_value() &&
                partition_three_result.has_value(),
            "Equivalent elapsed-time partitions advance "
            "successfully");

        check(
            state,
            single_scheduler.
                completed_simulation_tick_count() ==
                partitioned_scheduler.
                    completed_simulation_tick_count(),
            "Equivalent elapsed time produces the same "
            "tick count");

        check(
            state,
            single_scheduler.simulated_time() ==
                partitioned_scheduler.simulated_time(),
            "Equivalent elapsed time produces the same "
            "simulated time");

        check(
            state,
            single_scheduler.accumulator() ==
                partitioned_scheduler.accumulator(),
            "Equivalent elapsed time produces the same "
            "remainder");

        check(
            state,
            single_scheduler.dropped_time() ==
                partitioned_scheduler.dropped_time(),
            "Equivalent elapsed time produces the same "
            "dropped time");

        check(
            state,
            single_scheduler.
                completed_simulation_tick_count() ==
                9ULL,
            "Ninety-five milliseconds completes nine "
            "fixed ticks");

        check(
            state,
            single_scheduler.accumulator() == 5ms,
            "Ninety-five milliseconds leaves five "
            "milliseconds accumulated");
    }

    std::cout
        << "\nFixed-step scheduler test summary: "
        << state.checks - state.failures
        << '/'
        << state.checks
        << " passed.\n";

    return state.failures == 0 ? 0 : 1;
}