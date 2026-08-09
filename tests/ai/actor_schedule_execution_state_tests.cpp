#include "oros/ai/actor_schedule_execution_state.hpp"

#include "oros/ai/actor_activity_intent_key.hpp"
#include "oros/foundation/result.hpp"
#include "oros/world/entity_id.hpp"

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

    std::cout
        << state.checks
        << " checks, "
        << state.failures
        << " failures\n";

    return state.failures == 0
        ? 0
        : 1;
}