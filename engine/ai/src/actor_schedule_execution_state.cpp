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