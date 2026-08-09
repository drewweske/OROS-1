#include "oros/ai/actor_faction_membership.hpp"

#include <new>
#include <utility>

namespace oros::ai
{
    foundation::Result<
        ActorFactionMembership>
    ActorFactionMembership::create(
        const world::EntityId actor,
        const FactionKey& faction)
    {
        if (!actor.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor faction membership "
                "requires a valid actor entity "
                "identity.");
        }

        try
        {
            return ActorFactionMembership{
                actor,
                faction
            };
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Actor faction membership could "
                "not allocate its owned faction "
                "identity.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "Actor faction membership "
                "construction failed.");
        }
    }

    world::EntityId
    ActorFactionMembership::
        actor()
        const noexcept
    {
        return actor_;
    }

    const FactionKey&
    ActorFactionMembership::
        faction()
        const noexcept
    {
        return faction_;
    }

    ActorFactionMembership::
        ActorFactionMembership(
            const world::EntityId actor,
            FactionKey faction)
            noexcept
        : actor_{actor},
          faction_{
              std::move(faction)
          }
    {
    }
}
