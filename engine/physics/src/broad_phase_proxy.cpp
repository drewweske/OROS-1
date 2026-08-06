#include "oros/physics/broad_phase_proxy.hpp"

namespace oros::physics
{
    BroadPhaseProxy::BroadPhaseProxy(
        const ColliderId collider,
        const AxisAlignedBounds bounds)
        noexcept
        : collider_{collider},
          bounds_{bounds}
    {
    }

    foundation::Result<
        BroadPhaseProxy>
    BroadPhaseProxy::create(
        const ColliderId collider,
        const AxisAlignedBounds bounds)
    {
        if (!collider.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Broad-phase proxy requires a "
                "valid persistent collider "
                "identity.");
        }

        return BroadPhaseProxy{
            collider,
            bounds
        };
    }

    ColliderId
    BroadPhaseProxy::collider()
        const noexcept
    {
        return collider_;
    }

    const AxisAlignedBounds&
    BroadPhaseProxy::bounds()
        const noexcept
    {
        return bounds_;
    }

    foundation::Result<
        BroadPhaseProxy>
    BroadPhaseProxy::with_bounds(
        const AxisAlignedBounds bounds)
        const
    {
        return create(
            collider_,
            bounds);
    }
}