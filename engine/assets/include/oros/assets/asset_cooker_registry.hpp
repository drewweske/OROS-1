#pragma once

#include "oros/assets/asset_cooker.hpp"
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
    class AssetCookerRegistry final
    {
    public:
        AssetCookerRegistry() = default;

        AssetCookerRegistry(
            const AssetCookerRegistry&) = delete;

        AssetCookerRegistry&
        operator=(
            const AssetCookerRegistry&) = delete;

        AssetCookerRegistry(
            AssetCookerRegistry&& other) noexcept;

        AssetCookerRegistry&
        operator=(
            AssetCookerRegistry&& other) noexcept;

        [[nodiscard]]
        foundation::Status register_cooker(
            std::unique_ptr<AssetCooker> cooker);

        [[nodiscard]]
        foundation::Status unregister_cooker(
            std::string_view cooker_name);

        [[nodiscard]]
        const AssetCooker* find(
            std::string_view cooker_name)
            const noexcept;

        [[nodiscard]]
        foundation::Result<
            const AssetCooker*>
        resolve(
            const AssetRecord& record)
            const;

        [[nodiscard]]
        std::size_t size() const noexcept;

        [[nodiscard]]
        bool empty() const noexcept;

    private:
        std::vector<
            std::unique_ptr<AssetCooker>
        > cookers_{};

        std::map<
            std::string,
            std::size_t,
            std::less<>
        > by_name_{};
    };
}