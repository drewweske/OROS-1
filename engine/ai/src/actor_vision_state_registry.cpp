#include "oros/ai/actor_vision_state_registry.hpp"

#include <algorithm>
#include <cstddef>
#include <new>
#include <utility>

namespace oros::ai
{
    foundation::Status
    ActorVisionStateRegistry::insert(ActorVisionState state)
    {
        if (!state.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::invalid_argument,
                "Actor vision registry requires a valid actor state.");
        }

        const world::EntityId actor = state.actor();
        const std::size_t index = lower_bound_index(actor);

        if (
            index < states_.size() &&
            states_[index].actor() == actor)
        {
            return foundation::fail(
                foundation::ErrorCode::invalid_state,
                "Actor vision registry already contains this actor.");
        }

        using difference_type =
            std::vector<ActorVisionState>::difference_type;

        try
        {
            states_.insert(
                states_.begin() +
                    static_cast<difference_type>(index),
                std::move(state));
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::out_of_memory,
                "Actor vision registry could not allocate storage.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::internal_failure,
                "Actor vision registry insertion failed.");
        }

        return {};
    }

    foundation::Status
    ActorVisionStateRegistry::remove(
        const world::EntityId actor)
    {
        if (!actor.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::invalid_argument,
                "Actor vision removal needs a valid EntityId.");
        }

        const std::size_t index = lower_bound_index(actor);

        if (
            index >= states_.size() ||
            states_[index].actor() != actor)
        {
            return foundation::fail(
                foundation::ErrorCode::not_found,
                "Actor vision registry does not contain this actor.");
        }

        using difference_type =
            std::vector<ActorVisionState>::difference_type;

        states_.erase(
            states_.begin() +
                static_cast<difference_type>(index));

        return {};
    }

    ActorVisionState*
    ActorVisionStateRegistry::find(
        const world::EntityId actor) noexcept
    {
        if (!actor.is_valid())
        {
            return nullptr;
        }

        const std::size_t index = lower_bound_index(actor);
        if (
            index >= states_.size() ||
            states_[index].actor() != actor)
        {
            return nullptr;
        }

        return &states_[index];
    }

    const ActorVisionState*
    ActorVisionStateRegistry::find(
        const world::EntityId actor) const noexcept
    {
        if (!actor.is_valid())
        {
            return nullptr;
        }

        const std::size_t index = lower_bound_index(actor);
        if (
            index >= states_.size() ||
            states_[index].actor() != actor)
        {
            return nullptr;
        }

        return &states_[index];
    }

    bool
    ActorVisionStateRegistry::contains(
        const world::EntityId actor) const noexcept
    {
        return find(actor) != nullptr;
    }

    std::size_t
    ActorVisionStateRegistry::size() const noexcept
    {
        return states_.size();
    }

    bool
    ActorVisionStateRegistry::empty() const noexcept
    {
        return states_.empty();
    }

    std::span<const ActorVisionState>
    ActorVisionStateRegistry::states_in_entity_order()
        const noexcept
    {
        return states_;
    }

    std::size_t
    ActorVisionStateRegistry::lower_bound_index(
        const world::EntityId actor) const noexcept
    {
        const auto it = std::lower_bound(
            states_.begin(),
            states_.end(),
            actor,
            [](const ActorVisionState& state,
               const world::EntityId candidate) noexcept
            {
                return state.actor() < candidate;
            });

        return static_cast<std::size_t>(it - states_.begin());
    }
}
