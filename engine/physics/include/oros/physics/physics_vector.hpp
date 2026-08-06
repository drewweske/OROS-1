#pragma once

#include "oros/foundation/result.hpp"

namespace oros::physics
{
    using PhysicsScalar = double;

    inline constexpr PhysicsScalar
        physics_vector_zero_tolerance{
            1.0e-12
        };

    inline constexpr PhysicsScalar
        physics_unit_length_tolerance{
            1.0e-9
        };

    [[nodiscard]]
    bool is_valid_physics_tolerance(
        PhysicsScalar tolerance) noexcept;

    struct PhysicsVector3 final
    {
        PhysicsScalar x{};
        PhysicsScalar y{};
        PhysicsScalar z{};

        [[nodiscard]]
        bool is_finite() const noexcept;

        [[nodiscard]]
        PhysicsScalar
        length_squared() const noexcept;

        [[nodiscard]]
        PhysicsScalar
        length() const noexcept;

        [[nodiscard]]
        bool is_near_zero(
            PhysicsScalar tolerance =
                physics_vector_zero_tolerance)
            const noexcept;

        [[nodiscard]]
        foundation::Result<
            PhysicsVector3>
        normalized(
            PhysicsScalar tolerance =
                physics_vector_zero_tolerance)
            const;

        bool operator==(
            const PhysicsVector3&)
            const noexcept = default;
    };

    inline constexpr PhysicsVector3
        physics_zero_vector{};

    inline constexpr PhysicsVector3
        physics_positive_x{
            1.0,
            0.0,
            0.0
        };

    inline constexpr PhysicsVector3
        physics_positive_y{
            0.0,
            1.0,
            0.0
        };

    inline constexpr PhysicsVector3
        physics_positive_z{
            0.0,
            0.0,
            1.0
        };

    [[nodiscard]]
    constexpr PhysicsVector3
    operator+(
        const PhysicsVector3 left,
        const PhysicsVector3 right)
        noexcept
    {
        return PhysicsVector3{
            left.x + right.x,
            left.y + right.y,
            left.z + right.z
        };
    }

    [[nodiscard]]
    constexpr PhysicsVector3
    operator-(
        const PhysicsVector3 left,
        const PhysicsVector3 right)
        noexcept
    {
        return PhysicsVector3{
            left.x - right.x,
            left.y - right.y,
            left.z - right.z
        };
    }

    [[nodiscard]]
    constexpr PhysicsVector3
    operator-(
        const PhysicsVector3 value)
        noexcept
    {
        return PhysicsVector3{
            -value.x,
            -value.y,
            -value.z
        };
    }

    [[nodiscard]]
    constexpr PhysicsVector3
    operator*(
        const PhysicsVector3 vector,
        const PhysicsScalar scalar)
        noexcept
    {
        return PhysicsVector3{
            vector.x * scalar,
            vector.y * scalar,
            vector.z * scalar
        };
    }

    [[nodiscard]]
    constexpr PhysicsVector3
    operator*(
        const PhysicsScalar scalar,
        const PhysicsVector3 vector)
        noexcept
    {
        return vector * scalar;
    }

    [[nodiscard]]
    constexpr PhysicsScalar
    dot(
        const PhysicsVector3 left,
        const PhysicsVector3 right)
        noexcept
    {
        return
            left.x * right.x +
            left.y * right.y +
            left.z * right.z;
    }

    [[nodiscard]]
    constexpr PhysicsVector3
    cross(
        const PhysicsVector3 left,
        const PhysicsVector3 right)
        noexcept
    {
        return PhysicsVector3{
            left.y * right.z -
                left.z * right.y,
            left.z * right.x -
                left.x * right.z,
            left.x * right.y -
                left.y * right.x
        };
    }

    [[nodiscard]]
    bool nearly_equal(
        PhysicsScalar left,
        PhysicsScalar right,
        PhysicsScalar tolerance =
            physics_unit_length_tolerance)
        noexcept;

    [[nodiscard]]
    bool nearly_equal(
        const PhysicsVector3& left,
        const PhysicsVector3& right,
        PhysicsScalar tolerance =
            physics_unit_length_tolerance)
        noexcept;

    class PhysicsUnitVector3 final
    {
    public:
        [[nodiscard]]
        static foundation::Result<
            PhysicsUnitVector3>
        create(
            PhysicsVector3 vector,
            PhysicsScalar tolerance =
                physics_vector_zero_tolerance);

        [[nodiscard]]
        static PhysicsUnitVector3
        positive_x() noexcept;

        [[nodiscard]]
        static PhysicsUnitVector3
        positive_y() noexcept;

        [[nodiscard]]
        static PhysicsUnitVector3
        positive_z() noexcept;

        [[nodiscard]]
        bool is_valid(
            PhysicsScalar tolerance =
                physics_unit_length_tolerance)
            const noexcept;

        [[nodiscard]]
        const PhysicsVector3&
        vector() const noexcept;

        bool operator==(
            const PhysicsUnitVector3&)
            const noexcept = default;

    private:
        explicit PhysicsUnitVector3(
            PhysicsVector3 vector)
            noexcept;

        PhysicsVector3 vector_{};
    };
}