#pragma once

#include "oros/ai/actor_activity_intent_key.hpp"
#include "oros/ai/actor_schedule_window.hpp"

namespace oros::ai
{
    class ActorScheduledActivity final
    {
    public:
        ActorScheduledActivity(
            ActorActivityIntentKey intent,
            ActorScheduleWindow window)
            noexcept;

        ActorScheduledActivity(
            const ActorScheduledActivity&) =
                default;

        ActorScheduledActivity&
        operator=(
            const ActorScheduledActivity&) =
                default;

        ActorScheduledActivity(
            ActorScheduledActivity&&)
            noexcept = default;

        ActorScheduledActivity&
        operator=(
            ActorScheduledActivity&&)
            noexcept = default;

        [[nodiscard]]
        const ActorActivityIntentKey&
        intent() const noexcept;

        [[nodiscard]]
        const ActorScheduleWindow&
        window() const noexcept;

        bool operator==(
            const ActorScheduledActivity&)
            const = default;

    private:
        ActorActivityIntentKey
            intent_;

        ActorScheduleWindow
            window_;
    };
}