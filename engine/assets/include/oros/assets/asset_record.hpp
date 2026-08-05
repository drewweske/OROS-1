#pragma once

#include "oros/assets/asset_id.hpp"
#include "oros/assets/content_hash.hpp"

#include <compare>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace oros::assets
{
    struct AssetRecord final
    {
        AssetId id{};

        std::string source_path{};
        std::string importer_name{};

        std::uint32_t importer_version{};
        std::uint32_t schema_version{};

        ContentHash source_hash{};

        std::optional<ContentHash>
            cooked_hash{};

        std::vector<AssetId>
            dependencies{};

        [[nodiscard]]
        bool is_valid() const noexcept
        {
            if (!id.is_valid() ||
                source_path.empty() ||
                importer_name.empty() ||
                importer_version == 0U ||
                schema_version == 0U)
            {
                return false;
            }

            for (std::size_t index = 0U;
                 index < dependencies.size();
                 ++index)
            {
                const AssetId dependency =
                    dependencies[index];

                if (!dependency.is_valid() ||
                    dependency == id)
                {
                    return false;
                }

                for (std::size_t earlier = 0U;
                     earlier < index;
                     ++earlier)
                {
                    if (dependencies[earlier] ==
                        dependency)
                    {
                        return false;
                    }
                }
            }

            return true;
        }

        [[nodiscard]]
        bool is_cooked() const noexcept
        {
            return cooked_hash.has_value();
        }

        auto operator<=>(
            const AssetRecord&) const = default;
    };
}