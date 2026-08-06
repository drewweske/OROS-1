#pragma once

#include "oros/assets/content_hash.hpp"
#include "oros/assets/cooked_asset_artifact.hpp"
#include "oros/assets/cooked_asset_artifact_serialization.hpp"
#include "oros/foundation/result.hpp"

#include <filesystem>

namespace oros::assets
{
    class CookedAssetArtifactStore final
    {
    public:
        explicit CookedAssetArtifactStore(
            std::filesystem::path root_directory,
            CookedAssetArtifactSerializationLimits
                serialization_limits = {});

        ~CookedAssetArtifactStore() = default;

        CookedAssetArtifactStore(
            const CookedAssetArtifactStore&) =
                delete;

        CookedAssetArtifactStore&
        operator=(
            const CookedAssetArtifactStore&) =
                delete;

        CookedAssetArtifactStore(
            CookedAssetArtifactStore&& other)
            noexcept;

        CookedAssetArtifactStore&
        operator=(
            CookedAssetArtifactStore&& other)
            noexcept;

        [[nodiscard]]
        bool
        is_valid() const noexcept
        {
            return
                !root_directory_.empty() &&
                serialization_limits_.is_valid();
        }

        [[nodiscard]]
        const std::filesystem::path&
        root_directory() const noexcept
        {
            return root_directory_;
        }

        [[nodiscard]]
        const CookedAssetArtifactSerializationLimits&
        serialization_limits() const noexcept
        {
            return serialization_limits_;
        }

        [[nodiscard]]
        foundation::Result<std::filesystem::path>
        path_for(
            const ContentHash& artifact_hash)
            const noexcept;

        [[nodiscard]]
        foundation::Result<ContentHash>
        store(
            const CookedAssetArtifact& artifact)
            const noexcept;

        [[nodiscard]]
        foundation::Result<CookedAssetArtifact>
        load(
            const ContentHash& artifact_hash)
            const noexcept;

        [[nodiscard]]
        foundation::Result<bool>
        contains(
            const ContentHash& artifact_hash)
            const noexcept;

        [[nodiscard]]
        foundation::Status
        erase(
            const ContentHash& artifact_hash)
            const noexcept;

    private:
        std::filesystem::path root_directory_{};

        CookedAssetArtifactSerializationLimits
            serialization_limits_{};
    };
}