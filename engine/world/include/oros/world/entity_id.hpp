#pragma once

#include "oros/foundation/result.hpp"

#include <compare>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace oros::world
{
    struct EntityId final
    {
        std::uint64_t world_namespace{};
        std::uint64_t entity_sequence{};

        [[nodiscard]] constexpr bool
        is_valid() const noexcept
        {
            return world_namespace != 0ULL &&
                entity_sequence != 0ULL;
        }

        [[nodiscard]] constexpr explicit
        operator bool() const noexcept
        {
            return is_valid();
        }

        auto operator<=>(
            const EntityId&) const noexcept = default;
    };

    inline constexpr EntityId invalid_entity_id{};

    struct EntityIdHash final
    {
        [[nodiscard]] std::size_t
        operator()(
            EntityId id) const noexcept;
    };

    [[nodiscard]] std::string
    to_string(
        EntityId id);

    [[nodiscard]] foundation::Result<EntityId>
    parse_entity_id(
        std::string_view text);

    class EntityIdGenerator final
    {
    public:
        [[nodiscard]] static
        foundation::Result<EntityIdGenerator>
        create(
            std::uint64_t world_namespace,
            std::uint64_t first_sequence = 1ULL);

        [[nodiscard]] foundation::Result<EntityId>
        generate();

        [[nodiscard]] std::uint64_t
        world_namespace() const noexcept;

        [[nodiscard]] bool
        is_exhausted() const noexcept;

    private:
        EntityIdGenerator(
            std::uint64_t world_namespace,
            std::uint64_t first_sequence) noexcept;

        std::uint64_t world_namespace_{};
        std::uint64_t next_sequence_{};
        bool exhausted_{};
    };
}