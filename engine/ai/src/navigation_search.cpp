#include "oros/ai/navigation_search.hpp"

#include "oros/ai/navigation_cell_topology.hpp"
#include "oros/ai/navigation_node_record.hpp"

#include <algorithm>
#include <cstddef>
#include <expected>
#include <new>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace oros::ai
{
    namespace
    {
        struct SearchRecord final
        {
            NavigationNodeId node{};

            std::optional<
                std::size_t>
                predecessor_index{};

            NavigationPortalId
                incoming_portal{};
        };

        struct Frontier final
        {
            std::size_t
                terminal_record_index{};

            NavigationPortalId
                continuation_portal{};

            NavigationNodeId
                continuation_node{};
        };

        struct ReconstructedPath final
        {
            std::vector<
                NavigationNodeId>
                nodes{};

            std::vector<
                NavigationPortalId>
                portals{};
        };

        [[nodiscard]]
        const NavigationNodeRecord*
        find_node_record(
            const NavigationCellTopology&
                topology,
            const NavigationNodeId id)
            noexcept
        {
            const std::span<
                const NavigationNodeRecord>
                nodes =
                    topology.
                        nodes_in_canonical_order();

            const auto iterator =
                std::lower_bound(
                    nodes.begin(),
                    nodes.end(),
                    id,
                    [](
                        const NavigationNodeRecord&
                            record,
                        const NavigationNodeId
                            candidate)
                        noexcept
                    {
                        return
                            record.id() <
                            candidate;
                    });

            if (
                iterator == nodes.end() ||
                iterator->id() != id)
            {
                return nullptr;
            }

            return &*iterator;
        }

        [[nodiscard]]
        bool
        contains_discovered_node(
            const std::vector<
                SearchRecord>& records,
            const NavigationNodeId node)
            noexcept
        {
            for (
                const SearchRecord& record :
                records)
            {
                if (record.node == node)
                {
                    return true;
                }
            }

            return false;
        }

        [[nodiscard]]
        ReconstructedPath
        reconstruct_path(
            const std::vector<
                SearchRecord>& records,
            std::size_t terminal_index)
        {
            ReconstructedPath path{};

            std::size_t current_index =
                terminal_index;

            while (true)
            {
                const SearchRecord& record =
                    records[current_index];

                path.nodes.push_back(
                    record.node);

                if (
                    !record.
                        predecessor_index.
                        has_value())
                {
                    break;
                }

                path.portals.push_back(
                    record.incoming_portal);

                current_index =
                    *record.
                        predecessor_index;
            }

            std::reverse(
                path.nodes.begin(),
                path.nodes.end());

            std::reverse(
                path.portals.begin(),
                path.portals.end());

            return path;
        }
    }

    foundation::Result<
        NavigationSearchResult>
    search_navigation_route(
        const NavigationTopology& topology,
        const NavigationObstacleOverlay& overlay,
        const NavigationNodeId start,
        const NavigationNodeId
            requested_destination)
    {
        try
        {
            if (!start.is_valid())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Navigation search requires "
                    "a valid start node identity.");
            }

            if (
                !requested_destination.
                    is_valid())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Navigation search requires "
                    "a valid requested destination "
                    "identity.");
            }

            const NavigationCellTopology*
                start_topology =
                    topology.
                        find_cell_topology(
                            start.cell);

            if (start_topology == nullptr)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Navigation search start "
                    "topology is unavailable.");
            }

            if (
                find_node_record(
                    *start_topology,
                    start) == nullptr)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Navigation search start node "
                    "is absent from supplied "
                    "topology.");
            }

            const NavigationCellTopology*
                destination_topology =
                    topology.
                        find_cell_topology(
                            requested_destination.
                                cell);

            if (
                destination_topology != nullptr &&
                find_node_record(
                    *destination_topology,
                    requested_destination) ==
                    nullptr)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Navigation search requested "
                    "destination is absent from "
                    "supplied topology.");
            }

            if (
                start ==
                requested_destination)
            {
                const std::vector<
                    NavigationNodeId>
                    nodes{
                        start
                    };

                const std::span<
                    const NavigationPortalId>
                    portals{};

                auto route_result =
                    NavigationRoute::
                        create_complete(
                            topology,
                            requested_destination,
                            nodes,
                            portals);

                if (!route_result.has_value())
                {
                    if (
                        route_result.error().code ==
                        foundation::ErrorCode::
                            out_of_memory)
                    {
                        return
                            std::unexpected<
                                foundation::Error>{
                                    std::move(
                                        route_result.
                                            error())
                                };
                    }

                    return foundation::fail(
                        foundation::ErrorCode::
                            internal_failure,
                        "Navigation search failed "
                        "to construct its validated "
                        "zero-edge complete route.");
                }

                return NavigationSearchResult{
                    start,
                    requested_destination,
                    std::optional<
                        NavigationRoute>{
                            std::move(
                                route_result.value())
                        }
                };
            }

            std::vector<
                SearchRecord>
                records{};

            records.push_back(
                SearchRecord{
                    start,
                    std::nullopt,
                    NavigationPortalId{}
                });

            std::optional<
                Frontier>
                exact_destination_frontier{};

            std::optional<
                Frontier>
                destination_cell_frontier{};

            std::optional<
                Frontier>
                unrelated_frontier{};

            for (
                std::size_t expansion_index = 0;
                expansion_index < records.size();
                ++expansion_index)
            {
                const NavigationNodeId
                    current_node =
                        records[
                            expansion_index].
                            node;

                auto effective_result =
                    query_navigation_effective_traversal_candidates(
                        topology,
                        overlay,
                        current_node);

                if (!effective_result.has_value())
                {
                    if (
                        effective_result.
                            error().code ==
                        foundation::ErrorCode::
                            out_of_memory)
                    {
                        return
                            std::unexpected<
                                foundation::Error>{
                                    std::move(
                                        effective_result.
                                            error())
                                };
                    }

                    return foundation::fail(
                        foundation::ErrorCode::
                            internal_failure,
                        "Navigation search received "
                        "an invalid effective-"
                        "traversal failure for an "
                        "already-discovered supplied "
                        "node.");
                }

                const std::vector<
                    NavigationTraversalCandidate>&
                    candidates =
                        effective_result.value();

                for (
                    const NavigationTraversalCandidate&
                        candidate :
                    candidates)
                {
                    if (
                        candidate.target_kind ==
                        NavigationTraversalTargetKind::
                            unavailable)
                    {
                        const Frontier frontier{
                            expansion_index,
                            candidate.portal,
                            candidate.target
                        };

                        if (
                            candidate.target ==
                            requested_destination)
                        {
                            if (
                                !exact_destination_frontier.
                                    has_value())
                            {
                                exact_destination_frontier =
                                    frontier;
                            }
                        }
                        else if (
                            candidate.target.cell ==
                            requested_destination.
                                cell)
                        {
                            if (
                                !destination_cell_frontier.
                                    has_value())
                            {
                                destination_cell_frontier =
                                    frontier;
                            }
                        }
                        else if (
                            !unrelated_frontier.
                                has_value())
                        {
                            unrelated_frontier =
                                frontier;
                        }

                        continue;
                    }

                    if (
                        candidate.target_kind !=
                        NavigationTraversalTargetKind::
                            supplied)
                    {
                        return foundation::fail(
                            foundation::ErrorCode::
                                internal_failure,
                            "Navigation search received "
                            "an invalid effective "
                            "traversal target kind.");
                    }

                    if (
                        contains_discovered_node(
                            records,
                            candidate.target))
                    {
                        continue;
                    }

                    records.push_back(
                        SearchRecord{
                            candidate.target,
                            expansion_index,
                            candidate.portal
                        });

                    const std::size_t
                        discovered_index =
                            records.size() - 1;

                    if (
                        candidate.target ==
                        requested_destination)
                    {
                        ReconstructedPath path =
                            reconstruct_path(
                                records,
                                discovered_index);

                        auto route_result =
                            NavigationRoute::
                                create_complete(
                                    topology,
                                    requested_destination,
                                    path.nodes,
                                    path.portals);

                        if (
                            !route_result.
                                has_value())
                        {
                            if (
                                route_result.
                                    error().code ==
                                foundation::ErrorCode::
                                    out_of_memory)
                            {
                                return
                                    std::unexpected<
                                        foundation::Error>{
                                            std::move(
                                                route_result.
                                                    error())
                                        };
                            }

                            return foundation::fail(
                                foundation::ErrorCode::
                                    internal_failure,
                                "Navigation search failed "
                                "to construct its "
                                "validated complete "
                                "route.");
                        }

                        return NavigationSearchResult{
                            start,
                            requested_destination,
                            std::optional<
                                NavigationRoute>{
                                    std::move(
                                        route_result.
                                            value())
                                }
                        };
                    }
                }
            }

            const Frontier*
                selected_frontier =
                    nullptr;

            if (
                exact_destination_frontier.
                    has_value())
            {
                selected_frontier =
                    &*exact_destination_frontier;
            }
            else if (
                destination_cell_frontier.
                    has_value())
            {
                selected_frontier =
                    &*destination_cell_frontier;
            }
            else if (
                unrelated_frontier.
                    has_value())
            {
                selected_frontier =
                    &*unrelated_frontier;
            }

            if (selected_frontier != nullptr)
            {
                ReconstructedPath path =
                    reconstruct_path(
                        records,
                        selected_frontier->
                            terminal_record_index);

                auto route_result =
                    NavigationRoute::
                        create_partial(
                            topology,
                            requested_destination,
                            path.nodes,
                            path.portals,
                            selected_frontier->
                                continuation_portal);

                if (!route_result.has_value())
                {
                    if (
                        route_result.error().code ==
                        foundation::ErrorCode::
                            out_of_memory)
                    {
                        return
                            std::unexpected<
                                foundation::Error>{
                                    std::move(
                                        route_result.
                                            error())
                                };
                    }

                    return foundation::fail(
                        foundation::ErrorCode::
                            internal_failure,
                        "Navigation search failed "
                        "to construct its validated "
                        "partial route.");
                }

                return NavigationSearchResult{
                    start,
                    requested_destination,
                    std::optional<
                        NavigationRoute>{
                            std::move(
                                route_result.value())
                        }
                };
            }

            return NavigationSearchResult{
                start,
                requested_destination,
                std::optional<
                    NavigationRoute>{}
            };
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Navigation search could not "
                "allocate deterministic proof "
                "state.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "Navigation search failed during "
                "deterministic proof traversal.");
        }
    }

    NavigationSearchResultKind
    NavigationSearchResult::kind()
        const noexcept
    {
        if (!route_.has_value())
        {
            return
                NavigationSearchResultKind::
                    known_unreachable;
        }

        return
            route_->kind() ==
                    NavigationRouteKind::complete
                ? NavigationSearchResultKind::
                    complete
                : NavigationSearchResultKind::
                    partial;
    }

    NavigationNodeId
    NavigationSearchResult::start_node()
        const noexcept
    {
        return start_;
    }

    NavigationNodeId
    NavigationSearchResult::
        requested_destination()
        const noexcept
    {
        return requested_destination_;
    }

    const NavigationRoute*
    NavigationSearchResult::route()
        const noexcept
    {
        if (!route_.has_value())
        {
            return nullptr;
        }

        return &*route_;
    }

    NavigationSearchResult::
        NavigationSearchResult(
            const NavigationNodeId start,
            const NavigationNodeId
                requested_destination,
            std::optional<
                NavigationRoute>&& route)
        noexcept
        : start_{
              start
          },
          requested_destination_{
              requested_destination
          },
          route_{
              std::move(route)
          }
    {
    }
}
