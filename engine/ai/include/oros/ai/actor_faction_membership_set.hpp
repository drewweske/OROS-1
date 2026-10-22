#pragma once

#include "oros/ai/actor_faction_membership.hpp"
#include "oros/foundation/result.hpp"

#include <cstddef>
#include <span>
#include <type_traits>
#include <vector>

namespace oros::ai
{
    static_assert(
        std::is_nothrow_move_constructible_v<
            ActorFactionMembership>);

    static_assert(
        std::is_nothrow_move_assignable_v<
            ActorFactionMembership>);

    class ActorFactionMembershipSet final
    {
    public:
        ActorFactionMembershipSet() = default;

        ActorFactionMembershipSet(
            const ActorFactionMembershipSet&) =
                default;

        ActorFactionMembershipSet&
        operator=(
            const ActorFactionMembershipSet&) =
                default;

        ActorFactionMembershipSet(
            ActorFactionMembershipSet&&)
            noexcept = default;

        ActorFactionMembershipSet&
        operator=(
            ActorFactionMembershipSet&&)
            noexcept = default;

        [[nodiscard]]
        foundation::Status
        insert(
            const ActorFactionMembership& membership);

        [[nodiscard]]
        foundation::Status
        remove(
            world::EntityId actor,
            const FactionKey& faction);

        [[nodiscard]]
        bool contains(
            world::EntityId actor,
            const FactionKey& faction)
            const noexcept;

        [[nodiscard]]
        foundation::Result<
            std::span<
                const ActorFactionMembership>>
        memberships_for_actor(
            world::EntityId actor) const;

        [[nodiscard]]
        std::size_t
        size() const noexcept;

        [[nodiscard]]
        bool
        empty() const noexcept;

        [[nodiscard]]
        std::span<
            const ActorFactionMembership>
        memberships_in_canonical_order()
            const noexcept;

    private:
        [[nodiscard]]
        std::size_t
        lower_bound_index(
            world::EntityId actor,
            const FactionKey& faction)
            const noexcept;

        [[nodiscard]]
        std::size_t
        lower_bound_actor_index(
            world::EntityId actor)
            const noexcept;

        [[nodiscard]]
        std::size_t
        upper_bound_actor_index(
            world::EntityId actor)
            const noexcept;

        std::vector<
            ActorFactionMembership>
            memberships_{};
    };
}
