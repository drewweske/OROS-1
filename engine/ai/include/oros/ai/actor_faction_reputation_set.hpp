#pragma once

#include "oros/ai/actor_faction_reputation.hpp"
#include "oros/foundation/result.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>
#include <vector>

namespace oros::ai
{
    static_assert(
        std::is_nothrow_move_constructible_v<
            ActorFactionReputation>);

    static_assert(
        std::is_nothrow_move_assignable_v<
            ActorFactionReputation>);

    class ActorFactionReputationSet final
    {
    public:
        ActorFactionReputationSet() = default;

        ActorFactionReputationSet(
            const ActorFactionReputationSet&) =
                default;

        ActorFactionReputationSet&
        operator=(
            const ActorFactionReputationSet&) =
                default;

        ActorFactionReputationSet(
            ActorFactionReputationSet&&)
            noexcept = default;

        ActorFactionReputationSet&
        operator=(
            ActorFactionReputationSet&&)
            noexcept = default;

        [[nodiscard]]
        foundation::Status
        set_reputation(
            const ActorFactionReputation& reputation);

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
            std::int32_t>
        score(
            world::EntityId actor,
            const FactionKey& faction) const;

        [[nodiscard]]
        foundation::Result<
            std::span<
                const ActorFactionReputation>>
        reputations_for_actor(
            world::EntityId actor) const;

        [[nodiscard]]
        std::size_t
        size() const noexcept;

        [[nodiscard]]
        bool
        empty() const noexcept;

        [[nodiscard]]
        std::span<
            const ActorFactionReputation>
        reputations_in_canonical_order()
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
            ActorFactionReputation>
            reputations_{};
    };
}
