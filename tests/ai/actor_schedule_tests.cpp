#include "oros/ai/actor_schedule.hpp"

#include "oros/ai/actor_activity_intent_key.hpp"
#include "oros/ai/actor_schedule_window.hpp"
#include "oros/foundation/result.hpp"
#include "oros/world/world_time.hpp"

#include <cstdint>
#include <expected>
#include <iostream>
#include <span>
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

    oros::foundation::Result<
        oros::ai::ActorScheduledActivity>
    make_activity(
        const std::string_view intent_name,
        const std::uint64_t
            start_microseconds,
        const std::uint64_t
            end_microseconds)
    {
        using namespace oros::ai;
        using namespace oros::foundation;
        using namespace oros::world;

        Result<ActorActivityIntentKey>
            intent_result =
                ActorActivityIntentKey::create(
                    "oros",
                    intent_name);

        if (!intent_result.has_value())
        {
            return std::unexpected{
                std::move(
                    intent_result.error())
            };
        }

        Result<ActorScheduleWindow>
            window_result =
                ActorScheduleWindow::create(
                    WorldTime::
                        from_microseconds_since_epoch(
                            start_microseconds),
                    WorldTime::
                        from_microseconds_since_epoch(
                            end_microseconds));

        if (!window_result.has_value())
        {
            return std::unexpected{
                std::move(
                    window_result.error())
            };
        }

        return ActorScheduledActivity{
            std::move(
                intent_result.value()),
            std::move(
                window_result.value())
        };
    }
}

int main()
{
    using namespace oros::ai;
    using namespace oros::foundation;

    static_assert(
        std::is_default_constructible_v<
            ActorSchedule>);

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const ActorSchedule&>().
                    activities_in_time_order()),
            std::span<
                const ActorScheduledActivity>>);

    TestState state{};

    ActorSchedule empty_schedule{};

    check(
        state,
        empty_schedule.empty() &&
            empty_schedule.size() == 0U,
        "Default ActorSchedule is a valid empty schedule");

    check(
        state,
        empty_schedule.
            activities_in_time_order().
            empty(),
        "Empty schedule exposes an empty read-only activity span");

    const Result<ActorScheduledActivity>
        sleep_result =
            make_activity(
                "sleep",
                0ULL,
                100ULL);

    const Result<ActorScheduledActivity>
        work_result =
            make_activity(
                "work",
                100ULL,
                200ULL);

    const Result<ActorScheduledActivity>
        eat_result =
            make_activity(
                "eat",
                200ULL,
                300ULL);

    const Result<ActorScheduledActivity>
        relax_result =
            make_activity(
                "relax",
                300ULL,
                400ULL);

    check(
        state,
        sleep_result.has_value() &&
            work_result.has_value() &&
            eat_result.has_value() &&
            relax_result.has_value(),
        "Canonical schedule test activities construct successfully");

    if (
        !sleep_result.has_value() ||
        !work_result.has_value() ||
        !eat_result.has_value() ||
        !relax_result.has_value())
    {
        return 1;
    }

    const ActorScheduledActivity&
        sleep =
            sleep_result.value();

    const ActorScheduledActivity&
        work =
            work_result.value();

    const ActorScheduledActivity&
        eat =
            eat_result.value();

    const ActorScheduledActivity&
        relax =
            relax_result.value();

    ActorSchedule canonical_schedule{};

    Status status =
        canonical_schedule.insert(
            work);

    check(
        state,
        status.has_value() &&
            canonical_schedule.size() == 1U &&
            !canonical_schedule.empty(),
        "First scheduled activity inserts successfully");

    status =
        canonical_schedule.insert(
            relax);

    check(
        state,
        status.has_value(),
        "Later activity inserts successfully");

    status =
        canonical_schedule.insert(
            sleep);

    check(
        state,
        status.has_value(),
        "Earlier activity inserts successfully");

    status =
        canonical_schedule.insert(
            eat);

    check(
        state,
        status.has_value(),
        "Activity adjacent to both neighbors inserts successfully");

    const std::span<
        const ActorScheduledActivity>
        canonical =
            canonical_schedule.
                activities_in_time_order();

    check(
        state,
        canonical.size() == 4U,
        "Canonical schedule exposes every inserted activity exactly once");

    check(
        state,
        canonical[0] == sleep &&
            canonical[1] == work &&
            canonical[2] == eat &&
            canonical[3] == relax,
        "Insertion order is normalized to chronological start-time order");

    check(
        state,
        canonical[0].
                window().
                end_exclusive() ==
            canonical[1].
                window().
                start_inclusive() &&
            canonical[1].
                window().
                end_exclusive() ==
            canonical[2].
                window().
                start_inclusive() &&
            canonical[2].
                window().
                end_exclusive() ==
            canonical[3].
                window().
                start_inclusive(),
        "Exact half-open adjacency remains valid throughout the schedule");

    ActorSchedule opposite_order_schedule{};

    status =
        opposite_order_schedule.insert(
            relax);

    if (!status.has_value())
    {
        return 1;
    }

    status =
        opposite_order_schedule.insert(
            eat);

    if (!status.has_value())
    {
        return 1;
    }

    status =
        opposite_order_schedule.insert(
            work);

    if (!status.has_value())
    {
        return 1;
    }

    status =
        opposite_order_schedule.insert(
            sleep);

    check(
        state,
        status.has_value(),
        "Reverse insertion order remains valid");

    const auto opposite =
        opposite_order_schedule.
            activities_in_time_order();

    bool same_canonical_order =
        canonical.size() ==
        opposite.size();

    if (same_canonical_order)
    {
        for (
            std::size_t index = 0U;
            index < canonical.size();
            ++index)
        {
            if (
                !(canonical[index] ==
                    opposite[index]))
            {
                same_canonical_order =
                    false;

                break;
            }
        }
    }

    check(
        state,
        same_canonical_order,
        "Different insertion histories produce identical canonical schedule order");

    const Result<ActorScheduledActivity>
        gap_first_result =
            make_activity(
                "sleep",
                0ULL,
                100ULL);

    const Result<ActorScheduledActivity>
        gap_second_result =
            make_activity(
                "work",
                200ULL,
                300ULL);

    if (
        !gap_first_result.has_value() ||
        !gap_second_result.has_value())
    {
        return 1;
    }

    ActorSchedule gap_schedule{};

    status =
        gap_schedule.insert(
            gap_second_result.value());

    if (!status.has_value())
    {
        return 1;
    }

    status =
        gap_schedule.insert(
            gap_first_result.value());

    check(
        state,
        status.has_value() &&
            gap_schedule.size() == 2U,
        "Temporal gaps are valid schedule state");

    const auto gap_activities =
        gap_schedule.
            activities_in_time_order();

    check(
        state,
        gap_activities[0].
                window().
                end_exclusive() <
            gap_activities[1].
                window().
                start_inclusive(),
        "Gap schedule preserves chronological ordering without filling gaps");

    const Result<ActorScheduledActivity>
        existing_result =
            make_activity(
                "work",
                100ULL,
                200ULL);

    const Result<ActorScheduledActivity>
        overlap_previous_result =
            make_activity(
                "sleep",
                150ULL,
                250ULL);

    if (
        !existing_result.has_value() ||
        !overlap_previous_result.has_value())
    {
        return 1;
    }

    ActorSchedule previous_overlap_schedule{};

    status =
        previous_overlap_schedule.insert(
            existing_result.value());

    if (!status.has_value())
    {
        return 1;
    }

    const std::size_t
        previous_overlap_size_before =
            previous_overlap_schedule.size();

    status =
        previous_overlap_schedule.insert(
            overlap_previous_result.value());

    check(
        state,
        !status.has_value() &&
            status.error().code ==
                ErrorCode::invalid_state,
        "Overlap with the immediate predecessor is rejected");

    check(
        state,
        previous_overlap_schedule.size() ==
            previous_overlap_size_before &&
            previous_overlap_schedule.
                activities_in_time_order()[0] ==
            existing_result.value(),
        "Predecessor-overlap rejection leaves the schedule unchanged");

    const Result<ActorScheduledActivity>
        next_existing_result =
            make_activity(
                "work",
                200ULL,
                300ULL);

    const Result<ActorScheduledActivity>
        overlap_next_result =
            make_activity(
                "sleep",
                150ULL,
                250ULL);

    if (
        !next_existing_result.has_value() ||
        !overlap_next_result.has_value())
    {
        return 1;
    }

    ActorSchedule next_overlap_schedule{};

    status =
        next_overlap_schedule.insert(
            next_existing_result.value());

    if (!status.has_value())
    {
        return 1;
    }

    const std::size_t
        next_overlap_size_before =
            next_overlap_schedule.size();

    status =
        next_overlap_schedule.insert(
            overlap_next_result.value());

    check(
        state,
        !status.has_value() &&
            status.error().code ==
                ErrorCode::invalid_state,
        "Overlap with the immediate successor is rejected");

    check(
        state,
        next_overlap_schedule.size() ==
            next_overlap_size_before &&
            next_overlap_schedule.
                activities_in_time_order()[0] ==
            next_existing_result.value(),
        "Successor-overlap rejection leaves the schedule unchanged");

    const Result<ActorScheduledActivity>
        enclosing_existing_result =
            make_activity(
                "work",
                150ULL,
                200ULL);

    const Result<ActorScheduledActivity>
        enclosing_candidate_result =
            make_activity(
                "sleep",
                100ULL,
                300ULL);

    if (
        !enclosing_existing_result.has_value() ||
        !enclosing_candidate_result.has_value())
    {
        return 1;
    }

    ActorSchedule enclosing_schedule{};

    status =
        enclosing_schedule.insert(
            enclosing_existing_result.value());

    if (!status.has_value())
    {
        return 1;
    }

    status =
        enclosing_schedule.insert(
            enclosing_candidate_result.value());

    check(
        state,
        !status.has_value() &&
            status.error().code ==
                ErrorCode::invalid_state &&
            enclosing_schedule.size() ==
                1U,
        "Candidate that encloses an existing activity is rejected without mutation");

    const Result<ActorScheduledActivity>
        containing_existing_result =
            make_activity(
                "work",
                100ULL,
                300ULL);

    const Result<ActorScheduledActivity>
        contained_candidate_result =
            make_activity(
                "sleep",
                150ULL,
                200ULL);

    if (
        !containing_existing_result.has_value() ||
        !contained_candidate_result.has_value())
    {
        return 1;
    }

    ActorSchedule contained_schedule{};

    status =
        contained_schedule.insert(
            containing_existing_result.value());

    if (!status.has_value())
    {
        return 1;
    }

    status =
        contained_schedule.insert(
            contained_candidate_result.value());

    check(
        state,
        !status.has_value() &&
            status.error().code ==
                ErrorCode::invalid_state &&
            contained_schedule.size() ==
                1U,
        "Candidate contained inside an existing activity is rejected without mutation");

    const Result<ActorScheduledActivity>
        same_start_existing_result =
            make_activity(
                "work",
                100ULL,
                200ULL);

    const Result<ActorScheduledActivity>
        same_start_candidate_result =
            make_activity(
                "sleep",
                100ULL,
                150ULL);

    if (
        !same_start_existing_result.has_value() ||
        !same_start_candidate_result.has_value())
    {
        return 1;
    }

    ActorSchedule same_start_schedule{};

    status =
        same_start_schedule.insert(
            same_start_existing_result.value());

    if (!status.has_value())
    {
        return 1;
    }

    status =
        same_start_schedule.insert(
            same_start_candidate_result.value());

    check(
        state,
        !status.has_value() &&
            status.error().code ==
                ErrorCode::invalid_state &&
            same_start_schedule.size() ==
                1U,
        "Equal-start activities are rejected by the ordinary overlap rule");

    ActorSchedule duplicate_schedule{};

    status =
        duplicate_schedule.insert(
            sleep);

    if (!status.has_value())
    {
        return 1;
    }

    status =
        duplicate_schedule.insert(
            sleep);

    check(
        state,
        !status.has_value() &&
            status.error().code ==
                ErrorCode::invalid_state &&
            duplicate_schedule.size() ==
                1U &&
            duplicate_schedule.
                activities_in_time_order()[0] ==
            sleep,
        "Duplicate scheduled fact is rejected by overlap without separate identity policy");

    const Result<ActorScheduledActivity>
        left_result =
            make_activity(
                "sleep",
                0ULL,
                100ULL);

    const Result<ActorScheduledActivity>
        right_result =
            make_activity(
                "work",
                200ULL,
                300ULL);

    const Result<ActorScheduledActivity>
        bridge_result =
            make_activity(
                "eat",
                100ULL,
                200ULL);

    if (
        !left_result.has_value() ||
        !right_result.has_value() ||
        !bridge_result.has_value())
    {
        return 1;
    }

    ActorSchedule bridge_schedule{};

    status =
        bridge_schedule.insert(
            right_result.value());

    if (!status.has_value())
    {
        return 1;
    }

    status =
        bridge_schedule.insert(
            left_result.value());

    if (!status.has_value())
    {
        return 1;
    }

    status =
        bridge_schedule.insert(
            bridge_result.value());

    check(
        state,
        status.has_value(),
        "Activity exactly adjacent to predecessor and successor is accepted");

    const auto bridged =
        bridge_schedule.
            activities_in_time_order();

    check(
        state,
        bridged.size() == 3U &&
            bridged[0] ==
                left_result.value() &&
            bridged[1] ==
                bridge_result.value() &&
            bridged[2] ==
                right_result.value(),
        "Two-sided adjacency inserts at the deterministic chronological position");

    std::cout
        << state.checks
        << " checks, "
        << state.failures
        << " failures\n";

    return state.failures == 0
        ? 0
        : 1;
}