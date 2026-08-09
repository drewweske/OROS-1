#include "oros/ai/actor_schedule_window.hpp"

namespace oros::ai
{
    foundation::Result<
        ActorScheduleWindow>
    ActorScheduleWindow::create(
        const world::WorldTime start_inclusive,
        const world::WorldTime end_exclusive)
    {
        if (!(start_inclusive < end_exclusive))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor schedule window requires "
                "start_inclusive to precede "
                "end_exclusive.");
        }

        return ActorScheduleWindow{
            start_inclusive,
            end_exclusive
        };
    }

    world::WorldTime
    ActorScheduleWindow::start_inclusive()
        const noexcept
    {
        return start_inclusive_;
    }

    world::WorldTime
    ActorScheduleWindow::end_exclusive()
        const noexcept
    {
        return end_exclusive_;
    }

    bool
    ActorScheduleWindow::contains(
        const world::WorldTime time)
        const noexcept
    {
        return
            start_inclusive_ <= time &&
            time < end_exclusive_;
    }

    ActorScheduleWindow::
        ActorScheduleWindow(
            const world::WorldTime
                start_inclusive,
            const world::WorldTime
                end_exclusive)
            noexcept
        : start_inclusive_{
              start_inclusive
          },
          end_exclusive_{
              end_exclusive
          }
    {
    }
}