#include "oros/physical_world/world_capsule_contact_query.hpp"

#include "oros/physics/collider_geometry.hpp"
#include "oros/physics/narrow_phase.hpp"
#include "oros/physics/physics_vector.hpp"

#include <algorithm>
#include <new>
#include <vector>

namespace oros::physical_world
{
    foundation::Result<
        std::vector<
            physics::CollisionContact>>
    query_world_capsule_contacts(
        const WorldCellColliderRegistry& registry,
        const std::uint64_t world_namespace,
        const physics::ColliderId capsule_collider,
        const physics::CapsuleShape& capsule_shape,
        const world::WorldPosition&
            capsule_center)
    {
        if (world_namespace == 0ULL)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World capsule contact queries "
                "require a valid world namespace.");
        }

        if (!capsule_collider.is_valid() ||
            capsule_collider.
                owner.
                world_namespace !=
                world_namespace)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World capsule contact queries "
                "require a valid capsule collider "
                "identity in the requested world "
                "namespace.");
        }

        if (registry.contains(
                capsule_collider))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World capsule contact query "
                "identity must not duplicate an "
                "active world collider identity.");
        }

        const physics::CollisionShape
            collision_shape{
                capsule_shape
            };

        const auto capsule_geometry_result =
            physics::ColliderGeometry::create(
                capsule_collider,
                collision_shape,
                physics::physics_zero_vector);

        if (!capsule_geometry_result.has_value())
        {
            return foundation::fail(
                capsule_geometry_result.
                    error().
                    code,
                capsule_geometry_result.
                    error().
                    message);
        }

        const auto relative_colliders_result =
            registry.colliders_relative_to(
                world_namespace,
                capsule_center);

        if (!relative_colliders_result.has_value())
        {
            return foundation::fail(
                relative_colliders_result.
                    error().
                    code,
                relative_colliders_result.
                    error().
                    message);
        }

        try
        {
            std::vector<
                physics::CollisionContact>
                contacts;

            for (const physics::
                     ColliderGeometry&
                     world_geometry :
                 relative_colliders_result.
                     value())
            {
                const auto contact_result =
                    physics::
                        generate_collision_contact(
                            capsule_geometry_result.
                                value(),
                            world_geometry);

                if (!contact_result.has_value())
                {
                    return foundation::fail(
                        contact_result.
                            error().
                            code,
                        contact_result.
                            error().
                            message);
                }

                if (!contact_result.
                        value().
                        has_value())
                {
                    continue;
                }

                contacts.push_back(
                    contact_result.
                        value().
                        value());
            }

            std::sort(
                contacts.begin(),
                contacts.end(),
                [](
                    const physics::
                        CollisionContact& left,
                    const physics::
                        CollisionContact& right)
                {
                    return
                        left.pair() <
                        right.pair();
                });

            return contacts;
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Unable to allocate world capsule "
                "contact query storage.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "An unexpected failure occurred "
                "while querying world capsule "
                "contacts.");
        }
    }
}