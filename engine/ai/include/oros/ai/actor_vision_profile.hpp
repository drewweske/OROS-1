#pragma once

#include "oros/foundation/result.hpp"
#include "oros/world/world_position.hpp"

#include <cstdint>

namespace oros::ai
{
    inline constexpr double
        actor_vision_maximum_field_of_view_degrees{
            360.0
        };

    inline constexpr double
        actor_vision_direction_unit_tolerance{
            1.0e-9
        };

    enum class ActorVisionFieldState :
        std::uint8_t
    {
        invalid = 0,
        within_vision_field,
        out_of_range,
        outside_field_of_view
    };

    [[nodiscard]]
    constexpr bool
    is_valid_actor_vision_field_state(
        const ActorVisionFieldState state)
        noexcept
    {
        return
            state ==
                ActorVisionFieldState::
                    within_vision_field ||
            state ==
                ActorVisionFieldState::
                    out_of_range ||
            state ==
                ActorVisionFieldState::
                    outside_field_of_view;
    }

    class ActorVisionDirection final
    {
    public:
        [[nodiscard]]
        static foundation::Result<
            ActorVisionDirection>
        create(
            world::WorldDisplacement direction);

        [[nodiscard]]
        bool is_valid() const noexcept;

        [[nodiscard]]
        const world::WorldDisplacement&
        vector() const noexcept;

        bool operator==(
            const ActorVisionDirection&)
            const noexcept = default;

    private:
        explicit ActorVisionDirection(
            world::WorldDisplacement direction)
            noexcept;

        world::WorldDisplacement
            direction_{};
    };

    class ActorVisionProfile final
    {
    public:
        [[nodiscard]]
        static foundation::Result<
            ActorVisionProfile>
        create(
            double maximum_range_meters,
            double field_of_view_degrees);

        [[nodiscard]]
        bool is_valid() const noexcept;

        [[nodiscard]]
        foundation::Result<
            ActorVisionFieldState>
        classify(
            const world::WorldPosition&
                observer_position,
            const ActorVisionDirection&
                observer_forward,
            const world::WorldPosition&
                target_position) const;

        [[nodiscard]]
        double maximum_range_meters()
            const noexcept;

        [[nodiscard]]
        double field_of_view_degrees()
            const noexcept;

        [[nodiscard]]
        double minimum_forward_dot()
            const noexcept;

    private:
        ActorVisionProfile(
            double maximum_range_meters,
            double field_of_view_degrees,
            double minimum_forward_dot)
            noexcept;

        double maximum_range_meters_{};
        double field_of_view_degrees_{};
        double minimum_forward_dot_{1.0};
    };
}