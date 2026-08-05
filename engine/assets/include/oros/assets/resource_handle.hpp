#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>

namespace oros::assets
{
    struct ResourceHandle final
    {
        std::uint32_t slot{};
        std::uint32_t generation{};

        [[nodiscard]]
        constexpr bool is_valid() const noexcept
        {
            return
                slot != 0U &&
                generation != 0U;
        }

        [[nodiscard]]
        constexpr explicit
        operator bool() const noexcept
        {
            return is_valid();
        }

        [[nodiscard]]
        constexpr std::uint64_t
        packed_value() const noexcept
        {
            return
                (static_cast<std::uint64_t>(
                    generation) << 32U) |
                static_cast<std::uint64_t>(
                    slot);
        }

        [[nodiscard]]
        static constexpr ResourceHandle
        from_packed_value(
            const std::uint64_t value) noexcept
        {
            return ResourceHandle{
                static_cast<std::uint32_t>(
                    value &
                    0xFFFFFFFFULL),
                static_cast<std::uint32_t>(
                    value >>
                    32U)
            };
        }

        auto operator<=>(
            const ResourceHandle&) const noexcept =
                default;
    };

    inline constexpr ResourceHandle
        invalid_resource_handle{};

    struct ResourceHandleHash final
    {
        [[nodiscard]]
        std::size_t operator()(
            ResourceHandle handle) const noexcept;
    };
}