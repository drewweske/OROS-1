#include "oros/physics/narrow_phase.hpp"

#include <optional>
#include <variant>

namespace oros::physics
{
    foundation::Result<
        std::optional<CollisionContact>>
    generate_collision_contact(
        const ColliderGeometry& first_geometry,
        const ColliderGeometry& second_geometry)
    {
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

        const bool first_is_capsule =
            std::holds_alternative<
                CapsuleShape>(
                first_geometry.shape());

        const bool second_is_capsule =
            std::holds_alternative<
                CapsuleShape>(
                second_geometry.shape());

        if (first_is_sphere &&
            second_is_sphere)
        {
            return generate_sphere_sphere_contact(
                first_geometry,
                second_geometry);
        }

        if ((first_is_sphere &&
                second_is_box) ||
            (first_is_box &&
                second_is_sphere))
        {
            return generate_sphere_box_contact(
                first_geometry,
                second_geometry);
        }

        if (first_is_box &&
            second_is_box)
        {
            return generate_box_box_contact(
                first_geometry,
                second_geometry);
        }

        if ((first_is_sphere &&
                second_is_capsule) ||
            (first_is_capsule &&
                second_is_sphere))
        {
            return generate_sphere_capsule_contact(
                first_geometry,
                second_geometry);
        }

        if ((first_is_capsule &&
                second_is_box) ||
            (first_is_box &&
                second_is_capsule))
        {
            return generate_capsule_box_contact(
                first_geometry,
                second_geometry);
        }

        if (first_is_capsule &&
            second_is_capsule)
        {
            return generate_capsule_capsule_contact(
                first_geometry,
                second_geometry);
        }

        return foundation::fail(
            foundation::ErrorCode::
                invalid_argument,
            "Collision contact generation does "
            "not support this shape pair.");
    }
}