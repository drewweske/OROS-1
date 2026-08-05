#include "oros/assets/resource_handle.hpp"

#include <cstdint>

namespace oros::assets
{
    namespace
    {
        [[nodiscard]]
        constexpr std::uint64_t mix_hash(
            std::uint64_t value) noexcept
        {
            value +=
                0x9E3779B97F4A7C15ULL;

            value =
                (value ^ (value >> 30U)) *
                0xBF58476D1CE4E5B9ULL;

            value =
                (value ^ (value >> 27U)) *
                0x94D049BB133111EBULL;

            return
                value ^
                (value >> 31U);
        }
    }

    std::size_t
    ResourceHandleHash::operator()(
        const ResourceHandle handle) const noexcept
    {
        const std::uint64_t mixed =
            mix_hash(
                handle.packed_value());

        if constexpr (
            sizeof(std::size_t) >=
            sizeof(std::uint64_t))
        {
            return static_cast<std::size_t>(
                mixed);
        }
        else
        {
            return static_cast<std::size_t>(
                mixed ^
                (mixed >> 32U));
        }
    }
}