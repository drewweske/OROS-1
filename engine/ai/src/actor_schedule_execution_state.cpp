#include "oros/ai/actor_schedule_execution_state.hpp"

#include <new>
#include <optional>
#include <utility>

namespace oros::ai
{
    foundation::Result<
        ActorScheduleExecutionState>
    ActorScheduleExecutionState::
        create_following(
            const world::EntityId actor)
    {
        if (!actor.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor schedule execution state "
                "requires a valid actor EntityId.");
        }

        return ActorScheduleExecutionState{
            actor,
            std::nullopt,
            false
        };
    }

    foundation::Result<
        ActorScheduleExecutionState>
    ActorScheduleExecutionState::
        create_following(
            const world::EntityId actor,
            const ActorActivityIntentKey&
                persistent_intent)
    {
        if (!actor.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor schedule execution state "
                "requires a valid actor EntityId.");
        }

        try
        {
            return ActorScheduleExecutionState{
                actor,
                std::optional<
                    ActorActivityIntentKey>{
                        persistent_intent
                    },
                false
            };
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Actor schedule execution state "
                "could not allocate its owned "
                "persistent intent.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "Actor schedule execution state "
                "persistent-intent construction "
                "failed.");
        }
    }

    foundation::Result<
        ActorScheduleExecutionState>
    ActorScheduleExecutionState::
        create_interrupted(
            const world::EntityId actor,
            const ActorActivityIntentKey&
                persistent_intent)
    {
        if (!actor.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor schedule execution state "
                "requires a valid actor EntityId.");
        }

        try
        {
            return ActorScheduleExecutionState{
                actor,
                std::optional<
                    ActorActivityIntentKey>{
                        persistent_intent
                    },
                true
            };
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Actor schedule execution state "
                "could not allocate its owned "
                "persistent intent.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "Actor schedule execution state "
                "persistent-intent construction "
                "failed.");
        }
    }

    foundation::Status
    ActorScheduleExecutionState::
        synchronize_following_intent_from_schedule(
            const ActorSchedule& schedule,
            const world::WorldTime time)
    {
        if (interrupted_)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Interrupted actor schedule execution "
                "state cannot synchronize its "
                "persistent intent from the authored "
                "schedule.");
        }

        const ActorScheduledActivity*
            scheduled_activity =
                schedule.
                    scheduled_activity_at(
                        time);

        if (scheduled_activity == nullptr)
        {
            persistent_intent_.reset();
            return {};
        }

        const ActorActivityIntentKey&
            authored_intent =
                scheduled_activity->intent();

        if (
            persistent_intent_.has_value() &&
            persistent_intent_.value() ==
                authored_intent)
        {
            return {};
        }

        std::optional<
            ActorActivityIntentKey>
            replacement_intent{};

        try
        {
            replacement_intent.emplace(
                authored_intent);
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Actor schedule execution state "
                "could not allocate a synchronized "
                "persistent intent.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "Actor schedule execution state "
                "persistent-intent synchronization "
                "failed.");
        }

        persistent_intent_ =
            std::move(
                replacement_intent);

        return {};
    }
    world::EntityId
    ActorScheduleExecutionState::actor()
        const noexcept
    {
        return actor_;
    }

    const std::optional<
        ActorActivityIntentKey>&
    ActorScheduleExecutionState::
        persistent_intent()
        const noexcept
    {
        return persistent_intent_;
    }

    bool
    ActorScheduleExecutionState::
        is_interrupted()
        const noexcept
    {
        return interrupted_;
    }

    ActorScheduleExecutionState::
        ActorScheduleExecutionState(
            const world::EntityId actor,
            std::optional<
                ActorActivityIntentKey>
                    persistent_intent,
            const bool interrupted)
            noexcept
        : actor_{actor},
          persistent_intent_{
              std::move(
                  persistent_intent)
          },
          interrupted_{interrupted}
    {
    }
}