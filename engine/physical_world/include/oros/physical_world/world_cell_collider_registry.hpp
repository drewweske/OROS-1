#pragma once

#include "oros/foundation/result.hpp"
#include "oros/physical_world/world_cell_collider_set.hpp"
#include "oros/physics/collider_geometry.hpp"
#include "oros/physics/collider_id.hpp"
#include "oros/streaming/world_cell_key.hpp"
#include "oros/streaming/world_cell_residency.hpp"
#include "oros/streaming/world_cell_revision_id.hpp"
#include "oros/world/world_position.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace oros::physical_world
{
    class WorldCellColliderRegistry final
    {
    public:
        WorldCellColliderRegistry() = default;

        WorldCellColliderRegistry(
            const WorldCellColliderRegistry&) =
                default;

        WorldCellColliderRegistry&
        operator=(
            const WorldCellColliderRegistry&) =
                default;

        WorldCellColliderRegistry(
            WorldCellColliderRegistry&&)
            noexcept = default;

        WorldCellColliderRegistry&
        operator=(
            WorldCellColliderRegistry&&)
            noexcept = default;

        [[nodiscard]]
        bool is_valid() const noexcept;

        [[nodiscard]]
        foundation::Status
        synchronize(
            const streaming::WorldCellResidency&
                residency) noexcept;

        [[nodiscard]]
        foundation::Result<
            std::vector<
                physics::ColliderGeometry>>
        colliders_relative_to(
            std::uint64_t world_namespace,
            const world::WorldPosition&
                reference_position) const;

        [[nodiscard]]
        const WorldCellColliderSet*
        find_cell(
            streaming::WorldCellKey cell_key)
            const noexcept;

        [[nodiscard]]
        const physics::ColliderGeometry*
        find(
            physics::ColliderId collider)
            const noexcept;

        [[nodiscard]]
        bool contains_cell(
            streaming::WorldCellKey cell_key)
            const noexcept;

        [[nodiscard]]
        bool contains(
            physics::ColliderId collider)
            const noexcept;

        [[nodiscard]]
        std::size_t
        active_cell_count() const noexcept;

        [[nodiscard]]
        std::size_t
        collider_count() const noexcept;

        [[nodiscard]]
        bool empty() const noexcept;

    private:
        struct ActiveCell final
        {
            streaming::WorldCellRevisionId
                revision;

            WorldCellColliderSet
                collider_set;
        };

        [[nodiscard]]
        std::size_t lower_bound_index(
            streaming::WorldCellKey cell_key)
            const noexcept;

        std::vector<ActiveCell>
            active_cells_{};
    };
}