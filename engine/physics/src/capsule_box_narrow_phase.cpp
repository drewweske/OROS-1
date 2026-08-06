#include "oros/physics/narrow_phase.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <optional>
#include <variant>

namespace oros::physics
{
    namespace
    {
        struct CapsuleSegment final
        {
            PhysicsVector3 first{};
            PhysicsVector3 second{};
        };

        struct SegmentBoxClosestPoints final
        {
            PhysicsVector3 segment_point{};
            PhysicsVector3 box_point{};
            PhysicsVector3 half_delta{};
            PhysicsScalar half_distance{};
        };

        enum class ContactAxis
        {
            x,
            y,
            z
        };

        struct CapsuleBoxExitSelection final
        {
            PhysicsScalar half_depth{};
            PhysicsVector3 box_to_capsule_normal{};
            PhysicsVector3 segment_point{};
            PhysicsVector3 box_surface{};
        };

        [[nodiscard]]
        foundation::Result<
            CapsuleSegment>
        capsule_segment(
            const ColliderGeometry&
                capsule_geometry,
            const CapsuleShape& capsule)
        {
            const auto bounds_result =
                capsule_geometry.bounds();

            if (!bounds_result.has_value())
            {
                return foundation::fail(
                    bounds_result.error().code,
                    bounds_result.error().message);
            }

            const PhysicsVector3 offset =
                capsule.axis().vector() *
                capsule.half_segment_length();

            const PhysicsVector3 first =
                capsule_geometry.center() -
                offset;

            const PhysicsVector3 second =
                capsule_geometry.center() +
                offset;

            if (!offset.is_finite() ||
                !first.is_finite() ||
                !second.is_finite())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Capsule segment endpoints "
                    "exceed the finite physics "
                    "range.");
            }

            return CapsuleSegment{
                first,
                second
            };
        }

        [[nodiscard]]
        bool append_axis_breakpoints(
            std::array<
                PhysicsScalar,
                8U>& breakpoints,
            std::size_t& breakpoint_count,
            const PhysicsScalar start,
            const PhysicsScalar direction,
            const PhysicsScalar minimum,
            const PhysicsScalar maximum)
        {
            if (direction == 0.0)
            {
                return true;
            }

            const std::array<
                PhysicsScalar,
                2U> boundaries{
                    minimum,
                    maximum
                };

            for (const PhysicsScalar boundary :
                 boundaries)
            {
                const PhysicsScalar numerator =
                    boundary -
                    start;

                if (!std::isfinite(
                        numerator))
                {
                    return false;
                }

                const PhysicsScalar parameter =
                    numerator /
                    direction;

                if (!std::isfinite(
                        parameter))
                {
                    return false;
                }

                if (parameter > 0.0 &&
                    parameter < 1.0)
                {
                    if (breakpoint_count >=
                        breakpoints.size())
                    {
                        return false;
                    }

                    breakpoints[
                        breakpoint_count] =
                            parameter;

                    ++breakpoint_count;
                }
            }

            return true;
        }

        [[nodiscard]]
        foundation::Result<
            SegmentBoxClosestPoints>
        evaluate_segment_box_points(
            const PhysicsVector3 start_half,
            const PhysicsVector3 direction_half,
            const PhysicsVector3 minimum_half,
            const PhysicsVector3 maximum_half,
            const PhysicsScalar parameter)
        {
            const PhysicsVector3 segment_half =
                start_half +
                direction_half *
                    parameter;

            const PhysicsVector3 box_half{
                std::clamp(
                    segment_half.x,
                    minimum_half.x,
                    maximum_half.x),
                std::clamp(
                    segment_half.y,
                    minimum_half.y,
                    maximum_half.y),
                std::clamp(
                    segment_half.z,
                    minimum_half.z,
                    maximum_half.z)
            };

            const PhysicsVector3 half_delta =
                segment_half -
                box_half;

            const PhysicsScalar half_distance =
                half_delta.length();

            const PhysicsVector3 segment_point =
                segment_half *
                2.0;

            const PhysicsVector3 box_point =
                box_half *
                2.0;

            if (!segment_half.is_finite() ||
                !box_half.is_finite() ||
                !half_delta.is_finite() ||
                !std::isfinite(
                    half_distance) ||
                !segment_point.is_finite() ||
                !box_point.is_finite())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Segment-to-box closest points "
                    "exceed the finite physics "
                    "range.");
            }

            return SegmentBoxClosestPoints{
                segment_point,
                box_point,
                half_delta,
                half_distance
            };
        }

        void select_active_component(
            const PhysicsScalar sample,
            const PhysicsScalar minimum,
            const PhysicsScalar maximum,
            const PhysicsScalar start,
            const PhysicsScalar direction,
            PhysicsScalar& offset,
            PhysicsScalar& slope)
        {
            if (sample < minimum)
            {
                offset =
                    start -
                    minimum;

                slope =
                    direction;

                return;
            }

            if (sample > maximum)
            {
                offset =
                    start -
                    maximum;

                slope =
                    direction;

                return;
            }

            offset = 0.0;
            slope = 0.0;
        }

        [[nodiscard]]
        PhysicsScalar closest_parameter_in_interval(
            const PhysicsVector3 start_half,
            const PhysicsVector3 direction_half,
            const PhysicsVector3 minimum_half,
            const PhysicsVector3 maximum_half,
            const PhysicsScalar lower,
            const PhysicsScalar upper)
        {
            const PhysicsScalar sample_parameter =
                std::midpoint(
                    lower,
                    upper);

            const PhysicsVector3 sample =
                start_half +
                direction_half *
                    sample_parameter;

            PhysicsVector3 offset{};
            PhysicsVector3 slope{};

            select_active_component(
                sample.x,
                minimum_half.x,
                maximum_half.x,
                start_half.x,
                direction_half.x,
                offset.x,
                slope.x);

            select_active_component(
                sample.y,
                minimum_half.y,
                maximum_half.y,
                start_half.y,
                direction_half.y,
                offset.y,
                slope.y);

            select_active_component(
                sample.z,
                minimum_half.z,
                maximum_half.z,
                start_half.z,
                direction_half.z,
                offset.z,
                slope.z);

            const PhysicsScalar scale =
                std::max({
                    std::abs(
                        offset.x),
                    std::abs(
                        offset.y),
                    std::abs(
                        offset.z),
                    std::abs(
                        slope.x),
                    std::abs(
                        slope.y),
                    std::abs(
                        slope.z)
                });

            if (scale == 0.0 ||
                !std::isfinite(
                    scale))
            {
                return lower;
            }

            const PhysicsVector3 scaled_offset{
                offset.x /
                    scale,
                offset.y /
                    scale,
                offset.z /
                    scale
            };

            const PhysicsVector3 scaled_slope{
                slope.x /
                    scale,
                slope.y /
                    scale,
                slope.z /
                    scale
            };

            const PhysicsScalar denominator =
                dot(
                    scaled_slope,
                    scaled_slope);

            if (denominator == 0.0 ||
                !std::isfinite(
                    denominator))
            {
                return lower;
            }

            const PhysicsScalar parameter =
                -dot(
                    scaled_offset,
                    scaled_slope) /
                denominator;

            if (!std::isfinite(
                    parameter))
            {
                return lower;
            }

            return std::clamp(
                parameter,
                lower,
                upper);
        }

        [[nodiscard]]
        foundation::Result<
            SegmentBoxClosestPoints>
        closest_segment_box_points(
            const CapsuleSegment& segment,
            const AxisAlignedBounds& bounds)
        {
            const PhysicsVector3 start_half =
                segment.first *
                0.5;

            const PhysicsVector3 end_half =
                segment.second *
                0.5;

            const PhysicsVector3 direction_half =
                end_half -
                start_half;

            const PhysicsVector3 minimum_half =
                bounds.minimum() *
                0.5;

            const PhysicsVector3 maximum_half =
                bounds.maximum() *
                0.5;

            if (!start_half.is_finite() ||
                !end_half.is_finite() ||
                !direction_half.is_finite() ||
                !minimum_half.is_finite() ||
                !maximum_half.is_finite())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Segment-to-box scaled "
                    "coordinates exceed the finite "
                    "physics range.");
            }

            std::array<
                PhysicsScalar,
                8U> breakpoints{};

            breakpoints[0U] = 0.0;
            breakpoints[1U] = 1.0;

            std::size_t breakpoint_count{
                2U
            };

            const bool x_valid =
                append_axis_breakpoints(
                    breakpoints,
                    breakpoint_count,
                    start_half.x,
                    direction_half.x,
                    minimum_half.x,
                    maximum_half.x);

            const bool y_valid =
                append_axis_breakpoints(
                    breakpoints,
                    breakpoint_count,
                    start_half.y,
                    direction_half.y,
                    minimum_half.y,
                    maximum_half.y);

            const bool z_valid =
                append_axis_breakpoints(
                    breakpoints,
                    breakpoint_count,
                    start_half.z,
                    direction_half.z,
                    minimum_half.z,
                    maximum_half.z);

            if (!x_valid ||
                !y_valid ||
                !z_valid)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Segment-to-box breakpoints "
                    "cannot be represented safely.");
            }

            std::sort(
                breakpoints.begin(),
                breakpoints.begin() +
                    static_cast<
                        std::ptrdiff_t>(
                            breakpoint_count));

            const auto unique_end =
                std::unique(
                    breakpoints.begin(),
                    breakpoints.begin() +
                        static_cast<
                            std::ptrdiff_t>(
                                breakpoint_count));

            breakpoint_count =
                static_cast<
                    std::size_t>(
                        unique_end -
                        breakpoints.begin());

            bool has_best{
                false
            };

            SegmentBoxClosestPoints best{};

            const auto consider_parameter =
                [&](
                    const PhysicsScalar parameter)
                    -> foundation::Result<bool>
                {
                    const auto candidate_result =
                        evaluate_segment_box_points(
                            start_half,
                            direction_half,
                            minimum_half,
                            maximum_half,
                            parameter);

                    if (!candidate_result.has_value())
                    {
                        return foundation::fail(
                            candidate_result.
                                error().
                                code,
                            candidate_result.
                                error().
                                message);
                    }

                    const SegmentBoxClosestPoints&
                        candidate =
                            candidate_result.value();

                    if (!has_best ||
                        candidate.half_distance <
                            best.half_distance)
                    {
                        best = candidate;
                        has_best = true;
                    }

                    return true;
                };

            for (std::size_t index{
                     0U
                 };
                 index < breakpoint_count;
                 ++index)
            {
                const auto consideration_result =
                    consider_parameter(
                        breakpoints[index]);

                if (!consideration_result.
                        has_value())
                {
                    return foundation::fail(
                        consideration_result.
                            error().
                            code,
                        consideration_result.
                            error().
                            message);
                }
            }

            for (std::size_t index{
                     0U
                 };
                 index + 1U <
                    breakpoint_count;
                 ++index)
            {
                const PhysicsScalar lower =
                    breakpoints[index];

                const PhysicsScalar upper =
                    breakpoints[index + 1U];

                const PhysicsScalar parameter =
                    closest_parameter_in_interval(
                        start_half,
                        direction_half,
                        minimum_half,
                        maximum_half,
                        lower,
                        upper);

                const auto consideration_result =
                    consider_parameter(
                        parameter);

                if (!consideration_result.
                        has_value())
                {
                    return foundation::fail(
                        consideration_result.
                            error().
                            code,
                        consideration_result.
                            error().
                            message);
                }
            }

            if (!has_best)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Segment-to-box closest points "
                    "could not be selected.");
            }

            return best;
        }

        [[nodiscard]]
        foundation::Result<
            CapsuleBoxExitSelection>
        select_capsule_box_exit(
            const CapsuleSegment& segment,
            const CapsuleShape& capsule,
            const AxisAlignedBounds& bounds)
        {
            const PhysicsVector3 minimum =
                bounds.minimum();

            const PhysicsVector3 maximum =
                bounds.maximum();

            const PhysicsScalar half_radius =
                capsule.radius() *
                0.5;

            const PhysicsVector3&
                x_minimum_point =
                    segment.first.x <=
                            segment.second.x
                        ? segment.first
                        : segment.second;

            const PhysicsVector3&
                x_maximum_point =
                    segment.first.x >=
                            segment.second.x
                        ? segment.first
                        : segment.second;

            const PhysicsVector3&
                y_minimum_point =
                    segment.first.y <=
                            segment.second.y
                        ? segment.first
                        : segment.second;

            const PhysicsVector3&
                y_maximum_point =
                    segment.first.y >=
                            segment.second.y
                        ? segment.first
                        : segment.second;

            const PhysicsVector3&
                z_minimum_point =
                    segment.first.z <=
                            segment.second.z
                        ? segment.first
                        : segment.second;

            const PhysicsVector3&
                z_maximum_point =
                    segment.first.z >=
                            segment.second.z
                        ? segment.first
                        : segment.second;

            bool has_selection{
                false
            };

            CapsuleBoxExitSelection selection{};

            const auto consider_exit =
                [&](
                    const PhysicsScalar half_depth,
                    const PhysicsVector3 normal,
                    const PhysicsVector3
                        segment_point,
                    const ContactAxis axis)
                    -> bool
                {
                    if (!std::isfinite(
                            half_depth) ||
                        half_depth < 0.0 ||
                        !normal.is_finite() ||
                        !segment_point.is_finite())
                    {
                        return false;
                    }

                    PhysicsVector3 box_surface{
                        std::clamp(
                            segment_point.x,
                            minimum.x,
                            maximum.x),
                        std::clamp(
                            segment_point.y,
                            minimum.y,
                            maximum.y),
                        std::clamp(
                            segment_point.z,
                            minimum.z,
                            maximum.z)
                    };

                    switch (axis)
                    {
                    case ContactAxis::x:
                        box_surface.x =
                            normal.x > 0.0
                                ? maximum.x
                                : minimum.x;

                        break;

                    case ContactAxis::y:
                        box_surface.y =
                            normal.y > 0.0
                                ? maximum.y
                                : minimum.y;

                        break;

                    case ContactAxis::z:
                        box_surface.z =
                            normal.z > 0.0
                                ? maximum.z
                                : minimum.z;

                        break;
                    }

                    if (!box_surface.is_finite())
                    {
                        return false;
                    }

                    if (!has_selection ||
                        half_depth <
                            selection.half_depth)
                    {
                        selection =
                            CapsuleBoxExitSelection{
                                half_depth,
                                normal,
                                segment_point,
                                box_surface
                            };

                        has_selection = true;
                    }

                    return true;
                };

            const PhysicsScalar
                positive_x_half_depth =
                    maximum.x *
                        0.5 +
                    half_radius -
                    x_minimum_point.x *
                        0.5;

            const PhysicsScalar
                negative_x_half_depth =
                    x_maximum_point.x *
                        0.5 -
                    minimum.x *
                        0.5 +
                    half_radius;

            const PhysicsScalar
                positive_y_half_depth =
                    maximum.y *
                        0.5 +
                    half_radius -
                    y_minimum_point.y *
                        0.5;

            const PhysicsScalar
                negative_y_half_depth =
                    y_maximum_point.y *
                        0.5 -
                    minimum.y *
                        0.5 +
                    half_radius;

            const PhysicsScalar
                positive_z_half_depth =
                    maximum.z *
                        0.5 +
                    half_radius -
                    z_minimum_point.z *
                        0.5;

            const PhysicsScalar
                negative_z_half_depth =
                    z_maximum_point.z *
                        0.5 -
                    minimum.z *
                        0.5 +
                    half_radius;

            const bool positive_x_valid =
                consider_exit(
                    positive_x_half_depth,
                    physics_positive_x,
                    x_minimum_point,
                    ContactAxis::x);

            const bool negative_x_valid =
                consider_exit(
                    negative_x_half_depth,
                    -physics_positive_x,
                    x_maximum_point,
                    ContactAxis::x);

            const bool positive_y_valid =
                consider_exit(
                    positive_y_half_depth,
                    physics_positive_y,
                    y_minimum_point,
                    ContactAxis::y);

            const bool negative_y_valid =
                consider_exit(
                    negative_y_half_depth,
                    -physics_positive_y,
                    y_maximum_point,
                    ContactAxis::y);

            const bool positive_z_valid =
                consider_exit(
                    positive_z_half_depth,
                    physics_positive_z,
                    z_minimum_point,
                    ContactAxis::z);

            const bool negative_z_valid =
                consider_exit(
                    negative_z_half_depth,
                    -physics_positive_z,
                    z_maximum_point,
                    ContactAxis::z);

            if (!positive_x_valid ||
                !negative_x_valid ||
                !positive_y_valid ||
                !negative_y_valid ||
                !positive_z_valid ||
                !negative_z_valid ||
                !has_selection)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Capsule-box exit selection "
                    "exceeds the finite physics "
                    "range.");
            }

            return selection;
        }

        [[nodiscard]]
        foundation::Result<
            PhysicsUnitVector3>
        unit_normal(
            const PhysicsVector3 direction)
        {
            const auto normalized_result =
                direction.normalized(
                    0.0);

            if (!normalized_result.has_value())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Capsule-box contact direction "
                    "cannot be normalized.");
            }

            const auto unit_result =
                PhysicsUnitVector3::create(
                    normalized_result.value());

            if (!unit_result.has_value())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Capsule-box contact normal "
                    "cannot satisfy the unit-vector "
                    "contract.");
            }

            return unit_result.value();
        }

        [[nodiscard]]
        foundation::Result<
            PhysicsUnitVector3>
        orient_capsule_box_normal(
            const BroadPhasePair& pair,
            const ColliderId capsule_collider,
            const PhysicsUnitVector3&
                capsule_to_box_normal)
        {
            const PhysicsVector3 direction =
                pair.first_collider() ==
                    capsule_collider
                    ? capsule_to_box_normal.
                        vector()
                    : -capsule_to_box_normal.
                        vector();

            const auto normal_result =
                PhysicsUnitVector3::create(
                    direction);

            if (!normal_result.has_value())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Canonical capsule-box normal "
                    "cannot satisfy the unit-vector "
                    "contract.");
            }

            return normal_result.value();
        }

        [[nodiscard]]
        foundation::Result<
            std::optional<CollisionContact>>
        create_capsule_box_contact(
            const BroadPhasePair& pair,
            const ColliderId capsule_collider,
            const PhysicsVector3 box_surface,
            const PhysicsVector3 capsule_surface,
            const PhysicsUnitVector3&
                box_to_capsule_normal,
            const PhysicsScalar penetration_depth)
        {
            if (!box_surface.is_finite() ||
                !capsule_surface.is_finite() ||
                !std::isfinite(
                    penetration_depth) ||
                penetration_depth < 0.0)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Capsule-box contact data "
                    "exceeds the finite physics "
                    "range.");
            }

            const PhysicsVector3 contact_point =
                box_surface *
                    0.5 +
                capsule_surface *
                    0.5;

            if (!contact_point.is_finite())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Capsule-box contact point "
                    "exceeds the finite physics "
                    "range.");
            }

            const auto capsule_to_box_result =
                PhysicsUnitVector3::create(
                    -box_to_capsule_normal.
                        vector());

            if (!capsule_to_box_result.has_value())
            {
                return foundation::fail(
                    capsule_to_box_result.
                        error().
                        code,
                    capsule_to_box_result.
                        error().
                        message);
            }

            const auto canonical_normal_result =
                orient_capsule_box_normal(
                    pair,
                    capsule_collider,
                    capsule_to_box_result.value());

            if (!canonical_normal_result.has_value())
            {
                return foundation::fail(
                    canonical_normal_result.
                        error().
                        code,
                    canonical_normal_result.
                        error().
                        message);
            }

            const auto contact_result =
                CollisionContact::create(
                    pair,
                    contact_point,
                    canonical_normal_result.value(),
                    penetration_depth);

            if (!contact_result.has_value())
            {
                return foundation::fail(
                    contact_result.error().code,
                    contact_result.error().message);
            }

            return std::optional<
                CollisionContact>{
                    contact_result.value()
                };
        }
    }

    foundation::Result<
        std::optional<CollisionContact>>
    generate_capsule_box_contact(
        const ColliderGeometry& first_geometry,
        const ColliderGeometry& second_geometry)
    {
        const auto pair_result =
            BroadPhasePair::create(
                first_geometry.collider(),
                second_geometry.collider());

        if (!pair_result.has_value())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Capsule-box contact generation "
                "requires two distinct persistent "
                "collider identities.");
        }

        const bool first_is_capsule =
            std::holds_alternative<
                CapsuleShape>(
                first_geometry.shape());

        const bool second_is_capsule =
            std::holds_alternative<
                CapsuleShape>(
                second_geometry.shape());

        const bool first_is_box =
            std::holds_alternative<
                BoxShape>(
                first_geometry.shape());

        const bool second_is_box =
            std::holds_alternative<
                BoxShape>(
                second_geometry.shape());

        const bool valid_shape_pair =
            (first_is_capsule &&
                second_is_box) ||
            (first_is_box &&
                second_is_capsule);

        if (!valid_shape_pair)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Capsule-box contact generation "
                "requires exactly one capsule and "
                "one box.");
        }

        const ColliderGeometry*
            capsule_geometry =
                first_is_capsule
                    ? &first_geometry
                    : &second_geometry;

        const ColliderGeometry*
            box_geometry =
                first_is_box
                    ? &first_geometry
                    : &second_geometry;

        const CapsuleShape& capsule =
            std::get<CapsuleShape>(
                capsule_geometry->shape());

        const auto segment_result =
            capsule_segment(
                *capsule_geometry,
                capsule);

        if (!segment_result.has_value())
        {
            return foundation::fail(
                segment_result.error().code,
                segment_result.error().message);
        }

        const auto bounds_result =
            box_geometry->bounds();

        if (!bounds_result.has_value())
        {
            return foundation::fail(
                bounds_result.error().code,
                bounds_result.error().message);
        }

        const CapsuleSegment segment =
            segment_result.value();

        const AxisAlignedBounds bounds =
            bounds_result.value();

        const auto closest_result =
            closest_segment_box_points(
                segment,
                bounds);

        if (!closest_result.has_value())
        {
            return foundation::fail(
                closest_result.error().code,
                closest_result.error().message);
        }

        const BroadPhasePair pair =
            pair_result.value();

        const SegmentBoxClosestPoints closest =
            closest_result.value();

        if (closest.half_distance > 0.0)
        {
            const PhysicsScalar half_radius =
                capsule.radius() *
                0.5;

            if (closest.half_distance >
                half_radius)
            {
                return
                    std::optional<
                        CollisionContact>{};
            }

            const auto normal_result =
                unit_normal(
                    closest.half_delta);

            if (!normal_result.has_value())
            {
                return foundation::fail(
                    normal_result.error().code,
                    normal_result.error().message);
            }

            const PhysicsUnitVector3
                box_to_capsule_normal =
                    normal_result.value();

            const PhysicsScalar
                half_penetration_depth =
                    half_radius -
                    closest.half_distance;

            const PhysicsScalar penetration_depth =
                half_penetration_depth *
                2.0;

            if (!std::isfinite(
                    penetration_depth))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Capsule-box penetration depth "
                    "exceeds the finite physics "
                    "range.");
            }

            const PhysicsVector3 capsule_surface =
                closest.segment_point -
                box_to_capsule_normal.
                    vector() *
                    capsule.radius();

            return create_capsule_box_contact(
                pair,
                capsule_geometry->collider(),
                closest.box_point,
                capsule_surface,
                box_to_capsule_normal,
                penetration_depth);
        }

        const auto exit_result =
            select_capsule_box_exit(
                segment,
                capsule,
                bounds);

        if (!exit_result.has_value())
        {
            return foundation::fail(
                exit_result.error().code,
                exit_result.error().message);
        }

        const CapsuleBoxExitSelection exit =
            exit_result.value();

        const PhysicsScalar penetration_depth =
            exit.half_depth *
            2.0;

        if (!std::isfinite(
                penetration_depth))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Capsule-box penetration depth "
                "exceeds the finite physics "
                "range.");
        }

        const auto normal_result =
            PhysicsUnitVector3::create(
                exit.box_to_capsule_normal);

        if (!normal_result.has_value())
        {
            return foundation::fail(
                normal_result.error().code,
                normal_result.error().message);
        }

        const PhysicsUnitVector3
            box_to_capsule_normal =
                normal_result.value();

        const PhysicsVector3 capsule_surface =
            exit.segment_point +
            box_to_capsule_normal.
                vector() *
                capsule.radius();

        return create_capsule_box_contact(
            pair,
            capsule_geometry->collider(),
            exit.box_surface,
            capsule_surface,
            box_to_capsule_normal,
            penetration_depth);
    }
}