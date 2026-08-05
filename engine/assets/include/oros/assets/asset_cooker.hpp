#pragma once

#include "oros/assets/asset_record.hpp"
#include "oros/foundation/result.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace oros::assets
{
    struct CookRequest final
    {
        AssetRecord record{};

        std::span<const std::byte>
            intermediate_bytes{};

        [[nodiscard]]
        bool is_valid() const noexcept
        {
            return
                record.is_valid() &&
                !record.is_cooked();
        }
    };

    struct CookResult final
    {
        std::vector<std::byte>
            cooked_bytes{};

        [[nodiscard]]
        bool has_cooked_data()
            const noexcept
        {
            return
                !cooked_bytes.empty();
        }
    };

    class AssetCooker
    {
    public:
        AssetCooker() = default;

        AssetCooker(
            const AssetCooker&) = delete;

        AssetCooker&
        operator=(
            const AssetCooker&) = delete;

        AssetCooker(
            AssetCooker&&) = delete;

        AssetCooker&
        operator=(
            AssetCooker&&) = delete;

        virtual ~AssetCooker() = default;

        [[nodiscard]]
        virtual std::string_view name()
            const noexcept = 0;

        [[nodiscard]]
        virtual std::uint32_t version()
            const noexcept = 0;

        [[nodiscard]]
        virtual bool supports(
            const AssetRecord& record)
            const noexcept = 0;

        [[nodiscard]]
        virtual foundation::Result<CookResult>
        cook(
            const CookRequest& request)
            const = 0;
    };
}