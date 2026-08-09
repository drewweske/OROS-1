#pragma once

#include "oros/ai/actor_scheduled_activity.hpp"
#include "oros/foundation/result.hpp"
#include "oros/world/world_time.hpp"

#include <cstddef>
#include <span>
#include <vector>

namespace oros::ai
{
    class ActorSchedule final
    {
    public:
        ActorSchedule() = default;

        [[nodiscard]]
        foundation::Status
        insert(
            const ActorScheduledActivity&
                activity);

        [[nodiscard]]
        std::size_t
        size() const noexcept;

        [[nodiscard]]
        bool
        empty() const noexcept;

        [[nodiscard]]
        std::span<
            const ActorScheduledActivity>
        activities_in_time_order()
            const noexcept;

        // Non-owning pointer into schedule storage.
        // A later successful insert may invalidate it.
        [[nodiscard]]
        const ActorScheduledActivity*
        scheduled_activity_at(
            world::WorldTime time)
            const noexcept;

    private:
        [[nodiscard]]
        std::size_t
        lower_bound_index(
            const ActorScheduledActivity&
                activity)
            const noexcept;

        std::vector<
            ActorScheduledActivity>
            activities_{};
    };
}