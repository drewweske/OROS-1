#pragma once

#include "oros/ai/actor_activity_intent_key.hpp"
#include "oros/ai/actor_schedule.hpp"
#include "oros/foundation/result.hpp"
#include "oros/world/entity_id.hpp"
#include "oros/world/world_time.hpp"

#include <optional>

namespace oros::ai
{
    class ActorScheduleExecutionState final
    {
    public:
        [[nodiscard]]
        static foundation::Result<
            ActorScheduleExecutionState>
        create_following(
            world::EntityId actor);

        [[nodiscard]]
        static foundation::Result<
            ActorScheduleExecutionState>
        create_following(
            world::EntityId actor,
            const ActorActivityIntentKey&
                persistent_intent);

        [[nodiscard]]
        static foundation::Result<
            ActorScheduleExecutionState>
        create_interrupted(
            world::EntityId actor,
            const ActorActivityIntentKey&
                persistent_intent);

        ActorScheduleExecutionState(
            const ActorScheduleExecutionState&) =
                delete;

        ActorScheduleExecutionState&
        operator=(
            const ActorScheduleExecutionState&) =
                delete;

        ActorScheduleExecutionState(
            ActorScheduleExecutionState&&)
            noexcept = default;

        ActorScheduleExecutionState&
        operator=(
            ActorScheduleExecutionState&&)
            noexcept = default;

        [[nodiscard]]
        foundation::Status
        begin_interruption();

        [[nodiscard]]
        foundation::Status
        end_interruption_and_rejoin_schedule(
            const ActorSchedule& schedule,
            world::WorldTime time);

        [[nodiscard]]
        foundation::Status
        synchronize_following_intent_from_schedule(
            const ActorSchedule& schedule,
            world::WorldTime time);

        [[nodiscard]]
        world::EntityId
        actor() const noexcept;

        [[nodiscard]]
        const std::optional<
            ActorActivityIntentKey>&
        persistent_intent()
            const noexcept;

        [[nodiscard]]
        bool
        is_interrupted() const noexcept;

        bool operator==(
            const ActorScheduleExecutionState&)
            const = default;

    private:
        ActorScheduleExecutionState(
            world::EntityId actor,
            std::optional<
                ActorActivityIntentKey>
                    persistent_intent,
            bool interrupted)
            noexcept;

        world::EntityId actor_{};

        std::optional<
            ActorActivityIntentKey>
            persistent_intent_{};

        bool interrupted_{};
    };
}