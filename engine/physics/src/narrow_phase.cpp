#include "oros/physics/narrow_phase.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
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

        enum class BoxContactAxis
        {
            x,
            y,
            z
        };

        struct BoxOverlapSelection final
        {
            BoxContactAxis axis{
                BoxContactAxis::x
            };

            PhysicsScalar half_overlap{};
            PhysicsVector3 normal{};
        };

        [[nodiscard]]
        foundation::Result<
            std::optional<BoxOverlapSelection>>
        select_box_overlap(
            const BoxShape& first_box,
            const PhysicsVector3 first_center,
            const BoxShape& second_box,
            const PhysicsVector3 second_center)
        {
            const PhysicsVector3
                first_half_extents =
                    first_box.half_extents();

            const PhysicsVector3
                second_half_extents =
                    second_box.half_extents();

            const PhysicsVector3
                half_center_delta =
                    second_center * 0.5 -
                    first_center * 0.5;

            if (!half_center_delta.is_finite())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Box center separation exceeds "
                    "the finite physics range.");
            }

            const PhysicsScalar
                half_overlap_x =
                    first_half_extents.x * 0.5 +
                    second_half_extents.x * 0.5 -
                    std::abs(
                        half_center_delta.x);

            const PhysicsScalar
                half_overlap_y =
                    first_half_extents.y * 0.5 +
                    second_half_extents.y * 0.5 -
                    std::abs(
                        half_center_delta.y);

            const PhysicsScalar
                half_overlap_z =
                    first_half_extents.z * 0.5 +
                    second_half_extents.z * 0.5 -
                    std::abs(
                        half_center_delta.z);

            if (!std::isfinite(
                    half_overlap_x) ||
                !std::isfinite(
                    half_overlap_y) ||
                !std::isfinite(
                    half_overlap_z))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Box overlap calculation exceeds "
                    "the finite physics range.");
            }

            if (half_overlap_x < 0.0 ||
                half_overlap_y < 0.0 ||
                half_overlap_z < 0.0)
            {
                return std::optional<
                    BoxOverlapSelection>{};
            }

            BoxOverlapSelection selection{
                BoxContactAxis::x,
                half_overlap_x,
                half_center_delta.x < 0.0
                    ? -physics_positive_x
                    : physics_positive_x
            };

            if (half_overlap_y <
                selection.half_overlap)
            {
                selection =
                    BoxOverlapSelection{
                        BoxContactAxis::y,
                        half_overlap_y,
                        half_center_delta.y < 0.0
                            ? -physics_positive_y
                            : physics_positive_y
                    };
            }

            if (half_overlap_z <
                selection.half_overlap)
            {
                selection =
                    BoxOverlapSelection{
                        BoxContactAxis::z,
                        half_overlap_z,
                        half_center_delta.z < 0.0
                            ? -physics_positive_z
                            : physics_positive_z
                    };
            }

            return std::optional<
                BoxOverlapSelection>{
                    selection
                };
        }

        [[nodiscard]]
        foundation::Result<
            PhysicsVector3>
        box_contact_point(
            const AxisAlignedBounds& first_bounds,
            const AxisAlignedBounds& second_bounds,
            const BoxOverlapSelection& selection)
        {
            const PhysicsVector3 first_minimum =
                first_bounds.minimum();

            const PhysicsVector3 first_maximum =
                first_bounds.maximum();

            const PhysicsVector3 second_minimum =
                second_bounds.minimum();

            const PhysicsVector3 second_maximum =
                second_bounds.maximum();

            const PhysicsVector3
                intersection_minimum{
                    std::max(
                        first_minimum.x,
                        second_minimum.x),
                    std::max(
                        first_minimum.y,
                        second_minimum.y),
                    std::max(
                        first_minimum.z,
                        second_minimum.z)
                };

            const PhysicsVector3
                intersection_maximum{
                    std::min(
                        first_maximum.x,
                        second_maximum.x),
                    std::min(
                        first_maximum.y,
                        second_maximum.y),
                    std::min(
                        first_maximum.z,
                        second_maximum.z)
                };

            PhysicsVector3 point{
                std::midpoint(
                    intersection_minimum.x,
                    intersection_maximum.x),
                std::midpoint(
                    intersection_minimum.y,
                    intersection_maximum.y),
                std::midpoint(
                    intersection_minimum.z,
                    intersection_maximum.z)
            };

            switch (selection.axis)
            {
            case BoxContactAxis::x:
            {
                const PhysicsScalar first_surface =
                    selection.normal.x > 0.0
                        ? first_maximum.x
                        : first_minimum.x;

                const PhysicsScalar second_surface =
                    selection.normal.x > 0.0
                        ? second_minimum.x
                        : second_maximum.x;

                point.x =
                    std::midpoint(
                        first_surface,
                        second_surface);

                break;
            }

            case BoxContactAxis::y:
            {
                const PhysicsScalar first_surface =
                    selection.normal.y > 0.0
                        ? first_maximum.y
                        : first_minimum.y;

                const PhysicsScalar second_surface =
                    selection.normal.y > 0.0
                        ? second_minimum.y
                        : second_maximum.y;

                point.y =
                    std::midpoint(
                        first_surface,
                        second_surface);

                break;
            }

            case BoxContactAxis::z:
            {
                const PhysicsScalar first_surface =
                    selection.normal.z > 0.0
                        ? first_maximum.z
                        : first_minimum.z;

                const PhysicsScalar second_surface =
                    selection.normal.z > 0.0
                        ? second_minimum.z
                        : second_maximum.z;

                point.z =
                    std::midpoint(
                        first_surface,
                        second_surface);

                break;
            }
            }

            if (!point.is_finite())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Box contact point exceeds the "
                    "finite physics range.");
            }

            return point;
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

    foundation::Result<
        std::optional<CollisionContact>>
    generate_box_box_contact(
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
                "Box contact generation requires "
                "two distinct persistent collider "
                "identities.");
        }

        if (!std::holds_alternative<
                BoxShape>(
                first_geometry.shape()) ||
            !std::holds_alternative<
                BoxShape>(
                second_geometry.shape()))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Box contact generation requires "
                "two box shapes.");
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

        const BoxShape& first_box =
            std::get<BoxShape>(
                canonical_first->shape());

        const BoxShape& second_box =
            std::get<BoxShape>(
                canonical_second->shape());

        const auto overlap_result =
            select_box_overlap(
                first_box,
                canonical_first->center(),
                second_box,
                canonical_second->center());

        if (!overlap_result.has_value())
        {
            return foundation::fail(
                overlap_result.error().code,
                overlap_result.error().message);
        }

        if (!overlap_result.value().has_value())
        {
            return
                std::optional<
                    CollisionContact>{};
        }

        const BoxOverlapSelection selection =
            overlap_result.value().value();

        const PhysicsScalar penetration_depth =
            selection.half_overlap *
            2.0;

        if (!std::isfinite(
                penetration_depth) ||
            penetration_depth <
                0.0)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Box contact penetration depth "
                "exceeds the finite physics "
                "range.");
        }

        const auto first_bounds_result =
            canonical_first->bounds();

        const auto second_bounds_result =
            canonical_second->bounds();

        if (!first_bounds_result.has_value())
        {
            return foundation::fail(
                first_bounds_result.error().code,
                first_bounds_result.error().message);
        }

        if (!second_bounds_result.has_value())
        {
            return foundation::fail(
                second_bounds_result.error().code,
                second_bounds_result.error().message);
        }

        const auto point_result =
            box_contact_point(
                first_bounds_result.value(),
                second_bounds_result.value(),
                selection);

        if (!point_result.has_value())
        {
            return foundation::fail(
                point_result.error().code,
                point_result.error().message);
        }

        const auto normal_result =
            PhysicsUnitVector3::create(
                selection.normal);

        if (!normal_result.has_value())
        {
            return foundation::fail(
                normal_result.error().code,
                normal_result.error().message);
        }

        return create_optional_contact(
            pair,
            point_result.value(),
            normal_result.value(),
            penetration_depth);
    }
}