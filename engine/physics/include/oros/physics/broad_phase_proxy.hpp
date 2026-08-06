#pragma once

#include "oros/foundation/result.hpp"
#include "oros/physics/axis_aligned_bounds.hpp"
#include "oros/physics/collider_id.hpp"

namespace oros::physics
{
    class BroadPhaseProxy final
    {
    public:
        [[nodiscard]]
        static foundation::Result<
            BroadPhaseProxy>
        create(
            ColliderId collider,
            AxisAlignedBounds bounds);

        [[nodiscard]]
        ColliderId
        collider() const noexcept;

        [[nodiscard]]
        const AxisAlignedBounds&
        bounds() const noexcept;

        [[nodiscard]]
        foundation::Result<
            BroadPhaseProxy>
        with_bounds(
            AxisAlignedBounds bounds)
            const;

        bool operator==(
            const BroadPhaseProxy&)
            const noexcept = default;

    private:
        BroadPhaseProxy(
            ColliderId collider,
            AxisAlignedBounds bounds)
            noexcept;

        ColliderId collider_{};
        AxisAlignedBounds bounds_;
    };
}