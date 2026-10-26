#pragma once

#include "oros/foundation/result.hpp"
#include "oros/world/component_store.hpp"
#include "oros/world/entity_id.hpp"
#include "oros/world/world_position.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
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

        // Restore a live identity set from an authoritative snapshot.
        // last_issued_sequence includes even destroyed/unloaded IDs.
        // Positions/components must be reattached by the caller.
        [[nodiscard]] static foundation::Result<World>
        restore(
            std::uint64_t world_namespace,
            std::uint64_t last_issued_sequence,
            std::span<const EntityId> live_entities);

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