#pragma once

#include "oros/assets/asset_cooker.hpp"
#include "oros/assets/binary_asset_importer.hpp"

#include <cstdint>
#include <string_view>

namespace oros::assets
{
    inline constexpr std::string_view
        binary_asset_cooker_name{
            "oros.binary"
        };

    inline constexpr std::uint32_t
        binary_asset_cooker_version{
            1U
        };

    class BinaryAssetCooker final
        : public AssetCooker
    {
    public:
        BinaryAssetCooker() = default;

        ~BinaryAssetCooker()
            override = default;

        [[nodiscard]]
        std::string_view
        name() const noexcept override;

        [[nodiscard]]
        std::uint32_t
        version() const noexcept override;

        [[nodiscard]]
        bool
        supports(
            const AssetRecord& record)
            const noexcept override;

        [[nodiscard]]
        foundation::Result<CookResult>
        cook(
            const CookRequest& request)
            const override;
    };
}