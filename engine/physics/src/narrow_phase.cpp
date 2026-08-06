#include "oros/physics/narrow_phase.hpp"

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
                    "Sphere contact direction "
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
                    "Sphere contact normal "
                    "cannot satisfy the unit-vector "
                    "contract.");
            }

            return unit_result.value();
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