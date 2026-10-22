#pragma once

#include "oros/ai/navigation_node_record.hpp"
#include "oros/ai/navigation_portal_record.hpp"
#include "oros/foundation/result.hpp"
#include "oros/world/world_position.hpp"

#include <span>
#include <vector>

namespace oros::ai
{
    class NavigationCellTopology final
    {
    public:
        [[nodiscard]]
        static foundation::Result<
            NavigationCellTopology>
        create(
            world::WorldCell cell,
            std::span<
                const NavigationNodeRecord>
                nodes,
            std::span<
                const NavigationPortalRecord>
                portals);

        NavigationCellTopology(
            const NavigationCellTopology&) =
                default;

        NavigationCellTopology&
        operator=(
            const NavigationCellTopology&) =
                default;

        NavigationCellTopology(
            NavigationCellTopology&&)
            noexcept = default;

        NavigationCellTopology&
        operator=(
            NavigationCellTopology&&)
            noexcept = default;

        [[nodiscard]]
        const world::WorldCell&
        cell() const noexcept;

        [[nodiscard]]
        std::span<
            const NavigationNodeRecord>
        nodes_in_canonical_order()
            const noexcept;

        [[nodiscard]]
        std::span<
            const NavigationPortalRecord>
        portals_in_canonical_order()
            const noexcept;

        [[nodiscard]]
        bool empty() const noexcept;

        bool operator==(
            const NavigationCellTopology&)
            const = default;

    private:
        NavigationCellTopology(
            world::WorldCell cell,
            std::vector<
                NavigationNodeRecord>&& nodes,
            std::vector<
                NavigationPortalRecord>&& portals)
            noexcept;

        world::WorldCell
            cell_;

        std::vector<
            NavigationNodeRecord>
            nodes_;

        std::vector<
            NavigationPortalRecord>
            portals_;
    };
}
