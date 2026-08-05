#pragma once

#include "oros/assets/asset_record.hpp"
#include "oros/assets/cooked_asset_cache.hpp"
#include "oros/assets/resource_handle.hpp"
#include "oros/assets/resource_table.hpp"
#include "oros/foundation/result.hpp"

#include <cstddef>
#include <vector>

namespace oros::assets
{
    struct LoadedAsset final
    {
        AssetRecord record{};

        std::vector<std::byte>
            runtime_bytes{};

        [[nodiscard]]
        bool is_valid() const noexcept
        {
            return
                record.is_valid() &&
                record.is_cooked();
        }

        [[nodiscard]]
        bool has_runtime_data()
            const noexcept
        {
            return
                !runtime_bytes.empty();
        }
    };

    class AssetLoadPipeline final
    {
    public:
        AssetLoadPipeline(
            const CookedAssetCache& cache,
            ResourceTable<LoadedAsset>&
                resources) noexcept;

        AssetLoadPipeline(
            const AssetLoadPipeline&) = delete;

        AssetLoadPipeline&
        operator=(
            const AssetLoadPipeline&) = delete;

        AssetLoadPipeline(
            AssetLoadPipeline&&) = delete;

        AssetLoadPipeline&
        operator=(
            AssetLoadPipeline&&) = delete;

        [[nodiscard]]
        foundation::Result<ResourceHandle>
        load(
            const AssetRecord& record);

        [[nodiscard]]
        foundation::Status unload(
            ResourceHandle handle);

        [[nodiscard]]
        LoadedAsset* find(
            ResourceHandle handle)
            noexcept;

        [[nodiscard]]
        const LoadedAsset* find(
            ResourceHandle handle)
            const noexcept;

        [[nodiscard]]
        ResourceHandle handle_for(
            AssetId asset) const noexcept;

        [[nodiscard]]
        bool contains(
            ResourceHandle handle)
            const noexcept;

        [[nodiscard]]
        bool contains(
            AssetId asset) const noexcept;

        [[nodiscard]]
        std::size_t loaded_count()
            const noexcept;

        [[nodiscard]]
        bool empty() const noexcept;

    private:
        const CookedAssetCache*
            cache_{};

        ResourceTable<LoadedAsset>*
            resources_{};
    };
}