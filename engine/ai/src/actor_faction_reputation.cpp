#include "oros/ai/actor_faction_reputation.hpp"

#include <new>
#include <utility>

namespace oros::ai
{
    foundation::Result<
        ActorFactionReputation>
    ActorFactionReputation::create(
        const world::EntityId actor,
        const FactionKey& faction,
        const std::int32_t score)
    {
        if (!actor.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor faction reputation "
                "requires a valid actor entity "
                "identity.");
        }

        try
        {
            FactionKey
                owned_faction{
                    faction
                };

            return ActorFactionReputation{
                actor,
                std::move(
                    owned_faction),
                score
            };
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Actor faction reputation could "
                "not allocate its owned faction "
                "identity.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "Actor faction reputation "
                "construction failed.");
        }
    }

    world::EntityId
    ActorFactionReputation::
        actor()
        const noexcept
    {
        return actor_;
    }

    const FactionKey&
    ActorFactionReputation::
        faction()
        const noexcept
    {
        return faction_;
    }

    std::int32_t
    ActorFactionReputation::
        score()
        const noexcept
    {
        return score_;
    }

    ActorFactionReputation::
        ActorFactionReputation(
            const world::EntityId actor,
            FactionKey faction,
            const std::int32_t score)
            noexcept
        : actor_{actor},
          faction_{
              std::move(
                  faction)
          },
          score_{score}
    {
    }
}
