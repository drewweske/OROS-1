#include "oros/ai/actor_simulation_fidelity_registry.hpp"

#include <algorithm>
#include <cstddef>
#include <new>
#include <expected>
#include <utility>

namespace oros::ai
{
    foundation::Status
    ActorSimulationFidelityRegistry::insert(
        const world::EntityId actor,
        const ActorSimulationFidelity fidelity)
    {
        foundation::Result<
            ActorSimulationFidelityState>
            state_result =
                ActorSimulationFidelityState::create(
                    actor,
                    fidelity);

        if (!state_result.has_value())
        {
            return std::unexpected{
                std::move(state_result.error())
            };
        }

        const std::size_t index =
            lower_bound_index(actor);

        if (
            index < states_.size() &&
            states_[index].actor() == actor)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Actor simulation fidelity registry "
                "already contains this persistent "
                "World entity.");
        }

        using difference_type =
            std::vector<
                ActorSimulationFidelityState>::
                    difference_type;

        try
        {
            states_.insert(
                states_.begin() +
                    static_cast<difference_type>(
                        index),
                std::move(
                    state_result.value()));
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Actor simulation fidelity registry "
                "could not allocate storage for "
                "another actor.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "Actor simulation fidelity registry "
                "failed while inserting an actor.");
        }

        return {};
    }

    foundation::Status
    ActorSimulationFidelityRegistry::remove(
        const world::EntityId actor)
    {
        if (!actor.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor simulation fidelity removal "
                "requires a valid World entity "
                "identity.");
        }

        const std::size_t index =
            lower_bound_index(actor);

        if (
            index >= states_.size() ||
            states_[index].actor() != actor)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    not_found,
                "Actor simulation fidelity registry "
                "does not contain this actor.");
        }

        using difference_type =
            std::vector<
                ActorSimulationFidelityState>::
                    difference_type;

        states_.erase(
            states_.begin() +
                static_cast<difference_type>(
                    index));

        return {};
    }

    foundation::Result<
        ActorSimulationFidelityTransition>
    ActorSimulationFidelityRegistry::transition(
        const world::EntityId actor,
        const ActorSimulationFidelity fidelity)
    {
        if (!actor.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor simulation fidelity transition "
                "requires a valid World entity "
                "identity.");
        }

        if (!is_valid_actor_simulation_fidelity(
                fidelity))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor simulation fidelity transition "
                "requires a valid target fidelity.");
        }

        const std::size_t index =
            lower_bound_index(actor);

        if (
            index >= states_.size() ||
            states_[index].actor() != actor)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    not_found,
                "Actor simulation fidelity registry "
                "does not contain this actor.");
        }

        return states_[index].transition_to(
            fidelity);
    }

    foundation::Result<
        ActorSimulationFidelity>
    ActorSimulationFidelityRegistry::fidelity(
        const world::EntityId actor) const
    {
        if (!actor.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor simulation fidelity lookup "
                "requires a valid World entity "
                "identity.");
        }

        const std::size_t index =
            lower_bound_index(actor);

        if (
            index >= states_.size() ||
            states_[index].actor() != actor)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    not_found,
                "Actor simulation fidelity registry "
                "does not contain this actor.");
        }

        return states_[index].fidelity();
    }

    bool
    ActorSimulationFidelityRegistry::contains(
        const world::EntityId actor)
        const noexcept
    {
        if (!actor.is_valid())
        {
            return false;
        }

        const std::size_t index =
            lower_bound_index(actor);

        return
            index < states_.size() &&
            states_[index].actor() == actor;
    }

    std::size_t
    ActorSimulationFidelityRegistry::size()
        const noexcept
    {
        return states_.size();
    }

    bool
    ActorSimulationFidelityRegistry::empty()
        const noexcept
    {
        return states_.empty();
    }

    std::span<
        const ActorSimulationFidelityState>
    ActorSimulationFidelityRegistry::
        states_in_entity_order()
        const noexcept
    {
        return states_;
    }

    std::size_t
    ActorSimulationFidelityRegistry::
        lower_bound_index(
            const world::EntityId actor)
        const noexcept
    {
        const auto iterator =
            std::lower_bound(
                states_.begin(),
                states_.end(),
                actor,
                [](
                    const ActorSimulationFidelityState&
                        state,
                    const world::EntityId candidate)
                    noexcept
                {
                    return state.actor() <
                        candidate;
                });

        return static_cast<std::size_t>(
            iterator - states_.begin());
    }
}