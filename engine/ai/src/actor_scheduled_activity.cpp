#include "oros/ai/actor_scheduled_activity.hpp"

#include <utility>

namespace oros::ai
{
    ActorScheduledActivity::
        ActorScheduledActivity(
            ActorActivityIntentKey intent,
            ActorScheduleWindow window)
            noexcept
        : intent_{
              std::move(intent)
          },
          window_{
              std::move(window)
          }
    {
    }

    const ActorActivityIntentKey&
    ActorScheduledActivity::intent()
        const noexcept
    {
        return intent_;
    }

    const ActorScheduleWindow&
    ActorScheduledActivity::window()
        const noexcept
    {
        return window_;
    }
}