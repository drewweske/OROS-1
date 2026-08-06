#pragma once

#include "oros/foundation/result.hpp"
#include "oros/physics/physics_vector.hpp"

#include <cstdint>
#include <variant>

namespace oros::physics
{
    enum class CollisionShapeKind :
        std::uint8_t
    {
        sphere = 1U,
        box = 2U,
        capsule = 3U
    };

    class SphereShape final
    {
    public:
        [[nodiscard]]
        static foundation::Result<
            SphereShape>
        create(
            PhysicsScalar radius);

        [[nodiscard]]
        PhysicsScalar
        radius() const noexcept;

        [[nodiscard]]
        constexpr CollisionShapeKind
        kind() const noexcept
        {
            return
                CollisionShapeKind::
                    sphere;
        }

        bool operator==(
            const SphereShape&)
            const noexcept = default;

    private:
        explicit SphereShape(
            PhysicsScalar radius)
            noexcept;

        PhysicsScalar radius_{};
    };

    class BoxShape final
    {
    public:
        [[nodiscard]]
        static foundation::Result<
            BoxShape>
        create(
            PhysicsVector3
                half_extents);

        [[nodiscard]]
        const PhysicsVector3&
        half_extents() const noexcept;

        [[nodiscard]]
        constexpr CollisionShapeKind
        kind() const noexcept
        {
            return
                CollisionShapeKind::
                    box;
        }

        bool operator==(
            const BoxShape&)
            const noexcept = default;

    private:
        explicit BoxShape(
            PhysicsVector3
                half_extents)
            noexcept;

        PhysicsVector3
            half_extents_{};
    };

    class CapsuleShape final
    {
    public:
        [[nodiscard]]
        static foundation::Result<
            CapsuleShape>
        create(
            PhysicsScalar radius,
            PhysicsScalar
                half_segment_length,
            PhysicsUnitVector3 axis);

        [[nodiscard]]
        PhysicsScalar
        radius() const noexcept;

        [[nodiscard]]
        PhysicsScalar
        half_segment_length()
            const noexcept;

        [[nodiscard]]
        PhysicsScalar
        total_half_height()
            const noexcept;

        [[nodiscard]]
        const PhysicsUnitVector3&
        axis() const noexcept;

        [[nodiscard]]
        constexpr CollisionShapeKind
        kind() const noexcept
        {
            return
                CollisionShapeKind::
                    capsule;
        }

        bool operator==(
            const CapsuleShape&)
            const noexcept = default;

    private:
        CapsuleShape(
            PhysicsScalar radius,
            PhysicsScalar
                half_segment_length,
            PhysicsUnitVector3 axis)
            noexcept;

        PhysicsScalar radius_{};

        PhysicsScalar
            half_segment_length_{};

        PhysicsUnitVector3 axis_;
    };

    using CollisionShape =
        std::variant<
            SphereShape,
            BoxShape,
            CapsuleShape>;

    [[nodiscard]]
    CollisionShapeKind
    collision_shape_kind(
        const CollisionShape& shape)
        noexcept;
}