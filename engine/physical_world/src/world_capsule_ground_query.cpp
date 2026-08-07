#include "oros/physical_world/world_capsule_ground_query.hpp"

#include "oros/physical_world/world_capsule_contact_query.hpp"
#include "oros/physics/physics_vector.hpp"

#include "world_capsule_contact_normal.hpp"

#include <cmath>
#include <optional>
#include <vector>

namespace oros::physical_world
{
    namespace
    {
        inline constexpr
            physics::PhysicsScalar
            capsule_ground_support_probe_distance{
                physics::
                    physics_vector_zero_tolerance
            };

        [[nodiscard]]
        foundation::Result<
            std::optional<
                physics::CollisionContact>>
        select_walkable_ground_contact(
            const std::vector<
                physics::CollisionContact>&
                contacts,
            const physics::ColliderId
                capsule_collider,
            const WorldCapsuleTraversalSettings&
                traversal_settings)
        {
            const physics::CollisionContact*
                best_contact{};

            physics::PhysicsScalar
                best_up_dot{};

            for (const physics::CollisionContact&
                     contact :
                 contacts)
            {
                const auto outward_normal_result =
                    detail::
                        capsule_outward_contact_normal(
                            contact,
                            capsule_collider);

                if (!outward_normal_result.has_value())
                {
                    return foundation::fail(
                        outward_normal_result.
                            error().
                            code,
                        outward_normal_result.
                            error().
                            message);
                }

                const physics::PhysicsScalar
                    up_dot =
                        physics::dot(
                            outward_normal_result.
                                value(),
                            traversal_settings.
                                up_direction().
                                vector());

                if (!std::isfinite(up_dot))
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            internal_failure,
                        "Capsule ground query produced "
                        "a non-finite ground normal "
                        "alignment.");
                }

                if (up_dot <
                    traversal_settings.
                        minimum_walkable_up_dot())
                {
                    continue;
                }

                if (best_contact == nullptr)
                {
                    best_contact =
                        &contact;

                    best_up_dot =
                        up_dot;

                    continue;
                }

                if (up_dot > best_up_dot)
                {
                    best_contact =
                        &contact;

                    best_up_dot =
                        up_dot;

                    continue;
                }

                if (up_dot == best_up_dot &&
                    contact.penetration_depth() >
                        best_contact->
                            penetration_depth())
                {
                    best_contact =
                        &contact;

                    best_up_dot =
                        up_dot;
                }
            }

            if (best_contact == nullptr)
            {
                return std::optional<
                    physics::CollisionContact>{};
            }

            return std::optional<
                physics::CollisionContact>{
                    *best_contact
                };
        }
    }

    foundation::Result<
        std::optional<
            physics::CollisionContact>>
    query_world_capsule_ground_contact(
        const WorldCellColliderRegistry& registry,
        const std::uint64_t world_namespace,
        const physics::ColliderId capsule_collider,
        const physics::CapsuleShape& capsule_shape,
        const world::WorldPosition&
            capsule_center,
        const WorldCapsuleTraversalSettings&
            traversal_settings)
    {
        const auto contacts_result =
            query_world_capsule_contacts(
                registry,
                world_namespace,
                capsule_collider,
                capsule_shape,
                capsule_center);

        if (!contacts_result.has_value())
        {
            return foundation::fail(
                contacts_result.error().code,
                contacts_result.error().message);
        }

        const auto exact_ground_result =
            select_walkable_ground_contact(
                contacts_result.value(),
                capsule_collider,
                traversal_settings);

        if (!exact_ground_result.has_value())
        {
            return foundation::fail(
                exact_ground_result.error().code,
                exact_ground_result.error().message);
        }

        if (exact_ground_result.
                value().
                has_value())
        {
            return exact_ground_result.value();
        }

        const physics::PhysicsVector3
            support_probe_displacement =
                traversal_settings.
                    up_direction().
                    vector() *
                -capsule_ground_support_probe_distance;

        const auto support_probe_center_result =
            capsule_center.translated(
                world::WorldDisplacement{
                    support_probe_displacement.x,
                    support_probe_displacement.y,
                    support_probe_displacement.z
                });

        if (!support_probe_center_result.has_value())
        {
            return foundation::fail(
                support_probe_center_result.
                    error().
                    code,
                support_probe_center_result.
                    error().
                    message);
        }

        const auto support_contacts_result =
            query_world_capsule_contacts(
                registry,
                world_namespace,
                capsule_collider,
                capsule_shape,
                support_probe_center_result.value());

        if (!support_contacts_result.has_value())
        {
            return foundation::fail(
                support_contacts_result.error().code,
                support_contacts_result.error().message);
        }

        const auto support_ground_result =
            select_walkable_ground_contact(
                support_contacts_result.value(),
                capsule_collider,
                traversal_settings);

        if (!support_ground_result.has_value())
        {
            return foundation::fail(
                support_ground_result.error().code,
                support_ground_result.error().message);
        }

        return support_ground_result.value();
    }
}