#include "oros/assets/cooked_asset_artifact.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string_view>
#include <type_traits>
#include <vector>

namespace
{
    struct TestState final
    {
        int checks{};
        int failures{};
    };

    void check(
        TestState& state,
        const bool condition,
        const std::string_view name)
    {
        ++state.checks;

        if (condition)
        {
            std::cout
                << "[pass] "
                << name
                << '\n';

            return;
        }

        ++state.failures;

        std::cerr
            << "[fail] "
            << name
            << '\n';
    }

    [[nodiscard]]
    oros::assets::AssetRecord
    make_valid_cooked_record()
    {
        using namespace oros::assets;

        AssetRecord record{};

        record.id =
            AssetId{
                0x4F524F53ULL,
                100ULL
            };

        record.source_path =
            "textures/terrain/rock.png";

        record.importer_name =
            "oros.texture";

        record.importer_version = 3U;
        record.schema_version = 2U;

        record.source_hash =
            hash_text(
                "source texture data");

        record.cooked_hash =
            hash_text(
                "cooked texture data");

        record.dependencies = {
            AssetId{
                0x4F524F53ULL,
                200ULL
            },
            AssetId{
                0x4F524F53ULL,
                201ULL
            }
        };

        return record;
    }

    [[nodiscard]]
    oros::assets::CookedAssetArtifact
    make_valid_uncompressed_artifact()
    {
        using namespace oros::assets;

        CookedAssetArtifact artifact{};

        artifact.record =
            make_valid_cooked_record();

        artifact.cooker_name =
            "oros.texture";

        artifact.cooker_version = 4U;

        artifact.uncompressed_byte_count =
            4ULL;

        artifact.payload_bytes = {
            std::byte{0x10},
            std::byte{0x20},
            std::byte{0x30},
            std::byte{0x40}
        };

        return artifact;
    }

    [[nodiscard]]
    oros::assets::CookedAssetArtifact
    make_valid_compressed_artifact()
    {
        using namespace oros::assets;

        CookedAssetArtifact artifact =
            make_valid_uncompressed_artifact();

        artifact.compression_codec_name =
            "oros.rle";

        artifact.compression_codec_version =
            1U;

        artifact.uncompressed_byte_count =
            12ULL;

        artifact.payload_bytes = {
            std::byte{0x0C},
            std::byte{0x7A}
        };

        return artifact;
    }

    static_assert(
        std::is_same_v<
            decltype(
                oros::assets::
                    CookedAssetArtifact{}.
                        uncompressed_byte_count),
            std::uint64_t>);

    static_assert(
        std::is_same_v<
            decltype(
                oros::assets::
                    CookedAssetArtifact{}.
                        encoded_byte_count()),
            std::uint64_t>);
}

int main()
{
    using namespace oros::assets;

    TestState state{};

    check(
        state,
        cooked_asset_artifact_format_version ==
            1U,
        "Cooked artifact format begins at version one");

    const CookedAssetArtifact
        default_artifact{};

    check(
        state,
        !default_artifact.is_valid(),
        "Default cooked artifact is invalid");

    check(
        state,
        !default_artifact.is_compressed(),
        "Default cooked artifact is not compressed");

    check(
        state,
        !default_artifact.has_payload_data(),
        "Default cooked artifact has no payload data");

    check(
        state,
        default_artifact.
            encoded_byte_count() == 0ULL,
        "Default cooked artifact has zero encoded bytes");

    const CookedAssetArtifact
        uncompressed_artifact =
            make_valid_uncompressed_artifact();

    check(
        state,
        uncompressed_artifact.is_valid(),
        "Complete uncompressed artifact is valid");

    check(
        state,
        !uncompressed_artifact.is_compressed(),
        "Uncompressed artifact reports no compression");

    check(
        state,
        uncompressed_artifact.
            has_payload_data(),
        "Uncompressed artifact reports payload data");

    check(
        state,
        uncompressed_artifact.
            encoded_byte_count() == 4ULL,
        "Uncompressed artifact reports its encoded size");

    check(
        state,
        uncompressed_artifact.
            uncompressed_byte_count == 4ULL,
        "Uncompressed artifact records its original size");

    check(
        state,
        uncompressed_artifact.record.
            is_cooked(),
        "Valid artifact contains a cooked record");

    check(
        state,
        uncompressed_artifact.cooker_name ==
            "oros.texture",
        "Valid artifact preserves its cooker name");

    check(
        state,
        uncompressed_artifact.cooker_version ==
            4U,
        "Valid artifact preserves its cooker version");

    CookedAssetArtifact
        empty_uncompressed_artifact =
            make_valid_uncompressed_artifact();

    empty_uncompressed_artifact.
        uncompressed_byte_count = 0ULL;

    empty_uncompressed_artifact.
        payload_bytes.clear();

    check(
        state,
        empty_uncompressed_artifact.is_valid(),
        "Empty uncompressed payload is valid");

    check(
        state,
        !empty_uncompressed_artifact.
            has_payload_data(),
        "Empty uncompressed artifact has no payload data");

    check(
        state,
        empty_uncompressed_artifact.
            encoded_byte_count() == 0ULL,
        "Empty uncompressed artifact has zero encoded bytes");

    CookedAssetArtifact
        wrong_format_artifact =
            make_valid_uncompressed_artifact();

    wrong_format_artifact.format_version =
        cooked_asset_artifact_format_version +
        1U;

    check(
        state,
        !wrong_format_artifact.is_valid(),
        "Unsupported artifact format is invalid");

    CookedAssetArtifact
        invalid_record_artifact =
            make_valid_uncompressed_artifact();

    invalid_record_artifact.record.id = {};

    check(
        state,
        !invalid_record_artifact.is_valid(),
        "Artifact with invalid asset metadata is invalid");

    CookedAssetArtifact
        uncooked_record_artifact =
            make_valid_uncompressed_artifact();

    uncooked_record_artifact.record.
        cooked_hash.reset();

    check(
        state,
        !uncooked_record_artifact.is_valid(),
        "Artifact with an uncooked record is invalid");

    CookedAssetArtifact
        missing_cooker_name_artifact =
            make_valid_uncompressed_artifact();

    missing_cooker_name_artifact.
        cooker_name.clear();

    check(
        state,
        !missing_cooker_name_artifact.is_valid(),
        "Artifact without a cooker name is invalid");

    CookedAssetArtifact
        missing_cooker_version_artifact =
            make_valid_uncompressed_artifact();

    missing_cooker_version_artifact.
        cooker_version = 0U;

    check(
        state,
        !missing_cooker_version_artifact.
            is_valid(),
        "Artifact without a cooker version is invalid");

    CookedAssetArtifact
        codec_name_without_version =
            make_valid_uncompressed_artifact();

    codec_name_without_version.
        compression_codec_name =
            "oros.rle";

    check(
        state,
        !codec_name_without_version.is_valid(),
        "Compression name without a version is invalid");

    CookedAssetArtifact
        codec_version_without_name =
            make_valid_uncompressed_artifact();

    codec_version_without_name.
        compression_codec_version = 1U;

    check(
        state,
        !codec_version_without_name.is_valid(),
        "Compression version without a name is invalid");

    CookedAssetArtifact
        uncompressed_count_too_small =
            make_valid_uncompressed_artifact();

    uncompressed_count_too_small.
        uncompressed_byte_count = 3ULL;

    check(
        state,
        !uncompressed_count_too_small.is_valid(),
        "Uncompressed payload larger than its declared size is invalid");

    CookedAssetArtifact
        uncompressed_count_too_large =
            make_valid_uncompressed_artifact();

    uncompressed_count_too_large.
        uncompressed_byte_count = 5ULL;

    check(
        state,
        !uncompressed_count_too_large.is_valid(),
        "Uncompressed payload smaller than its declared size is invalid");

    const CookedAssetArtifact
        compressed_artifact =
            make_valid_compressed_artifact();

    check(
        state,
        compressed_artifact.is_valid(),
        "Complete compressed artifact is valid");

    check(
        state,
        compressed_artifact.is_compressed(),
        "Compressed artifact reports compression");

    check(
        state,
        compressed_artifact.
            has_payload_data(),
        "Compressed artifact reports encoded data");

    check(
        state,
        compressed_artifact.
            encoded_byte_count() == 2ULL,
        "Compressed artifact reports its encoded size");

    check(
        state,
        compressed_artifact.
            uncompressed_byte_count == 12ULL,
        "Compressed artifact preserves its decoded size");

    check(
        state,
        compressed_artifact.
            compression_codec_name ==
            "oros.rle",
        "Compressed artifact preserves its codec name");

    check(
        state,
        compressed_artifact.
            compression_codec_version == 1U,
        "Compressed artifact preserves its codec version");

    CookedAssetArtifact
        empty_compressed_artifact =
            make_valid_compressed_artifact();

    empty_compressed_artifact.
        uncompressed_byte_count = 0ULL;

    empty_compressed_artifact.
        payload_bytes.clear();

    check(
        state,
        empty_compressed_artifact.is_valid(),
        "Empty compressed source with no encoded data is valid");

    check(
        state,
        empty_compressed_artifact.
            is_compressed(),
        "Empty compressed artifact retains its codec identity");

    check(
        state,
        !empty_compressed_artifact.
            has_payload_data(),
        "Empty compressed artifact may contain no encoded data");

    CookedAssetArtifact
        missing_compressed_payload =
            make_valid_compressed_artifact();

    missing_compressed_payload.
        payload_bytes.clear();

    check(
        state,
        !missing_compressed_payload.is_valid(),
        "Nonempty compressed source requires encoded data");

    CookedAssetArtifact
        encoded_empty_compressed_artifact =
            make_valid_compressed_artifact();

    encoded_empty_compressed_artifact.
        uncompressed_byte_count = 0ULL;

    encoded_empty_compressed_artifact.
        payload_bytes = {
            std::byte{0x4F},
            std::byte{0x52}
        };

    check(
        state,
        encoded_empty_compressed_artifact.
            is_valid(),
        "Codec header for an empty source is valid");

    check(
        state,
        encoded_empty_compressed_artifact.
            has_payload_data(),
        "Encoded empty source reports codec data");

    check(
        state,
        encoded_empty_compressed_artifact.
            encoded_byte_count() == 2ULL,
        "Encoded empty source reports its header size");

    std::cout
        << "\nCooked asset artifact test summary: "
        << state.checks - state.failures
        << '/'
        << state.checks
        << " passed.\n";

    return
        state.failures == 0
            ? 0
            : 1;
}