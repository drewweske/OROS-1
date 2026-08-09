#include "oros/ai/actor_schedule.hpp"

#include <algorithm>
#include <cstddef>
#include <new>

namespace oros::ai
{
    foundation::Status
    ActorSchedule::insert(
        const ActorScheduledActivity&
            activity)
    {
        const std::size_t index =
            lower_bound_index(
                activity);

        const auto new_start =
            activity.window().
                start_inclusive();

        const auto new_end =
            activity.window().
                end_exclusive();

        if (
            index > 0U &&
            new_start <
                activities_[
                    index - 1U].
                    window().
                    end_exclusive())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Actor schedule activity "
                "overlaps the immediately "
                "preceding activity.");
        }

        if (
            index < activities_.size() &&
            activities_[index].
                    window().
                    start_inclusive() <
                new_end)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Actor schedule activity "
                "overlaps the immediately "
                "following activity.");
        }

        using difference_type =
            std::vector<
                ActorScheduledActivity>::
                    difference_type;

        try
        {
            activities_.insert(
                activities_.begin() +
                    static_cast<
                        difference_type>(
                        index),
                activity);
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Actor schedule could not "
                "allocate storage for another "
                "scheduled activity.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "Actor schedule insertion "
                "failed.");
        }

        return {};
    }

    std::size_t
    ActorSchedule::size()
        const noexcept
    {
        return activities_.size();
    }

    bool
    ActorSchedule::empty()
        const noexcept
    {
        return activities_.empty();
    }

    std::span<
        const ActorScheduledActivity>
    ActorSchedule::
        activities_in_time_order()
        const noexcept
    {
        return activities_;
    }

    std::size_t
    ActorSchedule::lower_bound_index(
        const ActorScheduledActivity&
            activity)
        const noexcept
    {
        const auto iterator =
            std::lower_bound(
                activities_.begin(),
                activities_.end(),
                activity,
                [](
                    const ActorScheduledActivity&
                        existing,
                    const ActorScheduledActivity&
                        candidate)
                    noexcept
                {
                    return
                        existing.window().
                                start_inclusive() <
                        candidate.window().
                                start_inclusive();
                });

        return static_cast<std::size_t>(
            iterator -
            activities_.begin());
    }
}