#include "oros/ai/actor_faction_membership_set.hpp"

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
        membership_matches(
            const ActorFactionMembership& membership,
            const world::EntityId actor,
            const FactionKey& faction)
            noexcept
        {
            return
                membership.actor() == actor &&
                membership.faction() == faction;
        }
    }

    foundation::Status
    ActorFactionMembershipSet::insert(
        const ActorFactionMembership& membership)
    {
        const std::size_t index =
            lower_bound_index(
                membership.actor(),
                membership.faction());

        if (
            index < memberships_.size() &&
            memberships_[index] == membership)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Actor faction membership set "
                "already contains this exact "
                "actor and faction pair.");
        }

        using difference_type =
            std::vector<
                ActorFactionMembership>::
                    difference_type;

        try
        {
            ActorFactionMembership
                owned_membership{
                    membership
                };

            memberships_.insert(
                memberships_.begin() +
                    static_cast<
                        difference_type>(
                            index),
                std::move(
                    owned_membership));
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Actor faction membership set "
                "could not allocate storage for "
                "another membership.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "Actor faction membership set "
                "failed while inserting a "
                "membership.");
        }

        return {};
    }

    foundation::Status
    ActorFactionMembershipSet::remove(
        const world::EntityId actor,
        const FactionKey& faction)
    {
        if (!actor.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor faction membership removal "
                "requires a valid actor entity "
                "identity.");
        }

        const std::size_t index =
            lower_bound_index(
                actor,
                faction);

        if (
            index >= memberships_.size() ||
            !membership_matches(
                memberships_[index],
                actor,
                faction))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    not_found,
                "Actor faction membership set "
                "does not contain this exact "
                "actor and faction pair.");
        }

        using difference_type =
            std::vector<
                ActorFactionMembership>::
                    difference_type;

        memberships_.erase(
            memberships_.begin() +
                static_cast<
                    difference_type>(
                        index));

        return {};
    }

    bool
    ActorFactionMembershipSet::contains(
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
            index < memberships_.size() &&
            membership_matches(
                memberships_[index],
                actor,
                faction);
    }

    foundation::Result<
        std::span<
            const ActorFactionMembership>>
    ActorFactionMembershipSet::
        memberships_for_actor(
            const world::EntityId actor) const
    {
        if (!actor.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor faction membership range "
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
                const ActorFactionMembership>{
                    memberships_
                }.
                subspan(
                    first,
                    last - first);
    }

    std::size_t
    ActorFactionMembershipSet::size()
        const noexcept
    {
        return memberships_.size();
    }

    bool
    ActorFactionMembershipSet::empty()
        const noexcept
    {
        return memberships_.empty();
    }

    std::span<
        const ActorFactionMembership>
    ActorFactionMembershipSet::
        memberships_in_canonical_order()
        const noexcept
    {
        return
            std::span<
                const ActorFactionMembership>{
                    memberships_
                };
    }

    std::size_t
    ActorFactionMembershipSet::
        lower_bound_index(
            const world::EntityId actor,
            const FactionKey& faction)
        const noexcept
    {
        const auto iterator =
            std::lower_bound(
                memberships_.begin(),
                memberships_.end(),
                faction,
                [actor](
                    const ActorFactionMembership&
                        membership,
                    const FactionKey& candidate)
                    noexcept
                {
                    if (
                        membership.actor() <
                        actor)
                    {
                        return true;
                    }

                    if (
                        actor <
                        membership.actor())
                    {
                        return false;
                    }

                    return faction_key_less(
                        membership.faction(),
                        candidate);
                });

        return static_cast<std::size_t>(
            iterator -
            memberships_.begin());
    }

    std::size_t
    ActorFactionMembershipSet::
        lower_bound_actor_index(
            const world::EntityId actor)
        const noexcept
    {
        const auto iterator =
            std::lower_bound(
                memberships_.begin(),
                memberships_.end(),
                actor,
                [](
                    const ActorFactionMembership&
                        membership,
                    const world::EntityId candidate)
                    noexcept
                {
                    return
                        membership.actor() <
                        candidate;
                });

        return static_cast<std::size_t>(
            iterator -
            memberships_.begin());
    }

    std::size_t
    ActorFactionMembershipSet::
        upper_bound_actor_index(
            const world::EntityId actor)
        const noexcept
    {
        const auto iterator =
            std::upper_bound(
                memberships_.begin(),
                memberships_.end(),
                actor,
                [](
                    const world::EntityId candidate,
                    const ActorFactionMembership&
                        membership)
                    noexcept
                {
                    return
                        candidate <
                        membership.actor();
                });

        return static_cast<std::size_t>(
            iterator -
            memberships_.begin());
    }
}
