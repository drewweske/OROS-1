#pragma once

#include "oros/foundation/result.hpp"
#include "oros/physics/collider_geometry.hpp"
#include "oros/physics/collider_id.hpp"
#include "oros/streaming/world_cell_key.hpp"

#include <cstddef>
#include <span>
#include <vector>

namespace oros::physical_world
{
    class WorldCellColliderSet final
    {
    public:
        [[nodiscard]]
        static foundation::Result<
            WorldCellColliderSet>
        create(
            streaming::WorldCellKey cell_key,
            std::span<
                const physics::ColliderGeometry>
                colliders);

        WorldCellColliderSet(
            const WorldCellColliderSet&) =
                default;

        WorldCellColliderSet&
        operator=(
            const WorldCellColliderSet&) =
                default;

        WorldCellColliderSet(
            WorldCellColliderSet&&)
            noexcept = default;

        WorldCellColliderSet&
        operator=(
            WorldCellColliderSet&&)
            noexcept = default;

        [[nodiscard]]
        bool is_valid() const noexcept;

        [[nodiscard]]
        const streaming::WorldCellKey&
        cell_key() const noexcept;

        [[nodiscard]]
        std::span<
            const physics::ColliderGeometry>
        colliders() const noexcept;

        [[nodiscard]]
        const physics::ColliderGeometry*
        find(
            physics::ColliderId collider)
            const noexcept;

        [[nodiscard]]
        bool contains(
            physics::ColliderId collider)
            const noexcept;

        [[nodiscard]]
        std::size_t size() const noexcept;

        [[nodiscard]]
        bool empty() const noexcept;

        bool operator==(
            const WorldCellColliderSet&)
            const = default;

    private:
        WorldCellColliderSet(
            streaming::WorldCellKey cell_key,
            std::vector<
                physics::ColliderGeometry>
                colliders) noexcept;

        streaming::WorldCellKey
            cell_key_{};

        std::vector<
            physics::ColliderGeometry>
            colliders_{};
    };
}