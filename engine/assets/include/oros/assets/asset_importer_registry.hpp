#pragma once

#include "oros/assets/asset_importer.hpp"
#include "oros/foundation/result.hpp"

#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace oros::assets
{
    class AssetImporterRegistry final
    {
    public:
        AssetImporterRegistry() = default;

        AssetImporterRegistry(
            const AssetImporterRegistry&) = delete;

        AssetImporterRegistry&
        operator=(
            const AssetImporterRegistry&) = delete;

        AssetImporterRegistry(
            AssetImporterRegistry&& other) noexcept;

        AssetImporterRegistry&
        operator=(
            AssetImporterRegistry&& other) noexcept;

        [[nodiscard]]
        foundation::Status register_importer(
            std::unique_ptr<AssetImporter> importer);

        [[nodiscard]]
        foundation::Status unregister_importer(
            std::string_view importer_name);

        [[nodiscard]]
        const AssetImporter* find(
            std::string_view importer_name)
            const noexcept;

        [[nodiscard]]
        foundation::Result<
            const AssetImporter*>
        resolve(
            std::string_view source_path)
            const;

        [[nodiscard]]
        std::size_t size() const noexcept;

        [[nodiscard]]
        bool empty() const noexcept;

    private:
        std::vector<
            std::unique_ptr<AssetImporter>
        > importers_{};

        std::map<
            std::string,
            std::size_t,
            std::less<>
        > by_name_{};
    };
}