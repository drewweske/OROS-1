#include "oros/ai/actor_schedule_window.hpp"

#include <cstdint>
#include <iostream>
#include <limits>
#include <string_view>
#include <type_traits>
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
    using namespace oros::ai;
    using namespace oros::foundation;
    using namespace oros::world;

    static_assert(
        !std::is_default_constructible_v<
            ActorScheduleWindow>);

    static_assert(
        std::is_nothrow_copy_constructible_v<
            ActorScheduleWindow>);

    static_assert(
        std::is_nothrow_copy_assignable_v<
            ActorScheduleWindow>);

    static_assert(
        std::is_nothrow_move_constructible_v<
            ActorScheduleWindow>);

    static_assert(
        std::is_nothrow_move_assignable_v<
            ActorScheduleWindow>);

    static_assert(
        std::is_trivially_copyable_v<
            ActorScheduleWindow>);

    static_assert(
        noexcept(
            std::declval<
                const ActorScheduleWindow&>().
                contains(
                    std::declval<
                        WorldTime>())));

    TestState state{};

    const WorldTime epoch =
        WorldTime::epoch();

    const WorldTime t1 =
        WorldTime::
            from_microseconds_since_epoch(
                1ULL);

    const WorldTime t99 =
        WorldTime::
            from_microseconds_since_epoch(
                99ULL);

    const WorldTime t100 =
        WorldTime::
            from_microseconds_since_epoch(
                100ULL);

    const WorldTime t150 =
        WorldTime::
            from_microseconds_since_epoch(
                150ULL);

    const WorldTime t199 =
        WorldTime::
            from_microseconds_since_epoch(
                199ULL);

    const WorldTime t200 =
        WorldTime::
            from_microseconds_since_epoch(
                200ULL);

    const WorldTime t300 =
        WorldTime::
            from_microseconds_since_epoch(
                300ULL);

    const WorldTime maximum =
        WorldTime::
            from_microseconds_since_epoch(
                (
                    std::numeric_limits<
                        std::uint64_t>::max
                )());

    const WorldTime before_maximum =
        WorldTime::
            from_microseconds_since_epoch(
                (
                    std::numeric_limits<
                        std::uint64_t>::max
                )() - 1ULL);

    const Result<ActorScheduleWindow>
        ordinary_result =
            ActorScheduleWindow::create(
                t100,
                t200);

    check(
        state,
        ordinary_result.has_value(),
        "Strictly increasing WorldTime endpoints create a schedule window");

    if (!ordinary_result.has_value())
    {
        return 1;
    }

    const ActorScheduleWindow&
        ordinary =
            ordinary_result.value();

    check(
        state,
        ordinary.start_inclusive() ==
            t100 &&
            ordinary.end_exclusive() ==
                t200,
        "Schedule window preserves exact WorldTime endpoints");

    check(
        state,
        ordinary.contains(
            t100),
        "Schedule window includes its start boundary");

    check(
        state,
        ordinary.contains(
            t150),
        "Schedule window contains interior world time");

    check(
        state,
        ordinary.contains(
            t199),
        "Schedule window contains the final coordinate before its end");

    check(
        state,
        !ordinary.contains(
            t200),
        "Schedule window excludes its end boundary");

    check(
        state,
        !ordinary.contains(
            t99),
        "Schedule window excludes world time before its start");

    const Result<ActorScheduleWindow>
        adjacent_result =
            ActorScheduleWindow::create(
                t200,
                t300);

    check(
        state,
        adjacent_result.has_value(),
        "Adjacent schedule window constructs successfully");

    if (!adjacent_result.has_value())
    {
        return 1;
    }

    const ActorScheduleWindow&
        adjacent =
            adjacent_result.value();

    check(
        state,
        !ordinary.contains(
            t200) &&
            adjacent.contains(
                t200),
        "Half-open windows hand off exactly at a shared boundary");

    const Result<ActorScheduleWindow>
        equal_result =
            ActorScheduleWindow::create(
                t200,
                t200);

    check(
        state,
        !equal_result.has_value() &&
            equal_result.error().code ==
                ErrorCode::invalid_argument,
        "Zero-length schedule window is rejected");

    const Result<ActorScheduleWindow>
        reversed_result =
            ActorScheduleWindow::create(
                t300,
                t200);

    check(
        state,
        !reversed_result.has_value() &&
            reversed_result.error().code ==
                ErrorCode::invalid_argument,
        "Reversed schedule window is rejected");

    const Result<ActorScheduleWindow>
        epoch_result =
            ActorScheduleWindow::create(
                epoch,
                t1);

    check(
        state,
        epoch_result.has_value() &&
            epoch_result.value().
                contains(epoch) &&
            !epoch_result.value().
                contains(t1),
        "World epoch is a valid inclusive schedule start");

    const Result<ActorScheduleWindow>
        maximum_result =
            ActorScheduleWindow::create(
                before_maximum,
                maximum);

    check(
        state,
        maximum_result.has_value(),
        "Schedule window supports the upper WorldTime range");

    if (!maximum_result.has_value())
    {
        return 1;
    }

    check(
        state,
        maximum_result.value().
                contains(
                    before_maximum) &&
            !maximum_result.value().
                contains(
                    maximum),
        "Maximum endpoint obeys the same end-exclusive rule without arithmetic");

    const ActorScheduleWindow copied =
        ordinary;

    check(
        state,
        copied ==
            ordinary,
        "Schedule window copy preserves exact temporal meaning");

    ActorScheduleWindow assigned =
        adjacent;

    assigned =
        ordinary;

    check(
        state,
        assigned ==
            ordinary,
        "Schedule window assignment preserves exact temporal meaning");

    const Result<ActorScheduleWindow>
        same_history_result =
            ActorScheduleWindow::create(
                t100,
                t200);

    check(
        state,
        same_history_result.has_value() &&
            same_history_result.value() ==
                ordinary,
        "Equal endpoints deterministically produce equal schedule windows");

    std::cout
        << state.checks
        << " checks, "
        << state.failures
        << " failures\n";

    return state.failures == 0
        ? 0
        : 1;
}