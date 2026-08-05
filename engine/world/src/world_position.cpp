#include "oros/world/world_position.hpp"

#include <cmath>
#include <limits>

namespace oros::world
{
    namespace
    {
        struct NormalizedAxis final
        {
            std::int64_t cell{};
            double local{};
        };

        inline constexpr double
            int64_minimum_as_double{
                -9'223'372'036'854'775'808.0
            };

        inline constexpr double
            int64_upper_exclusive_as_double{
                9'223'372'036'854'775'808.0
            };

        [[nodiscard]] foundation::Result<
            NormalizedAxis>
        normalize_axis(
            const std::int64_t initial_cell,
            const double initial_local)
        {
            if (!std::isfinite(initial_local))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "World local coordinates must be "
                    "finite.");
            }

            const double cell_offset_value =
                std::floor(
                    initial_local /
                        world_cell_extent_meters +
                    0.5);

            if (!std::isfinite(cell_offset_value) ||
                cell_offset_value <
                    int64_minimum_as_double ||
                cell_offset_value >=
                    int64_upper_exclusive_as_double)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "World local coordinate exceeds the "
                    "representable cell range.");
            }

            const std::int64_t cell_offset =
                static_cast<std::int64_t>(
                    cell_offset_value);

            const std::int64_t minimum_cell =
                (std::numeric_limits<
                    std::int64_t>::min)();

            const std::int64_t maximum_cell =
                (std::numeric_limits<
                    std::int64_t>::max)();

            if (cell_offset > 0 &&
                initial_cell >
                    maximum_cell - cell_offset)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "World coordinate normalization would "
                    "overflow the positive cell range.");
            }

            if (cell_offset < 0 &&
                initial_cell <
                    minimum_cell - cell_offset)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "World coordinate normalization would "
                    "overflow the negative cell range.");
            }

            std::int64_t normalized_cell =
                initial_cell + cell_offset;

            double normalized_local =
                initial_local -
                static_cast<double>(cell_offset) *
                    world_cell_extent_meters;

            if (!std::isfinite(normalized_local))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "World local coordinate could not be "
                    "normalized.");
            }

            if (normalized_local >=
                world_cell_half_extent_meters)
            {
                if (normalized_cell == maximum_cell)
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "World coordinate normalization would "
                        "overflow the positive cell range.");
                }

                ++normalized_cell;

                normalized_local -=
                    world_cell_extent_meters;
            }
            else if (normalized_local <
                -world_cell_half_extent_meters)
            {
                if (normalized_cell == minimum_cell)
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "World coordinate normalization would "
                        "overflow the negative cell range.");
                }

                --normalized_cell;

                normalized_local +=
                    world_cell_extent_meters;
            }

            if (normalized_local <
                    -world_cell_half_extent_meters ||
                normalized_local >=
                    world_cell_half_extent_meters)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "World local coordinate could not be "
                    "placed inside the canonical cell range.");
            }

            return NormalizedAxis{
                normalized_cell,
                normalized_local
            };
        }

        [[nodiscard]] foundation::Result<double>
        calculate_axis_displacement(
            const std::int64_t source_cell,
            const double source_local,
            const std::int64_t destination_cell,
            const double destination_local)
        {
            const long double cell_difference =
                static_cast<long double>(
                    destination_cell) -
                static_cast<long double>(
                    source_cell);

            const long double local_difference =
                static_cast<long double>(
                    destination_local) -
                static_cast<long double>(
                    source_local);

            const long double displacement =
                cell_difference *
                    static_cast<long double>(
                        world_cell_extent_meters) +
                local_difference;

            const long double maximum_double =
                static_cast<long double>(
                    (std::numeric_limits<
                        double>::max)());

            if (!std::isfinite(displacement) ||
                displacement > maximum_double ||
                displacement < -maximum_double)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_state,
                    "World displacement cannot be represented "
                    "as a finite double.");
            }

            return static_cast<double>(
                displacement);
        }
    }

    foundation::Result<WorldPosition>
    WorldPosition::create(
        const WorldCell cell,
        const LocalPosition local)
    {
        const foundation::Result<NormalizedAxis>
            x_result =
                normalize_axis(
                    cell.x,
                    local.x);

        if (!x_result.has_value())
        {
            return foundation::fail(
                x_result.error().code,
                x_result.error().message);
        }

        const foundation::Result<NormalizedAxis>
            y_result =
                normalize_axis(
                    cell.y,
                    local.y);

        if (!y_result.has_value())
        {
            return foundation::fail(
                y_result.error().code,
                y_result.error().message);
        }

        const foundation::Result<NormalizedAxis>
            z_result =
                normalize_axis(
                    cell.z,
                    local.z);

        if (!z_result.has_value())
        {
            return foundation::fail(
                z_result.error().code,
                z_result.error().message);
        }

        return WorldPosition{
            WorldCell{
                x_result.value().cell,
                y_result.value().cell,
                z_result.value().cell
            },
            LocalPosition{
                x_result.value().local,
                y_result.value().local,
                z_result.value().local
            }
        };
    }

    foundation::Result<WorldPosition>
    WorldPosition::origin()
    {
        return WorldPosition{
            WorldCell{},
            LocalPosition{}
        };
    }

    WorldPosition::WorldPosition(
        const WorldCell cell,
        const LocalPosition local) noexcept
        : cell_{cell},
          local_{local}
    {
    }

    const WorldCell&
    WorldPosition::cell() const noexcept
    {
        return cell_;
    }

    const LocalPosition&
    WorldPosition::local() const noexcept
    {
        return local_;
    }

    bool
    WorldPosition::is_normalized() const noexcept
    {
        return
            std::isfinite(local_.x) &&
            std::isfinite(local_.y) &&
            std::isfinite(local_.z) &&
            local_.x >=
                -world_cell_half_extent_meters &&
            local_.x <
                world_cell_half_extent_meters &&
            local_.y >=
                -world_cell_half_extent_meters &&
            local_.y <
                world_cell_half_extent_meters &&
            local_.z >=
                -world_cell_half_extent_meters &&
            local_.z <
                world_cell_half_extent_meters;
    }

    foundation::Result<WorldPosition>
    WorldPosition::translated(
        const WorldDisplacement displacement) const
    {
        if (!std::isfinite(displacement.x) ||
            !std::isfinite(displacement.y) ||
            !std::isfinite(displacement.z))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World translation displacement must be "
                "finite.");
        }

        const LocalPosition translated_local{
            local_.x + displacement.x,
            local_.y + displacement.y,
            local_.z + displacement.z
        };

        if (!std::isfinite(translated_local.x) ||
            !std::isfinite(translated_local.y) ||
            !std::isfinite(translated_local.z))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World translation exceeded the finite "
                "coordinate range.");
        }

        return create(
            cell_,
            translated_local);
    }

    foundation::Result<WorldDisplacement>
    WorldPosition::displacement_to(
        const WorldPosition& destination) const
    {
        const foundation::Result<double>
            x_result =
                calculate_axis_displacement(
                    cell_.x,
                    local_.x,
                    destination.cell_.x,
                    destination.local_.x);

        if (!x_result.has_value())
        {
            return foundation::fail(
                x_result.error().code,
                x_result.error().message);
        }

        const foundation::Result<double>
            y_result =
                calculate_axis_displacement(
                    cell_.y,
                    local_.y,
                    destination.cell_.y,
                    destination.local_.y);

        if (!y_result.has_value())
        {
            return foundation::fail(
                y_result.error().code,
                y_result.error().message);
        }

        const foundation::Result<double>
            z_result =
                calculate_axis_displacement(
                    cell_.z,
                    local_.z,
                    destination.cell_.z,
                    destination.local_.z);

        if (!z_result.has_value())
        {
            return foundation::fail(
                z_result.error().code,
                z_result.error().message);
        }

        return WorldDisplacement{
            x_result.value(),
            y_result.value(),
            z_result.value()
        };
    }
}