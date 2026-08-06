#include "oros/physics/narrow_phase.hpp"

#include <algorithm>
#include <cmath>
#include <optional>
#include <variant>

namespace oros::physics
{
    namespace
    {
        [[nodiscard]]
        foundation::Result<
            PhysicsScalar>
        clamped_capsule_segment_parameter(
            const PhysicsVector3 point,
            const PhysicsVector3 capsule_center,
            const CapsuleShape& capsule)
        {
            const PhysicsVector3
                half_relative_position =
                    point * 0.5 -
                    capsule_center * 0.5;

            if (!half_relative_position.is_finite())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Sphere-capsule relative "
                    "position exceeds the finite "
                    "physics range.");
            }

            const PhysicsScalar scale =
                std::max({
                    std::abs(
                        half_relative_position.x),
                    std::abs(
                        half_relative_position.y),
                    std::abs(
                        half_relative_position.z)
                });

            if (scale == 0.0)
            {
                return 0.0;
            }

            const PhysicsVector3
                normalized_relative_position{
                    half_relative_position.x /
                        scale,
                    half_relative_position.y /
                        scale,
                    half_relative_position.z /
                        scale
                };

            const PhysicsScalar
                normalized_projection =
                    dot(
                        normalized_relative_position,
                        capsule.axis().vector());

            if (!std::isfinite(
                    normalized_projection))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Sphere-capsule projection "
                    "exceeds the finite physics "
                    "range.");
            }

            const PhysicsScalar
                absolute_projection =
                    std::abs(
                        normalized_projection);

            if (absolute_projection == 0.0)
            {
                return 0.0;
            }

            const PhysicsScalar
                half_segment_length =
                    capsule.
                        half_segment_length() *
                    0.5;

            const PhysicsScalar
                clamp_threshold =
                    half_segment_length /
                    absolute_projection;

            if (scale > clamp_threshold)
            {
                return
                    normalized_projection < 0.0
                        ? -capsule.
                            half_segment_length()
                        : capsule.
                            half_segment_length();
            }

            const PhysicsScalar
                half_projection =
                    scale *
                    normalized_projection;

            const PhysicsScalar
                segment_parameter =
                    std::clamp(
                        half_projection * 2.0,
                        -capsule.
                            half_segment_length(),
                        capsule.
                            half_segment_length());

            if (!std::isfinite(
                    segment_parameter))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Sphere-capsule segment "
                    "parameter exceeds the finite "
                    "physics range.");
            }

            return segment_parameter;
        }

        [[nodiscard]]
        foundation::Result<
            PhysicsVector3>
        closest_point_on_capsule_segment(
            const PhysicsVector3 point,
            const ColliderGeometry&
                capsule_geometry,
            const CapsuleShape& capsule)
        {
            const auto parameter_result =
                clamped_capsule_segment_parameter(
                    point,
                    capsule_geometry.center(),
                    capsule);

            if (!parameter_result.has_value())
            {
                return foundation::fail(
                    parameter_result.error().code,
                    parameter_result.error().message);
            }

            const PhysicsVector3 closest_point =
                capsule_geometry.center() +
                capsule.axis().vector() *
                    parameter_result.value();

            if (!closest_point.is_finite())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Closest capsule segment point "
                    "exceeds the finite physics "
                    "range.");
            }

            return closest_point;
        }

        [[nodiscard]]
        foundation::Result<
            PhysicsUnitVector3>
        deterministic_capsule_normal(
            const PhysicsVector3 direct_delta,
            const PhysicsVector3 scaled_delta,
            const CapsuleShape& capsule)
        {
            const PhysicsVector3 direction =
                direct_delta.is_finite()
                    ? direct_delta
                    : scaled_delta;

            if (direction !=
                physics_zero_vector)
            {
                const auto normalized_result =
                    direction.normalized(
                        0.0);

                if (!normalized_result.has_value())
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "Sphere-capsule contact "
                        "direction cannot be "
                        "normalized.");
                }

                const auto unit_result =
                    PhysicsUnitVector3::create(
                        normalized_result.value());

                if (!unit_result.has_value())
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "Sphere-capsule contact "
                        "normal cannot satisfy the "
                        "unit-vector contract.");
                }

                return unit_result.value();
            }

            if (capsule.half_segment_length() ==
                0.0)
            {
                return
                    PhysicsUnitVector3::
                        positive_x();
            }

            const PhysicsVector3& axis =
                capsule.axis().vector();

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

            const auto normal_result =
                PhysicsUnitVector3::create(
                    perpendicular_direction);

            if (!normal_result.has_value())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "A deterministic capsule "
                    "fallback normal could not be "
                    "created.");
            }

            return normal_result.value();
        }

        [[nodiscard]]
        foundation::Result<
            PhysicsUnitVector3>
        orient_sphere_capsule_normal(
            const BroadPhasePair& pair,
            const ColliderId sphere_collider,
            const PhysicsUnitVector3&
                sphere_to_capsule_normal)
        {
            const PhysicsVector3 direction =
                pair.first_collider() ==
                    sphere_collider
                    ? sphere_to_capsule_normal.
                        vector()
                    : -sphere_to_capsule_normal.
                        vector();

            const auto normal_result =
                PhysicsUnitVector3::create(
                    direction);

            if (!normal_result.has_value())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Canonical sphere-capsule "
                    "normal cannot satisfy the "
                    "unit-vector contract.");
            }

            return normal_result.value();
        }
    }

    foundation::Result<
        std::optional<CollisionContact>>
    generate_sphere_capsule_contact(
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
                "Sphere-capsule contact generation "
                "requires two distinct persistent "
                "collider identities.");
        }

        const bool first_is_sphere =
            std::holds_alternative<
                SphereShape>(
                first_geometry.shape());

        const bool second_is_sphere =
            std::holds_alternative<
                SphereShape>(
                second_geometry.shape());

        const bool first_is_capsule =
            std::holds_alternative<
                CapsuleShape>(
                first_geometry.shape());

        const bool second_is_capsule =
            std::holds_alternative<
                CapsuleShape>(
                second_geometry.shape());

        const bool valid_shape_pair =
            (first_is_sphere &&
                second_is_capsule) ||
            (first_is_capsule &&
                second_is_sphere);

        if (!valid_shape_pair)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Sphere-capsule contact generation "
                "requires exactly one sphere and "
                "one capsule.");
        }

        const ColliderGeometry*
            sphere_geometry =
                first_is_sphere
                    ? &first_geometry
                    : &second_geometry;

        const ColliderGeometry*
            capsule_geometry =
                first_is_capsule
                    ? &first_geometry
                    : &second_geometry;

        const SphereShape& sphere =
            std::get<SphereShape>(
                sphere_geometry->shape());

        const CapsuleShape& capsule =
            std::get<CapsuleShape>(
                capsule_geometry->shape());

        const PhysicsVector3 sphere_center =
            sphere_geometry->center();

        const auto closest_point_result =
            closest_point_on_capsule_segment(
                sphere_center,
                *capsule_geometry,
                capsule);

        if (!closest_point_result.has_value())
        {
            return foundation::fail(
                closest_point_result.error().code,
                closest_point_result.
                    error().
                    message);
        }

        const PhysicsVector3
            closest_segment_point =
                closest_point_result.value();

        const PhysicsVector3 direct_delta =
            sphere_center -
            closest_segment_point;

        const PhysicsVector3 scaled_delta =
            sphere_center * 0.5 -
            closest_segment_point * 0.5;

        const PhysicsScalar half_distance =
            scaled_delta.length();

        const PhysicsScalar
            half_combined_radius =
                sphere.radius() * 0.5 +
                capsule.radius() * 0.5;

        if (!std::isfinite(
                half_distance) ||
            half_distance >
                half_combined_radius)
        {
            return
                std::optional<
                    CollisionContact>{};
        }

        const auto capsule_to_sphere_result =
            deterministic_capsule_normal(
                direct_delta,
                scaled_delta,
                capsule);

        if (!capsule_to_sphere_result.has_value())
        {
            return foundation::fail(
                capsule_to_sphere_result.
                    error().
                    code,
                capsule_to_sphere_result.
                    error().
                    message);
        }

        const PhysicsUnitVector3
            capsule_to_sphere_normal =
                capsule_to_sphere_result.value();

        const PhysicsScalar
            half_penetration_depth =
                half_combined_radius -
                half_distance;

        const PhysicsScalar penetration_depth =
            half_penetration_depth *
            2.0;

        if (!std::isfinite(
                penetration_depth) ||
            penetration_depth <
                0.0)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Sphere-capsule penetration depth "
                "exceeds the finite physics "
                "range.");
        }

        const PhysicsVector3 capsule_surface =
            closest_segment_point +
            capsule_to_sphere_normal.
                vector() *
                capsule.radius();

        const PhysicsVector3 sphere_surface =
            sphere_center -
            capsule_to_sphere_normal.
                vector() *
                sphere.radius();

        if (!capsule_surface.is_finite() ||
            !sphere_surface.is_finite())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Sphere-capsule contact surface "
                "points exceed the finite physics "
                "range.");
        }

        const PhysicsVector3 contact_point =
            capsule_surface * 0.5 +
            sphere_surface * 0.5;

        if (!contact_point.is_finite())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Sphere-capsule contact point "
                "exceeds the finite physics "
                "range.");
        }

        const auto sphere_to_capsule_result =
            PhysicsUnitVector3::create(
                -capsule_to_sphere_normal.
                    vector());

        if (!sphere_to_capsule_result.has_value())
        {
            return foundation::fail(
                sphere_to_capsule_result.
                    error().
                    code,
                sphere_to_capsule_result.
                    error().
                    message);
        }

        const BroadPhasePair pair =
            pair_result.value();

        const auto canonical_normal_result =
            orient_sphere_capsule_normal(
                pair,
                sphere_geometry->collider(),
                sphere_to_capsule_result.value());

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