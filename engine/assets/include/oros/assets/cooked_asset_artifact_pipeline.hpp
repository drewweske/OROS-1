#pragma once

#include "oros/assets/asset_compression_codec.hpp"
#include "oros/assets/asset_cook_pipeline.hpp"
#include "oros/assets/cooked_asset_artifact.hpp"
#include "oros/foundation/result.hpp"

#include <cstdint>

namespace oros::assets
{
    enum class CookedAssetArtifactCompressionPolicy
        : std::uint8_t
    {
        disabled = 0U,
        when_smaller,
        required
    };

    struct CookedAssetArtifactBuildRequest final
    {
        const CookedAsset* asset{};

        const AssetCompressionCodec*
            compression_codec{};

        CookedAssetArtifactCompressionPolicy
            compression_policy{
                CookedAssetArtifactCompressionPolicy::
                    when_smaller
            };

        [[nodiscard]]
        bool
        is_valid() const noexcept
        {
            if (asset == nullptr ||
                !asset->is_valid())
            {
                return false;
            }

            switch (compression_policy)
            {
            case CookedAssetArtifactCompressionPolicy::
                disabled:
                return compression_codec == nullptr;

            case CookedAssetArtifactCompressionPolicy::
                when_smaller:
            case CookedAssetArtifactCompressionPolicy::
                required:
                return
                    compression_codec != nullptr &&
                    !compression_codec->name().empty() &&
                    compression_codec->version() != 0U;
            }

            return false;
        }
    };

    struct CookedAssetArtifactMaterializationRequest final
    {
        const CookedAssetArtifact* artifact{};

        const AssetCompressionCodec*
            compression_codec{};

        [[nodiscard]]
        bool
        is_valid() const noexcept
        {
            if (artifact == nullptr ||
                !artifact->is_valid())
            {
                return false;
            }

            if (!artifact->is_compressed())
            {
                return true;
            }

            return
                compression_codec != nullptr &&
                compression_codec->name() ==
                    artifact->
                        compression_codec_name &&
                compression_codec->version() ==
                    artifact->
                        compression_codec_version;
        }
    };

    [[nodiscard]]
    foundation::Result<CookedAssetArtifact>
    build_cooked_asset_artifact(
        const CookedAssetArtifactBuildRequest&
            request) noexcept;

    [[nodiscard]]
    foundation::Result<CookedAsset>
    materialize_cooked_asset_artifact(
        const CookedAssetArtifactMaterializationRequest&
            request) noexcept;
}