#pragma once

#include "oros/foundation/result.hpp"
#include "oros/world/component_store.hpp"
#include "oros/world/entity_id.hpp"
#include "oros/world/world_position.hpp"

#include <cstddef>
#include <cstdint>
#include <unordered_set>

namespace oros::world
{
    class World final
    {
    public:
        [[nodiscard]] static
        foundation::Result<World>
        create(
            std::uint64_t world_namespace);

        World(
            const World&) = delete;

        World&
        operator=(
            const World&) = delete;

        World(
            World&&) noexcept = default;

        World&
        operator=(
            World&&) noexcept = default;

        [[nodiscard]] foundation::Result<EntityId>
        create_entity();

        [[nodiscard]] foundation::Status
        destroy_entity(
            EntityId entity);

        [[nodiscard]] bool
        contains(
            EntityId entity) const noexcept;

        [[nodiscard]] std::size_t
        entity_count() const noexcept;

        [[nodiscard]] foundation::Status
        add_position(
            EntityId entity,
            WorldPosition position);

        [[nodiscard]] foundation::Status
        remove_position(
            EntityId entity);

        [[nodiscard]] WorldPosition*
        find_position(
            EntityId entity) noexcept;

        [[nodiscard]] const WorldPosition*
        find_position(
            EntityId entity) const noexcept;

        [[nodiscard]] std::size_t
        position_count() const noexcept;

        [[nodiscard]] const ComponentStore<WorldPosition>&
        positions() const noexcept;

        [[nodiscard]] std::uint64_t
        world_namespace() const noexcept;

    private:
        explicit World(
            EntityIdGenerator id_generator) noexcept;

        EntityIdGenerator id_generator_;

        std::unordered_set<
            EntityId,
            EntityIdHash>
            entities_{};

        ComponentStore<WorldPosition> positions_{};
    };
}