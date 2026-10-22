#pragma once

#include "oros/foundation/result.hpp"
#include "oros/world/entity_id.hpp"
#include "oros/world/world_position.hpp"
#include "oros/world/world_time.hpp"

#include <cstdint>

namespace oros::ai
{
    class ActorVisualObservationConfidence final
    {
    public:
        static constexpr double
            minimum_value{0.0};

        static constexpr double
            maximum_value{1.0};

        [[nodiscard]]
        static foundation::Result<
            ActorVisualObservationConfidence>
        create(
            double value);

        [[nodiscard]]
        bool is_valid() const noexcept;

        [[nodiscard]]
        double value() const noexcept;

        bool operator==(
            const ActorVisualObservationConfidence&)
            const noexcept = default;

    private:
        explicit ActorVisualObservationConfidence(
            double value)
            noexcept;

        double value_{};
    };

    class ActorVisualObservation final
    {
    public:
        [[nodiscard]]
        static foundation::Result<
            ActorVisualObservation>
        create(
            world::EntityId observer,
            world::EntityId target,
            const world::WorldPosition&
                target_position,
            world::WorldTime observed_at,
            ActorVisualObservationConfidence
                confidence);

        ActorVisualObservation(
            const ActorVisualObservation&) =
                default;

        ActorVisualObservation&
        operator=(
            const ActorVisualObservation&) =
                default;

        ActorVisualObservation(
            ActorVisualObservation&&)
            noexcept = default;

        ActorVisualObservation&
        operator=(
            ActorVisualObservation&&)
            noexcept = default;

        [[nodiscard]]
        bool is_valid() const noexcept;

        [[nodiscard]]
        world::EntityId
        observer() const noexcept;

        [[nodiscard]]
        world::EntityId
        target() const noexcept;

        [[nodiscard]]
        const world::WorldPosition&
        target_position() const noexcept;

        [[nodiscard]]
        world::WorldTime
        observed_at() const noexcept;

        [[nodiscard]]
        const ActorVisualObservationConfidence&
        confidence() const noexcept;

        [[nodiscard]]
        foundation::Result<
            std::uint64_t>
        age_microseconds_at(
            world::WorldTime current_time)
            const;

        bool operator==(
            const ActorVisualObservation&)
            const noexcept = default;

    private:
        ActorVisualObservation(
            world::EntityId observer,
            world::EntityId target,
            world::WorldPosition target_position,
            world::WorldTime observed_at,
            ActorVisualObservationConfidence
                confidence)
            noexcept;

        world::EntityId
            observer_{};

        world::EntityId
            target_{};

        world::WorldPosition
            target_position_;

        world::WorldTime
            observed_at_{};

        ActorVisualObservationConfidence
            confidence_;
    };
}