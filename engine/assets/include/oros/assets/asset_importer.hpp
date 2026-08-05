#pragma once

#include "oros/assets/asset_id.hpp"
#include "oros/foundation/result.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace oros::assets
{
    struct ImportRequest final
    {
        AssetId asset{};
        std::string_view source_path{};
        std::span<const std::byte> source_bytes{};

        [[nodiscard]]
        bool is_valid() const noexcept
        {
            return
                asset.is_valid() &&
                !source_path.empty();
        }
    };

    struct ImportResult final
    {
        std::vector<std::byte>
            intermediate_bytes{};

        std::vector<AssetId>
            dependencies{};

        [[nodiscard]]
        bool has_intermediate_data()
            const noexcept
        {
            return
                !intermediate_bytes.empty();
        }
    };

    class AssetImporter
    {
    public:
        AssetImporter() = default;

        AssetImporter(
            const AssetImporter&) = delete;

        AssetImporter&
        operator=(
            const AssetImporter&) = delete;

        AssetImporter(
            AssetImporter&&) = delete;

        AssetImporter&
        operator=(
            AssetImporter&&) = delete;

        virtual ~AssetImporter() = default;

        [[nodiscard]]
        virtual std::string_view name()
            const noexcept = 0;

        [[nodiscard]]
        virtual std::uint32_t version()
            const noexcept = 0;

        [[nodiscard]]
        virtual std::uint32_t schema_version()
            const noexcept = 0;

        [[nodiscard]]
        virtual bool supports(
            std::string_view source_path)
            const noexcept = 0;

        [[nodiscard]]
        virtual foundation::Result<ImportResult>
        import(
            const ImportRequest& request)
            const = 0;
    };
}