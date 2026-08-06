#pragma once

#include "oros/foundation/result.hpp"
#include "oros/physics/physics_vector.hpp"

namespace oros::physics
{
    class AxisAlignedBounds final
    {
    public:
        [[nodiscard]]
        static foundation::Result<
            AxisAlignedBounds>
        create(
            PhysicsVector3 minimum,
            PhysicsVector3 maximum);

        [[nodiscard]]
        static foundation::Result<
            AxisAlignedBounds>
        from_center_and_half_extents(
            PhysicsVector3 center,
            PhysicsVector3 half_extents);

        [[nodiscard]]
        const PhysicsVector3&
        minimum() const noexcept;

        [[nodiscard]]
        const PhysicsVector3&
        maximum() const noexcept;

        [[nodiscard]]
        PhysicsVector3
        center() const noexcept;

        [[nodiscard]]
        PhysicsVector3
        half_extents() const noexcept;

        [[nodiscard]]
        bool contains(
            PhysicsVector3 point)
            const noexcept;

        [[nodiscard]]
        bool contains(
            const AxisAlignedBounds&
                other)
            const noexcept;

        [[nodiscard]]
        bool overlaps(
            const AxisAlignedBounds&
                other)
            const noexcept;

        [[nodiscard]]
        AxisAlignedBounds
        merged_with(
            const AxisAlignedBounds&
                other)
            const noexcept;

        [[nodiscard]]
        foundation::Result<
            AxisAlignedBounds>
        translated(
            PhysicsVector3 offset)
            const;

        [[nodiscard]]
        foundation::Result<
            AxisAlignedBounds>
        expanded(
            PhysicsScalar margin)
            const;

        bool operator==(
            const AxisAlignedBounds&)
            const noexcept = default;

    private:
        AxisAlignedBounds(
            PhysicsVector3 minimum,
            PhysicsVector3 maximum)
            noexcept;

        PhysicsVector3 minimum_{};
        PhysicsVector3 maximum_{};
    };
}