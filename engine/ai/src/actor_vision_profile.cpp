#include "oros/ai/actor_vision_profile.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace oros::ai
{
    namespace
    {
        [[nodiscard]]
        bool is_finite_direction(
            const world::WorldDisplacement&
                direction) noexcept
        {
            return
                std::isfinite(direction.x) &&
                std::isfinite(direction.y) &&
                std::isfinite(direction.z);
        }

        [[nodiscard]]
        foundation::Result<double>
        calculate_minimum_forward_dot(
            const double field_of_view_degrees)
        {
            if (field_of_view_degrees == 0.0)
            {
                return 1.0;
            }

            if (field_of_view_degrees == 180.0)
            {
                return 0.0;
            }

            if (
                field_of_view_degrees ==
                actor_vision_maximum_field_of_view_degrees)
            {
                return -1.0;
            }

            const double half_angle_radians =
                field_of_view_degrees *
                std::numbers::pi_v<double> /
                360.0;

            const double minimum_forward_dot =
                std::cos(
                    half_angle_radians);

            if (!std::isfinite(
                    minimum_forward_dot))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_state,
                    "Actor vision field-of-view "
                    "threshold could not be represented "
                    "as a finite value.");
            }

            return
                (std::clamp)(
                    minimum_forward_dot,
                    -1.0,
                    1.0);
        }
    }

    foundation::Result<
        ActorVisionDirection>
    ActorVisionDirection::create(
        const world::WorldDisplacement direction)
    {
        if (!is_finite_direction(direction))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor vision direction requires "
                "finite components.");
        }

        const double length =
            std::hypot(
                direction.x,
                direction.y,
                direction.z);

        if (
            !std::isfinite(length) ||
            length <= 0.0)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor vision direction requires "
                "a finite non-zero magnitude.");
        }

        const world::WorldDisplacement
            normalized_direction{
                direction.x / length,
                direction.y / length,
                direction.z / length
            };

        const ActorVisionDirection candidate{
            normalized_direction
        };

        if (!candidate.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor vision direction could not "
                "be normalized into a finite unit "
                "direction.");
        }

        return candidate;
    }

    bool
    ActorVisionDirection::is_valid()
        const noexcept
    {
        if (!is_finite_direction(
                direction_))
        {
            return false;
        }

        const double length =
            std::hypot(
                direction_.x,
                direction_.y,
                direction_.z);

        return
            std::isfinite(length) &&
            std::abs(
                length - 1.0) <=
                actor_vision_direction_unit_tolerance;
    }

    const world::WorldDisplacement&
    ActorVisionDirection::vector()
        const noexcept
    {
        return direction_;
    }

    ActorVisionDirection::
        ActorVisionDirection(
            const world::WorldDisplacement direction)
            noexcept
        : direction_{
              direction
          }
    {
    }

    foundation::Result<
        ActorVisionProfile>
    ActorVisionProfile::create(
        const double maximum_range_meters,
        const double field_of_view_degrees)
    {
        if (
            !std::isfinite(
                maximum_range_meters) ||
            maximum_range_meters < 0.0)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor vision range must be finite "
                "and non-negative.");
        }

        if (
            !std::isfinite(
                field_of_view_degrees) ||
            field_of_view_degrees < 0.0 ||
            field_of_view_degrees >
                actor_vision_maximum_field_of_view_degrees)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor vision field of view must "
                "be finite and inside the inclusive "
                "[0, 360] degree range.");
        }

        const foundation::Result<double>
            minimum_forward_dot_result =
                calculate_minimum_forward_dot(
                    field_of_view_degrees);

        if (!minimum_forward_dot_result.has_value())
        {
            return foundation::fail(
                minimum_forward_dot_result.
                    error().code,
                minimum_forward_dot_result.
                    error().message);
        }

        const ActorVisionProfile candidate{
            maximum_range_meters,
            field_of_view_degrees,
            minimum_forward_dot_result.value()
        };

        if (!candidate.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Actor vision profile could not "
                "establish a valid range/FOV "
                "configuration.");
        }

        return candidate;
    }

    bool
    ActorVisionProfile::is_valid()
        const noexcept
    {
        return
            std::isfinite(
                maximum_range_meters_) &&
            maximum_range_meters_ >= 0.0 &&
            std::isfinite(
                field_of_view_degrees_) &&
            field_of_view_degrees_ >= 0.0 &&
            field_of_view_degrees_ <=
                actor_vision_maximum_field_of_view_degrees &&
            std::isfinite(
                minimum_forward_dot_) &&
            minimum_forward_dot_ >= -1.0 &&
            minimum_forward_dot_ <= 1.0;
    }

    foundation::Result<
        ActorVisionFieldState>
    ActorVisionProfile::classify(
        const world::WorldPosition&
            observer_position,
        const ActorVisionDirection&
            observer_forward,
        const world::WorldPosition&
            target_position) const
    {
        if (!is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Actor vision classification requires "
                "a valid vision profile.");
        }

        if (!observer_forward.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor vision classification requires "
                "a valid normalized forward direction.");
        }

        if (
            !observer_position.is_normalized() ||
            !target_position.is_normalized())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor vision classification requires "
                "normalized WorldPosition values.");
        }

        const foundation::Result<
            world::WorldDisplacement>
            displacement_result =
                observer_position.displacement_to(
                    target_position);

        if (!displacement_result.has_value())
        {
            return foundation::fail(
                displacement_result.error().code,
                displacement_result.error().message);
        }

        const world::WorldDisplacement&
            displacement =
                displacement_result.value();

        const double distance_meters =
            std::hypot(
                displacement.x,
                displacement.y,
                displacement.z);

        if (!std::isfinite(distance_meters))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Actor vision target distance could "
                "not be represented as a finite "
                "value.");
        }

        if (
            distance_meters >
            maximum_range_meters_)
        {
            return
                ActorVisionFieldState::
                    out_of_range;
        }

        if (distance_meters == 0.0)
        {
            return
                ActorVisionFieldState::
                    within_vision_field;
        }

        const world::WorldDisplacement&
            forward =
                observer_forward.vector();

        const double raw_forward_dot =
            (
                forward.x * displacement.x +
                forward.y * displacement.y +
                forward.z * displacement.z
            ) /
            distance_meters;

        if (!std::isfinite(raw_forward_dot))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Actor vision forward projection "
                "could not be represented as a "
                "finite value.");
        }

        const double forward_dot =
            (std::clamp)(
                raw_forward_dot,
                -1.0,
                1.0);

        if (
            forward_dot >=
            minimum_forward_dot_)
        {
            return
                ActorVisionFieldState::
                    within_vision_field;
        }

        return
            ActorVisionFieldState::
                outside_field_of_view;
    }

    double
    ActorVisionProfile::maximum_range_meters()
        const noexcept
    {
        return maximum_range_meters_;
    }

    double
    ActorVisionProfile::field_of_view_degrees()
        const noexcept
    {
        return field_of_view_degrees_;
    }

    double
    ActorVisionProfile::minimum_forward_dot()
        const noexcept
    {
        return minimum_forward_dot_;
    }

    ActorVisionProfile::
        ActorVisionProfile(
            const double maximum_range_meters,
            const double field_of_view_degrees,
            const double minimum_forward_dot)
            noexcept
        : maximum_range_meters_{
              maximum_range_meters
          },
          field_of_view_degrees_{
              field_of_view_degrees
          },
          minimum_forward_dot_{
              minimum_forward_dot
          }
    {
    }
}