#include "oros/ai/actor_simulation_focus_source_registry.hpp"

#include <algorithm>
#include <cstddef>
#include <new>
#include <utility>

namespace oros::ai
{
    world::EntityId
    ActorSimulationFocusSource::
        source_entity()
        const noexcept
    {
        return source_entity_;
    }

    const world::WorldPosition&
    ActorSimulationFocusSource::
        position()
        const noexcept
    {
        return position_;
    }

    ActorSimulationFocusSource::
        ActorSimulationFocusSource(
            const world::EntityId source_entity,
            world::WorldPosition position)
            noexcept
        : source_entity_{source_entity},
          position_{std::move(position)}
    {
    }

    foundation::Status
    ActorSimulationFocusSourceRegistry::
        insert(
            const world::EntityId source_entity,
            world::WorldPosition position)
    {
        if (!source_entity.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor simulation focus source "
                "requires a valid World entity "
                "identity.");
        }

        if (!position.is_normalized())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor simulation focus source "
                "requires a normalized "
                "WorldPosition.");
        }

        const std::size_t index =
            lower_bound_index(
                source_entity);

        if (
            index < sources_.size() &&
            sources_[index].source_entity() ==
                source_entity)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Actor simulation focus source "
                "registry already contains this "
                "World entity.");
        }

        using difference_type =
            std::vector<
                ActorSimulationFocusSource>::
                    difference_type;

        try
        {
            sources_.insert(
                sources_.begin() +
                    static_cast<difference_type>(
                        index),
                ActorSimulationFocusSource{
                    source_entity,
                    std::move(position)
                });
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Actor simulation focus source "
                "registry could not allocate "
                "storage.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "Actor simulation focus source "
                "registry failed while inserting "
                "a source.");
        }

        return {};
    }

    foundation::Status
    ActorSimulationFocusSourceRegistry::
        update_position(
            const world::EntityId source_entity,
            world::WorldPosition position)
    {
        if (!source_entity.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor simulation focus source "
                "position update requires a valid "
                "World entity identity.");
        }

        if (!position.is_normalized())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor simulation focus source "
                "position update requires a "
                "normalized WorldPosition.");
        }

        const std::size_t index =
            lower_bound_index(
                source_entity);

        if (
            index >= sources_.size() ||
            sources_[index].source_entity() !=
                source_entity)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    not_found,
                "Actor simulation focus source "
                "registry does not contain this "
                "World entity.");
        }

        sources_[index].position_ =
            std::move(position);

        return {};
    }

    foundation::Status
    ActorSimulationFocusSourceRegistry::
        remove(
            const world::EntityId source_entity)
    {
        if (!source_entity.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor simulation focus source "
                "removal requires a valid World "
                "entity identity.");
        }

        const std::size_t index =
            lower_bound_index(
                source_entity);

        if (
            index >= sources_.size() ||
            sources_[index].source_entity() !=
                source_entity)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    not_found,
                "Actor simulation focus source "
                "registry does not contain this "
                "World entity.");
        }

        using difference_type =
            std::vector<
                ActorSimulationFocusSource>::
                    difference_type;

        sources_.erase(
            sources_.begin() +
                static_cast<difference_type>(
                    index));

        return {};
    }

    foundation::Result<
        world::WorldPosition>
    ActorSimulationFocusSourceRegistry::
        position(
            const world::EntityId source_entity)
        const
    {
        if (!source_entity.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor simulation focus source "
                "lookup requires a valid World "
                "entity identity.");
        }

        const std::size_t index =
            lower_bound_index(
                source_entity);

        if (
            index >= sources_.size() ||
            sources_[index].source_entity() !=
                source_entity)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    not_found,
                "Actor simulation focus source "
                "registry does not contain this "
                "World entity.");
        }

        return sources_[index].position();
    }

    bool
    ActorSimulationFocusSourceRegistry::
        contains(
            const world::EntityId source_entity)
        const noexcept
    {
        if (!source_entity.is_valid())
        {
            return false;
        }

        const std::size_t index =
            lower_bound_index(
                source_entity);

        return
            index < sources_.size() &&
            sources_[index].source_entity() ==
                source_entity;
    }

    std::size_t
    ActorSimulationFocusSourceRegistry::
        size()
        const noexcept
    {
        return sources_.size();
    }

    bool
    ActorSimulationFocusSourceRegistry::
        empty()
        const noexcept
    {
        return sources_.empty();
    }

    std::span<
        const ActorSimulationFocusSource>
    ActorSimulationFocusSourceRegistry::
        sources_in_entity_order()
        const noexcept
    {
        return sources_;
    }

    std::size_t
    ActorSimulationFocusSourceRegistry::
        lower_bound_index(
            const world::EntityId source_entity)
        const noexcept
    {
        const auto iterator =
            std::lower_bound(
                sources_.begin(),
                sources_.end(),
                source_entity,
                [](
                    const ActorSimulationFocusSource&
                        source,
                    const world::EntityId candidate)
                    noexcept
                {
                    return
                        source.source_entity() <
                        candidate;
                });

        return static_cast<std::size_t>(
            iterator - sources_.begin());
    }
}