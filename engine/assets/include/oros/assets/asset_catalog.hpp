#pragma once

#include "oros/assets/asset_id.hpp"
#include "oros/assets/asset_record.hpp"
#include "oros/foundation/result.hpp"

#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <unordered_map>

namespace oros::assets
{
    class AssetCatalog final
    {
    public:
        AssetCatalog() = default;

        AssetCatalog(
            const AssetCatalog&) = default;

        AssetCatalog&
        operator=(
            const AssetCatalog&) = default;

        AssetCatalog(
            AssetCatalog&&) noexcept = default;

        AssetCatalog&
        operator=(
            AssetCatalog&&) noexcept = default;

        [[nodiscard]]
        foundation::Status insert(
            AssetRecord record);

        [[nodiscard]]
        foundation::Status replace(
            AssetRecord record);

        [[nodiscard]]
        foundation::Status erase(
            AssetId asset);

        [[nodiscard]]
        const AssetRecord* find(
            AssetId asset) const noexcept;

        [[nodiscard]]
        const AssetRecord* find_by_source_path(
            std::string_view source_path)
            const noexcept;

        [[nodiscard]]
        AssetId id_for_source_path(
            std::string_view source_path)
            const noexcept;

        [[nodiscard]]
        bool contains(
            AssetId asset) const noexcept;

        [[nodiscard]]
        bool contains_source_path(
            std::string_view source_path)
            const noexcept;

        [[nodiscard]]
        std::size_t size() const noexcept;

        [[nodiscard]]
        bool empty() const noexcept;

    private:
        std::unordered_map<
            AssetId,
            AssetRecord,
            AssetIdHash
        > records_{};

        std::map<
            std::string,
            AssetId,
            std::less<>
        > by_source_path_{};
    };
}