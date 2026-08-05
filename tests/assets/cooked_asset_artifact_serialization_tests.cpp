#include "oros/assets/cooked_asset_artifact_serialization.hpp"

#include "oros/assets/content_hash.hpp"
#include "oros/foundation/error.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <string_view>
#include <vector>

namespace
{
    inline constexpr std::size_t
        format_version_offset{8U};

    inline constexpr std::size_t
        flags_offset{12U};

    inline constexpr std::size_t
        asset_namespace_offset{16U};

    inline constexpr std::size_t
        importer_version_offset{32U};

    inline constexpr std::size_t
        schema_version_offset{36U};

    inline constexpr std::size_t
        cooker_version_offset{40U};

    inline constexpr std::size_t
        codec_version_offset{44U};

    inline constexpr std::size_t
        reserved_u32_offset{68U};

    inline constexpr std::size_t
        uncompressed_byte_count_offset{72U};

    inline constexpr std::size_t
        payload_byte_count_offset{80U};

    inline constexpr std::size_t
        cooked_hash_offset{120U};

    inline constexpr std::size_t
        reserved_u64_offset{152U};

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

    void write_u32_little_endian(
        std::vector<std::byte>& bytes,
        const std::size_t offset,
        const std::uint32_t value) noexcept
    {
        for (std::size_t index = 0U;
             index < 4U;
             ++index)
        {
            const auto shift =
                static_cast<unsigned int>(
                    index * 8U);

            const auto component =
                static_cast<unsigned char>(
                    (value >> shift) &
                    0xFFU);

            bytes[offset + index] =
                static_cast<std::byte>(
                    component);
        }
    }

    void write_u64_little_endian(
        std::vector<std::byte>& bytes,
        const std::size_t offset,
        const std::uint64_t value) noexcept
    {
        for (std::size_t index = 0U;
             index < 8U;
             ++index)
        {
            const auto shift =
                static_cast<unsigned int>(
                    index * 8U);

            const auto component =
                static_cast<unsigned char>(
                    (value >> shift) &
                    0xFFULL);

            bytes[offset + index] =
                static_cast<std::byte>(
                    component);
        }
    }

    [[nodiscard]]
    std::uint32_t read_u32_little_endian(
        const std::span<const std::byte> bytes,
        const std::size_t offset) noexcept
    {
        std::uint32_t value{};

        for (std::size_t index = 0U;
             index < 4U;
             ++index)
        {
            const auto shift =
                static_cast<unsigned int>(
                    index * 8U);

            value |=
                static_cast<std::uint32_t>(
                    std::to_integer<
                        unsigned char>(
                            bytes[offset + index]))
                << shift;
        }

        return value;
    }

    [[nodiscard]]
    oros::assets::AssetRecord
    make_valid_cooked_record(
        const std::span<const std::byte>
            cooked_bytes)
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
            hash_bytes(
                cooked_bytes);

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
    make_uncompressed_artifact()
    {
        using namespace oros::assets;

        CookedAssetArtifact artifact{};

        artifact.payload_bytes = {
            std::byte{0x10},
            std::byte{0x20},
            std::byte{0x30},
            std::byte{0x40}
        };

        artifact.record =
            make_valid_cooked_record(
                std::span<const std::byte>{
                    artifact.payload_bytes
                });

        artifact.cooker_name =
            "oros.texture";

        artifact.cooker_version = 4U;

        artifact.uncompressed_byte_count =
            static_cast<std::uint64_t>(
                artifact.payload_bytes.size());

        return artifact;
    }

    [[nodiscard]]
    oros::assets::CookedAssetArtifact
    make_compressed_artifact()
    {
        using namespace oros::assets;

        const std::vector<std::byte>
            decoded_bytes(
                12U,
                std::byte{0x7A});

        CookedAssetArtifact artifact{};

        artifact.record =
            make_valid_cooked_record(
                std::span<const std::byte>{
                    decoded_bytes
                });

        artifact.cooker_name =
            "oros.texture";

        artifact.cooker_version = 4U;

        artifact.compression_codec_name =
            "oros.rle";

        artifact.compression_codec_version =
            1U;

        artifact.uncompressed_byte_count =
            static_cast<std::uint64_t>(
                decoded_bytes.size());

        artifact.payload_bytes = {
            std::byte{0x0C},
            std::byte{0x7A}
        };

        return artifact;
    }

    [[nodiscard]]
    bool artifacts_equal(
        const oros::assets::
            CookedAssetArtifact& left,
        const oros::assets::
            CookedAssetArtifact& right)
    {
        return
            left.format_version ==
                right.format_version &&
            left.record ==
                right.record &&
            left.cooker_name ==
                right.cooker_name &&
            left.cooker_version ==
                right.cooker_version &&
            left.compression_codec_name ==
                right.compression_codec_name &&
            left.compression_codec_version ==
                right.compression_codec_version &&
            left.uncompressed_byte_count ==
                right.uncompressed_byte_count &&
            left.payload_bytes ==
                right.payload_bytes;
    }

    template <typename T>
    void check_failure(
        TestState& state,
        const oros::foundation::Result<T>& result,
        const oros::foundation::ErrorCode
            expected_code,
        const std::string_view name)
    {
        const bool passed =
            !result.has_value() &&
            result.error().code ==
                expected_code;

        check(
            state,
            passed,
            name);
    }

    [[nodiscard]]
    std::uint64_t expected_serialized_byte_count(
        const oros::assets::
            CookedAssetArtifact& artifact)
    {
        return
            oros::assets::
                cooked_asset_artifact_header_byte_count +
            static_cast<std::uint64_t>(
                artifact.record.source_path.size()) +
            static_cast<std::uint64_t>(
                artifact.record.importer_name.size()) +
            static_cast<std::uint64_t>(
                artifact.cooker_name.size()) +
            static_cast<std::uint64_t>(
                artifact.
                    compression_codec_name.size()) +
            static_cast<std::uint64_t>(
                artifact.record.dependencies.size()) *
                16ULL +
            static_cast<std::uint64_t>(
                artifact.payload_bytes.size());
    }
}

int main()
{
    using namespace oros::assets;
    using oros::foundation::ErrorCode;

    TestState state{};

    static_assert(
        cooked_asset_artifact_header_byte_count ==
            160ULL);

    const CookedAssetArtifactSerializationLimits
        default_limits{};

    check(
        state,
        default_limits.is_valid(),
        "Default serialization limits are valid");

    CookedAssetArtifactSerializationLimits
        header_too_large_limits =
            default_limits;

    header_too_large_limits.
        max_artifact_byte_count =
            cooked_asset_artifact_header_byte_count -
            1ULL;

    check(
        state,
        !header_too_large_limits.is_valid(),
        "Artifact limit smaller than the header is invalid");

    CookedAssetArtifactSerializationLimits
        payload_larger_than_artifact_limits =
            default_limits;

    payload_larger_than_artifact_limits.
        max_payload_byte_count =
            payload_larger_than_artifact_limits.
                max_artifact_byte_count +
            1ULL;

    check(
        state,
        !payload_larger_than_artifact_limits.
            is_valid(),
        "Payload limit larger than artifact limit is invalid");

    CookedAssetArtifactSerializationLimits
        zero_source_path_limit =
            default_limits;

    zero_source_path_limit.
        max_source_path_byte_count = 0U;

    check(
        state,
        !zero_source_path_limit.is_valid(),
        "Zero source path limit is invalid");

    CookedAssetArtifactSerializationLimits
        zero_name_limit =
            default_limits;

    zero_name_limit.max_name_byte_count = 0U;

    check(
        state,
        !zero_name_limit.is_valid(),
        "Zero component name limit is invalid");

    const CookedAssetArtifact
        uncompressed_artifact =
            make_uncompressed_artifact();

    const auto uncompressed_result =
        serialize_cooked_asset_artifact(
            uncompressed_artifact);

    check(
        state,
        uncompressed_result.has_value(),
        "Valid uncompressed artifact serializes");

    if (!uncompressed_result.has_value())
    {
        std::cout
            << "\nCooked asset artifact serialization "
               "test summary: "
            << state.checks - state.failures
            << '/'
            << state.checks
            << " passed.\n";

        return 1;
    }

    const std::vector<std::byte>
        uncompressed_bytes =
            uncompressed_result.value();

    check(
        state,
        static_cast<std::uint64_t>(
            uncompressed_bytes.size()) ==
            expected_serialized_byte_count(
                uncompressed_artifact),
        "Uncompressed artifact has the expected serialized size");

    check(
        state,
        uncompressed_bytes.size() >=
                cooked_asset_artifact_magic.size() &&
            std::equal(
                cooked_asset_artifact_magic.begin(),
                cooked_asset_artifact_magic.end(),
                uncompressed_bytes.begin()),
        "Serialized artifact begins with the stable magic value");

    check(
        state,
        read_u32_little_endian(
            std::span<const std::byte>{
                uncompressed_bytes
            },
            format_version_offset) ==
            cooked_asset_artifact_format_version,
        "Format version is encoded in little-endian order");

    check(
        state,
        read_u32_little_endian(
            std::span<const std::byte>{
                uncompressed_bytes
            },
            flags_offset) == 0U,
        "Uncompressed artifact has no compression flag");

    const auto deterministic_result =
        serialize_cooked_asset_artifact(
            uncompressed_artifact);

    check(
        state,
        deterministic_result.has_value() &&
            deterministic_result.value() ==
                uncompressed_bytes,
        "Repeated serialization is byte-for-byte deterministic");

    const auto uncompressed_round_trip =
        deserialize_cooked_asset_artifact(
            std::span<const std::byte>{
                uncompressed_bytes
            });

    check(
        state,
        uncompressed_round_trip.has_value(),
        "Uncompressed artifact deserializes");

    check(
        state,
        uncompressed_round_trip.has_value() &&
            artifacts_equal(
                uncompressed_artifact,
                uncompressed_round_trip.value()),
        "Uncompressed artifact survives an exact round trip");

    const CookedAssetArtifact
        compressed_artifact =
            make_compressed_artifact();

    const auto compressed_result =
        serialize_cooked_asset_artifact(
            compressed_artifact);

    check(
        state,
        compressed_result.has_value(),
        "Valid compressed artifact serializes");

    if (!compressed_result.has_value())
    {
        std::cout
            << "\nCooked asset artifact serialization "
               "test summary: "
            << state.checks - state.failures
            << '/'
            << state.checks
            << " passed.\n";

        return 1;
    }

    const std::vector<std::byte>
        compressed_bytes =
            compressed_result.value();

    check(
        state,
        read_u32_little_endian(
            std::span<const std::byte>{
                compressed_bytes
            },
            flags_offset) ==
            cooked_asset_artifact_compressed_flag,
        "Compressed artifact records its compression flag");

    check(
        state,
        static_cast<std::uint64_t>(
            compressed_bytes.size()) ==
            expected_serialized_byte_count(
                compressed_artifact),
        "Compressed artifact has the expected serialized size");

    const auto compressed_round_trip =
        deserialize_cooked_asset_artifact(
            std::span<const std::byte>{
                compressed_bytes
            });

    check(
        state,
        compressed_round_trip.has_value(),
        "Compressed artifact deserializes");

    check(
        state,
        compressed_round_trip.has_value() &&
            artifacts_equal(
                compressed_artifact,
                compressed_round_trip.value()),
        "Compressed artifact survives an exact round trip");

    CookedAssetArtifact
        empty_uncompressed_artifact =
            make_uncompressed_artifact();

    empty_uncompressed_artifact.
        payload_bytes.clear();

    empty_uncompressed_artifact.
        uncompressed_byte_count = 0ULL;

    empty_uncompressed_artifact.record.
        cooked_hash =
            hash_bytes(
                std::span<const std::byte>{});

    const auto empty_uncompressed_result =
        serialize_cooked_asset_artifact(
            empty_uncompressed_artifact);

    check(
        state,
        empty_uncompressed_result.has_value(),
        "Empty uncompressed artifact serializes");

    if (empty_uncompressed_result.has_value())
    {
        const auto round_trip =
            deserialize_cooked_asset_artifact(
                std::span<const std::byte>{
                    empty_uncompressed_result.value()
                });

        check(
            state,
            round_trip.has_value() &&
                artifacts_equal(
                    empty_uncompressed_artifact,
                    round_trip.value()),
            "Empty uncompressed artifact survives a round trip");
    }
    else
    {
        check(
            state,
            false,
            "Empty uncompressed artifact survives a round trip");
    }

    CookedAssetArtifact
        empty_compressed_artifact =
            make_compressed_artifact();

    empty_compressed_artifact.
        payload_bytes.clear();

    empty_compressed_artifact.
        uncompressed_byte_count = 0ULL;

    empty_compressed_artifact.record.
        cooked_hash =
            hash_bytes(
                std::span<const std::byte>{});

    const auto empty_compressed_result =
        serialize_cooked_asset_artifact(
            empty_compressed_artifact);

    check(
        state,
        empty_compressed_result.has_value(),
        "Empty compressed artifact serializes");

    if (empty_compressed_result.has_value())
    {
        const auto round_trip =
            deserialize_cooked_asset_artifact(
                std::span<const std::byte>{
                    empty_compressed_result.value()
                });

        check(
            state,
            round_trip.has_value() &&
                artifacts_equal(
                    empty_compressed_artifact,
                    round_trip.value()),
            "Empty compressed artifact survives a round trip");
    }
    else
    {
        check(
            state,
            false,
            "Empty compressed artifact survives a round trip");
    }

    check_failure(
        state,
        serialize_cooked_asset_artifact(
            CookedAssetArtifact{}),
        ErrorCode::invalid_argument,
        "Invalid artifact is rejected by serialization");

    CookedAssetArtifact
        wrong_uncompressed_hash =
            make_uncompressed_artifact();

    wrong_uncompressed_hash.record.
        cooked_hash =
            hash_text(
                "incorrect cooked hash");

    check_failure(
        state,
        serialize_cooked_asset_artifact(
            wrong_uncompressed_hash),
        ErrorCode::invalid_argument,
        "Uncompressed payload with a mismatched hash is rejected");

    CookedAssetArtifactSerializationLimits
        tight_source_path_limits =
            default_limits;

    tight_source_path_limits.
        max_source_path_byte_count = 3U;

    check_failure(
        state,
        serialize_cooked_asset_artifact(
            uncompressed_artifact,
            tight_source_path_limits),
        ErrorCode::invalid_argument,
        "Serialization enforces the source path limit");

    CookedAssetArtifactSerializationLimits
        tight_name_limits =
            default_limits;

    tight_name_limits.max_name_byte_count = 3U;

    check_failure(
        state,
        serialize_cooked_asset_artifact(
            uncompressed_artifact,
            tight_name_limits),
        ErrorCode::invalid_argument,
        "Serialization enforces component name limits");

    CookedAssetArtifactSerializationLimits
        tight_dependency_limits =
            default_limits;

    tight_dependency_limits.
        max_dependency_count = 1U;

    check_failure(
        state,
        serialize_cooked_asset_artifact(
            uncompressed_artifact,
            tight_dependency_limits),
        ErrorCode::invalid_argument,
        "Serialization enforces the dependency limit");

    CookedAssetArtifactSerializationLimits
        tight_payload_limits =
            default_limits;

    tight_payload_limits.
        max_payload_byte_count = 3ULL;

    check_failure(
        state,
        serialize_cooked_asset_artifact(
            uncompressed_artifact,
            tight_payload_limits),
        ErrorCode::invalid_argument,
        "Serialization enforces the encoded payload limit");

    CookedAssetArtifactSerializationLimits
        tight_uncompressed_limits =
            default_limits;

    tight_uncompressed_limits.
        max_uncompressed_byte_count = 3ULL;

    check_failure(
        state,
        serialize_cooked_asset_artifact(
            uncompressed_artifact,
            tight_uncompressed_limits),
        ErrorCode::invalid_argument,
        "Serialization enforces the decoded payload limit");

    CookedAssetArtifactSerializationLimits
        tight_artifact_limits =
            default_limits;

    tight_artifact_limits.
        max_artifact_byte_count =
            expected_serialized_byte_count(
                uncompressed_artifact) -
            1ULL;

    tight_artifact_limits.
        max_payload_byte_count =
            tight_artifact_limits.
                max_artifact_byte_count;

    check_failure(
        state,
        serialize_cooked_asset_artifact(
            uncompressed_artifact,
            tight_artifact_limits),
        ErrorCode::invalid_argument,
        "Serialization enforces the total artifact limit");

    check_failure(
        state,
        serialize_cooked_asset_artifact(
            uncompressed_artifact,
            header_too_large_limits),
        ErrorCode::invalid_argument,
        "Serialization rejects invalid limits");

    check_failure(
        state,
        deserialize_cooked_asset_artifact(
            std::span<const std::byte>{},
            header_too_large_limits),
        ErrorCode::invalid_argument,
        "Deserialization rejects invalid limits");

    check_failure(
        state,
        deserialize_cooked_asset_artifact(
            std::span<const std::byte>{}),
        ErrorCode::invalid_argument,
        "Empty artifact bytes are rejected");

    std::vector<std::byte>
        truncated_header(
            static_cast<std::size_t>(
                cooked_asset_artifact_header_byte_count -
                1ULL));

    check_failure(
        state,
        deserialize_cooked_asset_artifact(
            std::span<const std::byte>{
                truncated_header
            }),
        ErrorCode::invalid_argument,
        "Truncated binary header is rejected");

    std::vector<std::byte>
        bad_magic =
            uncompressed_bytes;

    bad_magic[0U] ^=
        std::byte{0x01};

    check_failure(
        state,
        deserialize_cooked_asset_artifact(
            std::span<const std::byte>{
                bad_magic
            }),
        ErrorCode::invalid_argument,
        "Invalid artifact magic is rejected");

    std::vector<std::byte>
        unsupported_version =
            uncompressed_bytes;

    write_u32_little_endian(
        unsupported_version,
        format_version_offset,
        cooked_asset_artifact_format_version +
            1U);

    check_failure(
        state,
        deserialize_cooked_asset_artifact(
            std::span<const std::byte>{
                unsupported_version
            }),
        ErrorCode::unsupported_operation,
        "Unsupported artifact format is reported distinctly");

    std::vector<std::byte>
        unknown_flags =
            uncompressed_bytes;

    write_u32_little_endian(
        unknown_flags,
        flags_offset,
        0x80000000U);

    check_failure(
        state,
        deserialize_cooked_asset_artifact(
            std::span<const std::byte>{
                unknown_flags
            }),
        ErrorCode::invalid_argument,
        "Unknown artifact flags are rejected");

    std::vector<std::byte>
        nonzero_reserved_u32 =
            uncompressed_bytes;

    write_u32_little_endian(
        nonzero_reserved_u32,
        reserved_u32_offset,
        1U);

    check_failure(
        state,
        deserialize_cooked_asset_artifact(
            std::span<const std::byte>{
                nonzero_reserved_u32
            }),
        ErrorCode::invalid_argument,
        "Nonzero reserved 32-bit field is rejected");

    std::vector<std::byte>
        nonzero_reserved_u64 =
            uncompressed_bytes;

    write_u64_little_endian(
        nonzero_reserved_u64,
        reserved_u64_offset,
        1ULL);

    check_failure(
        state,
        deserialize_cooked_asset_artifact(
            std::span<const std::byte>{
                nonzero_reserved_u64
            }),
        ErrorCode::invalid_argument,
        "Nonzero reserved 64-bit field is rejected");

    std::vector<std::byte>
        trailing_byte =
            uncompressed_bytes;

    trailing_byte.push_back(
        std::byte{0x00});

    check_failure(
        state,
        deserialize_cooked_asset_artifact(
            std::span<const std::byte>{
                trailing_byte
            }),
        ErrorCode::invalid_argument,
        "Trailing artifact bytes are rejected");

    std::vector<std::byte>
        truncated_body =
            uncompressed_bytes;

    truncated_body.pop_back();

    check_failure(
        state,
        deserialize_cooked_asset_artifact(
            std::span<const std::byte>{
                truncated_body
            }),
        ErrorCode::invalid_argument,
        "Truncated artifact body is rejected");

    std::vector<std::byte>
        mismatched_payload_count =
            uncompressed_bytes;

    write_u64_little_endian(
        mismatched_payload_count,
        payload_byte_count_offset,
        uncompressed_artifact.
            encoded_byte_count() +
            1ULL);

    check_failure(
        state,
        deserialize_cooked_asset_artifact(
            std::span<const std::byte>{
                mismatched_payload_count
            }),
        ErrorCode::invalid_argument,
        "Declared payload size mismatch is rejected");

    check_failure(
        state,
        deserialize_cooked_asset_artifact(
            std::span<const std::byte>{
                uncompressed_bytes
            },
            tight_source_path_limits),
        ErrorCode::invalid_argument,
        "Deserialization enforces the source path limit");

    check_failure(
        state,
        deserialize_cooked_asset_artifact(
            std::span<const std::byte>{
                uncompressed_bytes
            },
            tight_name_limits),
        ErrorCode::invalid_argument,
        "Deserialization enforces component name limits");

    check_failure(
        state,
        deserialize_cooked_asset_artifact(
            std::span<const std::byte>{
                uncompressed_bytes
            },
            tight_dependency_limits),
        ErrorCode::invalid_argument,
        "Deserialization enforces the dependency limit");

    check_failure(
        state,
        deserialize_cooked_asset_artifact(
            std::span<const std::byte>{
                uncompressed_bytes
            },
            tight_payload_limits),
        ErrorCode::invalid_argument,
        "Deserialization enforces the encoded payload limit");

    check_failure(
        state,
        deserialize_cooked_asset_artifact(
            std::span<const std::byte>{
                uncompressed_bytes
            },
            tight_uncompressed_limits),
        ErrorCode::invalid_argument,
        "Deserialization enforces the decoded payload limit");

    std::vector<std::byte>
        compressed_flag_without_codec =
            uncompressed_bytes;

    write_u32_little_endian(
        compressed_flag_without_codec,
        flags_offset,
        cooked_asset_artifact_compressed_flag);

    check_failure(
        state,
        deserialize_cooked_asset_artifact(
            std::span<const std::byte>{
                compressed_flag_without_codec
            }),
        ErrorCode::invalid_argument,
        "Compression flag without codec metadata is rejected");

    std::vector<std::byte>
        codec_without_compressed_flag =
            compressed_bytes;

    write_u32_little_endian(
        codec_without_compressed_flag,
        flags_offset,
        0U);

    check_failure(
        state,
        deserialize_cooked_asset_artifact(
            std::span<const std::byte>{
                codec_without_compressed_flag
            }),
        ErrorCode::invalid_argument,
        "Codec metadata without compression flag is rejected");

    std::vector<std::byte>
        corrupted_payload =
            uncompressed_bytes;

    corrupted_payload.back() ^=
        std::byte{0x01};

    check_failure(
        state,
        deserialize_cooked_asset_artifact(
            std::span<const std::byte>{
                corrupted_payload
            }),
        ErrorCode::invalid_argument,
        "Corrupted uncompressed payload hash is rejected");

    std::vector<std::byte>
        corrupted_cooked_hash =
            uncompressed_bytes;

    corrupted_cooked_hash[
        cooked_hash_offset] ^=
            std::byte{0x01};

    check_failure(
        state,
        deserialize_cooked_asset_artifact(
            std::span<const std::byte>{
                corrupted_cooked_hash
            }),
        ErrorCode::invalid_argument,
        "Corrupted cooked hash metadata is rejected");

    std::vector<std::byte>
        invalid_asset_namespace =
            uncompressed_bytes;

    write_u64_little_endian(
        invalid_asset_namespace,
        asset_namespace_offset,
        0ULL);

    check_failure(
        state,
        deserialize_cooked_asset_artifact(
            std::span<const std::byte>{
                invalid_asset_namespace
            }),
        ErrorCode::invalid_argument,
        "Invalid persistent asset identity is rejected");

    std::vector<std::byte>
        zero_importer_version =
            uncompressed_bytes;

    write_u32_little_endian(
        zero_importer_version,
        importer_version_offset,
        0U);

    check_failure(
        state,
        deserialize_cooked_asset_artifact(
            std::span<const std::byte>{
                zero_importer_version
            }),
        ErrorCode::invalid_argument,
        "Zero importer version is rejected");

    std::vector<std::byte>
        zero_schema_version =
            uncompressed_bytes;

    write_u32_little_endian(
        zero_schema_version,
        schema_version_offset,
        0U);

    check_failure(
        state,
        deserialize_cooked_asset_artifact(
            std::span<const std::byte>{
                zero_schema_version
            }),
        ErrorCode::invalid_argument,
        "Zero schema version is rejected");

    std::vector<std::byte>
        zero_cooker_version =
            uncompressed_bytes;

    write_u32_little_endian(
        zero_cooker_version,
        cooker_version_offset,
        0U);

    check_failure(
        state,
        deserialize_cooked_asset_artifact(
            std::span<const std::byte>{
                zero_cooker_version
            }),
        ErrorCode::invalid_argument,
        "Zero cooker version is rejected");

    std::vector<std::byte>
        codec_version_without_name =
            uncompressed_bytes;

    write_u32_little_endian(
        codec_version_without_name,
        codec_version_offset,
        1U);

    check_failure(
        state,
        deserialize_cooked_asset_artifact(
            std::span<const std::byte>{
                codec_version_without_name
            }),
        ErrorCode::invalid_argument,
        "Codec version without codec name is rejected");

    std::vector<std::byte>
        compressed_without_codec_version =
            compressed_bytes;

    write_u32_little_endian(
        compressed_without_codec_version,
        codec_version_offset,
        0U);

    check_failure(
        state,
        deserialize_cooked_asset_artifact(
            std::span<const std::byte>{
                compressed_without_codec_version
            }),
        ErrorCode::invalid_argument,
        "Compressed codec name without version is rejected");

    std::vector<std::byte>
        oversized_decoded_payload =
            compressed_bytes;

    write_u64_little_endian(
        oversized_decoded_payload,
        uncompressed_byte_count_offset,
        default_limits.
            max_uncompressed_byte_count +
            1ULL);

    check_failure(
        state,
        deserialize_cooked_asset_artifact(
            std::span<const std::byte>{
                oversized_decoded_payload
            }),
        ErrorCode::invalid_argument,
        "Oversized decoded payload declaration is rejected");

    const std::size_t dependency_body_offset =
        static_cast<std::size_t>(
            cooked_asset_artifact_header_byte_count) +
        uncompressed_artifact.
            record.source_path.size() +
        uncompressed_artifact.
            record.importer_name.size() +
        uncompressed_artifact.
            cooker_name.size() +
        uncompressed_artifact.
            compression_codec_name.size();

    const bool dependency_fixture_valid =
        dependency_body_offset <=
            uncompressed_bytes.size() &&
        32U <=
            uncompressed_bytes.size() -
            dependency_body_offset;

    check(
        state,
        dependency_fixture_valid,
        "Serialized dependency body has the expected size");

    if (dependency_fixture_valid)
    {
        std::vector<std::byte>
            duplicate_dependencies =
                uncompressed_bytes;

        for (std::size_t index = 0U;
             index < 16U;
             ++index)
        {
            duplicate_dependencies[
                dependency_body_offset +
                16U +
                index] =
                    duplicate_dependencies[
                        dependency_body_offset +
                        index];
        }

        check_failure(
            state,
            deserialize_cooked_asset_artifact(
                std::span<const std::byte>{
                    duplicate_dependencies
                }),
            ErrorCode::invalid_argument,
            "Duplicate dependencies are rejected");

        std::vector<std::byte>
            self_dependency =
                uncompressed_bytes;

        write_u64_little_endian(
            self_dependency,
            dependency_body_offset,
            uncompressed_artifact.record.id.
                catalog_namespace);

        write_u64_little_endian(
            self_dependency,
            dependency_body_offset + 8U,
            uncompressed_artifact.record.id.
                asset_sequence);

        check_failure(
            state,
            deserialize_cooked_asset_artifact(
                std::span<const std::byte>{
                    self_dependency
                }),
            ErrorCode::invalid_argument,
            "Self dependency is rejected");
    }
    else
    {
        check(
            state,
            false,
            "Duplicate dependencies are rejected");

        check(
            state,
            false,
            "Self dependency is rejected");
    }

    std::cout
        << "\nCooked asset artifact serialization "
           "test summary: "
        << state.checks - state.failures
        << '/'
        << state.checks
        << " passed.\n";

    return
        state.failures == 0
            ? 0
            : 1;
}