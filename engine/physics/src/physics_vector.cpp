#include "oros/physics/physics_vector.hpp"

#include <algorithm>
#include <cmath>

namespace oros::physics
{
    namespace
    {
        [[nodiscard]]
        PhysicsScalar
        maximum_absolute_component(
            const PhysicsVector3& vector)
            noexcept
        {
            return std::max(
                {
                    std::abs(
                        vector.x),
                    std::abs(
                        vector.y),
                    std::abs(
                        vector.z)
                });
        }

        [[nodiscard]]
        PhysicsScalar
        scaled_vector_length(
            const PhysicsVector3& vector,
            const PhysicsScalar scale)
            noexcept
        {
            return std::hypot(
                vector.x / scale,
                vector.y / scale,
                vector.z / scale);
        }
    }

    bool is_valid_physics_tolerance(
        const PhysicsScalar tolerance)
        noexcept
    {
        return
            std::isfinite(
                tolerance) &&
            tolerance >= 0.0;
    }

    bool PhysicsVector3::is_finite()
        const noexcept
    {
        return
            std::isfinite(
                x) &&
            std::isfinite(
                y) &&
            std::isfinite(
                z);
    }

    PhysicsScalar
    PhysicsVector3::length_squared()
        const noexcept
    {
        return dot(
            *this,
            *this);
    }

    PhysicsScalar
    PhysicsVector3::length()
        const noexcept
    {
        return std::hypot(
            x,
            y,
            z);
    }

    bool PhysicsVector3::is_near_zero(
        const PhysicsScalar tolerance)
        const noexcept
    {
        if (!is_valid_physics_tolerance(
                tolerance) ||
            !is_finite())
        {
            return false;
        }

        const PhysicsScalar scale =
            maximum_absolute_component(
                *this);

        if (scale == 0.0)
        {
            return true;
        }

        const PhysicsScalar
            scaled_length =
                scaled_vector_length(
                    *this,
                    scale);

        if (!std::isfinite(
                scaled_length) ||
            scaled_length <= 0.0)
        {
            return false;
        }

        return
            scale <=
                tolerance /
                    scaled_length;
    }

    foundation::Result<
        PhysicsVector3>
    PhysicsVector3::normalized(
        const PhysicsScalar tolerance)
        const
    {
        if (!is_valid_physics_tolerance(
                tolerance))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Physics vector normalization "
                "requires a finite, non-negative "
                "tolerance.");
        }

        if (!is_finite())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cannot normalize a physics vector "
                "containing a non-finite component.");
        }

        const PhysicsScalar scale =
            maximum_absolute_component(
                *this);

        if (scale == 0.0)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cannot normalize the zero physics "
                "vector.");
        }

        const PhysicsScalar
            scaled_length =
                scaled_vector_length(
                    *this,
                    scale);

        if (!std::isfinite(
                scaled_length) ||
            scaled_length <= 0.0)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cannot normalize a physics vector "
                "with an invalid magnitude.");
        }

        if (scale <=
            tolerance /
                scaled_length)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cannot normalize a physics vector "
                "whose magnitude is within the "
                "zero tolerance.");
        }

        const PhysicsVector3
            scaled_vector{
                x / scale,
                y / scale,
                z / scale
            };

        const PhysicsScalar
            inverse_scaled_length{
                1.0 /
                scaled_length
            };

        const PhysicsVector3 result =
            scaled_vector *
            inverse_scaled_length;

        if (!result.is_finite())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "Physics vector normalization "
                "produced a non-finite result.");
        }

        return result;
    }

    bool nearly_equal(
        const PhysicsScalar left,
        const PhysicsScalar right,
        const PhysicsScalar tolerance)
        noexcept
    {
        if (!is_valid_physics_tolerance(
                tolerance) ||
            !std::isfinite(
                left) ||
            !std::isfinite(
                right))
        {
            return false;
        }

        return
            std::abs(
                left -
                right) <=
            tolerance;
    }

    bool nearly_equal(
        const PhysicsVector3& left,
        const PhysicsVector3& right,
        const PhysicsScalar tolerance)
        noexcept
    {
        return
            nearly_equal(
                left.x,
                right.x,
                tolerance) &&
            nearly_equal(
                left.y,
                right.y,
                tolerance) &&
            nearly_equal(
                left.z,
                right.z,
                tolerance);
    }

    foundation::Result<
        PhysicsUnitVector3>
    PhysicsUnitVector3::create(
        const PhysicsVector3 vector,
        const PhysicsScalar tolerance)
    {
        const auto normalized_result =
            vector.normalized(
                tolerance);

        if (!normalized_result.has_value())
        {
            return foundation::fail(
                normalized_result.error().code,
                normalized_result.error().message);
        }

        return PhysicsUnitVector3{
            normalized_result.value()
        };
    }

    PhysicsUnitVector3
    PhysicsUnitVector3::positive_x()
        noexcept
    {
        return PhysicsUnitVector3{
            physics_positive_x
        };
    }

    PhysicsUnitVector3
    PhysicsUnitVector3::positive_y()
        noexcept
    {
        return PhysicsUnitVector3{
            physics_positive_y
        };
    }

    PhysicsUnitVector3
    PhysicsUnitVector3::positive_z()
        noexcept
    {
        return PhysicsUnitVector3{
            physics_positive_z
        };
    }

    bool PhysicsUnitVector3::is_valid(
        const PhysicsScalar tolerance)
        const noexcept
    {
        return
            vector_.is_finite() &&
            is_valid_physics_tolerance(
                tolerance) &&
            nearly_equal(
                vector_.length(),
                1.0,
                tolerance);
    }

    const PhysicsVector3&
    PhysicsUnitVector3::vector()
        const noexcept
    {
        return vector_;
    }

    PhysicsUnitVector3::
    PhysicsUnitVector3(
        const PhysicsVector3 vector)
        noexcept
        : vector_{
              vector
          }
    {
    }
}