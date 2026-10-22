#include "oros/physics/collider_segment_query.hpp"

#include "oros/physics/axis_aligned_bounds.hpp"
#include "oros/physics/collision_shape.hpp"

#include <algorithm>
#include <cmath>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>

namespace oros::physics
{
    namespace
    {
        using SegmentFractionResult =
            foundation::Result<
                std::optional<PhysicsScalar>>;

        [[nodiscard]]
        SegmentFractionResult
        query_segment_sphere_fraction(
            const PhysicsVector3 segment_start,
            const PhysicsVector3 segment_direction,
            const PhysicsVector3 sphere_center,
            const PhysicsScalar sphere_radius)
        {
            const PhysicsVector3
                start_relative =
                    segment_start -
                    sphere_center;

            if (!start_relative.is_finite())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Segment/sphere relative "
                    "position exceeds the finite "
                    "physics range.");
            }

            const PhysicsScalar
                radius_squared =
                    sphere_radius *
                    sphere_radius;

            const PhysicsScalar
                start_distance_squared =
                    dot(
                        start_relative,
                        start_relative);

            if (
                !std::isfinite(
                    radius_squared) ||
                !std::isfinite(
                    start_distance_squared))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Segment/sphere squared "
                    "geometry exceeds the finite "
                    "physics range.");
            }

            if (
                start_distance_squared <=
                radius_squared)
            {
                return std::optional<
                    PhysicsScalar>{
                        0.0
                    };
            }

            const PhysicsScalar
                direction_squared =
                    dot(
                        segment_direction,
                        segment_direction);

            if (!std::isfinite(
                    direction_squared))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Segment direction magnitude "
                    "exceeds the finite physics "
                    "range.");
            }

            if (direction_squared == 0.0)
            {
                return std::optional<
                    PhysicsScalar>{};
            }

            const PhysicsScalar
                projection =
                    dot(
                        start_relative,
                        segment_direction);

            const PhysicsScalar
                outside_squared =
                    start_distance_squared -
                    radius_squared;

            if (
                !std::isfinite(projection) ||
                !std::isfinite(
                    outside_squared))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Segment/sphere projection "
                    "exceeds the finite physics "
                    "range.");
            }

            const PhysicsScalar
                projection_squared =
                    projection *
                    projection;

            const PhysicsScalar
                scaled_outside_squared =
                    direction_squared *
                    outside_squared;

            if (
                !std::isfinite(
                    projection_squared) ||
                !std::isfinite(
                    scaled_outside_squared))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Segment/sphere quadratic "
                    "terms exceed the finite "
                    "physics range.");
            }

            const PhysicsScalar
                discriminant =
                    projection_squared -
                    scaled_outside_squared;

            if (!std::isfinite(discriminant))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Segment/sphere discriminant "
                    "exceeds the finite physics "
                    "range.");
            }

            if (discriminant < 0.0)
            {
                return std::optional<
                    PhysicsScalar>{};
            }

            const PhysicsScalar root =
                std::sqrt(discriminant);

            const PhysicsScalar numerator =
                -projection -
                root;

            if (!std::isfinite(numerator))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Segment/sphere intersection "
                    "numerator exceeds the finite "
                    "physics range.");
            }

            const PhysicsScalar
                segment_fraction =
                    numerator /
                    direction_squared;

            if (!std::isfinite(
                    segment_fraction))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Segment/sphere intersection "
                    "fraction is not finite.");
            }

            if (
                segment_fraction < 0.0 ||
                segment_fraction > 1.0)
            {
                return std::optional<
                    PhysicsScalar>{};
            }

            return std::optional<
                PhysicsScalar>{
                    segment_fraction
                };
        }

        [[nodiscard]]
        SegmentFractionResult
        query_segment_box_fraction(
            const PhysicsVector3 segment_start,
            const PhysicsVector3 segment_direction,
            const ColliderGeometry& geometry)
        {
            const auto bounds_result =
                geometry.bounds();

            if (!bounds_result.has_value())
            {
                return foundation::fail(
                    bounds_result.error().code,
                    bounds_result.error().message);
            }

            const AxisAlignedBounds& bounds =
                bounds_result.value();

            if (bounds.contains(segment_start))
            {
                return std::optional<
                    PhysicsScalar>{
                        0.0
                    };
            }

            if (
                segment_direction ==
                physics_zero_vector)
            {
                return std::optional<
                    PhysicsScalar>{};
            }

            PhysicsScalar
                enter_fraction{0.0};

            PhysicsScalar
                exit_fraction{1.0};

            const auto intersect_axis =
                [&enter_fraction,
                 &exit_fraction](
                    const PhysicsScalar start,
                    const PhysicsScalar direction,
                    const PhysicsScalar minimum,
                    const PhysicsScalar maximum)
                    -> foundation::Result<bool>
                {
                    if (direction == 0.0)
                    {
                        return
                            start >= minimum &&
                            start <= maximum;
                    }

                    const PhysicsScalar
                        minimum_delta =
                            minimum -
                            start;

                    const PhysicsScalar
                        maximum_delta =
                            maximum -
                            start;

                    if (
                        !std::isfinite(
                            minimum_delta) ||
                        !std::isfinite(
                            maximum_delta))
                    {
                        return foundation::fail(
                            foundation::ErrorCode::
                                invalid_argument,
                            "Segment/box slab "
                            "difference exceeds the "
                            "finite physics range.");
                    }

                    PhysicsScalar
                        first_fraction =
                            minimum_delta /
                            direction;

                    PhysicsScalar
                        second_fraction =
                            maximum_delta /
                            direction;

                    if (
                        !std::isfinite(
                            first_fraction) ||
                        !std::isfinite(
                            second_fraction))
                    {
                        return foundation::fail(
                            foundation::ErrorCode::
                                invalid_argument,
                            "Segment/box slab "
                            "fraction is not finite.");
                    }

                    if (
                        first_fraction >
                        second_fraction)
                    {
                        std::swap(
                            first_fraction,
                            second_fraction);
                    }

                    enter_fraction =
                        std::max(
                            enter_fraction,
                            first_fraction);

                    exit_fraction =
                        std::min(
                            exit_fraction,
                            second_fraction);

                    return
                        enter_fraction <=
                        exit_fraction;
                };

            const PhysicsVector3 minimum =
                bounds.minimum();

            const PhysicsVector3 maximum =
                bounds.maximum();

            const auto x_result =
                intersect_axis(
                    segment_start.x,
                    segment_direction.x,
                    minimum.x,
                    maximum.x);

            if (!x_result.has_value())
            {
                return foundation::fail(
                    x_result.error().code,
                    x_result.error().message);
            }

            if (!x_result.value())
            {
                return std::optional<
                    PhysicsScalar>{};
            }

            const auto y_result =
                intersect_axis(
                    segment_start.y,
                    segment_direction.y,
                    minimum.y,
                    maximum.y);

            if (!y_result.has_value())
            {
                return foundation::fail(
                    y_result.error().code,
                    y_result.error().message);
            }

            if (!y_result.value())
            {
                return std::optional<
                    PhysicsScalar>{};
            }

            const auto z_result =
                intersect_axis(
                    segment_start.z,
                    segment_direction.z,
                    minimum.z,
                    maximum.z);

            if (!z_result.has_value())
            {
                return foundation::fail(
                    z_result.error().code,
                    z_result.error().message);
            }

            if (!z_result.value())
            {
                return std::optional<
                    PhysicsScalar>{};
            }

            if (
                enter_fraction < 0.0 ||
                enter_fraction > 1.0 ||
                !std::isfinite(
                    enter_fraction))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "Segment/box query produced "
                    "an invalid hit fraction.");
            }

            return std::optional<
                PhysicsScalar>{
                    enter_fraction
                };
        }

        [[nodiscard]]
        foundation::Result<bool>
        point_is_inside_capsule(
            const PhysicsVector3 point,
            const ColliderGeometry& geometry,
            const CapsuleShape& capsule)
        {
            const PhysicsVector3 relative =
                point -
                geometry.center();

            if (!relative.is_finite())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Point/capsule relative "
                    "position exceeds the finite "
                    "physics range.");
            }

            const PhysicsVector3 axis =
                capsule.axis().vector();

            const PhysicsScalar
                axial_projection =
                    dot(
                        relative,
                        axis);

            if (!std::isfinite(
                    axial_projection))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Point/capsule projection "
                    "exceeds the finite physics "
                    "range.");
            }

            const PhysicsScalar
                clamped_projection =
                    std::clamp(
                        axial_projection,
                        -capsule.
                            half_segment_length(),
                        capsule.
                            half_segment_length());

            const PhysicsVector3
                closest_axis_offset =
                    axis *
                    clamped_projection;

            const PhysicsVector3 radial =
                relative -
                closest_axis_offset;

            if (
                !closest_axis_offset.
                    is_finite() ||
                !radial.is_finite())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Point/capsule closest-point "
                    "calculation exceeds the "
                    "finite physics range.");
            }

            const PhysicsScalar
                radial_squared =
                    dot(
                        radial,
                        radial);

            const PhysicsScalar
                radius_squared =
                    capsule.radius() *
                    capsule.radius();

            if (
                !std::isfinite(
                    radial_squared) ||
                !std::isfinite(
                    radius_squared))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Point/capsule squared "
                    "geometry exceeds the finite "
                    "physics range.");
            }

            return
                radial_squared <=
                radius_squared;
        }

        [[nodiscard]]
        SegmentFractionResult
        query_segment_capsule_fraction(
            const PhysicsVector3 segment_start,
            const PhysicsVector3 segment_direction,
            const ColliderGeometry& geometry,
            const CapsuleShape& capsule)
        {
            const auto inside_result =
                point_is_inside_capsule(
                    segment_start,
                    geometry,
                    capsule);

            if (!inside_result.has_value())
            {
                return foundation::fail(
                    inside_result.error().code,
                    inside_result.error().message);
            }

            if (inside_result.value())
            {
                return std::optional<
                    PhysicsScalar>{
                        0.0
                    };
            }

            const PhysicsScalar
                direction_squared =
                    dot(
                        segment_direction,
                        segment_direction);

            if (!std::isfinite(
                    direction_squared))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Segment direction magnitude "
                    "exceeds the finite physics "
                    "range.");
            }

            if (direction_squared == 0.0)
            {
                return std::optional<
                    PhysicsScalar>{};
            }

            if (
                capsule.
                    half_segment_length() ==
                0.0)
            {
                return
                    query_segment_sphere_fraction(
                        segment_start,
                        segment_direction,
                        geometry.center(),
                        capsule.radius());
            }

            const PhysicsVector3 axis =
                capsule.axis().vector();

            const PhysicsVector3
                start_relative =
                    segment_start -
                    geometry.center();

            if (!start_relative.is_finite())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Segment/capsule relative "
                    "position exceeds the finite "
                    "physics range.");
            }

            const PhysicsScalar
                start_axial =
                    dot(
                        start_relative,
                        axis);

            const PhysicsScalar
                direction_axial =
                    dot(
                        segment_direction,
                        axis);

            if (
                !std::isfinite(
                    start_axial) ||
                !std::isfinite(
                    direction_axial))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Segment/capsule axial "
                    "projection exceeds the finite "
                    "physics range.");
            }

            const PhysicsVector3
                start_radial =
                    start_relative -
                    axis *
                    start_axial;

            const PhysicsVector3
                direction_radial =
                    segment_direction -
                    axis *
                    direction_axial;

            if (
                !start_radial.is_finite() ||
                !direction_radial.is_finite())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Segment/capsule radial "
                    "projection exceeds the finite "
                    "physics range.");
            }

            std::optional<PhysicsScalar>
                earliest_fraction{};

            const auto consider =
                [&earliest_fraction](
                    const std::optional<
                        PhysicsScalar>
                        candidate)
                {
                    if (
                        candidate.has_value() &&
                        (
                            !earliest_fraction.
                                has_value() ||
                            candidate.value() <
                                earliest_fraction.
                                    value()
                        )
                    )
                    {
                        earliest_fraction =
                            candidate;
                    }
                };

            const PhysicsScalar
                cylinder_a =
                    dot(
                        direction_radial,
                        direction_radial);

            const PhysicsScalar
                cylinder_b =
                    dot(
                        start_radial,
                        direction_radial);

            const PhysicsScalar
                radius_squared =
                    capsule.radius() *
                    capsule.radius();

            const PhysicsScalar
                cylinder_c =
                    dot(
                        start_radial,
                        start_radial) -
                    radius_squared;

            if (
                !std::isfinite(cylinder_a) ||
                !std::isfinite(cylinder_b) ||
                !std::isfinite(cylinder_c) ||
                !std::isfinite(
                    radius_squared))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Segment/capsule cylinder "
                    "terms exceed the finite "
                    "physics range.");
            }

            if (cylinder_a > 0.0)
            {
                const PhysicsScalar
                    b_squared =
                        cylinder_b *
                        cylinder_b;

                const PhysicsScalar
                    ac =
                        cylinder_a *
                        cylinder_c;

                if (
                    !std::isfinite(b_squared) ||
                    !std::isfinite(ac))
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "Segment/capsule cylinder "
                        "quadratic exceeds the "
                        "finite physics range.");
                }

                const PhysicsScalar
                    discriminant =
                        b_squared -
                        ac;

                if (!std::isfinite(
                        discriminant))
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "Segment/capsule cylinder "
                        "discriminant exceeds the "
                        "finite physics range.");
                }

                if (discriminant >= 0.0)
                {
                    const PhysicsScalar root =
                        std::sqrt(
                            discriminant);

                    const PhysicsScalar
                        numerator =
                            -cylinder_b -
                            root;

                    if (!std::isfinite(
                            numerator))
                    {
                        return foundation::fail(
                            foundation::ErrorCode::
                                invalid_argument,
                            "Segment/capsule "
                            "cylinder numerator "
                            "exceeds the finite "
                            "physics range.");
                    }

                    const PhysicsScalar
                        fraction =
                            numerator /
                            cylinder_a;

                    if (!std::isfinite(
                            fraction))
                    {
                        return foundation::fail(
                            foundation::ErrorCode::
                                invalid_argument,
                            "Segment/capsule "
                            "cylinder fraction is "
                            "not finite.");
                    }

                    if (
                        fraction >= 0.0 &&
                        fraction <= 1.0)
                    {
                        const PhysicsScalar
                            axial_at_hit =
                                start_axial +
                                fraction *
                                    direction_axial;

                        if (!std::isfinite(
                                axial_at_hit))
                        {
                            return foundation::fail(
                                foundation::
                                    ErrorCode::
                                        invalid_argument,
                                "Segment/capsule "
                                "hit projection "
                                "exceeds the finite "
                                "physics range.");
                        }

                        if (
                            axial_at_hit >=
                                -capsule.
                                    half_segment_length() &&
                            axial_at_hit <=
                                capsule.
                                    half_segment_length())
                        {
                            consider(
                                std::optional<
                                    PhysicsScalar>{
                                        fraction
                                    });
                        }
                    }
                }
            }

            const PhysicsVector3
                cap_offset =
                    axis *
                    capsule.
                        half_segment_length();

            const PhysicsVector3
                first_cap_center =
                    geometry.center() -
                    cap_offset;

            const PhysicsVector3
                second_cap_center =
                    geometry.center() +
                    cap_offset;

            if (
                !cap_offset.is_finite() ||
                !first_cap_center.is_finite() ||
                !second_cap_center.is_finite())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Capsule cap centers exceed "
                    "the finite physics range.");
            }

            const auto first_cap_result =
                query_segment_sphere_fraction(
                    segment_start,
                    segment_direction,
                    first_cap_center,
                    capsule.radius());

            if (!first_cap_result.has_value())
            {
                return foundation::fail(
                    first_cap_result.error().code,
                    first_cap_result.error().message);
            }

            consider(
                first_cap_result.value());

            const auto second_cap_result =
                query_segment_sphere_fraction(
                    segment_start,
                    segment_direction,
                    second_cap_center,
                    capsule.radius());

            if (!second_cap_result.has_value())
            {
                return foundation::fail(
                    second_cap_result.error().code,
                    second_cap_result.error().message);
            }

            consider(
                second_cap_result.value());

            return earliest_fraction;
        }
    }

    ColliderSegmentHit::
        ColliderSegmentHit(
            const ColliderId collider,
            const PhysicsScalar
                segment_fraction)
        noexcept
        : collider_{collider},
          segment_fraction_{
              segment_fraction
          }
    {
    }

    foundation::Result<
        ColliderSegmentHit>
    ColliderSegmentHit::create(
        const ColliderId collider,
        const PhysicsScalar
            segment_fraction)
    {
        if (!collider.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Collider segment hit requires a "
                "valid collider identity.");
        }

        if (
            !std::isfinite(
                segment_fraction) ||
            segment_fraction < 0.0 ||
            segment_fraction > 1.0)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Collider segment hit fraction "
                "must be finite and inside the "
                "closed interval [0, 1].");
        }

        return ColliderSegmentHit{
            collider,
            segment_fraction
        };
    }

    ColliderId
    ColliderSegmentHit::collider()
        const noexcept
    {
        return collider_;
    }

    PhysicsScalar
    ColliderSegmentHit::
        segment_fraction()
        const noexcept
    {
        return segment_fraction_;
    }

    foundation::Result<
        std::optional<ColliderSegmentHit>>
    query_collider_segment_hit(
        const PhysicsVector3 segment_start,
        const PhysicsVector3 segment_end,
        const ColliderGeometry& geometry)
    {
        if (
            !segment_start.is_finite() ||
            !segment_end.is_finite())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Collider segment query endpoints "
                "must contain only finite values.");
        }

        const PhysicsVector3
            segment_direction =
                segment_end -
                segment_start;

        if (!segment_direction.is_finite())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Collider segment direction "
                "exceeds the finite physics range.");
        }

        const auto fraction_result =
            std::visit(
                [&](
                    const auto&
                        concrete_shape)
                    -> SegmentFractionResult
                {
                    using ShapeType =
                        std::remove_cvref_t<
                            decltype(
                                concrete_shape)>;

                    if constexpr (
                        std::is_same_v<
                            ShapeType,
                            SphereShape>)
                    {
                        return
                            query_segment_sphere_fraction(
                                segment_start,
                                segment_direction,
                                geometry.center(),
                                concrete_shape.
                                    radius());
                    }
                    else if constexpr (
                        std::is_same_v<
                            ShapeType,
                            BoxShape>)
                    {
                        return
                            query_segment_box_fraction(
                                segment_start,
                                segment_direction,
                                geometry);
                    }
                    else
                    {
                        static_assert(
                            std::is_same_v<
                                ShapeType,
                                CapsuleShape>);

                        return
                            query_segment_capsule_fraction(
                                segment_start,
                                segment_direction,
                                geometry,
                                concrete_shape);
                    }
                },
                geometry.shape());

        if (!fraction_result.has_value())
        {
            return foundation::fail(
                fraction_result.error().code,
                fraction_result.error().message);
        }

        if (!fraction_result.value().has_value())
        {
            return std::optional<
                ColliderSegmentHit>{};
        }

        const auto hit_result =
            ColliderSegmentHit::create(
                geometry.collider(),
                fraction_result.value().
                    value());

        if (!hit_result.has_value())
        {
            return foundation::fail(
                hit_result.error().code,
                hit_result.error().message);
        }

        return std::optional<
            ColliderSegmentHit>{
                hit_result.value()
            };
    }
}
