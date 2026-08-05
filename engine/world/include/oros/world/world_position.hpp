#pragma once

#include "oros/foundation/result.hpp"

#include <compare>
#include <cstdint>

namespace oros::world
{
    inline constexpr double
        world_cell_extent_meters{1024.0};

    inline constexpr double
        world_cell_half_extent_meters{
            world_cell_extent_meters / 2.0
        };

    struct WorldCell final
    {
        std::int64_t x{};
        std::int64_t y{};
        std::int64_t z{};

        auto operator<=>(
            const WorldCell&) const noexcept = default;
    };

    struct LocalPosition final
    {
        double x{};
        double y{};
        double z{};

        bool operator==(
            const LocalPosition&) const noexcept = default;
    };

    struct WorldDisplacement final
    {
        double x{};
        double y{};
        double z{};

        bool operator==(
            const WorldDisplacement&) const noexcept =
                default;
    };

    class WorldPosition final
    {
    public:
        [[nodiscard]] static
        foundation::Result<WorldPosition>
        create(
            WorldCell cell,
            LocalPosition local);

        [[nodiscard]] static
        foundation::Result<WorldPosition>
        origin();

        [[nodiscard]] const WorldCell&
        cell() const noexcept;

        [[nodiscard]] const LocalPosition&
        local() const noexcept;

        [[nodiscard]] bool
        is_normalized() const noexcept;

        [[nodiscard]] foundation::Result<WorldPosition>
        translated(
            WorldDisplacement displacement) const;

        [[nodiscard]] foundation::Result<
            WorldDisplacement>
        displacement_to(
            const WorldPosition& destination) const;

        bool operator==(
            const WorldPosition&) const noexcept = default;

    private:
        WorldPosition(
            WorldCell cell,
            LocalPosition local) noexcept;

        WorldCell cell_{};
        LocalPosition local_{};
    };
}