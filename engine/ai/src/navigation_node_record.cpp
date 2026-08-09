#include "oros/ai/navigation_node_record.hpp"

namespace oros::ai
{
    foundation::Result<
        NavigationNodeRecord>
    NavigationNodeRecord::create(
        const NavigationNodeId id,
        const world::WorldPosition& position)
    {
        if (!id.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Navigation node record requires "
                "a valid navigation node identity.");
        }

        if (position.cell() != id.cell)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Navigation node record position "
                "must occupy the node identity's "
                "owning world cell.");
        }

        return NavigationNodeRecord{
            id,
            position
        };
    }

    NavigationNodeId
    NavigationNodeRecord::id()
        const noexcept
    {
        return id_;
    }

    const world::WorldPosition&
    NavigationNodeRecord::position()
        const noexcept
    {
        return position_;
    }

    NavigationNodeRecord::
        NavigationNodeRecord(
            const NavigationNodeId id,
            const world::WorldPosition& position)
            noexcept
        : id_{id},
          position_{position}
    {
    }
}
