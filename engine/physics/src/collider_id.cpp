#include "oros/physics/collider_id.hpp"

#include <bit>
#include <cstdint>

namespace oros::physics
{
    namespace
    {
        [[nodiscard]]
        constexpr std::uint64_t
        mix_hash(
            std::uint64_t value)
            noexcept
        {
            value +=
                0x9E3779B97F4A7C15ULL;

            value =
                (value ^
                    (value >> 30U)) *
                0xBF58476D1CE4E5B9ULL;

            value =
                (value ^
                    (value >> 27U)) *
                0x94D049BB133111EBULL;

            return
                value ^
                (value >> 31U);
        }

        [[nodiscard]]
        constexpr std::size_t
        fold_hash(
            const std::uint64_t value)
            noexcept
        {
            if constexpr (
                sizeof(std::size_t) >=
                sizeof(std::uint64_t))
            {
                return
                    static_cast<std::size_t>(
                        value);
            }
            else
            {
                return
                    static_cast<std::size_t>(
                        value ^
                        (value >> 32U));
            }
        }
    }

    std::size_t
    ColliderIdHash::operator()(
        const ColliderId id)
        const noexcept
    {
        const std::uint64_t
            mixed_namespace =
                mix_hash(
                    id.owner.
                        world_namespace);

        const std::uint64_t
            mixed_entity_sequence =
                std::rotl(
                    mix_hash(
                        id.owner.
                            entity_sequence),
                    21);

        const std::uint64_t
            mixed_shape_slot =
                std::rotl(
                    mix_hash(
                        static_cast<
                            std::uint64_t>(
                            id.shape_slot)),
                    42);

        const std::uint64_t combined =
            mixed_namespace ^
            mixed_entity_sequence ^
            mixed_shape_slot;

        return fold_hash(
            combined);
    }
}