#include "oros/physics/narrow_phase.hpp"

#include <algorithm>
#include <cmath>
#include <optional>
#include <utility>
#include <variant>

namespace oros::physics
{
    namespace
    {
        [[nodiscard]]
        foundation::Result<
            PhysicsUnitVector3>
        contact_normal_from_delta(
            const PhysicsVector3 direct_delta,
            const PhysicsVector3 scaled_delta)
        {
            PhysicsVector3 direction =
                direct_delta.is_finite()
                    ? direct_delta
                    : scaled_delta;

            if (direction ==
                physics_zero_vector)
            {
                return
                    PhysicsUnitVector3::
                        positive_x();
            }

            const auto normalized_result =
                direction.normalized(
                    0.0);

            if (!normalized_result.has_value())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Contact direction cannot be "
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
                    "Contact normal cannot satisfy "
                    "the unit-vector contract.");
            }

            return unit_result.value();
        }

        [[nodiscard]]
        foundation::Result<
            PhysicsUnitVector3>
        orient_contact_normal(
            const BroadPhasePair& pair,
            const ColliderId source_collider,
            const PhysicsUnitVector3&
                source_to_target_normal)
        {
            const PhysicsVector3 direction =
                pair.first_collider() ==
                    source_collider
                    ? source_to_target_normal.
                        vector()
                    : -source_to_target_normal.
                        vector();

            const auto normal_result =
                PhysicsUnitVector3::create(
                    direction);

            if (!normal_result.has_value())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Canonical contact normal cannot "
                    "satisfy the unit-vector "
                    "contract.");
            }

            return normal_result.value();
        }

        [[nodiscard]]
        foundation::Result<
            std::optional<CollisionContact>>
        create_optional_contact(
            const BroadPhasePair& pair,
            const PhysicsVector3 point,
            const PhysicsUnitVector3 normal,
            const PhysicsScalar
                penetration_depth)
        {
            const auto contact_result =
                CollisionContact::create(
                    pair,
                    point,
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

        struct BoxFaceSelection final
        {
            PhysicsScalar half_distance{};
            PhysicsVector3 outward_normal{};
            PhysicsVector3 surface_point{};
        };

        [[nodiscard]]
        foundation::Result<
            BoxFaceSelection>
        nearest_box_face(
            const AxisAlignedBounds& bounds,
            const PhysicsVector3 point)
        {
            const PhysicsVector3 minimum =
                bounds.minimum();

            const PhysicsVector3 maximum =
                bounds.maximum();

            BoxFaceSelection selection{
                maximum.x * 0.5 -
                    point.x * 0.5,
                physics_positive_x,
                PhysicsVector3{
                    maximum.x,
                    point.y,
                    point.z
                }
            };

            const auto consider_face =
                [&selection](
                    const PhysicsScalar
                        half_distance,
                    const PhysicsVector3
                        outward_normal,
                    const PhysicsVector3
                        surface_point)
                {
                    if (half_distance <
                        selection.half_distance)
                    {
                        selection =
                            BoxFaceSelection{
                                half_distance,
                                outward_normal,
                                surface_point
                            };
                    }
                };

            consider_face(
                point.x * 0.5 -
                    minimum.x * 0.5,
                -physics_positive_x,
                PhysicsVector3{
                    minimum.x,
                    point.y,
                    point.z
                });

            consider_face(
                maximum.y * 0.5 -
                    point.y * 0.5,
                physics_positive_y,
                PhysicsVector3{
                    point.x,
                    maximum.y,
                    point.z
                });

            consider_face(
                point.y * 0.5 -
                    minimum.y * 0.5,
                -physics_positive_y,
                PhysicsVector3{
                    point.x,
                    minimum.y,
                    point.z
                });

            consider_face(
                maximum.z * 0.5 -
                    point.z * 0.5,
                physics_positive_z,
                PhysicsVector3{
                    point.x,
                    point.y,
                    maximum.z
                });

            consider_face(
                point.z * 0.5 -
                    minimum.z * 0.5,
                -physics_positive_z,
                PhysicsVector3{
                    point.x,
                    point.y,
                    minimum.z
                });

            if (!std::isfinite(
                    selection.half_distance) ||
                selection.half_distance <
                    0.0 ||
                !selection.outward_normal.
                    is_finite() ||
                !selection.surface_point.
                    is_finite())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "The nearest box face cannot be "
                    "represented safely.");
            }

            return selection;
        }
    }

    foundation::Result<
        std::optional<CollisionContact>>
    generate_sphere_sphere_contact(
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
                "Sphere contact generation "
                "requires two distinct persistent "
                "collider identities.");
        }

        if (!std::holds_alternative<
                SphereShape>(
                first_geometry.shape()) ||
            !std::holds_alternative<
                SphereShape>(
                second_geometry.shape()))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Sphere contact generation "
                "requires two sphere shapes.");
        }

        const BroadPhasePair pair =
            pair_result.value();

        const ColliderGeometry*
            canonical_first =
                &first_geometry;

        const ColliderGeometry*
            canonical_second =
                &second_geometry;

        if (canonical_first->collider() !=
            pair.first_collider())
        {
            std::swap(
                canonical_first,
                canonical_second);
        }

        const SphereShape& first_sphere =
            std::get<SphereShape>(
                canonical_first->shape());

        const SphereShape& second_sphere =
            std::get<SphereShape>(
                canonical_second->shape());

        const PhysicsVector3 first_center =
            canonical_first->center();

        const PhysicsVector3 second_center =
            canonical_second->center();

        const PhysicsVector3 direct_delta =
            second_center -
            first_center;

        const PhysicsVector3 scaled_delta =
            second_center * 0.5 -
            first_center * 0.5;

        const PhysicsScalar half_distance =
            scaled_delta.length();

        const PhysicsScalar
            half_combined_radius =
                first_sphere.radius() * 0.5 +
                second_sphere.radius() * 0.5;

        if (!std::isfinite(
                half_distance) ||
            half_distance >
                half_combined_radius)
        {
            return
                std::optional<
                    CollisionContact>{};
        }

        const auto normal_result =
            contact_normal_from_delta(
                direct_delta,
                scaled_delta);

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
                half_distance;

        const PhysicsScalar penetration_depth =
            half_penetration_depth *
            2.0;

        if (!std::isfinite(
                penetration_depth))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Sphere contact penetration "
                "depth exceeds the finite physics "
                "range.");
        }

        const PhysicsVector3 first_surface =
            first_center +
            normal.vector() *
                first_sphere.radius();

        const PhysicsVector3 second_surface =
            second_center -
            normal.vector() *
                second_sphere.radius();

        if (!first_surface.is_finite() ||
            !second_surface.is_finite())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Sphere contact surface points "
                "exceed the finite physics range.");
        }

        const PhysicsVector3 contact_point =
            first_surface * 0.5 +
            second_surface * 0.5;

        return create_optional_contact(
            pair,
            contact_point,
            normal,
            penetration_depth);
    }

    foundation::Result<
        std::optional<CollisionContact>>
    generate_sphere_box_contact(
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
                "Sphere-box contact generation "
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

        const bool first_is_box =
            std::holds_alternative<
                BoxShape>(
                first_geometry.shape());

        const bool second_is_box =
            std::holds_alternative<
                BoxShape>(
                second_geometry.shape());

        const bool valid_shape_pair =
            (first_is_sphere &&
                second_is_box) ||
            (first_is_box &&
                second_is_sphere);

        if (!valid_shape_pair)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Sphere-box contact generation "
                "requires exactly one sphere and "
                "one box.");
        }

        const ColliderGeometry*
            sphere_geometry =
                first_is_sphere
                    ? &first_geometry
                    : &second_geometry;

        const ColliderGeometry*
            box_geometry =
                first_is_box
                    ? &first_geometry
                    : &second_geometry;

        const SphereShape& sphere =
            std::get<SphereShape>(
                sphere_geometry->shape());

        const auto bounds_result =
            box_geometry->bounds();

        if (!bounds_result.has_value())
        {
            return foundation::fail(
                bounds_result.error().code,
                bounds_result.error().message);
        }

        const AxisAlignedBounds box_bounds =
            bounds_result.value();

        const PhysicsVector3 minimum =
            box_bounds.minimum();

        const PhysicsVector3 maximum =
            box_bounds.maximum();

        const PhysicsVector3 sphere_center =
            sphere_geometry->center();

        const PhysicsVector3 closest_point{
            std::clamp(
                sphere_center.x,
                minimum.x,
                maximum.x),
            std::clamp(
                sphere_center.y,
                minimum.y,
                maximum.y),
            std::clamp(
                sphere_center.z,
                minimum.z,
                maximum.z)
        };

        const BroadPhasePair pair =
            pair_result.value();

        PhysicsUnitVector3
            box_to_sphere_normal =
                PhysicsUnitVector3::
                    positive_x();

        PhysicsVector3 box_surface =
            closest_point;

        PhysicsVector3 sphere_surface{};

        PhysicsScalar penetration_depth{};

        if (!box_bounds.contains(
                sphere_center))
        {
            const PhysicsVector3
                direct_delta =
                    sphere_center -
                    closest_point;

            const PhysicsVector3
                scaled_delta =
                    sphere_center * 0.5 -
                    closest_point * 0.5;

            const PhysicsScalar half_distance =
                scaled_delta.length();

            const PhysicsScalar half_radius =
                sphere.radius() * 0.5;

            if (!std::isfinite(
                    half_distance) ||
                half_distance >
                    half_radius)
            {
                return
                    std::optional<
                        CollisionContact>{};
            }

            const auto normal_result =
                contact_normal_from_delta(
                    direct_delta,
                    scaled_delta);

            if (!normal_result.has_value())
            {
                return foundation::fail(
                    normal_result.error().code,
                    normal_result.error().message);
            }

            box_to_sphere_normal =
                normal_result.value();

            const PhysicsScalar
                half_penetration_depth =
                    half_radius -
                    half_distance;

            penetration_depth =
                half_penetration_depth *
                2.0;

            sphere_surface =
                sphere_center -
                box_to_sphere_normal.
                    vector() *
                    sphere.radius();
        }
        else
        {
            const auto face_result =
                nearest_box_face(
                    box_bounds,
                    sphere_center);

            if (!face_result.has_value())
            {
                return foundation::fail(
                    face_result.error().code,
                    face_result.error().message);
            }

            const BoxFaceSelection face =
                face_result.value();

            const auto normal_result =
                PhysicsUnitVector3::create(
                    face.outward_normal);

            if (!normal_result.has_value())
            {
                return foundation::fail(
                    normal_result.error().code,
                    normal_result.error().message);
            }

            box_to_sphere_normal =
                normal_result.value();

            box_surface =
                face.surface_point;

            const PhysicsScalar
                half_penetration_depth =
                    sphere.radius() * 0.5 +
                    face.half_distance;

            penetration_depth =
                half_penetration_depth *
                2.0;

            sphere_surface =
                sphere_center +
                box_to_sphere_normal.
                    vector() *
                    sphere.radius();
        }

        if (!std::isfinite(
                penetration_depth) ||
            penetration_depth <
                0.0)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Sphere-box penetration depth "
                "exceeds the finite physics "
                "range.");
        }

        if (!sphere_surface.is_finite() ||
            !box_surface.is_finite())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Sphere-box contact surface "
                "points exceed the finite physics "
                "range.");
        }

        const PhysicsVector3 contact_point =
            sphere_surface * 0.5 +
            box_surface * 0.5;

        if (!contact_point.is_finite())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Sphere-box contact point exceeds "
                "the finite physics range.");
        }

        const auto sphere_to_box_result =
            PhysicsUnitVector3::create(
                -box_to_sphere_normal.
                    vector());

        if (!sphere_to_box_result.has_value())
        {
            return foundation::fail(
                sphere_to_box_result.error().code,
                sphere_to_box_result.error().message);
        }

        const auto canonical_normal_result =
            orient_contact_normal(
                pair,
                sphere_geometry->collider(),
                sphere_to_box_result.value());

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

        return create_optional_contact(
            pair,
            contact_point,
            canonical_normal_result.value(),
            penetration_depth);
    }
}