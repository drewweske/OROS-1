#include "oros/ai/actor_faction_reputation_set.hpp"

#include <algorithm>
#include <cstddef>
#include <new>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace oros::ai
{
    namespace
    {
        [[nodiscard]]
        bool
        faction_key_less(
            const FactionKey& left,
            const FactionKey& right)
            noexcept
        {
            const std::string_view
                left_namespace =
                    left.faction_namespace();

            const std::string_view
                right_namespace =
                    right.faction_namespace();

            if (left_namespace < right_namespace)
            {
                return true;
            }

            if (right_namespace < left_namespace)
            {
                return false;
            }

            return
                left.faction_name() <
                right.faction_name();
        }

        [[nodiscard]]
        bool
        reputation_matches(
            const ActorFactionReputation& reputation,
            const world::EntityId actor,
            const FactionKey& faction)
            noexcept
        {
            return
                reputation.actor() == actor &&
                reputation.faction() == faction;
        }
    }

    foundation::Status
    ActorFactionReputationSet::
        set_reputation(
            const ActorFactionReputation& reputation)
    {
        const std::size_t index =
            lower_bound_index(
                reputation.actor(),
                reputation.faction());

        const bool existing =
            index < reputations_.size() &&
            reputation_matches(
                reputations_[index],
                reputation.actor(),
                reputation.faction());

        if (
            existing &&
            reputations_[index] ==
                reputation)
        {
            return {};
        }

        using difference_type =
            std::vector<
                ActorFactionReputation>::
                    difference_type;

        try
        {
            ActorFactionReputation
                owned_reputation{
                    reputation
                };

            if (existing)
            {
                reputations_[index] =
                    std::move(
                        owned_reputation);

                return {};
            }

            reputations_.insert(
                reputations_.begin() +
                    static_cast<
                        difference_type>(
                            index),
                std::move(
                    owned_reputation));
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Actor faction reputation set "
                "could not allocate storage for "
                "the reputation fact.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "Actor faction reputation set "
                "failed while setting a "
                "reputation fact.");
        }

        return {};
    }

    foundation::Status
    ActorFactionReputationSet::remove(
        const world::EntityId actor,
        const FactionKey& faction)
    {
        if (!actor.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor faction reputation "
                "removal requires a valid actor "
                "entity identity.");
        }

        const std::size_t index =
            lower_bound_index(
                actor,
                faction);

        if (
            index >= reputations_.size() ||
            !reputation_matches(
                reputations_[index],
                actor,
                faction))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    not_found,
                "Actor faction reputation set "
                "does not contain this actor "
                "and faction reputation fact.");
        }

        using difference_type =
            std::vector<
                ActorFactionReputation>::
                    difference_type;

        reputations_.erase(
            reputations_.begin() +
                static_cast<
                    difference_type>(
                        index));

        return {};
    }

    bool
    ActorFactionReputationSet::contains(
        const world::EntityId actor,
        const FactionKey& faction)
        const noexcept
    {
        if (!actor.is_valid())
        {
            return false;
        }

        const std::size_t index =
            lower_bound_index(
                actor,
                faction);

        return
            index < reputations_.size() &&
            reputation_matches(
                reputations_[index],
                actor,
                faction);
    }

    foundation::Result<
        std::int32_t>
    ActorFactionReputationSet::score(
        const world::EntityId actor,
        const FactionKey& faction) const
    {
        if (!actor.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor faction reputation score "
                "query requires a valid actor "
                "entity identity.");
        }

        const std::size_t index =
            lower_bound_index(
                actor,
                faction);

        if (
            index >= reputations_.size() ||
            !reputation_matches(
                reputations_[index],
                actor,
                faction))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    not_found,
                "Actor faction reputation set "
                "does not contain this actor "
                "and faction reputation fact.");
        }

        return reputations_[index].score();
    }

    foundation::Result<
        std::span<
            const ActorFactionReputation>>
    ActorFactionReputationSet::
        reputations_for_actor(
            const world::EntityId actor) const
    {
        if (!actor.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor faction reputation range "
                "requires a valid actor entity "
                "identity.");
        }

        const std::size_t first =
            lower_bound_actor_index(
                actor);

        const std::size_t last =
            upper_bound_actor_index(
                actor);

        return
            std::span<
                const ActorFactionReputation>{
                    reputations_
                }.
                subspan(
                    first,
                    last - first);
    }

    std::size_t
    ActorFactionReputationSet::size()
        const noexcept
    {
        return reputations_.size();
    }

    bool
    ActorFactionReputationSet::empty()
        const noexcept
    {
        return reputations_.empty();
    }

    std::span<
        const ActorFactionReputation>
    ActorFactionReputationSet::
        reputations_in_canonical_order()
        const noexcept
    {
        return
            std::span<
                const ActorFactionReputation>{
                    reputations_
                };
    }

    std::size_t
    ActorFactionReputationSet::
        lower_bound_index(
            const world::EntityId actor,
            const FactionKey& faction)
        const noexcept
    {
        const auto iterator =
            std::lower_bound(
                reputations_.begin(),
                reputations_.end(),
                faction,
                [actor](
                    const ActorFactionReputation&
                        reputation,
                    const FactionKey& candidate)
                    noexcept
                {
                    if (
                        reputation.actor() <
                        actor)
                    {
                        return true;
                    }

                    if (
                        actor <
                        reputation.actor())
                    {
                        return false;
                    }

                    return faction_key_less(
                        reputation.faction(),
                        candidate);
                });

        return static_cast<std::size_t>(
            iterator -
            reputations_.begin());
    }

    std::size_t
    ActorFactionReputationSet::
        lower_bound_actor_index(
            const world::EntityId actor)
        const noexcept
    {
        const auto iterator =
            std::lower_bound(
                reputations_.begin(),
                reputations_.end(),
                actor,
                [](
                    const ActorFactionReputation&
                        reputation,
                    const world::EntityId candidate)
                    noexcept
                {
                    return
                        reputation.actor() <
                        candidate;
                });

        return static_cast<std::size_t>(
            iterator -
            reputations_.begin());
    }

    std::size_t
    ActorFactionReputationSet::
        upper_bound_actor_index(
            const world::EntityId actor)
        const noexcept
    {
        const auto iterator =
            std::upper_bound(
                reputations_.begin(),
                reputations_.end(),
                actor,
                [](
                    const world::EntityId candidate,
                    const ActorFactionReputation&
                        reputation)
                    noexcept
                {
                    return
                        candidate <
                        reputation.actor();
                });

        return static_cast<std::size_t>(
            iterator -
            reputations_.begin());
    }
}
