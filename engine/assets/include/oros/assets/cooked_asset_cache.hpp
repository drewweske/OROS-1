#pragma once

#include "oros/assets/asset_cook_pipeline.hpp"
#include "oros/assets/content_hash.hpp"
#include "oros/foundation/result.hpp"

#include <cstddef>
#include <unordered_map>
#include <vector>

namespace oros::assets
{
    class CookedAssetCache final
    {
    public:
        CookedAssetCache() = default;

        CookedAssetCache(
            const CookedAssetCache&) = delete;

        CookedAssetCache&
        operator=(
            const CookedAssetCache&) = delete;

        CookedAssetCache(
            CookedAssetCache&& other) noexcept;

        CookedAssetCache&
        operator=(
            CookedAssetCache&& other) noexcept;

        [[nodiscard]]
        foundation::Result<bool> store(
            const CookedAsset& asset);

        [[nodiscard]]
        foundation::Status erase(
            const ContentHash& hash);

        void clear() noexcept;

        [[nodiscard]]
        const std::vector<std::byte>* find(
            const ContentHash& hash)
            const noexcept;

        [[nodiscard]]
        bool contains(
            const ContentHash& hash)
            const noexcept;

        [[nodiscard]]
        std::size_t size() const noexcept;

        [[nodiscard]]
        std::size_t total_byte_count()
            const noexcept;

        [[nodiscard]]
        bool empty() const noexcept;

    private:
        std::unordered_map<
            ContentHash,
            std::vector<std::byte>,
            ContentHashHash
        > entries_{};

        std::size_t total_byte_count_{};
    };
}