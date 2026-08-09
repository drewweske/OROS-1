#pragma once

#include "oros/ai/faction_key.hpp"
#include "oros/foundation/result.hpp"
#include "oros/world/entity_id.hpp"

namespace oros::ai
{
    class ActorFactionMembership final
    {
    public:
        [[nodiscard]]
        static foundation::Result<
            ActorFactionMembership>
        create(
            world::EntityId actor,
            const FactionKey& faction);

        ActorFactionMembership(
            const ActorFactionMembership&) =
                default;

        ActorFactionMembership&
        operator=(
            const ActorFactionMembership&) =
                default;

        ActorFactionMembership(
            ActorFactionMembership&&)
            noexcept = default;

        ActorFactionMembership&
        operator=(
            ActorFactionMembership&&)
            noexcept = default;

        [[nodiscard]]
        world::EntityId
        actor() const noexcept;

        [[nodiscard]]
        const FactionKey&
        faction() const noexcept;

        bool operator==(
            const ActorFactionMembership&)
            const = default;

    private:
        ActorFactionMembership(
            world::EntityId actor,
            FactionKey faction)
            noexcept;

        world::EntityId
            actor_{};

        FactionKey
            faction_;
    };
}
