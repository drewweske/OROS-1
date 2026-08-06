#pragma once

#include "oros/foundation/result.hpp"
#include "oros/physics/collider_id.hpp"

namespace oros::physics
{
    class BroadPhasePair final
    {
    public:
        [[nodiscard]]
        static foundation::Result<
            BroadPhasePair>
        create(
            ColliderId first_collider,
            ColliderId second_collider);

        [[nodiscard]]
        ColliderId
        first_collider() const noexcept;

        [[nodiscard]]
        ColliderId
        second_collider() const noexcept;

        [[nodiscard]]
        bool
        contains(
            ColliderId collider)
            const noexcept;

        bool operator==(
            const BroadPhasePair&)
            const noexcept = default;

        [[nodiscard]]
        bool operator<(
            const BroadPhasePair& other)
            const noexcept;

    private:
        BroadPhasePair(
            ColliderId first_collider,
            ColliderId second_collider)
            noexcept;

        ColliderId first_collider_{};
        ColliderId second_collider_{};
    };
}