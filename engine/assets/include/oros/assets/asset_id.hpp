#pragma once

#include "oros/foundation/result.hpp"

#include <compare>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace oros::assets
{
    struct AssetId final
    {
        std::uint64_t catalog_namespace{};
        std::uint64_t asset_sequence{};

        [[nodiscard]]
        constexpr bool is_valid() const noexcept
        {
            return
                catalog_namespace != 0ULL &&
                asset_sequence != 0ULL;
        }

        auto operator<=>(
            const AssetId&) const noexcept = default;
    };

    inline constexpr AssetId invalid_asset_id{};

    struct AssetIdHash final
    {
        [[nodiscard]]
        std::size_t operator()(
            AssetId asset) const noexcept;
    };

    [[nodiscard]]
    std::string to_string(
        AssetId asset);

    [[nodiscard]]
    foundation::Result<AssetId>
    parse_asset_id(
        std::string_view text);

    class AssetIdGenerator final
    {
    public:
        [[nodiscard]]
        static foundation::Result<AssetIdGenerator>
        create(
            std::uint64_t catalog_namespace,
            std::uint64_t first_sequence = 1ULL);

        AssetIdGenerator(
            const AssetIdGenerator&) = default;

        AssetIdGenerator&
        operator=(
            const AssetIdGenerator&) = default;

        AssetIdGenerator(
            AssetIdGenerator&&) noexcept = default;

        AssetIdGenerator&
        operator=(
            AssetIdGenerator&&) noexcept = default;

        [[nodiscard]]
        foundation::Result<AssetId> generate();

        [[nodiscard]]
        constexpr std::uint64_t
        catalog_namespace() const noexcept
        {
            return catalog_namespace_;
        }

        [[nodiscard]]
        constexpr std::uint64_t
        next_sequence() const noexcept
        {
            return next_sequence_;
        }

        [[nodiscard]]
        constexpr bool exhausted() const noexcept
        {
            return exhausted_;
        }

    private:
        AssetIdGenerator(
            std::uint64_t catalog_namespace,
            std::uint64_t first_sequence) noexcept;

        std::uint64_t catalog_namespace_{};
        std::uint64_t next_sequence_{};
        bool exhausted_{};
    };
}