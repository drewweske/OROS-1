#include "oros/physics/narrow_phase.hpp"

#include <algorithm>
#include <cmath>
#include <optional>
#include <variant>

namespace oros::physics
{
    namespace
    {
        struct CapsuleSegment final
        {
            PhysicsVector3 first_half{};
            PhysicsVector3 second_half{};
            PhysicsVector3 direction_half{};
        };

        struct SegmentParameters final
        {
            PhysicsScalar first{};
            PhysicsScalar second{};
        };

        struct SegmentClosestPoints final
        {
            PhysicsVector3 first_half{};
            PhysicsVector3 second_half{};
            PhysicsVector3 first{};
            PhysicsVector3 second{};
            PhysicsVector3 scaled_delta{};
            PhysicsVector3 direct_delta{};
            PhysicsScalar half_distance{};
        };

        [[nodiscard]]
        foundation::Result<
            PhysicsUnitVector3>
        normalized_unit_vector(
            const PhysicsVector3 direction,
            const char* const failure_message)
        {
            const auto normalized_result =
                direction.normalized(
                    0.0);

            if (!normalized_result.has_value())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    failure_message);
            }

            const auto unit_result =
                PhysicsUnitVector3::create(
                    normalized_result.value());

            if (!unit_result.has_value())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    failure_message);
            }

            return unit_result.value();
        }

        [[nodiscard]]
        foundation::Result<
            CapsuleSegment>
        capsule_segment(
            const ColliderGeometry& geometry,
            const CapsuleShape& capsule)
        {
            const auto bounds_result =
                geometry.bounds();

            if (!bounds_result.has_value())
            {
                return foundation::fail(
                    bounds_result.error().code,
                    bounds_result.error().message);
            }

            const PhysicsVector3 center_half =
                geometry.center() *
                0.5;

            const PhysicsVector3 offset_half =
                capsule.axis().vector() *
                (capsule.half_segment_length() *
                    0.5);

            const PhysicsVector3 first_half =
                center_half -
                offset_half;

            const PhysicsVector3 second_half =
                center_half +
                offset_half;

            const PhysicsVector3 direction_half =
                second_half -
                first_half;

            const PhysicsVector3 first =
                first_half *
                2.0;

            const PhysicsVector3 second =
                second_half *
                2.0;

            if (!center_half.is_finite() ||
                !offset_half.is_finite() ||
                !first_half.is_finite() ||
                !second_half.is_finite() ||
                !direction_half.is_finite() ||
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
                first_half,
                second_half,
                direction_half
            };
        }

        [[nodiscard]]
        foundation::Result<
            SegmentParameters>
        closest_segment_parameters(
            const CapsuleSegment& first,
            const CapsuleSegment& second)
        {
            const PhysicsVector3 relative_half =
                first.first_half -
                second.first_half;

            if (!relative_half.is_finite())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Capsule segment separation "
                    "exceeds the finite physics "
                    "range.");
            }

            const PhysicsScalar scale =
                std::max({
                    std::abs(
                        first.direction_half.x),
                    std::abs(
                        first.direction_half.y),
                    std::abs(
                        first.direction_half.z),
                    std::abs(
                        second.direction_half.x),
                    std::abs(
                        second.direction_half.y),
                    std::abs(
                        second.direction_half.z),
                    std::abs(
                        relative_half.x),
                    std::abs(
                        relative_half.y),
                    std::abs(
                        relative_half.z)
                });

            if (!std::isfinite(
                    scale))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Capsule segment scale exceeds "
                    "the finite physics range.");
            }

            if (scale == 0.0)
            {
                return SegmentParameters{
                    0.0,
                    0.0
                };
            }

            const PhysicsVector3 first_direction{
                first.direction_half.x /
                    scale,
                first.direction_half.y /
                    scale,
                first.direction_half.z /
                    scale
            };

            const PhysicsVector3 second_direction{
                second.direction_half.x /
                    scale,
                second.direction_half.y /
                    scale,
                second.direction_half.z /
                    scale
            };

            const PhysicsVector3 relative{
                relative_half.x /
                    scale,
                relative_half.y /
                    scale,
                relative_half.z /
                    scale
            };

            if (!first_direction.is_finite() ||
                !second_direction.is_finite() ||
                !relative.is_finite())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Scaled capsule segment data "
                    "exceeds the finite physics "
                    "range.");
            }

            const PhysicsScalar first_length_squared =
                dot(
                    first_direction,
                    first_direction);

            const PhysicsScalar
                second_length_squared =
                    dot(
                        second_direction,
                        second_direction);

            const PhysicsScalar
                second_relative_projection =
                    dot(
                        second_direction,
                        relative);

            if (!std::isfinite(
                    first_length_squared) ||
                !std::isfinite(
                    second_length_squared) ||
                !std::isfinite(
                    second_relative_projection))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Capsule segment projection "
                    "exceeds the finite physics "
                    "range.");
            }

            PhysicsScalar first_parameter{
                0.0
            };

            PhysicsScalar second_parameter{
                0.0
            };

            if (first_length_squared == 0.0 &&
                second_length_squared == 0.0)
            {
                return SegmentParameters{
                    first_parameter,
                    second_parameter
                };
            }

            if (first_length_squared == 0.0)
            {
                second_parameter =
                    std::clamp(
                        second_relative_projection /
                            second_length_squared,
                        0.0,
                        1.0);
            }
            else
            {
                const PhysicsScalar
                    first_relative_projection =
                        dot(
                            first_direction,
                            relative);

                if (!std::isfinite(
                        first_relative_projection))
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "Capsule segment projection "
                        "exceeds the finite physics "
                        "range.");
                }

                if (second_length_squared == 0.0)
                {
                    first_parameter =
                        std::clamp(
                            -first_relative_projection /
                                first_length_squared,
                            0.0,
                            1.0);
                }
                else
                {
                    const PhysicsScalar
                        direction_projection =
                            dot(
                                first_direction,
                                second_direction);

                    const PhysicsScalar denominator =
                        first_length_squared *
                            second_length_squared -
                        direction_projection *
                            direction_projection;

                    if (!std::isfinite(
                            direction_projection) ||
                        !std::isfinite(
                            denominator))
                    {
                        return foundation::fail(
                            foundation::ErrorCode::
                                invalid_argument,
                            "Capsule segment "
                            "intersection math exceeds "
                            "the finite physics range.");
                    }

                    if (denominator > 0.0)
                    {
                        first_parameter =
                            std::clamp(
                                (direction_projection *
                                    second_relative_projection -
                                    first_relative_projection *
                                        second_length_squared) /
                                    denominator,
                                0.0,
                                1.0);
                    }

                    second_parameter =
                        (direction_projection *
                            first_parameter +
                            second_relative_projection) /
                        second_length_squared;

                    if (second_parameter < 0.0)
                    {
                        second_parameter = 0.0;

                        first_parameter =
                            std::clamp(
                                -first_relative_projection /
                                    first_length_squared,
                                0.0,
                                1.0);
                    }
                    else if (second_parameter > 1.0)
                    {
                        second_parameter = 1.0;

                        first_parameter =
                            std::clamp(
                                (direction_projection -
                                    first_relative_projection) /
                                    first_length_squared,
                                0.0,
                                1.0);
                    }
                }
            }

            if (!std::isfinite(
                    first_parameter) ||
                !std::isfinite(
                    second_parameter))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Capsule closest-point "
                    "parameters exceed the finite "
                    "physics range.");
            }

            return SegmentParameters{
                first_parameter,
                second_parameter
            };
        }

        [[nodiscard]]
        foundation::Result<
            SegmentClosestPoints>
        closest_segment_points(
            const CapsuleSegment& first,
            const CapsuleSegment& second)
        {
            const auto parameters_result =
                closest_segment_parameters(
                    first,
                    second);

            if (!parameters_result.has_value())
            {
                return foundation::fail(
                    parameters_result.error().code,
                    parameters_result.
                        error().
                        message);
            }

            const SegmentParameters parameters =
                parameters_result.value();

            const PhysicsVector3 first_half =
                first.first_half +
                first.direction_half *
                    parameters.first;

            const PhysicsVector3 second_half =
                second.first_half +
                second.direction_half *
                    parameters.second;

            const PhysicsVector3 scaled_delta =
                second_half -
                first_half;

            const PhysicsVector3 first_point =
                first_half *
                2.0;

            const PhysicsVector3 second_point =
                second_half *
                2.0;

            const PhysicsVector3 direct_delta =
                second_point -
                first_point;

            const PhysicsScalar half_distance =
                scaled_delta.length();

            if (!first_half.is_finite() ||
                !second_half.is_finite() ||
                !scaled_delta.is_finite() ||
                !first_point.is_finite() ||
                !second_point.is_finite() ||
                !std::isfinite(
                    half_distance))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Capsule closest points exceed "
                    "the finite physics range.");
            }

            return SegmentClosestPoints{
                first_half,
                second_half,
                first_point,
                second_point,
                scaled_delta,
                direct_delta,
                half_distance
            };
        }

        [[nodiscard]]
        foundation::Result<
            PhysicsUnitVector3>
        deterministic_capsule_capsule_normal(
            const SegmentClosestPoints& closest,
            const CapsuleShape& first_capsule,
            const CapsuleShape& second_capsule)
        {
            const PhysicsVector3 direction =
                closest.direct_delta.is_finite()
                    ? closest.direct_delta
                    : closest.scaled_delta;

            if (direction !=
                physics_zero_vector)
            {
                return normalized_unit_vector(
                    direction,
                    "Capsule-capsule contact "
                    "direction cannot satisfy the "
                    "unit-vector contract.");
            }

            const bool first_has_segment =
                first_capsule.
                    half_segment_length() >
                0.0;

            const bool second_has_segment =
                second_capsule.
                    half_segment_length() >
                0.0;

            if (!first_has_segment &&
                !second_has_segment)
            {
                return
                    PhysicsUnitVector3::
                        positive_x();
            }

            if (first_has_segment &&
                second_has_segment)
            {
                const PhysicsVector3
                    cross_direction =
                        cross(
                            first_capsule.
                                axis().
                                vector(),
                            second_capsule.
                                axis().
                                vector());

                if (cross_direction.is_finite() &&
                    cross_direction !=
                        physics_zero_vector)
                {
                    return normalized_unit_vector(
                        cross_direction,
                        "Capsule-capsule crossed "
                        "axes cannot produce a "
                        "unit normal.");
                }
            }

            const PhysicsVector3& axis =
                first_has_segment
                    ? first_capsule.
                        axis().
                        vector()
                    : second_capsule.
                        axis().
                        vector();

            PhysicsVector3 reference =
                physics_positive_x;

            PhysicsScalar selected_alignment =
                std::abs(
                    axis.x);

            if (std::abs(
                    axis.y) <
                selected_alignment)
            {
                reference =
                    physics_positive_y;

                selected_alignment =
                    std::abs(
                        axis.y);
            }

            if (std::abs(
                    axis.z) <
                selected_alignment)
            {
                reference =
                    physics_positive_z;
            }

            const PhysicsVector3
                perpendicular_direction =
                    reference -
                    axis *
                        dot(
                            reference,
                            axis);

            return normalized_unit_vector(
                perpendicular_direction,
                "A deterministic capsule-capsule "
                "fallback normal could not be "
                "created.");
        }
    }

    foundation::Result<
        std::optional<CollisionContact>>
    generate_capsule_capsule_contact(
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
                "Capsule-capsule contact "
                "generation requires two distinct "
                "persistent collider identities.");
        }

        const bool first_is_capsule =
            std::holds_alternative<
                CapsuleShape>(
                first_geometry.shape());

        const bool second_is_capsule =
            std::holds_alternative<
                CapsuleShape>(
                second_geometry.shape());

        if (!first_is_capsule ||
            !second_is_capsule)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Capsule-capsule contact "
                "generation requires exactly two "
                "capsules.");
        }

        const BroadPhasePair pair =
            pair_result.value();

        const ColliderGeometry*
            canonical_first_geometry =
                pair.first_collider() ==
                        first_geometry.collider()
                    ? &first_geometry
                    : &second_geometry;

        const ColliderGeometry*
            canonical_second_geometry =
                pair.second_collider() ==
                        second_geometry.collider()
                    ? &second_geometry
                    : &first_geometry;

        const CapsuleShape& first_capsule =
            std::get<CapsuleShape>(
                canonical_first_geometry->
                    shape());

        const CapsuleShape& second_capsule =
            std::get<CapsuleShape>(
                canonical_second_geometry->
                    shape());

        const auto first_segment_result =
            capsule_segment(
                *canonical_first_geometry,
                first_capsule);

        if (!first_segment_result.has_value())
        {
            return foundation::fail(
                first_segment_result.error().code,
                first_segment_result.
                    error().
                    message);
        }

        const auto second_segment_result =
            capsule_segment(
                *canonical_second_geometry,
                second_capsule);

        if (!second_segment_result.has_value())
        {
            return foundation::fail(
                second_segment_result.error().code,
                second_segment_result.
                    error().
                    message);
        }

        const auto closest_result =
            closest_segment_points(
                first_segment_result.value(),
                second_segment_result.value());

        if (!closest_result.has_value())
        {
            return foundation::fail(
                closest_result.error().code,
                closest_result.error().message);
        }

        const SegmentClosestPoints closest =
            closest_result.value();

        const PhysicsScalar
            half_combined_radius =
                first_capsule.radius() *
                    0.5 +
                second_capsule.radius() *
                    0.5;

        if (!std::isfinite(
                half_combined_radius))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Capsule-capsule combined radius "
                "exceeds the finite physics "
                "range.");
        }

        if (closest.half_distance >
            half_combined_radius)
        {
            return
                std::optional<
                    CollisionContact>{};
        }

        const auto normal_result =
            deterministic_capsule_capsule_normal(
                closest,
                first_capsule,
                second_capsule);

        if (!normal_result.has_value())
        {
            return foundation::fail(
                normal_result.error().code,
                normal_result.error().message);
        }

        const PhysicsUnitVector3 normal =
            normal_result.value();

        const PhysicsScalar
            half_penetration_depth =
                half_combined_radius -
                closest.half_distance;

        const PhysicsScalar penetration_depth =
            half_penetration_depth *
            2.0;

        if (!std::isfinite(
                penetration_depth) ||
            penetration_depth < 0.0)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Capsule-capsule penetration "
                "depth exceeds the finite physics "
                "range.");
        }

        const PhysicsVector3
            first_surface_half =
                closest.first_half +
                normal.vector() *
                    (first_capsule.radius() *
                        0.5);

        const PhysicsVector3
            second_surface_half =
                closest.second_half -
                normal.vector() *
                    (second_capsule.radius() *
                        0.5);

        const PhysicsVector3 contact_point =
            first_surface_half +
            second_surface_half;

        if (!first_surface_half.is_finite() ||
            !second_surface_half.is_finite() ||
            !contact_point.is_finite())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Capsule-capsule contact point "
                "exceeds the finite physics "
                "range.");
        }

        const auto contact_result =
            CollisionContact::create(
                pair,
                contact_point,
                normal,
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