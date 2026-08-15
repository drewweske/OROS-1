#include "oros/ai/actor_visual_observation_memory.hpp"

#include <algorithm>
#include <cstddef>
#include <new>
#include <span>
#include <utility>
#include <vector>

namespace oros::ai
{
    namespace
    {
        [[nodiscard]]
        bool
        valid_memory_key(
            const world::EntityId observer,
            const world::EntityId target)
            noexcept
        {
            return
                observer.is_valid() &&
                target.is_valid() &&
                observer.world_namespace ==
                    target.world_namespace &&
                observer != target;
        }

        [[nodiscard]]
        bool
        observation_matches(
            const ActorVisualObservation&
                observation,
            const world::EntityId observer,
            const world::EntityId target)
            noexcept
        {
            return
                observation.observer() ==
                    observer &&
                observation.target() ==
                    target;
        }
    }

    foundation::Status
    ActorVisualObservationMemory::remember(
        const ActorVisualObservation&
            observation)
    {
        if (!observation.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor visual observation memory "
                "requires a valid observation.");
        }

        const std::size_t index =
            lower_bound_index(
                observation.observer(),
                observation.target());

        const bool existing =
            index < observations_.size() &&
            observation_matches(
                observations_[index],
                observation.observer(),
                observation.target());

        if (existing)
        {
            const ActorVisualObservation&
                current =
                    observations_[index];

            if (
                observation.observed_at() <
                current.observed_at())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Actor visual observation memory "
                    "cannot replace a newer fact with "
                    "an older observation.");
            }

            if (
                observation.observed_at() ==
                current.observed_at())
            {
                if (observation == current)
                {
                    return {};
                }

                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Actor visual observation memory "
                    "rejects conflicting facts at the "
                    "same WorldTime.");
            }
        }

        using difference_type =
            std::vector<
                ActorVisualObservation>::
                    difference_type;

        try
        {
            ActorVisualObservation
                owned_observation{
                    observation
                };

            if (existing)
            {
                observations_[index] =
                    std::move(
                        owned_observation);

                return {};
            }

            observations_.insert(
                observations_.begin() +
                    static_cast<
                        difference_type>(
                            index),
                std::move(
                    owned_observation));
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Actor visual observation memory "
                "could not allocate storage for "
                "the observation.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "Actor visual observation memory "
                "failed while remembering an "
                "observation.");
        }

        return {};
    }

    foundation::Status
    ActorVisualObservationMemory::forget(
        const world::EntityId observer,
        const world::EntityId target)
    {
        if (!valid_memory_key(
                observer,
                target))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor visual observation forget "
                "requires a valid observer-target "
                "identity pair.");
        }

        const std::size_t index =
            lower_bound_index(
                observer,
                target);

        if (
            index >= observations_.size() ||
            !observation_matches(
                observations_[index],
                observer,
                target))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    not_found,
                "Actor visual observation memory "
                "does not contain this observer-"
                "target fact.");
        }

        using difference_type =
            std::vector<
                ActorVisualObservation>::
                    difference_type;

        observations_.erase(
            observations_.begin() +
                static_cast<
                    difference_type>(
                        index));

        return {};
    }

    bool
    ActorVisualObservationMemory::contains(
        const world::EntityId observer,
        const world::EntityId target)
        const noexcept
    {
        if (!valid_memory_key(
                observer,
                target))
        {
            return false;
        }

        const std::size_t index =
            lower_bound_index(
                observer,
                target);

        return
            index < observations_.size() &&
            observation_matches(
                observations_[index],
                observer,
                target);
    }

    foundation::Result<
        ActorVisualObservation>
    ActorVisualObservationMemory::find(
        const world::EntityId observer,
        const world::EntityId target)
        const
    {
        if (!valid_memory_key(
                observer,
                target))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor visual observation lookup "
                "requires a valid observer-target "
                "identity pair.");
        }

        const std::size_t index =
            lower_bound_index(
                observer,
                target);

        if (
            index >= observations_.size() ||
            !observation_matches(
                observations_[index],
                observer,
                target))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    not_found,
                "Actor visual observation memory "
                "does not contain this observer-"
                "target fact.");
        }

        return observations_[index];
    }

    foundation::Result<
        std::span<
            const ActorVisualObservation>>
    ActorVisualObservationMemory::
        observations_for_observer(
            const world::EntityId observer)
            const
    {
        if (!observer.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor visual observation range "
                "requires a valid observer EntityId.");
        }

        const std::size_t first =
            lower_bound_observer_index(
                observer);

        const std::size_t last =
            upper_bound_observer_index(
                observer);

        return
            std::span<
                const ActorVisualObservation>{
                    observations_
                }.
                subspan(
                    first,
                    last - first);
    }

    std::size_t
    ActorVisualObservationMemory::size()
        const noexcept
    {
        return observations_.size();
    }

    bool
    ActorVisualObservationMemory::empty()
        const noexcept
    {
        return observations_.empty();
    }

    std::span<
        const ActorVisualObservation>
    ActorVisualObservationMemory::
        observations_in_canonical_order()
        const noexcept
    {
        return
            std::span<
                const ActorVisualObservation>{
                    observations_
                };
    }

    std::size_t
    ActorVisualObservationMemory::
        lower_bound_index(
            const world::EntityId observer,
            const world::EntityId target)
            const noexcept
    {
        const auto iterator =
            std::lower_bound(
                observations_.begin(),
                observations_.end(),
                target,
                [observer](
                    const ActorVisualObservation&
                        observation,
                    const world::EntityId
                        candidate_target)
                    noexcept
                {
                    if (
                        observation.observer() <
                        observer)
                    {
                        return true;
                    }

                    if (
                        observer <
                        observation.observer())
                    {
                        return false;
                    }

                    return
                        observation.target() <
                        candidate_target;
                });

        return static_cast<std::size_t>(
            iterator -
            observations_.begin());
    }

    std::size_t
    ActorVisualObservationMemory::
        lower_bound_observer_index(
            const world::EntityId observer)
            const noexcept
    {
        const auto iterator =
            std::lower_bound(
                observations_.begin(),
                observations_.end(),
                observer,
                [](
                    const ActorVisualObservation&
                        observation,
                    const world::EntityId
                        candidate)
                    noexcept
                {
                    return
                        observation.observer() <
                        candidate;
                });

        return static_cast<std::size_t>(
            iterator -
            observations_.begin());
    }

    std::size_t
    ActorVisualObservationMemory::
        upper_bound_observer_index(
            const world::EntityId observer)
            const noexcept
    {
        const auto iterator =
            std::upper_bound(
                observations_.begin(),
                observations_.end(),
                observer,
                [](
                    const world::EntityId
                        candidate,
                    const ActorVisualObservation&
                        observation)
                    noexcept
                {
                    return
                        candidate <
                        observation.observer();
                });

        return static_cast<std::size_t>(
            iterator -
            observations_.begin());
    }
}