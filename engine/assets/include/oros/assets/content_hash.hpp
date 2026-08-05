#pragma once

#include "oros/foundation/result.hpp"

#include <array>
#include <compare>
#include <cstddef>
#include <span>
#include <string>
#include <string_view>

namespace oros::assets
{
    inline constexpr std::size_t
        content_hash_byte_count{32U};

    inline constexpr std::size_t
        content_hash_text_length{
            content_hash_byte_count * 2U
        };

    struct ContentHash final
    {
        std::array<
            std::byte,
            content_hash_byte_count
        > bytes{};

        auto operator<=>(
            const ContentHash&) const noexcept = default;
    };

    struct ContentHashHash final
    {
        [[nodiscard]]
        std::size_t operator()(
            const ContentHash& hash) const noexcept;
    };

    [[nodiscard]]
    ContentHash hash_bytes(
        std::span<const std::byte> bytes) noexcept;

    [[nodiscard]]
    ContentHash hash_text(
        std::string_view text) noexcept;

    [[nodiscard]]
    std::string to_string(
        const ContentHash& hash);

    [[nodiscard]]
    foundation::Result<ContentHash>
    parse_content_hash(
        std::string_view text);
}