#pragma once

#include "oros/foundation/result.hpp"
#include "oros/world/world_time.hpp"

namespace oros::ai
{
    class ActorScheduleWindow final
    {
    public:
        [[nodiscard]]
        static foundation::Result<
            ActorScheduleWindow>
        create(
            world::WorldTime start_inclusive,
            world::WorldTime end_exclusive);

        ActorScheduleWindow(
            const ActorScheduleWindow&) =
                default;

        ActorScheduleWindow&
        operator=(
            const ActorScheduleWindow&) =
                default;

        ActorScheduleWindow(
            ActorScheduleWindow&&)
            noexcept = default;

        ActorScheduleWindow&
        operator=(
            ActorScheduleWindow&&)
            noexcept = default;

        [[nodiscard]]
        world::WorldTime
        start_inclusive() const noexcept;

        [[nodiscard]]
        world::WorldTime
        end_exclusive() const noexcept;

        [[nodiscard]]
        bool contains(
            world::WorldTime time)
            const noexcept;

        bool operator==(
            const ActorScheduleWindow&)
            const = default;

    private:
        ActorScheduleWindow(
            world::WorldTime start_inclusive,
            world::WorldTime end_exclusive)
            noexcept;

        world::WorldTime
            start_inclusive_{};

        world::WorldTime
            end_exclusive_{};
    };
}