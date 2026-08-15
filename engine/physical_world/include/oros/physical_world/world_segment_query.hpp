#pragma once

#include "oros/foundation/result.hpp"
#include "oros/physical_world/world_cell_collider_registry.hpp"
#include "oros/physics/collider_segment_query.hpp"
#include "oros/streaming/world_cell_key.hpp"
#include "oros/world/world_position.hpp"

#include <cstdint>
#include <optional>

namespace oros::physical_world
{
    enum class WorldSegmentQueryState :
        std::uint8_t
    {
        invalid = 0,
        clear,
        blocked,
        unavailable
    };

    class WorldSegmentQueryResult final
    {
    public:
        [[nodiscard]]
        static foundation::Result<
            WorldSegmentQueryResult>
        create_clear();

        [[nodiscard]]
        static foundation::Result<
            WorldSegmentQueryResult>
        create_blocked(
            physics::ColliderSegmentHit hit);

        [[nodiscard]]
        static foundation::Result<
            WorldSegmentQueryResult>
        create_unavailable(
            streaming::WorldCellKey
                unavailable_cell);

        [[nodiscard]]
        bool is_valid() const noexcept;

        [[nodiscard]]
        WorldSegmentQueryState
        state() const noexcept;

        [[nodiscard]]
        const std::optional<
            physics::ColliderSegmentHit>&
        hit() const noexcept;

        [[nodiscard]]
        const std::optional<
            streaming::WorldCellKey>&
        unavailable_cell() const noexcept;

        bool operator==(
            const WorldSegmentQueryResult&)
            const noexcept = default;

    private:
        WorldSegmentQueryResult(
            WorldSegmentQueryState state,
            std::optional<
                physics::ColliderSegmentHit> hit,
            std::optional<
                streaming::WorldCellKey>
                unavailable_cell)
            noexcept;

        WorldSegmentQueryState
            state_{
                WorldSegmentQueryState::invalid
            };

        std::optional<
            physics::ColliderSegmentHit>
            hit_{};

        std::optional<
            streaming::WorldCellKey>
            unavailable_cell_{};
    };

    [[nodiscard]]
    foundation::Result<
        WorldSegmentQueryResult>
    query_world_segment(
        const WorldCellColliderRegistry& registry,
        std::uint64_t world_namespace,
        const world::WorldPosition& segment_start,
        const world::WorldPosition& segment_end);
}
