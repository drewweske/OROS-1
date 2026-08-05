#pragma once

#include "oros/assets/asset_load_pipeline.hpp"
#include "oros/assets/content_hash.hpp"
#include "oros/assets/cooked_asset_cache.hpp"
#include "oros/foundation/result.hpp"

namespace oros::assets
{
    struct AssetHotReloadResult final
    {
        ResourceHandle handle{};

        ContentHash previous_hash{};
        ContentHash current_hash{};

        bool runtime_replaced{};

        [[nodiscard]]
        bool is_valid() const noexcept
        {
            return handle.is_valid();
        }
    };

    class AssetHotReloadPipeline final
    {
    public:
        AssetHotReloadPipeline(
            const CookedAssetCache& cache,
            AssetLoadPipeline& loader)
            noexcept;

        AssetHotReloadPipeline(
            const AssetHotReloadPipeline&) =
                delete;

        AssetHotReloadPipeline&
        operator=(
            const AssetHotReloadPipeline&) =
                delete;

        AssetHotReloadPipeline(
            AssetHotReloadPipeline&&) =
                delete;

        AssetHotReloadPipeline&
        operator=(
            AssetHotReloadPipeline&&) =
                delete;

        [[nodiscard]]
        foundation::Result<
            AssetHotReloadResult>
        reload(
            const AssetRecord& record);

    private:
        const CookedAssetCache*
            cache_{};

        AssetLoadPipeline*
            loader_{};
    };
}