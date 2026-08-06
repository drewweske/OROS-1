#include "oros/physics/axis_aligned_bounds.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace oros::physics
{
    namespace
    {
        [[nodiscard]]
        bool has_ordered_components(
            const PhysicsVector3 minimum,
            const PhysicsVector3 maximum)
            noexcept
        {
            return
                minimum.x <= maximum.x &&
                minimum.y <= maximum.y &&
                minimum.z <= maximum.z;
        }

        [[nodiscard]]
        bool has_non_negative_components(
            const PhysicsVector3 value)
            noexcept
        {
            return
                value.x >= 0.0 &&
                value.y >= 0.0 &&
                value.z >= 0.0;
        }

        [[nodiscard]]
        bool preserves_centered_extent(
            const PhysicsScalar center,
            const PhysicsScalar half_extent,
            const PhysicsScalar minimum,
            const PhysicsScalar maximum)
            noexcept
        {
            if (half_extent == 0.0)
            {
                return
                    minimum == center &&
                    maximum == center;
            }

            return
                minimum < center &&
                maximum > center;
        }

        [[nodiscard]]
        bool preserves_centered_extents(
            const PhysicsVector3 center,
            const PhysicsVector3 half_extents,
            const PhysicsVector3 minimum,
            const PhysicsVector3 maximum)
            noexcept
        {
            return
                preserves_centered_extent(
                    center.x,
                    half_extents.x,
                    minimum.x,
                    maximum.x) &&
                preserves_centered_extent(
                    center.y,
                    half_extents.y,
                    minimum.y,
                    maximum.y) &&
                preserves_centered_extent(
                    center.z,
                    half_extents.z,
                    minimum.z,
                    maximum.z);
        }
    }

    AxisAlignedBounds::
        AxisAlignedBounds(
            const PhysicsVector3 minimum,
            const PhysicsVector3 maximum)
        noexcept
        : minimum_{minimum},
          maximum_{maximum}
    {
    }

    foundation::Result<
        AxisAlignedBounds>
    AxisAlignedBounds::create(
        const PhysicsVector3 minimum,
        const PhysicsVector3 maximum)
    {
        if (!minimum.is_finite() ||
            !maximum.is_finite())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Axis-aligned bounds endpoints "
                "must contain only finite values.");
        }

        if (!has_ordered_components(
                minimum,
                maximum))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Axis-aligned bounds minimum "
                "must not exceed its maximum.");
        }

        return AxisAlignedBounds{
            minimum,
            maximum
        };
    }

    foundation::Result<
        AxisAlignedBounds>
    AxisAlignedBounds::
        from_center_and_half_extents(
            const PhysicsVector3 center,
            const PhysicsVector3
                half_extents)
    {
        if (!center.is_finite() ||
            !half_extents.is_finite())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Bounds center and half extents "
                "must contain only finite values.");
        }

        if (!has_non_negative_components(
                half_extents))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Bounds half extents must be "
                "non-negative.");
        }

        const PhysicsVector3 minimum =
            center -
            half_extents;

        const PhysicsVector3 maximum =
            center +
            half_extents;

        if (!minimum.is_finite() ||
            !maximum.is_finite())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Bounds center and half extents "
                "exceed the finite physics range.");
        }

        if (!preserves_centered_extents(
                center,
                half_extents,
                minimum,
                maximum))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Bounds half extents cannot be "
                "represented at the requested "
                "center.");
        }

        return AxisAlignedBounds{
            minimum,
            maximum
        };
    }

    const PhysicsVector3&
    AxisAlignedBounds::minimum()
        const noexcept
    {
        return minimum_;
    }

    const PhysicsVector3&
    AxisAlignedBounds::maximum()
        const noexcept
    {
        return maximum_;
    }

    PhysicsVector3
    AxisAlignedBounds::center()
        const noexcept
    {
        return PhysicsVector3{
            std::midpoint(
                minimum_.x,
                maximum_.x),
            std::midpoint(
                minimum_.y,
                maximum_.y),
            std::midpoint(
                minimum_.z,
                maximum_.z)
        };
    }

    PhysicsVector3
    AxisAlignedBounds::half_extents()
        const noexcept
    {
        return PhysicsVector3{
            maximum_.x * 0.5 -
                minimum_.x * 0.5,
            maximum_.y * 0.5 -
                minimum_.y * 0.5,
            maximum_.z * 0.5 -
                minimum_.z * 0.5
        };
    }

    bool
    AxisAlignedBounds::contains(
        const PhysicsVector3 point)
        const noexcept
    {
        if (!point.is_finite())
        {
            return false;
        }

        return
            point.x >= minimum_.x &&
            point.x <= maximum_.x &&
            point.y >= minimum_.y &&
            point.y <= maximum_.y &&
            point.z >= minimum_.z &&
            point.z <= maximum_.z;
    }

    bool
    AxisAlignedBounds::contains(
        const AxisAlignedBounds& other)
        const noexcept
    {
        return
            other.minimum_.x >= minimum_.x &&
            other.maximum_.x <= maximum_.x &&
            other.minimum_.y >= minimum_.y &&
            other.maximum_.y <= maximum_.y &&
            other.minimum_.z >= minimum_.z &&
            other.maximum_.z <= maximum_.z;
    }

    bool
    AxisAlignedBounds::overlaps(
        const AxisAlignedBounds& other)
        const noexcept
    {
        return
            minimum_.x <= other.maximum_.x &&
            maximum_.x >= other.minimum_.x &&
            minimum_.y <= other.maximum_.y &&
            maximum_.y >= other.minimum_.y &&
            minimum_.z <= other.maximum_.z &&
            maximum_.z >= other.minimum_.z;
    }

    AxisAlignedBounds
    AxisAlignedBounds::merged_with(
        const AxisAlignedBounds& other)
        const noexcept
    {
        return AxisAlignedBounds{
            PhysicsVector3{
                std::min(
                    minimum_.x,
                    other.minimum_.x),
                std::min(
                    minimum_.y,
                    other.minimum_.y),
                std::min(
                    minimum_.z,
                    other.minimum_.z)
            },
            PhysicsVector3{
                std::max(
                    maximum_.x,
                    other.maximum_.x),
                std::max(
                    maximum_.y,
                    other.maximum_.y),
                std::max(
                    maximum_.z,
                    other.maximum_.z)
            }
        };
    }

    foundation::Result<
        AxisAlignedBounds>
    AxisAlignedBounds::translated(
        const PhysicsVector3 offset)
        const
    {
        if (!offset.is_finite())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Bounds translation must contain "
                "only finite values.");
        }

        const PhysicsVector3
            translated_minimum =
                minimum_ +
                offset;

        const PhysicsVector3
            translated_maximum =
                maximum_ +
                offset;

        if (!translated_minimum.is_finite() ||
            !translated_maximum.is_finite())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Bounds translation exceeds the "
                "finite physics range.");
        }

        return AxisAlignedBounds{
            translated_minimum,
            translated_maximum
        };
    }

    foundation::Result<
        AxisAlignedBounds>
    AxisAlignedBounds::expanded(
        const PhysicsScalar margin)
        const
    {
        if (!std::isfinite(
                margin) ||
            margin < 0.0)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Bounds expansion margin must be "
                "finite and non-negative.");
        }

        const PhysicsVector3 expansion{
            margin,
            margin,
            margin
        };

        const PhysicsVector3
            expanded_minimum =
                minimum_ -
                expansion;

        const PhysicsVector3
            expanded_maximum =
                maximum_ +
                expansion;

        if (!expanded_minimum.is_finite() ||
            !expanded_maximum.is_finite())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Bounds expansion exceeds the "
                "finite physics range.");
        }

        return AxisAlignedBounds{
            expanded_minimum,
            expanded_maximum
        };
    }
}