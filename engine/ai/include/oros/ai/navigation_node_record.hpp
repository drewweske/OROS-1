#pragma once

#include "oros/ai/navigation_node_id.hpp"
#include "oros/foundation/result.hpp"
#include "oros/world/world_position.hpp"

namespace oros::ai
{
    class NavigationNodeRecord final
    {
    public:
        [[nodiscard]]
        static foundation::Result<
            NavigationNodeRecord>
        create(
            NavigationNodeId id,
            const world::WorldPosition& position);

        NavigationNodeRecord(
            const NavigationNodeRecord&) =
                default;

        NavigationNodeRecord&
        operator=(
            const NavigationNodeRecord&) =
                default;

        NavigationNodeRecord(
            NavigationNodeRecord&&)
            noexcept = default;

        NavigationNodeRecord&
        operator=(
            NavigationNodeRecord&&)
            noexcept = default;

        [[nodiscard]]
        NavigationNodeId
        id() const noexcept;

        [[nodiscard]]
        const world::WorldPosition&
        position() const noexcept;

        bool operator==(
            const NavigationNodeRecord&)
            const = default;

    private:
        NavigationNodeRecord(
            NavigationNodeId id,
            const world::WorldPosition& position)
            noexcept;

        NavigationNodeId
            id_;

        world::WorldPosition
            position_;
    };
}
