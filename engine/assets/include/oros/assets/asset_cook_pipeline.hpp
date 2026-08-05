#pragma once

#include "oros/assets/asset_cooker_registry.hpp"
#include "oros/assets/asset_import_pipeline.hpp"
#include "oros/foundation/result.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace oros::assets
{
    struct CookedAsset final
    {
        AssetRecord record{};

        std::string cooker_name{};
        std::uint32_t cooker_version{};

        std::vector<std::byte>
            cooked_bytes{};

        [[nodiscard]]
        bool is_valid() const noexcept
        {
            return
                record.is_valid() &&
                record.is_cooked() &&
                !cooker_name.empty() &&
                cooker_version != 0U;
        }

        [[nodiscard]]
        bool has_cooked_data()
            const noexcept
        {
            return
                !cooked_bytes.empty();
        }
    };

    class AssetCookPipeline final
    {
    public:
        explicit AssetCookPipeline(
            const AssetCookerRegistry&
                registry) noexcept;

        AssetCookPipeline(
            const AssetCookPipeline&) = delete;

        AssetCookPipeline&
        operator=(
            const AssetCookPipeline&) = delete;

        AssetCookPipeline(
            AssetCookPipeline&&) = delete;

        AssetCookPipeline&
        operator=(
            AssetCookPipeline&&) = delete;

        [[nodiscard]]
        foundation::Result<CookedAsset>
        cook(
            const ImportedAsset& asset)
            const;

    private:
        const AssetCookerRegistry*
            registry_{};
    };
}