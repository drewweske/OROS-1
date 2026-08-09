#include "oros/ai/navigation_portal_record.hpp"

#include <utility>

namespace oros::ai
{
    foundation::Result<
        NavigationPortalRecord>
    NavigationPortalRecord::create(
        const NavigationPortalId id,
        const NavigationNodeId endpoint_a,
        const NavigationNodeId endpoint_b,
        const bool allow_a_to_b,
        const bool allow_b_to_a)
    {
        if (!id.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Navigation portal record requires "
                "a valid portal identity.");
        }

        if (!endpoint_a.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Navigation portal endpoint A "
                "must be valid.");
        }

        if (!endpoint_b.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Navigation portal endpoint B "
                "must be valid.");
        }

        if (endpoint_a == endpoint_b)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Navigation portal endpoints "
                "must be distinct.");
        }

        if (!allow_a_to_b &&
            !allow_b_to_a)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Navigation portal must permit "
                "at least one traversal direction.");
        }

        NavigationNodeId
            first_node{endpoint_a};

        NavigationNodeId
            second_node{endpoint_b};

        bool
            first_to_second{
                allow_a_to_b
            };

        bool
            second_to_first{
                allow_b_to_a
            };

        if (second_node < first_node)
        {
            std::swap(
                first_node,
                second_node);

            std::swap(
                first_to_second,
                second_to_first);
        }

        return NavigationPortalRecord{
            id,
            first_node,
            second_node,
            first_to_second,
            second_to_first
        };
    }

    NavigationPortalId
    NavigationPortalRecord::id()
        const noexcept
    {
        return id_;
    }

    NavigationNodeId
    NavigationPortalRecord::first_node()
        const noexcept
    {
        return first_node_;
    }

    NavigationNodeId
    NavigationPortalRecord::second_node()
        const noexcept
    {
        return second_node_;
    }

    bool
    NavigationPortalRecord::first_to_second()
        const noexcept
    {
        return first_to_second_;
    }

    bool
    NavigationPortalRecord::second_to_first()
        const noexcept
    {
        return second_to_first_;
    }

    NavigationPortalRecord::
        NavigationPortalRecord(
            const NavigationPortalId id,
            const NavigationNodeId first_node,
            const NavigationNodeId second_node,
            const bool first_to_second,
            const bool second_to_first)
            noexcept
        : id_{id},
          first_node_{first_node},
          second_node_{second_node},
          first_to_second_{first_to_second},
          second_to_first_{second_to_first}
    {
    }
}
