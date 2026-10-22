#pragma once

#include "oros/ai/navigation_node_id.hpp"
#include "oros/ai/navigation_portal_id.hpp"
#include "oros/foundation/result.hpp"

namespace oros::ai
{
    class NavigationPortalRecord final
    {
    public:
        [[nodiscard]]
        static foundation::Result<
            NavigationPortalRecord>
        create(
            NavigationPortalId id,
            NavigationNodeId endpoint_a,
            NavigationNodeId endpoint_b,
            bool allow_a_to_b,
            bool allow_b_to_a);

        NavigationPortalRecord(
            const NavigationPortalRecord&) =
                default;

        NavigationPortalRecord&
        operator=(
            const NavigationPortalRecord&) =
                default;

        NavigationPortalRecord(
            NavigationPortalRecord&&)
            noexcept = default;

        NavigationPortalRecord&
        operator=(
            NavigationPortalRecord&&)
            noexcept = default;

        [[nodiscard]]
        NavigationPortalId
        id() const noexcept;

        [[nodiscard]]
        NavigationNodeId
        first_node() const noexcept;

        [[nodiscard]]
        NavigationNodeId
        second_node() const noexcept;

        [[nodiscard]]
        bool
        first_to_second() const noexcept;

        [[nodiscard]]
        bool
        second_to_first() const noexcept;

        bool operator==(
            const NavigationPortalRecord&)
            const = default;

    private:
        NavigationPortalRecord(
            NavigationPortalId id,
            NavigationNodeId first_node,
            NavigationNodeId second_node,
            bool first_to_second,
            bool second_to_first)
            noexcept;

        NavigationPortalId
            id_;

        NavigationNodeId
            first_node_;

        NavigationNodeId
            second_node_;

        bool
            first_to_second_;

        bool
            second_to_first_;
    };
}
