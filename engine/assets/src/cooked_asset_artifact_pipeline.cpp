#include "oros/assets/cooked_asset_artifact_pipeline.hpp"

#include "oros/assets/content_hash.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace oros::assets
{
    foundation::Result<CookedAssetArtifact>
    build_cooked_asset_artifact(
        const CookedAssetArtifactBuildRequest&
            request) noexcept
    {
        if (!request.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cannot build a cooked artifact from "
                "an invalid request.");
        }

        const CookedAsset& asset =
            *request.asset;

        if (!asset.record.cooked_hash.
                has_value())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "The cooked asset does not contain a "
                "cooked content hash.");
        }

        const ContentHash actual_hash =
            hash_bytes(
                std::span<const std::byte>{
                    asset.cooked_bytes
                });

        if (actual_hash !=
            asset.record.cooked_hash.value())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "The cooked asset payload does not "
                "match its cooked content hash.");
        }

        if (!std::in_range<std::uint64_t>(
                asset.cooked_bytes.size()))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "The cooked asset byte count cannot "
                "be represented by the artifact "
                "format.");
        }

        try
        {
            CookedAssetArtifact artifact{};

            artifact.record =
                asset.record;

            artifact.cooker_name =
                asset.cooker_name;

            artifact.cooker_version =
                asset.cooker_version;

            artifact.uncompressed_byte_count =
                static_cast<std::uint64_t>(
                    asset.cooked_bytes.size());

            if (request.compression_policy ==
                CookedAssetArtifactCompressionPolicy::
                    disabled)
            {
                artifact.payload_bytes =
                    asset.cooked_bytes;

                if (!artifact.is_valid())
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            internal_failure,
                        "Building an uncompressed "
                        "artifact produced invalid "
                        "metadata.");
                }

                return artifact;
            }

            const AssetCompressionCodec& codec =
                *request.compression_codec;

            const std::string_view
                codec_name_view =
                    codec.name();

            const std::uint32_t codec_version =
                codec.version();

            if (codec_name_view.empty() ||
                codec_version == 0U)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_state,
                    "The artifact compression codec "
                    "exposes invalid metadata.");
            }

            const std::string codec_name{
                codec_name_view
            };

            const AssetCompressionRequest
                compression_request{
                    std::span<const std::byte>{
                        asset.cooked_bytes
                    }
                };

            if (!compression_request.is_valid())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "The artifact pipeline produced "
                    "an invalid compression request.");
            }

            foundation::Result<
                AssetCompressionResult>
                compression_result =
                    codec.compress(
                        compression_request);

            if (!compression_result.has_value())
            {
                return foundation::fail(
                    compression_result.error().code,
                    compression_result.error().
                        message);
            }

            if (codec.name() != codec_name ||
                codec.version() != codec_version)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_state,
                    "The artifact compression codec "
                    "changed its metadata while "
                    "compressing.");
            }

            AssetCompressionResult&
                compressed =
                    compression_result.value();

            if (!compressed.is_valid() ||
                compressed.original_byte_count !=
                    asset.cooked_bytes.size())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_state,
                    "The artifact compression codec "
                    "returned an invalid result.");
            }

            const bool compression_required =
                request.compression_policy ==
                CookedAssetArtifactCompressionPolicy::
                    required;

            const bool compression_smaller =
                compressed.bytes.size() <
                asset.cooked_bytes.size();

            if (compression_required ||
                compression_smaller)
            {
                artifact.compression_codec_name =
                    codec_name;

                artifact.
                    compression_codec_version =
                        codec_version;

                artifact.payload_bytes =
                    std::move(
                        compressed.bytes);
            }
            else
            {
                artifact.payload_bytes =
                    asset.cooked_bytes;
            }

            if (!artifact.is_valid())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "Building a cooked artifact "
                    "produced invalid metadata.");
            }

            return artifact;
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Unable to allocate cooked artifact "
                "packaging storage.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "An unexpected failure occurred while "
                "building a cooked artifact.");
        }
    }

    foundation::Result<CookedAsset>
    materialize_cooked_asset_artifact(
        const CookedAssetArtifactMaterializationRequest&
            request) noexcept
    {
        if (!request.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cannot materialize a cooked artifact "
                "from an invalid request.");
        }

        const CookedAssetArtifact& artifact =
            *request.artifact;

        if (!artifact.record.cooked_hash.
                has_value())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "The cooked artifact does not contain "
                "a cooked content hash.");
        }

        if (!std::in_range<std::size_t>(
                artifact.
                    uncompressed_byte_count))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "The artifact's decoded byte count "
                "cannot be represented on this "
                "platform.");
        }

        try
        {
            std::vector<std::byte>
                materialized_bytes{};

            if (!artifact.is_compressed())
            {
                materialized_bytes =
                    artifact.payload_bytes;
            }
            else
            {
                const AssetCompressionCodec& codec =
                    *request.compression_codec;

                const std::string_view
                    codec_name_view =
                        codec.name();

                const std::uint32_t codec_version =
                    codec.version();

                if (codec_name_view !=
                        artifact.
                            compression_codec_name ||
                    codec_version !=
                        artifact.
                            compression_codec_version)
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "The materialization codec does "
                        "not match the artifact "
                        "metadata.");
                }

                const std::string codec_name{
                    codec_name_view
                };

                const std::size_t
                    expected_byte_count =
                        static_cast<std::size_t>(
                            artifact.
                                uncompressed_byte_count);

                const AssetDecompressionRequest
                    decompression_request{
                        std::span<const std::byte>{
                            artifact.payload_bytes
                        },
                        expected_byte_count
                    };

                if (!decompression_request.is_valid())
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "The cooked artifact produced "
                        "an invalid decompression "
                        "request.");
                }

                foundation::Result<
                    std::vector<std::byte>>
                    decompression_result =
                        codec.decompress(
                            decompression_request);

                if (!decompression_result.
                        has_value())
                {
                    return foundation::fail(
                        decompression_result.error().
                            code,
                        decompression_result.error().
                            message);
                }

                if (codec.name() != codec_name ||
                    codec.version() != codec_version)
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_state,
                        "The artifact compression codec "
                        "changed its metadata while "
                        "decompressing.");
                }

                materialized_bytes =
                    std::move(
                        decompression_result.value());

                if (materialized_bytes.size() !=
                    expected_byte_count)
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_state,
                        "The artifact compression codec "
                        "returned an unexpected decoded "
                        "byte count.");
                }
            }

            if (!std::in_range<std::uint64_t>(
                    materialized_bytes.size()) ||
                static_cast<std::uint64_t>(
                    materialized_bytes.size()) !=
                    artifact.
                        uncompressed_byte_count)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "The materialized artifact byte "
                    "count does not match its "
                    "metadata.");
            }

            const ContentHash actual_hash =
                hash_bytes(
                    std::span<const std::byte>{
                        materialized_bytes
                    });

            if (actual_hash !=
                artifact.record.
                    cooked_hash.value())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "The materialized artifact payload "
                    "does not match its cooked content "
                    "hash.");
            }

            CookedAsset asset{};

            asset.record =
                artifact.record;

            asset.cooker_name =
                artifact.cooker_name;

            asset.cooker_version =
                artifact.cooker_version;

            asset.cooked_bytes =
                std::move(
                    materialized_bytes);

            if (!asset.is_valid())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "Materializing the cooked artifact "
                    "produced an invalid cooked asset.");
            }

            return asset;
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Unable to allocate cooked artifact "
                "materialization storage.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "An unexpected failure occurred while "
                "materializing a cooked artifact.");
        }
    }
}