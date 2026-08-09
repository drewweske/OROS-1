#include "oros/ai/actor_schedule_execution_state.hpp"

#include "oros/ai/actor_activity_intent_key.hpp"
#include "oros/ai/actor_schedule.hpp"
#include "oros/ai/actor_schedule_window.hpp"
#include "oros/foundation/result.hpp"
#include "oros/world/entity_id.hpp"
#include "oros/world/world_time.hpp"

#include <cstdint>
#include <expected>
#include <iostream>
#include <optional>
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
    make_scheduled_activity(
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
    using namespace oros::world;

    static_assert(
        !std::is_default_constructible_v<
            ActorScheduleExecutionState>);

    static_assert(
        !std::is_copy_constructible_v<
            ActorScheduleExecutionState>);

    static_assert(
        !std::is_copy_assignable_v<
            ActorScheduleExecutionState>);

    static_assert(
        std::is_nothrow_move_constructible_v<
            ActorScheduleExecutionState>);

    static_assert(
        std::is_nothrow_move_assignable_v<
            ActorScheduleExecutionState>);

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const ActorScheduleExecutionState&>().
                    actor()),
            EntityId>);

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const ActorScheduleExecutionState&>().
                    persistent_intent()),
            const std::optional<
                ActorActivityIntentKey>&>);

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const ActorScheduleExecutionState&>().
                    is_interrupted()),
            bool>);

    static_assert(
        noexcept(
            std::declval<
                const ActorScheduleExecutionState&>().
                actor()));

    static_assert(
        noexcept(
            std::declval<
                const ActorScheduleExecutionState&>().
                persistent_intent()));

    static_assert(
        noexcept(
            std::declval<
                const ActorScheduleExecutionState&>().
                is_interrupted()));

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    ActorScheduleExecutionState&>().
                    begin_interruption()),
            Status>);

    static_assert(
        !noexcept(
            std::declval<
                ActorScheduleExecutionState&>().
                begin_interruption()));
    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    ActorScheduleExecutionState&>().
                    end_interruption_and_rejoin_schedule(
                        std::declval<
                            const ActorSchedule&>(),
                        std::declval<
                            WorldTime>())),
            Status>);

    static_assert(
        !noexcept(
            std::declval<
                ActorScheduleExecutionState&>().
                end_interruption_and_rejoin_schedule(
                    std::declval<
                        const ActorSchedule&>(),
                    std::declval<
                        WorldTime>())));
    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    ActorScheduleExecutionState&>().
                    synchronize_following_intent_from_schedule(
                        std::declval<
                            const ActorSchedule&>(),
                        std::declval<
                            WorldTime>())),
            Status>);

    static_assert(
        !noexcept(
            std::declval<
                ActorScheduleExecutionState&>().
                synchronize_following_intent_from_schedule(
                    std::declval<
                        const ActorSchedule&>(),
                    std::declval<
                        WorldTime>())));

    static_assert(
        noexcept(
            std::declval<
                std::optional<
                    ActorActivityIntentKey>&>() =
            std::declval<
                std::optional<
                    ActorActivityIntentKey>&&>()));

    TestState state{};

    const EntityId actor_a{
        0x4F524F53ULL,
        1001ULL
    };

    const EntityId actor_b{
        0x4F524F53ULL,
        1002ULL
    };

    const Result<ActorActivityIntentKey>
        work_result =
            ActorActivityIntentKey::create(
                "oros",
                "work");

    const Result<ActorActivityIntentKey>
        sleep_result =
            ActorActivityIntentKey::create(
                "oros",
                "sleep");

    check(
        state,
        work_result.has_value() &&
            sleep_result.has_value(),
        "Execution-state intent prerequisites construct successfully");

    if (
        !work_result.has_value() ||
        !sleep_result.has_value())
    {
        return 1;
    }

    const ActorActivityIntentKey&
        work =
            work_result.value();

    const ActorActivityIntentKey&
        sleep =
            sleep_result.value();

    const Result<
        ActorScheduleExecutionState>
        gap_state_result =
            ActorScheduleExecutionState::
                create_following(
                    actor_a);

    check(
        state,
        gap_state_result.has_value(),
        "Following state without persistent intent constructs successfully");

    if (!gap_state_result.has_value())
    {
        return 1;
    }

    const ActorScheduleExecutionState&
        gap_state =
            gap_state_result.value();

    check(
        state,
        gap_state.actor() == actor_a,
        "Execution state preserves stable actor EntityId");

    check(
        state,
        !gap_state.
            persistent_intent().
            has_value(),
        "Following state may represent absence of persistent intent");

    check(
        state,
        !gap_state.is_interrupted(),
        "Intent-absent following state is not interrupted");

    const Result<
        ActorScheduleExecutionState>
        following_result =
            ActorScheduleExecutionState::
                create_following(
                    actor_a,
                    work);

    check(
        state,
        following_result.has_value(),
        "Following state with persistent intent constructs successfully");

    if (!following_result.has_value())
    {
        return 1;
    }

    const ActorScheduleExecutionState&
        following =
            following_result.value();

    check(
        state,
        following.actor() == actor_a &&
            following.
                persistent_intent().
                has_value() &&
            following.
                persistent_intent().
                value() ==
            work &&
            !following.is_interrupted(),
        "Following state owns actor identity, persistent intent, and non-interrupted state");

    check(
        state,
        following.
                persistent_intent().
                value().
                intent_namespace() ==
            "oros" &&
            following.
                persistent_intent().
                value().
                intent_name() ==
            "work",
        "Persistent intent retains complete semantic key value");

    const Result<
        ActorScheduleExecutionState>
        interrupted_result =
            ActorScheduleExecutionState::
                create_interrupted(
                    actor_a,
                    work);

    check(
        state,
        interrupted_result.has_value(),
        "Interrupted state with retained intent constructs successfully");

    if (!interrupted_result.has_value())
    {
        return 1;
    }

    const ActorScheduleExecutionState&
        interrupted =
            interrupted_result.value();

    check(
        state,
        interrupted.actor() == actor_a &&
            interrupted.
                persistent_intent().
                has_value() &&
            interrupted.
                persistent_intent().
                value() ==
            work &&
            interrupted.is_interrupted(),
        "Interrupted state structurally retains the persistent intent");

    const Result<
        ActorScheduleExecutionState>
        invalid_following_result =
            ActorScheduleExecutionState::
                create_following(
                    invalid_entity_id);

    check(
        state,
        !invalid_following_result.
                has_value() &&
            invalid_following_result.
                error().code ==
            ErrorCode::invalid_argument,
        "Intent-absent following state rejects invalid actor identity");

    const Result<
        ActorScheduleExecutionState>
        invalid_following_intent_result =
            ActorScheduleExecutionState::
                create_following(
                    invalid_entity_id,
                    work);

    check(
        state,
        !invalid_following_intent_result.
                has_value() &&
            invalid_following_intent_result.
                error().code ==
            ErrorCode::invalid_argument,
        "Intent-bearing following state rejects invalid actor identity");

    const Result<
        ActorScheduleExecutionState>
        invalid_interrupted_result =
            ActorScheduleExecutionState::
                create_interrupted(
                    invalid_entity_id,
                    work);

    check(
        state,
        !invalid_interrupted_result.
                has_value() &&
            invalid_interrupted_result.
                error().code ==
            ErrorCode::invalid_argument,
        "Interrupted state rejects invalid actor identity");

    const Result<
        ActorScheduleExecutionState>
        same_following_result =
            ActorScheduleExecutionState::
                create_following(
                    actor_a,
                    work);

    const Result<
        ActorScheduleExecutionState>
        different_actor_result =
            ActorScheduleExecutionState::
                create_following(
                    actor_b,
                    work);

    const Result<
        ActorScheduleExecutionState>
        different_intent_result =
            ActorScheduleExecutionState::
                create_following(
                    actor_a,
                    sleep);

    if (
        !same_following_result.has_value() ||
        !different_actor_result.has_value() ||
        !different_intent_result.has_value())
    {
        return 1;
    }

    check(
        state,
        following ==
            same_following_result.value(),
        "Equal actor intent and interruption state produce equal execution state");

    check(
        state,
        following !=
            different_actor_result.value(),
        "Different actor identity produces different execution state");

    check(
        state,
        following !=
            different_intent_result.value(),
        "Different persistent intent produces different execution state");

    check(
        state,
        following != interrupted,
        "Interruption flag participates in complete execution-state meaning");

    Result<ActorScheduleExecutionState>
        move_construct_source_result =
            ActorScheduleExecutionState::
                create_following(
                    actor_a,
                    work);

    if (!move_construct_source_result.has_value())
    {
        return 1;
    }

    ActorScheduleExecutionState
        moved =
            std::move(
                move_construct_source_result.
                    value());

    check(
        state,
        moved == following,
        "Execution-state move construction preserves complete deterministic meaning");

    Result<ActorScheduleExecutionState>
        move_assignment_source_result =
            ActorScheduleExecutionState::
                create_following(
                    actor_a,
                    work);

    Result<ActorScheduleExecutionState>
        move_assignment_destination_result =
            ActorScheduleExecutionState::
                create_interrupted(
                    actor_a,
                    work);

    if (
        !move_assignment_source_result.
            has_value() ||
        !move_assignment_destination_result.
            has_value())
    {
        return 1;
    }

    ActorScheduleExecutionState
        move_assigned =
            std::move(
                move_assignment_destination_result.
                    value());

    move_assigned =
        std::move(
            move_assignment_source_result.
                value());

    check(
        state,
        move_assigned == following,
        "Execution-state move assignment preserves complete deterministic meaning");

    const auto world_time =
        [](
            const std::uint64_t
                microseconds)
            noexcept
        {
            return
                WorldTime::
                    from_microseconds_since_epoch(
                        microseconds);
        };

    const Result<ActorScheduledActivity>
        sync_work_activity_result =
            make_scheduled_activity(
                "work",
                100ULL,
                200ULL);

    const Result<ActorScheduledActivity>
        sync_sleep_activity_result =
            make_scheduled_activity(
                "sleep",
                200ULL,
                300ULL);

    const Result<ActorScheduledActivity>
        sync_late_activity_result =
            make_scheduled_activity(
                "work",
                400ULL,
                500ULL);

    check(
        state,
        sync_work_activity_result.has_value() &&
            sync_sleep_activity_result.has_value() &&
            sync_late_activity_result.has_value(),
        "Synchronization schedule prerequisites construct successfully");

    if (
        !sync_work_activity_result.has_value() ||
        !sync_sleep_activity_result.has_value() ||
        !sync_late_activity_result.has_value())
    {
        return 1;
    }

    ActorSchedule sync_schedule{};

    Status sync_insert_status =
        sync_schedule.insert(
            sync_late_activity_result.value());

    if (!sync_insert_status.has_value())
    {
        return 1;
    }

    sync_insert_status =
        sync_schedule.insert(
            sync_sleep_activity_result.value());

    if (!sync_insert_status.has_value())
    {
        return 1;
    }

    sync_insert_status =
        sync_schedule.insert(
            sync_work_activity_result.value());

    check(
        state,
        sync_insert_status.has_value() &&
            sync_schedule.size() == 3U,
        "Synchronization schedule accepts non-authoritative reverse insertion history");

    if (!sync_insert_status.has_value())
    {
        return 1;
    }

    ActorSchedule empty_sync_schedule{};

    Result<ActorScheduleExecutionState>
        sync_empty_state_result =
            ActorScheduleExecutionState::
                create_following(
                    actor_a);

    if (!sync_empty_state_result.has_value())
    {
        return 1;
    }

    ActorScheduleExecutionState&
        sync_empty_state =
            sync_empty_state_result.value();

    const Status sync_empty_status =
        sync_empty_state.
            synchronize_following_intent_from_schedule(
                empty_sync_schedule,
                world_time(150ULL));

    check(
        state,
        sync_empty_status.has_value() &&
            !sync_empty_state.
                persistent_intent().
                has_value() &&
            sync_empty_state.actor() ==
                actor_a &&
            !sync_empty_state.is_interrupted(),
        "Empty following state synchronized against empty schedule remains without persistent intent");

    Result<ActorScheduleExecutionState>
        sync_gap_state_result =
            ActorScheduleExecutionState::
                create_following(
                    actor_a,
                    sleep);

    if (!sync_gap_state_result.has_value())
    {
        return 1;
    }

    ActorScheduleExecutionState&
        sync_gap_state =
            sync_gap_state_result.value();

    const Status sync_gap_status =
        sync_gap_state.
            synchronize_following_intent_from_schedule(
                sync_schedule,
                world_time(350ULL));

    check(
        state,
        sync_gap_status.has_value() &&
            !sync_gap_state.
                persistent_intent().
                has_value() &&
            sync_gap_state.actor() ==
                actor_a &&
            !sync_gap_state.is_interrupted(),
        "Stale following intent is cleared inside authored schedule gap");

    Result<ActorScheduleExecutionState>
        sync_adopt_state_result =
            ActorScheduleExecutionState::
                create_following(
                    actor_a);

    if (!sync_adopt_state_result.has_value())
    {
        return 1;
    }

    ActorScheduleExecutionState&
        sync_adopt_state =
            sync_adopt_state_result.value();

    const Status sync_adopt_status =
        sync_adopt_state.
            synchronize_following_intent_from_schedule(
                sync_schedule,
                world_time(150ULL));

    check(
        state,
        sync_adopt_status.has_value() &&
            sync_adopt_state.
                persistent_intent().
                has_value() &&
            sync_adopt_state.
                persistent_intent().
                value() ==
            work &&
            sync_adopt_state.actor() ==
                actor_a &&
            !sync_adopt_state.is_interrupted(),
        "Following state without intent adopts authored activity intent");

    Result<ActorScheduleExecutionState>
        sync_replace_state_result =
            ActorScheduleExecutionState::
                create_following(
                    actor_a,
                    sleep);

    if (!sync_replace_state_result.has_value())
    {
        return 1;
    }

    ActorScheduleExecutionState&
        sync_replace_state =
            sync_replace_state_result.value();

    const Status sync_replace_status =
        sync_replace_state.
            synchronize_following_intent_from_schedule(
                sync_schedule,
                world_time(150ULL));

    check(
        state,
        sync_replace_status.has_value() &&
            sync_replace_state.
                persistent_intent().
                has_value() &&
            sync_replace_state.
                persistent_intent().
                value() ==
            work &&
            sync_replace_state.actor() ==
                actor_a &&
            !sync_replace_state.is_interrupted(),
        "Different following intent is replaced by authored activity intent");

    Result<ActorScheduleExecutionState>
        sync_same_state_result =
            ActorScheduleExecutionState::
                create_following(
                    actor_a,
                    work);

    if (!sync_same_state_result.has_value())
    {
        return 1;
    }

    ActorScheduleExecutionState&
        sync_same_state =
            sync_same_state_result.value();

    const Status sync_same_status_first =
        sync_same_state.
            synchronize_following_intent_from_schedule(
                sync_schedule,
                world_time(150ULL));

    const Status sync_same_status_second =
        sync_same_state.
            synchronize_following_intent_from_schedule(
                sync_schedule,
                world_time(150ULL));

    check(
        state,
        sync_same_status_first.has_value() &&
            sync_same_status_second.has_value() &&
            sync_same_state.
                persistent_intent().
                has_value() &&
            sync_same_state.
                persistent_intent().
                value() ==
            work &&
            !sync_same_state.is_interrupted(),
        "Same authored intent synchronization is an idempotent successful no-op");

    Result<ActorScheduleExecutionState>
        sync_boundary_state_result =
            ActorScheduleExecutionState::
                create_following(
                    actor_a,
                    work);

    if (!sync_boundary_state_result.has_value())
    {
        return 1;
    }

    ActorScheduleExecutionState&
        sync_boundary_state =
            sync_boundary_state_result.value();

    const Status sync_boundary_status =
        sync_boundary_state.
            synchronize_following_intent_from_schedule(
                sync_schedule,
                world_time(200ULL));

    check(
        state,
        sync_boundary_status.has_value() &&
            sync_boundary_state.
                persistent_intent().
                has_value() &&
            sync_boundary_state.
                persistent_intent().
                value() ==
            sleep,
        "Exact adjacent boundary adopts the next authored activity intent");

    Result<ActorScheduleExecutionState>
        sync_before_state_result =
            ActorScheduleExecutionState::
                create_following(
                    actor_a,
                    sleep);

    if (!sync_before_state_result.has_value())
    {
        return 1;
    }

    ActorScheduleExecutionState&
        sync_before_state =
            sync_before_state_result.value();

    const Status sync_before_status =
        sync_before_state.
            synchronize_following_intent_from_schedule(
                sync_schedule,
                world_time(50ULL));

    check(
        state,
        sync_before_status.has_value() &&
            !sync_before_state.
                persistent_intent().
                has_value(),
        "Before-first authored time clears following persistent intent");

    Result<ActorScheduleExecutionState>
        sync_after_state_result =
            ActorScheduleExecutionState::
                create_following(
                    actor_a,
                    work);

    if (!sync_after_state_result.has_value())
    {
        return 1;
    }

    ActorScheduleExecutionState&
        sync_after_state =
            sync_after_state_result.value();

    const Status sync_after_status =
        sync_after_state.
            synchronize_following_intent_from_schedule(
                sync_schedule,
                world_time(600ULL));

    check(
        state,
        sync_after_status.has_value() &&
            !sync_after_state.
                persistent_intent().
                has_value(),
        "After-final authored time clears following persistent intent");

    Result<ActorScheduleExecutionState>
        sync_interrupted_activity_result =
            ActorScheduleExecutionState::
                create_interrupted(
                    actor_a,
                    work);

    if (!sync_interrupted_activity_result.has_value())
    {
        return 1;
    }

    ActorScheduleExecutionState&
        sync_interrupted_activity =
            sync_interrupted_activity_result.value();

    const Status
        sync_interrupted_activity_status =
            sync_interrupted_activity.
                synchronize_following_intent_from_schedule(
                    sync_schedule,
                    world_time(250ULL));

    check(
        state,
        !sync_interrupted_activity_status.
                has_value() &&
            sync_interrupted_activity_status.
                error().code ==
            ErrorCode::invalid_state,
        "Interrupted state rejects synchronization to a different authored intent");

    check(
        state,
        sync_interrupted_activity.actor() ==
                actor_a &&
            sync_interrupted_activity.
                persistent_intent().
                has_value() &&
            sync_interrupted_activity.
                persistent_intent().
                value() ==
                work &&
            sync_interrupted_activity.
                is_interrupted(),
        "Interrupted different-intent rejection preserves retained intent exactly");

    Result<ActorScheduleExecutionState>
        sync_interrupted_gap_result =
            ActorScheduleExecutionState::
                create_interrupted(
                    actor_a,
                    work);

    if (!sync_interrupted_gap_result.has_value())
    {
        return 1;
    }

    ActorScheduleExecutionState&
        sync_interrupted_gap =
            sync_interrupted_gap_result.value();

    const Status sync_interrupted_gap_status =
        sync_interrupted_gap.
            synchronize_following_intent_from_schedule(
                sync_schedule,
                world_time(350ULL));

    check(
        state,
        !sync_interrupted_gap_status.
                has_value() &&
            sync_interrupted_gap_status.
                error().code ==
            ErrorCode::invalid_state,
        "Interrupted state rejects synchronization during authored gap");

    check(
        state,
        sync_interrupted_gap.actor() ==
                actor_a &&
            sync_interrupted_gap.
                persistent_intent().
                has_value() &&
            sync_interrupted_gap.
                persistent_intent().
                value() ==
                work &&
            sync_interrupted_gap.
                is_interrupted(),
        "Interrupted gap rejection preserves retained intent exactly");

    check(
        state,
        sync_adopt_state.actor() ==
                actor_a &&
            !sync_adopt_state.is_interrupted() &&
            sync_replace_state.actor() ==
                actor_a &&
            !sync_replace_state.is_interrupted() &&
            sync_boundary_state.actor() ==
                actor_a &&
            !sync_boundary_state.is_interrupted(),
        "Synchronization preserves actor EntityId and interruption state");

    ActorSchedule sync_forward_schedule{};

    Status sync_forward_insert_status =
        sync_forward_schedule.insert(
            sync_work_activity_result.value());

    if (!sync_forward_insert_status.has_value())
    {
        return 1;
    }

    sync_forward_insert_status =
        sync_forward_schedule.insert(
            sync_sleep_activity_result.value());

    if (!sync_forward_insert_status.has_value())
    {
        return 1;
    }

    sync_forward_insert_status =
        sync_forward_schedule.insert(
            sync_late_activity_result.value());

    if (!sync_forward_insert_status.has_value())
    {
        return 1;
    }

    Result<ActorScheduleExecutionState>
        sync_history_a_result =
            ActorScheduleExecutionState::
                create_following(
                    actor_a);

    Result<ActorScheduleExecutionState>
        sync_history_b_result =
            ActorScheduleExecutionState::
                create_following(
                    actor_a);

    if (
        !sync_history_a_result.has_value() ||
        !sync_history_b_result.has_value())
    {
        return 1;
    }

    ActorScheduleExecutionState&
        sync_history_a =
            sync_history_a_result.value();

    ActorScheduleExecutionState&
        sync_history_b =
            sync_history_b_result.value();

    const Status sync_history_a_status =
        sync_history_a.
            synchronize_following_intent_from_schedule(
                sync_schedule,
                world_time(200ULL));

    const Status sync_history_b_status =
        sync_history_b.
            synchronize_following_intent_from_schedule(
                sync_forward_schedule,
                world_time(200ULL));

    check(
        state,
        sync_history_a_status.has_value() &&
            sync_history_b_status.has_value() &&
            sync_history_a ==
                sync_history_b &&
            sync_history_a.
                persistent_intent().
                has_value() &&
            sync_history_a.
                persistent_intent().
                value() ==
                sleep,
        "Equivalent schedule insertion histories produce equal synchronized state");
    Result<ActorScheduleExecutionState>
        begin_success_state_result =
            ActorScheduleExecutionState::
                create_following(
                    actor_a,
                    work);

    if (!begin_success_state_result.has_value())
    {
        return 1;
    }

    ActorScheduleExecutionState&
        begin_success_state =
            begin_success_state_result.value();

    const Status begin_success_status =
        begin_success_state.
            begin_interruption();

    check(
        state,
        begin_success_status.has_value(),
        "Following state with persistent intent begins interruption successfully");

    check(
        state,
        begin_success_state.actor() ==
                actor_a &&
            begin_success_state.
                persistent_intent().
                has_value() &&
            begin_success_state.
                persistent_intent().
                value() ==
                work &&
            begin_success_state.
                is_interrupted(),
        "Successful begin interruption preserves actor and persistent intent while changing interruption state");

    const Status begin_second_status =
        begin_success_state.
            begin_interruption();

    check(
        state,
        !begin_second_status.has_value() &&
            begin_second_status.
                error().code ==
            ErrorCode::invalid_state,
        "Second begin interruption is rejected with invalid_state");

    check(
        state,
        begin_success_state.actor() ==
                actor_a &&
            begin_success_state.
                persistent_intent().
                has_value() &&
            begin_success_state.
                persistent_intent().
                value() ==
                work &&
            begin_success_state.
                is_interrupted(),
        "Second begin interruption failure leaves complete state unchanged");

    Result<ActorScheduleExecutionState>
        begin_no_intent_state_result =
            ActorScheduleExecutionState::
                create_following(
                    actor_a);

    if (!begin_no_intent_state_result.has_value())
    {
        return 1;
    }

    ActorScheduleExecutionState&
        begin_no_intent_state =
            begin_no_intent_state_result.value();

    const Status begin_no_intent_status =
        begin_no_intent_state.
            begin_interruption();

    check(
        state,
        !begin_no_intent_status.has_value() &&
            begin_no_intent_status.
                error().code ==
            ErrorCode::invalid_state,
        "Following state without persistent intent rejects begin interruption");

    check(
        state,
        begin_no_intent_state.actor() ==
                actor_a &&
            !begin_no_intent_state.
                persistent_intent().
                has_value() &&
            !begin_no_intent_state.
                is_interrupted(),
        "No-intent begin interruption failure leaves complete state unchanged");

    Result<ActorScheduleExecutionState>
        begin_already_interrupted_result =
            ActorScheduleExecutionState::
                create_interrupted(
                    actor_a,
                    work);

    if (!begin_already_interrupted_result.has_value())
    {
        return 1;
    }

    ActorScheduleExecutionState&
        begin_already_interrupted =
            begin_already_interrupted_result.
                value();

    const Status
        begin_already_interrupted_status =
            begin_already_interrupted.
                begin_interruption();

    check(
        state,
        !begin_already_interrupted_status.
                has_value() &&
            begin_already_interrupted_status.
                error().code ==
            ErrorCode::invalid_state,
        "Already-interrupted state rejects begin interruption");

    check(
        state,
        begin_already_interrupted.actor() ==
                actor_a &&
            begin_already_interrupted.
                persistent_intent().
                has_value() &&
            begin_already_interrupted.
                persistent_intent().
                value() ==
                work &&
            begin_already_interrupted.
                is_interrupted(),
        "Already-interrupted begin failure preserves complete state exactly");
    Result<ActorScheduleExecutionState>
        rejoin_following_result =
            ActorScheduleExecutionState::
                create_following(
                    actor_a,
                    work);

    if (!rejoin_following_result.has_value())
    {
        return 1;
    }

    ActorScheduleExecutionState&
        rejoin_following =
            rejoin_following_result.value();

    const Status rejoin_following_status =
        rejoin_following.
            end_interruption_and_rejoin_schedule(
                sync_schedule,
                world_time(250ULL));

    check(
        state,
        !rejoin_following_status.has_value() &&
            rejoin_following_status.
                error().code ==
            ErrorCode::invalid_state,
        "Following state rejects interruption-end schedule rejoin");

    check(
        state,
        rejoin_following.actor() ==
                actor_a &&
            rejoin_following.
                persistent_intent().
                has_value() &&
            rejoin_following.
                persistent_intent().
                value() ==
                work &&
            !rejoin_following.is_interrupted(),
        "Following-state rejoin rejection leaves complete state unchanged");

    Result<ActorScheduleExecutionState>
        rejoin_same_result =
            ActorScheduleExecutionState::
                create_interrupted(
                    actor_a,
                    work);

    if (!rejoin_same_result.has_value())
    {
        return 1;
    }

    ActorScheduleExecutionState&
        rejoin_same =
            rejoin_same_result.value();

    const Status rejoin_same_status =
        rejoin_same.
            end_interruption_and_rejoin_schedule(
                sync_schedule,
                world_time(150ULL));

    check(
        state,
        rejoin_same_status.has_value() &&
            rejoin_same.actor() ==
                actor_a &&
            rejoin_same.
                persistent_intent().
                has_value() &&
            rejoin_same.
                persistent_intent().
                value() ==
                work &&
            !rejoin_same.is_interrupted(),
        "Interrupted state resumes matching retained authored intent");

    Result<ActorScheduleExecutionState>
        rejoin_different_result =
            ActorScheduleExecutionState::
                create_interrupted(
                    actor_a,
                    work);

    if (!rejoin_different_result.has_value())
    {
        return 1;
    }

    ActorScheduleExecutionState&
        rejoin_different =
            rejoin_different_result.value();

    const Status rejoin_different_status =
        rejoin_different.
            end_interruption_and_rejoin_schedule(
                sync_schedule,
                world_time(250ULL));

    check(
        state,
        rejoin_different_status.has_value() &&
            rejoin_different.actor() ==
                actor_a &&
            rejoin_different.
                persistent_intent().
                has_value() &&
            rejoin_different.
                persistent_intent().
                value() ==
                sleep &&
            !rejoin_different.is_interrupted(),
        "Interrupted state rejoins a different current authored intent");

    Result<ActorScheduleExecutionState>
        rejoin_gap_result =
            ActorScheduleExecutionState::
                create_interrupted(
                    actor_a,
                    work);

    if (!rejoin_gap_result.has_value())
    {
        return 1;
    }

    ActorScheduleExecutionState&
        rejoin_gap =
            rejoin_gap_result.value();

    const Status rejoin_gap_status =
        rejoin_gap.
            end_interruption_and_rejoin_schedule(
                sync_schedule,
                world_time(350ULL));

    check(
        state,
        rejoin_gap_status.has_value() &&
            rejoin_gap.actor() ==
                actor_a &&
            !rejoin_gap.
                persistent_intent().
                has_value() &&
            !rejoin_gap.is_interrupted(),
        "Interrupted state rejoins authored gap as following state without intent");

    Result<ActorScheduleExecutionState>
        rejoin_before_result =
            ActorScheduleExecutionState::
                create_interrupted(
                    actor_a,
                    work);

    if (!rejoin_before_result.has_value())
    {
        return 1;
    }

    ActorScheduleExecutionState&
        rejoin_before =
            rejoin_before_result.value();

    const Status rejoin_before_status =
        rejoin_before.
            end_interruption_and_rejoin_schedule(
                sync_schedule,
                world_time(50ULL));

    check(
        state,
        rejoin_before_status.has_value() &&
            rejoin_before.actor() ==
                actor_a &&
            !rejoin_before.
                persistent_intent().
                has_value() &&
            !rejoin_before.is_interrupted(),
        "Interrupted state before first authored window rejoins without intent");

    Result<ActorScheduleExecutionState>
        rejoin_after_result =
            ActorScheduleExecutionState::
                create_interrupted(
                    actor_a,
                    work);

    if (!rejoin_after_result.has_value())
    {
        return 1;
    }

    ActorScheduleExecutionState&
        rejoin_after =
            rejoin_after_result.value();

    const Status rejoin_after_status =
        rejoin_after.
            end_interruption_and_rejoin_schedule(
                sync_schedule,
                world_time(600ULL));

    check(
        state,
        rejoin_after_status.has_value() &&
            rejoin_after.actor() ==
                actor_a &&
            !rejoin_after.
                persistent_intent().
                has_value() &&
            !rejoin_after.is_interrupted(),
        "Interrupted state after final authored window rejoins without intent");

    Result<ActorScheduleExecutionState>
        rejoin_boundary_result =
            ActorScheduleExecutionState::
                create_interrupted(
                    actor_a,
                    work);

    if (!rejoin_boundary_result.has_value())
    {
        return 1;
    }

    ActorScheduleExecutionState&
        rejoin_boundary =
            rejoin_boundary_result.value();

    const Status rejoin_boundary_status =
        rejoin_boundary.
            end_interruption_and_rejoin_schedule(
                sync_schedule,
                world_time(200ULL));

    check(
        state,
        rejoin_boundary_status.has_value() &&
            rejoin_boundary.actor() ==
                actor_a &&
            rejoin_boundary.
                persistent_intent().
                has_value() &&
            rejoin_boundary.
                persistent_intent().
                value() ==
                sleep &&
            !rejoin_boundary.is_interrupted(),
        "Interruption rejoin at exact adjacent boundary adopts next authored intent");

    Result<ActorScheduleExecutionState>
        rejoin_history_a_result =
            ActorScheduleExecutionState::
                create_interrupted(
                    actor_a,
                    work);

    Result<ActorScheduleExecutionState>
        rejoin_history_b_result =
            ActorScheduleExecutionState::
                create_interrupted(
                    actor_a,
                    work);

    if (
        !rejoin_history_a_result.has_value() ||
        !rejoin_history_b_result.has_value())
    {
        return 1;
    }

    ActorScheduleExecutionState&
        rejoin_history_a =
            rejoin_history_a_result.value();

    ActorScheduleExecutionState&
        rejoin_history_b =
            rejoin_history_b_result.value();

    const Status rejoin_history_a_status =
        rejoin_history_a.
            end_interruption_and_rejoin_schedule(
                sync_schedule,
                world_time(200ULL));

    const Status rejoin_history_b_status =
        rejoin_history_b.
            end_interruption_and_rejoin_schedule(
                sync_forward_schedule,
                world_time(200ULL));

    check(
        state,
        rejoin_history_a_status.has_value() &&
            rejoin_history_b_status.has_value() &&
            rejoin_history_a ==
                rejoin_history_b &&
            rejoin_history_a.
                persistent_intent().
                has_value() &&
            rejoin_history_a.
                persistent_intent().
                value() ==
                sleep &&
            !rejoin_history_a.is_interrupted(),
        "Equivalent schedule insertion histories produce equal interruption-rejoin state");
    std::cout
        << state.checks
        << " checks, "
        << state.failures
        << " failures\n";

    return state.failures == 0
        ? 0
        : 1;
}