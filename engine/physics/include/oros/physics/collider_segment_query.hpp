#pragma once

#include "oros/foundation/result.hpp"
#include "oros/physics/collider_geometry.hpp"
#include "oros/physics/physics_vector.hpp"

#include <optional>

namespace oros::physics
{
    class ColliderSegmentHit final
    {
    public:
        [[nodiscard]]
        static foundation::Result<
            ColliderSegmentHit>
        create(
            ColliderId collider,
            PhysicsScalar segment_fraction);

        [[nodiscard]]
        ColliderId
        collider() const noexcept;

        [[nodiscard]]
        PhysicsScalar
        segment_fraction() const noexcept;

        bool operator==(
            const ColliderSegmentHit&)
            const noexcept = default;

    private:
        ColliderSegmentHit(
            ColliderId collider,
            PhysicsScalar segment_fraction)
            noexcept;

        ColliderId collider_{};

        PhysicsScalar
            segment_fraction_{};
    };

    [[nodiscard]]
    foundation::Result<
        std::optional<ColliderSegmentHit>>
    query_collider_segment_hit(
        PhysicsVector3 segment_start,
        PhysicsVector3 segment_end,
        const ColliderGeometry& geometry);
}
