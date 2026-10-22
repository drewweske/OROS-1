#pragma once

#include "oros/ai/navigation_cell_topology.hpp"
#include "oros/foundation/result.hpp"
#include "oros/world/world_position.hpp"

#include <cstddef>
#include <span>
#include <vector>

namespace oros::ai
{
    class NavigationTopology final
    {
    public:
        NavigationTopology() = default;

        NavigationTopology(
            const NavigationTopology&) =
                default;

        NavigationTopology&
        operator=(
            const NavigationTopology&) =
                default;

        NavigationTopology(
            NavigationTopology&&)
            noexcept = default;

        NavigationTopology&
        operator=(
            NavigationTopology&&)
            noexcept = default;

        [[nodiscard]]
        foundation::Status
        set_cell_topology(
            const NavigationCellTopology&
                topology);

        [[nodiscard]]
        bool
        remove_cell_topology(
            world::WorldCell cell)
            noexcept;

        [[nodiscard]]
        const NavigationCellTopology*
        find_cell_topology(
            world::WorldCell cell)
            const noexcept;

        [[nodiscard]]
        std::size_t
        size() const noexcept;

        [[nodiscard]]
        bool
        empty() const noexcept;

        [[nodiscard]]
        std::span<
            const NavigationCellTopology>
        cells_in_canonical_order()
            const noexcept;

        bool operator==(
            const NavigationTopology&)
            const = default;

    private:
        [[nodiscard]]
        std::size_t
        lower_bound_index(
            world::WorldCell cell)
            const noexcept;

        std::vector<
            NavigationCellTopology>
            cells_{};
    };
}
