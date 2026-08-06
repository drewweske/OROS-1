#pragma once

#include "oros/world/entity_id.hpp"

#include <compare>
#include <cstddef>
#include <cstdint>

namespace oros::physics
{
    using ColliderShapeSlot =
        std::uint32_t;

    struct ColliderId final
    {
        world::EntityId owner{};
        ColliderShapeSlot shape_slot{};

        [[nodiscard]]
        constexpr bool
        is_valid() const noexcept
        {
            return
                owner.is_valid() &&
                shape_slot != 0U;
        }

        [[nodiscard]]
        constexpr explicit
        operator bool() const noexcept
        {
            return is_valid();
        }

        auto operator<=>(
            const ColliderId&)
            const noexcept = default;
    };

    inline constexpr ColliderId
        invalid_collider_id{};

    struct ColliderIdHash final
    {
        [[nodiscard]]
        std::size_t
        operator()(
            ColliderId id)
            const noexcept;
    };
}