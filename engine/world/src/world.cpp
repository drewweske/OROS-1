#include "oros/world/world.hpp"

#include <new>
#include <utility>

namespace oros::world
{
    foundation::Result<World>
    World::create(
        const std::uint64_t world_namespace)
    {
        foundation::Result<EntityIdGenerator>
            generator_result =
                EntityIdGenerator::create(
                    world_namespace);

        if (!generator_result.has_value())
        {
            return foundation::fail(
                generator_result.error().code,
                generator_result.error().message);
        }

        return World{
            std::move(generator_result.value())
        };
    }

    World::World(
        EntityIdGenerator id_generator) noexcept
        : id_generator_{
            std::move(id_generator)
        }
    {
    }

    foundation::Result<EntityId>
    World::create_entity()
    {
        const std::size_t required_capacity =
            entities_.size() + 1U;

        try
        {
            entities_.reserve(
                required_capacity);
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "World could not reserve memory for "
                "another entity.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "World failed while reserving entity "
                "storage.");
        }

        foundation::Result<EntityId> id_result =
            id_generator_.generate();

        if (!id_result.has_value())
        {
            return foundation::fail(
                id_result.error().code,
                id_result.error().message);
        }

        const EntityId entity =
            id_result.value();

        try
        {
            const auto [iterator, inserted] =
                entities_.insert(entity);

            static_cast<void>(iterator);

            if (!inserted)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "World generated a duplicate entity "
                    "identity.");
            }
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "World could not store another entity.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "World failed while storing another "
                "entity.");
        }

        return entity;
    }

    foundation::Status
    World::destroy_entity(
        const EntityId entity)
    {
        if (!entity.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Entity destruction requires a valid "
                "entity identity.");
        }

        const auto entity_iterator =
            entities_.find(entity);

        if (entity_iterator == entities_.end())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    not_found,
                "The World does not contain this entity.");
        }

        if (positions_.contains(entity))
        {
            const foundation::Status position_status =
                positions_.remove(entity);

            if (!position_status.has_value())
            {
                return foundation::fail(
                    position_status.error().code,
                    position_status.error().message);
            }
        }

        entities_.erase(entity_iterator);

        return {};
    }

    bool
    World::contains(
        const EntityId entity) const noexcept
    {
        return
            entity.is_valid() &&
            entities_.contains(entity);
    }

    std::size_t
    World::entity_count() const noexcept
    {
        return entities_.size();
    }

    foundation::Status
    World::add_position(
        const EntityId entity,
        WorldPosition position)
    {
        if (!entity.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Position attachment requires a valid "
                "entity identity.");
        }

        if (!contains(entity))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    not_found,
                "A position can only be attached to an "
                "entity owned by this World.");
        }

        return positions_.insert(
            entity,
            std::move(position));
    }

    foundation::Status
    World::remove_position(
        const EntityId entity)
    {
        if (!entity.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Position removal requires a valid "
                "entity identity.");
        }

        if (!contains(entity))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    not_found,
                "A position can only be removed from an "
                "entity owned by this World.");
        }

        return positions_.remove(entity);
    }

    WorldPosition*
    World::find_position(
        const EntityId entity) noexcept
    {
        if (!contains(entity))
        {
            return nullptr;
        }

        return positions_.find(entity);
    }

    const WorldPosition*
    World::find_position(
        const EntityId entity) const noexcept
    {
        if (!contains(entity))
        {
            return nullptr;
        }

        return positions_.find(entity);
    }

    std::size_t
    World::position_count() const noexcept
    {
        return positions_.size();
    }

    const ComponentStore<WorldPosition>&
    World::positions() const noexcept
    {
        return positions_;
    }

    std::uint64_t
    World::world_namespace() const noexcept
    {
        return id_generator_.world_namespace();
    }
}