#include "oros/ai/actor_visual_observation.hpp"

#include <cmath>
#include <utility>

namespace oros::ai
{
    foundation::Result<
        ActorVisualObservationConfidence>
    ActorVisualObservationConfidence::create(
        const double value)
    {
        if (
            !std::isfinite(value) ||
            value < minimum_value ||
            value > maximum_value)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor visual observation confidence "
                "must be finite and within the "
                "inclusive range [0, 1].");
        }

        return
            ActorVisualObservationConfidence{
                value
            };
    }

    bool
    ActorVisualObservationConfidence::
        is_valid() const noexcept
    {
        return
            std::isfinite(value_) &&
            value_ >= minimum_value &&
            value_ <= maximum_value;
    }

    double
    ActorVisualObservationConfidence::
        value() const noexcept
    {
        return value_;
    }

    ActorVisualObservationConfidence::
        ActorVisualObservationConfidence(
            const double value)
            noexcept
        : value_{value}
    {
    }

    foundation::Result<
        ActorVisualObservation>
    ActorVisualObservation::create(
        const world::EntityId observer,
        const world::EntityId target,
        const world::WorldPosition&
            target_position,
        const world::WorldTime observed_at,
        ActorVisualObservationConfidence
            confidence)
    {
        if (
            !observer.is_valid() ||
            !target.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor visual observation requires "
                "valid observer and target EntityId "
                "values.");
        }

        if (
            observer.world_namespace !=
            target.world_namespace)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor visual observation observer "
                "and target must belong to the same "
                "world namespace.");
        }

        if (observer == target)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor visual observation requires "
                "distinct observer and target "
                "EntityId values.");
        }

        if (!target_position.is_normalized())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor visual observation requires "
                "a normalized target WorldPosition.");
        }

        if (!confidence.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor visual observation requires "
                "valid confidence.");
        }

        return ActorVisualObservation{
            observer,
            target,
            target_position,
            observed_at,
            std::move(confidence)
        };
    }

    bool
    ActorVisualObservation::
        is_valid() const noexcept
    {
        return
            observer_.is_valid() &&
            target_.is_valid() &&
            observer_.world_namespace ==
                target_.world_namespace &&
            observer_ != target_ &&
            target_position_.is_normalized() &&
            confidence_.is_valid();
    }

    world::EntityId
    ActorVisualObservation::
        observer() const noexcept
    {
        return observer_;
    }

    world::EntityId
    ActorVisualObservation::
        target() const noexcept
    {
        return target_;
    }

    const world::WorldPosition&
    ActorVisualObservation::
        target_position() const noexcept
    {
        return target_position_;
    }

    world::WorldTime
    ActorVisualObservation::
        observed_at() const noexcept
    {
        return observed_at_;
    }

    const ActorVisualObservationConfidence&
    ActorVisualObservation::
        confidence() const noexcept
    {
        return confidence_;
    }

    foundation::Result<
        std::uint64_t>
    ActorVisualObservation::
        age_microseconds_at(
            const world::WorldTime
                current_time) const
    {
        if (current_time < observed_at_)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor visual observation age "
                "cannot be evaluated before the "
                "observation's WorldTime.");
        }

        return
            current_time.
                microseconds_since_epoch() -
            observed_at_.
                microseconds_since_epoch();
    }

    ActorVisualObservation::
        ActorVisualObservation(
            const world::EntityId observer,
            const world::EntityId target,
            world::WorldPosition
                target_position,
            const world::WorldTime observed_at,
            ActorVisualObservationConfidence
                confidence)
            noexcept
        : observer_{observer},
          target_{target},
          target_position_{
              std::move(target_position)
          },
          observed_at_{observed_at},
          confidence_{
              std::move(confidence)
          }
    {
    }
}