#include "oros/ai/actor_scheduled_activity.hpp"

#include "oros/foundation/result.hpp"
#include "oros/world/world_time.hpp"

#include <iostream>
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
            ActorScheduledActivity>);

    static_assert(
        std::is_copy_constructible_v<
            ActorScheduledActivity>);

    static_assert(
        std::is_copy_assignable_v<
            ActorScheduledActivity>);

    static_assert(
        std::is_nothrow_move_constructible_v<
            ActorScheduledActivity>);

    static_assert(
        std::is_nothrow_move_assignable_v<
            ActorScheduledActivity>);

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const ActorScheduledActivity&>().
                    intent()),
            const ActorActivityIntentKey&>);

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const ActorScheduledActivity&>().
                    window()),
            const ActorScheduleWindow&>);

    TestState state{};

    const Result<ActorActivityIntentKey>
        sleep_intent_result =
            ActorActivityIntentKey::create(
                "oros",
                "sleep");

    const Result<ActorActivityIntentKey>
        work_intent_result =
            ActorActivityIntentKey::create(
                "oros",
                "work");

    const WorldTime t100 =
        WorldTime::
            from_microseconds_since_epoch(
                100ULL);

    const WorldTime t150 =
        WorldTime::
            from_microseconds_since_epoch(
                150ULL);

    const WorldTime t200 =
        WorldTime::
            from_microseconds_since_epoch(
                200ULL);

    const WorldTime t300 =
        WorldTime::
            from_microseconds_since_epoch(
                300ULL);

    const Result<ActorScheduleWindow>
        morning_window_result =
            ActorScheduleWindow::create(
                t100,
                t200);

    const Result<ActorScheduleWindow>
        later_window_result =
            ActorScheduleWindow::create(
                t200,
                t300);

    check(
        state,
        sleep_intent_result.has_value() &&
            work_intent_result.has_value() &&
            morning_window_result.has_value() &&
            later_window_result.has_value(),
        "Scheduled-activity prerequisites construct successfully");

    if (
        !sleep_intent_result.has_value() ||
        !work_intent_result.has_value() ||
        !morning_window_result.has_value() ||
        !later_window_result.has_value())
    {
        return 1;
    }

    const ActorActivityIntentKey&
        sleep_intent =
            sleep_intent_result.value();

    const ActorActivityIntentKey&
        work_intent =
            work_intent_result.value();

    const ActorScheduleWindow&
        morning_window =
            morning_window_result.value();

    const ActorScheduleWindow&
        later_window =
            later_window_result.value();

    const ActorScheduledActivity
        sleep_morning{
            sleep_intent,
            morning_window
        };

    check(
        state,
        sleep_morning.intent() ==
            sleep_intent,
        "Scheduled activity preserves exact activity-intent identity");

    check(
        state,
        sleep_morning.window() ==
            morning_window,
        "Scheduled activity preserves exact temporal window");

    check(
        state,
        sleep_morning.intent().
                intent_namespace() ==
            "oros" &&
            sleep_morning.intent().
                intent_name() ==
            "sleep",
        "Scheduled activity owns the semantic intent value");

    check(
        state,
        sleep_morning.window().
            contains(t100),
        "Stored schedule window retains inclusive-start semantics");

    check(
        state,
        sleep_morning.window().
            contains(t150),
        "Stored schedule window retains interior membership semantics");

    check(
        state,
        !sleep_morning.window().
            contains(t200),
        "Stored schedule window retains exclusive-end semantics");

    const ActorScheduledActivity
        same_sleep_morning{
            sleep_intent,
            morning_window
        };

    check(
        state,
        same_sleep_morning ==
            sleep_morning,
        "Equal intent and window deterministically produce equal scheduled facts");

    const ActorScheduledActivity
        work_morning{
            work_intent,
            morning_window
        };

    check(
        state,
        !(work_morning ==
            sleep_morning),
        "Different intent makes a different scheduled activity fact");

    const ActorScheduledActivity
        sleep_later{
            sleep_intent,
            later_window
        };

    check(
        state,
        !(sleep_later ==
            sleep_morning),
        "Different temporal window makes a different scheduled activity fact");

    const ActorScheduledActivity
        work_later{
            work_intent,
            later_window
        };

    check(
        state,
        !(work_later ==
            sleep_morning),
        "Different intent and window remain a distinct scheduled fact");

    const ActorScheduledActivity copied =
        sleep_morning;

    check(
        state,
        copied ==
            sleep_morning,
        "Scheduled activity copy preserves complete deterministic meaning");

    ActorScheduledActivity assigned =
        work_later;

    assigned =
        sleep_morning;

    check(
        state,
        assigned ==
            sleep_morning,
        "Scheduled activity copy assignment preserves complete meaning");

    ActorScheduledActivity moved =
        ActorScheduledActivity{
            sleep_intent,
            morning_window
        };

    ActorScheduledActivity move_target =
        std::move(moved);

    check(
        state,
        move_target ==
            sleep_morning,
        "Scheduled activity move preserves complete deterministic meaning");

    ActorScheduledActivity
        move_assigned{
            work_intent,
            later_window
        };

    ActorScheduledActivity
        move_source{
            sleep_intent,
            morning_window
        };

    move_assigned =
        std::move(move_source);

    check(
        state,
        move_assigned ==
            sleep_morning,
        "Scheduled activity move assignment preserves complete meaning");

    check(
        state,
        sleep_morning.intent() ==
            same_sleep_morning.intent() &&
            sleep_morning.window() ==
            same_sleep_morning.window(),
        "Scheduled fact equality is composed only from intent and window values");

    std::cout
        << state.checks
        << " checks, "
        << state.failures
        << " failures\n";

    return state.failures == 0
        ? 0
        : 1;
}