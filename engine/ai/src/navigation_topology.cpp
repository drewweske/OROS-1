#include "oros/ai/navigation_topology.hpp"

#include <algorithm>
#include <cstddef>
#include <new>
#include <span>
#include <utility>
#include <vector>

namespace oros::ai
{
    namespace
    {
        [[nodiscard]]
        const NavigationCellTopology*
        find_cell(
            const std::span<
                const NavigationCellTopology>
                cells,
            const world::WorldCell cell)
            noexcept
        {
            const auto iterator =
                std::lower_bound(
                    cells.begin(),
                    cells.end(),
                    cell,
                    [](
                        const NavigationCellTopology&
                            topology,
                        const world::WorldCell
                            candidate)
                        noexcept
                    {
                        return
                            topology.cell() <
                            candidate;
                    });

            if (
                iterator == cells.end() ||
                iterator->cell() != cell)
            {
                return nullptr;
            }

            return &*iterator;
        }

        [[nodiscard]]
        const NavigationPortalRecord*
        find_portal(
            const NavigationCellTopology&
                topology,
            const NavigationPortalId id)
            noexcept
        {
            const std::span<
                const NavigationPortalRecord>
                portals =
                    topology.
                        portals_in_canonical_order();

            const auto iterator =
                std::lower_bound(
                    portals.begin(),
                    portals.end(),
                    id,
                    [](
                        const NavigationPortalRecord&
                            portal,
                        const NavigationPortalId
                            candidate)
                        noexcept
                    {
                        return
                            portal.id() <
                            candidate;
                    });

            if (
                iterator == portals.end() ||
                iterator->id() != id)
            {
                return nullptr;
            }

            return &*iterator;
        }

        [[nodiscard]]
        foundation::Status
        require_neighbor_projection(
            const std::span<
                const NavigationCellTopology>
                cells,
            const world::WorldCell
                incoming_cell,
            const NavigationPortalRecord&
                incoming_portal,
            const world::WorldCell
                endpoint_cell)
        {
            if (endpoint_cell == incoming_cell)
            {
                return {};
            }

            const NavigationCellTopology*
                neighbor =
                    find_cell(
                        cells,
                        endpoint_cell);

            if (neighbor == nullptr)
            {
                return {};
            }

            const NavigationPortalRecord*
                neighbor_portal =
                    find_portal(
                        *neighbor,
                        incoming_portal.id());

            if (
                neighbor_portal == nullptr ||
                *neighbor_portal !=
                    incoming_portal)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Navigation topology supplied "
                    "neighbor does not contain the "
                    "same shared portal projection.");
            }

            return {};
        }

        [[nodiscard]]
        foundation::Status
        validate_cross_cell_consistency(
            const std::span<
                const NavigationCellTopology>
                cells,
            const NavigationCellTopology&
                incoming)
        {
            const world::WorldCell
                incoming_cell{
                    incoming.cell()
                };

            for (
                const NavigationPortalRecord&
                    incoming_portal :
                incoming.
                    portals_in_canonical_order())
            {
                for (
                    const NavigationCellTopology&
                        existing :
                    cells)
                {
                    if (
                        existing.cell() ==
                        incoming_cell)
                    {
                        continue;
                    }

                    const NavigationPortalRecord*
                        existing_portal =
                            find_portal(
                                existing,
                                incoming_portal.id());

                    if (
                        existing_portal != nullptr &&
                        *existing_portal !=
                            incoming_portal)
                    {
                        return foundation::fail(
                            foundation::ErrorCode::
                                invalid_argument,
                            "Navigation topology "
                            "contains conflicting "
                            "records for one portal "
                            "identity.");
                    }
                }

                foundation::Status
                    first_status =
                        require_neighbor_projection(
                            cells,
                            incoming_cell,
                            incoming_portal,
                            incoming_portal.
                                first_node().cell);

                if (!first_status.has_value())
                {
                    return first_status;
                }

                foundation::Status
                    second_status =
                        require_neighbor_projection(
                            cells,
                            incoming_cell,
                            incoming_portal,
                            incoming_portal.
                                second_node().cell);

                if (!second_status.has_value())
                {
                    return second_status;
                }
            }

            for (
                const NavigationCellTopology&
                    existing :
                cells)
            {
                if (
                    existing.cell() ==
                    incoming_cell)
                {
                    continue;
                }

                for (
                    const NavigationPortalRecord&
                        existing_portal :
                    existing.
                        portals_in_canonical_order())
                {
                    const bool
                        touches_incoming =
                            existing_portal.
                                first_node().cell ==
                                    incoming_cell ||
                            existing_portal.
                                second_node().cell ==
                                    incoming_cell;

                    if (!touches_incoming)
                    {
                        continue;
                    }

                    const NavigationPortalRecord*
                        incoming_portal =
                            find_portal(
                                incoming,
                                existing_portal.id());

                    if (
                        incoming_portal == nullptr ||
                        *incoming_portal !=
                            existing_portal)
                    {
                        return foundation::fail(
                            foundation::ErrorCode::
                                invalid_argument,
                            "Navigation topology "
                            "incoming cell does not "
                            "contain the matching "
                            "existing boundary portal.");
                    }
                }
            }

            return {};
        }
    }

    foundation::Status
    NavigationTopology::
        set_cell_topology(
            const NavigationCellTopology&
                topology)
    {
        const std::size_t index =
            lower_bound_index(
                topology.cell());

        const bool existing =
            index < cells_.size() &&
            cells_[index].cell() ==
                topology.cell();

        if (
            existing &&
            cells_[index] == topology)
        {
            return {};
        }

        const foundation::Status validation =
            validate_cross_cell_consistency(
                std::span<
                    const NavigationCellTopology>{
                        cells_
                    },
                topology);

        if (!validation.has_value())
        {
            return validation;
        }

        using difference_type =
            std::vector<
                NavigationCellTopology>::
                    difference_type;

        try
        {
            NavigationCellTopology
                owned_topology{
                    topology
                };

            if (existing)
            {
                cells_[index] =
                    std::move(
                        owned_topology);

                return {};
            }

            cells_.insert(
                cells_.begin() +
                    static_cast<
                        difference_type>(
                            index),
                std::move(
                    owned_topology));
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Navigation topology could not "
                "allocate supplied cell "
                "coverage.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "Navigation topology failed "
                "while setting supplied cell "
                "coverage.");
        }

        return {};
    }

    bool
    NavigationTopology::
        remove_cell_topology(
            const world::WorldCell cell)
        noexcept
    {
        const std::size_t index =
            lower_bound_index(
                cell);

        if (
            index >= cells_.size() ||
            cells_[index].cell() != cell)
        {
            return false;
        }

        using difference_type =
            std::vector<
                NavigationCellTopology>::
                    difference_type;

        cells_.erase(
            cells_.begin() +
                static_cast<
                    difference_type>(
                        index));

        return true;
    }

    const NavigationCellTopology*
    NavigationTopology::
        find_cell_topology(
            const world::WorldCell cell)
        const noexcept
    {
        const std::size_t index =
            lower_bound_index(
                cell);

        if (
            index >= cells_.size() ||
            cells_[index].cell() != cell)
        {
            return nullptr;
        }

        return &cells_[index];
    }

    std::size_t
    NavigationTopology::size()
        const noexcept
    {
        return cells_.size();
    }

    bool
    NavigationTopology::empty()
        const noexcept
    {
        return cells_.empty();
    }

    std::span<
        const NavigationCellTopology>
    NavigationTopology::
        cells_in_canonical_order()
        const noexcept
    {
        return
            std::span<
                const NavigationCellTopology>{
                    cells_
                };
    }

    std::size_t
    NavigationTopology::
        lower_bound_index(
            const world::WorldCell cell)
        const noexcept
    {
        const auto iterator =
            std::lower_bound(
                cells_.begin(),
                cells_.end(),
                cell,
                [](
                    const NavigationCellTopology&
                        topology,
                    const world::WorldCell
                        candidate)
                    noexcept
                {
                    return
                        topology.cell() <
                        candidate;
                });

        return static_cast<std::size_t>(
            iterator -
            cells_.begin());
    }
}
