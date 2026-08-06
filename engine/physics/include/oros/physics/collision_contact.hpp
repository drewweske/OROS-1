#pragma once

#include "oros/foundation/result.hpp"
#include "oros/physics/broad_phase_pair.hpp"
#include "oros/physics/physics_vector.hpp"

namespace oros::physics
{
    class CollisionContact final
    {
    public:
        [[nodiscard]]
        static foundation::Result<
            CollisionContact>
        create(
            BroadPhasePair pair,
            PhysicsVector3 point,
            PhysicsUnitVector3 normal,
            PhysicsScalar penetration_depth);

        [[nodiscard]]
        const BroadPhasePair&
        pair() const noexcept;

        [[nodiscard]]
        PhysicsVector3
        point() const noexcept;

        [[nodiscard]]
        const PhysicsUnitVector3&
        normal() const noexcept;

        [[nodiscard]]
        PhysicsScalar
        penetration_depth() const noexcept;

        [[nodiscard]]
        bool
        is_touching() const noexcept;

        bool operator==(
            const CollisionContact&)
            const noexcept = default;

    private:
        CollisionContact(
            BroadPhasePair pair,
            PhysicsVector3 point,
            PhysicsUnitVector3 normal,
            PhysicsScalar penetration_depth)
            noexcept;

        BroadPhasePair pair_;
        PhysicsVector3 point_{};
        PhysicsUnitVector3 normal_;
        PhysicsScalar penetration_depth_{};
    };
}