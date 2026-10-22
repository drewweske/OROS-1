#pragma once

#include "oros/ai/faction_key.hpp"
#include "oros/foundation/result.hpp"
#include "oros/world/entity_id.hpp"

#include <cstdint>

namespace oros::ai
{
    class ActorFactionReputation final
    {
    public:
        static constexpr std::int32_t
            neutral_score{0};

        [[nodiscard]]
        static foundation::Result<
            ActorFactionReputation>
        create(
            world::EntityId actor,
            const FactionKey& faction,
            std::int32_t score);

        ActorFactionReputation(
            const ActorFactionReputation&) =
                default;

        ActorFactionReputation&
        operator=(
            const ActorFactionReputation&) =
                default;

        ActorFactionReputation(
            ActorFactionReputation&&)
            noexcept = default;

        ActorFactionReputation&
        operator=(
            ActorFactionReputation&&)
            noexcept = default;

        [[nodiscard]]
        world::EntityId
        actor() const noexcept;

        [[nodiscard]]
        const FactionKey&
        faction() const noexcept;

        [[nodiscard]]
        std::int32_t
        score() const noexcept;

        bool operator==(
            const ActorFactionReputation&)
            const = default;

    private:
        ActorFactionReputation(
            world::EntityId actor,
            FactionKey faction,
            std::int32_t score)
            noexcept;

        world::EntityId
            actor_{};

        FactionKey
            faction_;

        std::int32_t
            score_{};
    };
}
