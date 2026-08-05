#pragma once

#include "oros/assets/asset_importer.hpp"
#include "oros/assets/asset_importer_registry.hpp"
#include "oros/assets/asset_record.hpp"
#include "oros/foundation/result.hpp"

#include <cstddef>
#include <vector>

namespace oros::assets
{
    struct ImportedAsset final
    {
        AssetRecord record{};

        std::vector<std::byte>
            intermediate_bytes{};

        [[nodiscard]]
        bool is_valid() const noexcept
        {
            return record.is_valid();
        }

        [[nodiscard]]
        bool has_intermediate_data()
            const noexcept
        {
            return
                !intermediate_bytes.empty();
        }
    };

    class AssetImportPipeline final
    {
    public:
        explicit AssetImportPipeline(
            const AssetImporterRegistry&
                importer_registry) noexcept;

        AssetImportPipeline(
            const AssetImportPipeline&) = delete;

        AssetImportPipeline&
        operator=(
            const AssetImportPipeline&) = delete;

        AssetImportPipeline(
            AssetImportPipeline&&) = delete;

        AssetImportPipeline&
        operator=(
            AssetImportPipeline&&) = delete;

        [[nodiscard]]
        foundation::Result<ImportedAsset>
        import(
            const ImportRequest& request)
            const;

    private:
        const AssetImporterRegistry*
            importer_registry_{};
    };
}