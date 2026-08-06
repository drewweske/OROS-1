#pragma once

#include "oros/assets/asset_importer.hpp"

#include <cstdint>
#include <string_view>

namespace oros::assets
{
    inline constexpr std::string_view
        binary_asset_importer_name{
            "oros.binary"
        };

    inline constexpr std::uint32_t
        binary_asset_importer_version{
            1U
        };

    inline constexpr std::uint32_t
        binary_asset_schema_version{
            1U
        };

    inline constexpr std::string_view
        binary_asset_source_extension{
            ".orosbin"
        };

    class BinaryAssetImporter final
        : public AssetImporter
    {
    public:
        BinaryAssetImporter() = default;

        ~BinaryAssetImporter()
            override = default;

        [[nodiscard]]
        std::string_view
        name() const noexcept override;

        [[nodiscard]]
        std::uint32_t
        version() const noexcept override;

        [[nodiscard]]
        std::uint32_t
        schema_version() const noexcept override;

        [[nodiscard]]
        bool
        supports(
            std::string_view source_path)
            const noexcept override;

        [[nodiscard]]
        foundation::Result<ImportResult>
        import(
            const ImportRequest& request)
            const override;
    };
}