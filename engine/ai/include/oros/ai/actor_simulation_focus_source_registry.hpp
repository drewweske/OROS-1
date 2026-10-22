#pragma once

#include "oros/foundation/result.hpp"
#include "oros/world/entity_id.hpp"
#include "oros/world/world_position.hpp"

#include <cstddef>
#include <span>
#include <type_traits>
#include <vector>

namespace oros::ai
{
    class ActorSimulationFocusSourceRegistry;

    class ActorSimulationFocusSource final
    {
    public:
        ActorSimulationFocusSource(
            const ActorSimulationFocusSource&) =
                default;

        ActorSimulationFocusSource&
        operator=(
            const ActorSimulationFocusSource&) =
                default;

        ActorSimulationFocusSource(
            ActorSimulationFocusSource&&)
            noexcept = default;

        ActorSimulationFocusSource&
        operator=(
            ActorSimulationFocusSource&&)
            noexcept = default;

        [[nodiscard]]
        world::EntityId source_entity()
            const noexcept;

        [[nodiscard]]
        const world::WorldPosition&
        position() const noexcept;

    private:
        friend class
            ActorSimulationFocusSourceRegistry;

        ActorSimulationFocusSource(
            world::EntityId source_entity,
            world::WorldPosition position)
            noexcept;

        world::EntityId source_entity_{};
        world::WorldPosition position_;
    };

    static_assert(
        std::is_nothrow_move_constructible_v<
            ActorSimulationFocusSource>);

    static_assert(
        std::is_nothrow_move_assignable_v<
            ActorSimulationFocusSource>);

    class ActorSimulationFocusSourceRegistry final
    {
    public:
        ActorSimulationFocusSourceRegistry() =
            default;

        ActorSimulationFocusSourceRegistry(
            const ActorSimulationFocusSourceRegistry&) =
                default;

        ActorSimulationFocusSourceRegistry&
        operator=(
            const ActorSimulationFocusSourceRegistry&) =
                default;

        ActorSimulationFocusSourceRegistry(
            ActorSimulationFocusSourceRegistry&&)
            noexcept = default;

        ActorSimulationFocusSourceRegistry&
        operator=(
            ActorSimulationFocusSourceRegistry&&)
            noexcept = default;

        [[nodiscard]]
        foundation::Status
        insert(
            world::EntityId source_entity,
            world::WorldPosition position);

        [[nodiscard]]
        foundation::Status
        update_position(
            world::EntityId source_entity,
            world::WorldPosition position);

        [[nodiscard]]
        foundation::Status
        remove(
            world::EntityId source_entity);

        [[nodiscard]]
        foundation::Result<
            world::WorldPosition>
        position(
            world::EntityId source_entity) const;

        [[nodiscard]]
        bool contains(
            world::EntityId source_entity)
            const noexcept;

        [[nodiscard]]
        std::size_t size() const noexcept;

        [[nodiscard]]
        bool empty() const noexcept;

        [[nodiscard]]
        std::span<
            const ActorSimulationFocusSource>
        sources_in_entity_order()
            const noexcept;

    private:
        [[nodiscard]]
        std::size_t lower_bound_index(
            world::EntityId source_entity)
            const noexcept;

        std::vector<
            ActorSimulationFocusSource>
            sources_{};
    };
}