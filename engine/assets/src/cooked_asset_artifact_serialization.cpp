#include "oros/assets/cooked_asset_artifact_serialization.hpp"

#include "oros/assets/content_hash.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace
{
    inline constexpr std::uint64_t
        serialized_dependency_byte_count{
            16ULL
        };

    [[nodiscard]]
    bool
    add_byte_count(
        std::uint64_t& total,
        const std::uint64_t amount) noexcept
    {
        if (amount >
            std::numeric_limits<
                std::uint64_t>::max() -
                total)
        {
            return false;
        }

        total += amount;
        return true;
    }

    class ByteWriter final
    {
    public:
        explicit ByteWriter(
            std::span<std::byte> bytes) noexcept
            : bytes_{bytes}
        {
        }

        [[nodiscard]]
        bool
        write_byte(
            const std::byte value) noexcept
        {
            if (!can_write(1U))
            {
                return false;
            }

            bytes_[offset_] = value;
            ++offset_;

            return true;
        }

        [[nodiscard]]
        bool
        write_u32(
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

                if (!write_byte(
                        static_cast<std::byte>(
                            component)))
                {
                    return false;
                }
            }

            return true;
        }

        [[nodiscard]]
        bool
        write_u64(
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

                if (!write_byte(
                        static_cast<std::byte>(
                            component)))
                {
                    return false;
                }
            }

            return true;
        }

        [[nodiscard]]
        bool
        write_bytes(
            const std::span<const std::byte>
                values) noexcept
        {
            if (!can_write(values.size()))
            {
                return false;
            }

            for (const std::byte value : values)
            {
                bytes_[offset_] = value;
                ++offset_;
            }

            return true;
        }

        [[nodiscard]]
        bool
        write_string(
            const std::string& value) noexcept
        {
            if (!can_write(value.size()))
            {
                return false;
            }

            for (const char character : value)
            {
                const auto component =
                    static_cast<unsigned char>(
                        character);

                bytes_[offset_] =
                    static_cast<std::byte>(
                        component);

                ++offset_;
            }

            return true;
        }

        [[nodiscard]]
        bool
        write_asset_id(
            const oros::assets::AssetId
                asset) noexcept
        {
            return
                write_u64(
                    asset.catalog_namespace) &&
                write_u64(
                    asset.asset_sequence);
        }

        [[nodiscard]]
        bool
        write_content_hash(
            const oros::assets::ContentHash&
                hash) noexcept
        {
            return write_bytes(
                std::span<const std::byte>{
                    hash.bytes
                });
        }

        [[nodiscard]]
        bool
        complete() const noexcept
        {
            return offset_ == bytes_.size();
        }

        [[nodiscard]]
        std::size_t
        offset() const noexcept
        {
            return offset_;
        }

    private:
        [[nodiscard]]
        bool
        can_write(
            const std::size_t count) const noexcept
        {
            return
                offset_ <= bytes_.size() &&
                count <=
                    bytes_.size() -
                    offset_;
        }

        std::span<std::byte> bytes_{};
        std::size_t offset_{};
    };

    class ByteReader final
    {
    public:
        explicit ByteReader(
            const std::span<const std::byte>
                bytes) noexcept
            : bytes_{bytes}
        {
        }

        [[nodiscard]]
        bool
        read_byte(
            std::byte& value) noexcept
        {
            if (!can_read(1U))
            {
                return false;
            }

            value = bytes_[offset_];
            ++offset_;

            return true;
        }

        [[nodiscard]]
        bool
        read_u32(
            std::uint32_t& value) noexcept
        {
            value = 0U;

            for (std::size_t index = 0U;
                 index < 4U;
                 ++index)
            {
                std::byte component{};

                if (!read_byte(component))
                {
                    return false;
                }

                const auto shift =
                    static_cast<unsigned int>(
                        index * 8U);

                value |=
                    static_cast<std::uint32_t>(
                        std::to_integer<
                            unsigned char>(
                                component))
                    << shift;
            }

            return true;
        }

        [[nodiscard]]
        bool
        read_u64(
            std::uint64_t& value) noexcept
        {
            value = 0ULL;

            for (std::size_t index = 0U;
                 index < 8U;
                 ++index)
            {
                std::byte component{};

                if (!read_byte(component))
                {
                    return false;
                }

                const auto shift =
                    static_cast<unsigned int>(
                        index * 8U);

                value |=
                    static_cast<std::uint64_t>(
                        std::to_integer<
                            unsigned char>(
                                component))
                    << shift;
            }

            return true;
        }

        [[nodiscard]]
        bool
        read_string(
            const std::uint32_t count,
            std::string& value)
        {
            const std::size_t size =
                static_cast<std::size_t>(
                    count);

            if (!can_read(size))
            {
                return false;
            }

            value.resize(size);

            for (std::size_t index = 0U;
                 index < size;
                 ++index)
            {
                value[index] =
                    static_cast<char>(
                        std::to_integer<
                            unsigned char>(
                                bytes_[offset_]));

                ++offset_;
            }

            return true;
        }

        [[nodiscard]]
        bool
        read_asset_id(
            oros::assets::AssetId&
                asset) noexcept
        {
            return
                read_u64(
                    asset.catalog_namespace) &&
                read_u64(
                    asset.asset_sequence);
        }

        [[nodiscard]]
        bool
        read_content_hash(
            oros::assets::ContentHash&
                hash) noexcept
        {
            if (!can_read(hash.bytes.size()))
            {
                return false;
            }

            for (std::byte& value : hash.bytes)
            {
                value = bytes_[offset_];
                ++offset_;
            }

            return true;
        }

        [[nodiscard]]
        bool
        read_payload(
            const std::uint64_t count,
            std::vector<std::byte>& value)
        {
            if (count >
                static_cast<std::uint64_t>(
                    std::numeric_limits<
                        std::size_t>::max()))
            {
                return false;
            }

            const std::size_t size =
                static_cast<std::size_t>(
                    count);

            if (!can_read(size))
            {
                return false;
            }

            value.resize(size);

            for (std::size_t index = 0U;
                 index < size;
                 ++index)
            {
                value[index] =
                    bytes_[offset_];

                ++offset_;
            }

            return true;
        }

        [[nodiscard]]
        bool
        complete() const noexcept
        {
            return offset_ == bytes_.size();
        }

        [[nodiscard]]
        std::size_t
        offset() const noexcept
        {
            return offset_;
        }

    private:
        [[nodiscard]]
        bool
        can_read(
            const std::size_t count) const noexcept
        {
            return
                offset_ <= bytes_.size() &&
                count <=
                    bytes_.size() -
                    offset_;
        }

        std::span<const std::byte> bytes_{};
        std::size_t offset_{};
    };
}

namespace oros::assets
{
    foundation::Result<std::vector<std::byte>>
    serialize_cooked_asset_artifact(
        const CookedAssetArtifact& artifact,
        const CookedAssetArtifactSerializationLimits
            limits) noexcept
    {
        if (!limits.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cooked artifact serialization "
                "limits are invalid.");
        }

        if (!artifact.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cannot serialize an invalid cooked "
                "asset artifact.");
        }

        if (!std::in_range<std::uint32_t>(
                artifact.record.source_path.size()) ||
            artifact.record.source_path.size() >
                limits.max_source_path_byte_count)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "The cooked artifact source path "
                "exceeds the serialization limit.");
        }

        if (!std::in_range<std::uint32_t>(
                artifact.record.importer_name.size()) ||
            artifact.record.importer_name.size() >
                limits.max_name_byte_count ||
            !std::in_range<std::uint32_t>(
                artifact.cooker_name.size()) ||
            artifact.cooker_name.size() >
                limits.max_name_byte_count ||
            !std::in_range<std::uint32_t>(
                artifact.compression_codec_name.
                    size()) ||
            artifact.compression_codec_name.size() >
                limits.max_name_byte_count)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cooked artifact component metadata "
                "exceeds the name limit.");
        }

        if (!std::in_range<std::uint32_t>(
                artifact.record.dependencies.size()) ||
            artifact.record.dependencies.size() >
                limits.max_dependency_count)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "The cooked artifact dependency "
                "count exceeds the serialization "
                "limit.");
        }

        if (!std::in_range<std::uint64_t>(
                artifact.payload_bytes.size()))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "The cooked artifact payload cannot "
                "be represented by the binary "
                "format.");
        }

        const std::uint64_t payload_byte_count =
            static_cast<std::uint64_t>(
                artifact.payload_bytes.size());

        if (payload_byte_count >
                limits.max_payload_byte_count ||
            artifact.uncompressed_byte_count >
                limits.max_uncompressed_byte_count)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "The cooked artifact payload exceeds "
                "the serialization limits.");
        }

        if (!artifact.record.cooked_hash.
                has_value())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "The cooked artifact does not contain "
                "a cooked content hash.");
        }

        if (!artifact.is_compressed())
        {
            const ContentHash actual_hash =
                hash_bytes(
                    std::span<const std::byte>{
                        artifact.payload_bytes
                    });

            if (actual_hash !=
                artifact.record.cooked_hash.value())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "The uncompressed artifact payload "
                    "does not match its cooked content "
                    "hash.");
            }
        }

        const std::uint32_t source_path_byte_count =
            static_cast<std::uint32_t>(
                artifact.record.source_path.size());

        const std::uint32_t importer_name_byte_count =
            static_cast<std::uint32_t>(
                artifact.record.importer_name.size());

        const std::uint32_t cooker_name_byte_count =
            static_cast<std::uint32_t>(
                artifact.cooker_name.size());

        const std::uint32_t codec_name_byte_count =
            static_cast<std::uint32_t>(
                artifact.compression_codec_name.
                    size());

        const std::uint32_t dependency_count =
            static_cast<std::uint32_t>(
                artifact.record.dependencies.size());

        const std::uint64_t dependency_byte_count =
            static_cast<std::uint64_t>(
                dependency_count) *
            serialized_dependency_byte_count;

        std::uint64_t total_byte_count =
            cooked_asset_artifact_header_byte_count;

        if (!add_byte_count(
                total_byte_count,
                source_path_byte_count) ||
            !add_byte_count(
                total_byte_count,
                importer_name_byte_count) ||
            !add_byte_count(
                total_byte_count,
                cooker_name_byte_count) ||
            !add_byte_count(
                total_byte_count,
                codec_name_byte_count) ||
            !add_byte_count(
                total_byte_count,
                dependency_byte_count) ||
            !add_byte_count(
                total_byte_count,
                payload_byte_count))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "The cooked artifact serialized size "
                "would overflow.");
        }

        if (total_byte_count >
                limits.max_artifact_byte_count ||
            total_byte_count >
                static_cast<std::uint64_t>(
                    std::numeric_limits<
                        std::size_t>::max()))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "The cooked artifact exceeds the "
                "maximum serialized size.");
        }

        try
        {
            std::vector<std::byte> bytes(
                static_cast<std::size_t>(
                    total_byte_count));

            ByteWriter writer{
                std::span<std::byte>{
                    bytes
                }
            };

            const std::uint32_t flags =
                artifact.is_compressed()
                    ? cooked_asset_artifact_compressed_flag
                    : 0U;

            if (!writer.write_bytes(
                    std::span<const std::byte>{
                        cooked_asset_artifact_magic
                    }) ||
                !writer.write_u32(
                    artifact.format_version) ||
                !writer.write_u32(
                    flags) ||
                !writer.write_asset_id(
                    artifact.record.id) ||
                !writer.write_u32(
                    artifact.record.
                        importer_version) ||
                !writer.write_u32(
                    artifact.record.
                        schema_version) ||
                !writer.write_u32(
                    artifact.cooker_version) ||
                !writer.write_u32(
                    artifact.
                        compression_codec_version) ||
                !writer.write_u32(
                    dependency_count) ||
                !writer.write_u32(
                    source_path_byte_count) ||
                !writer.write_u32(
                    importer_name_byte_count) ||
                !writer.write_u32(
                    cooker_name_byte_count) ||
                !writer.write_u32(
                    codec_name_byte_count) ||
                !writer.write_u32(
                    0U) ||
                !writer.write_u64(
                    artifact.
                        uncompressed_byte_count) ||
                !writer.write_u64(
                    payload_byte_count) ||
                !writer.write_content_hash(
                    artifact.record.source_hash) ||
                !writer.write_content_hash(
                    artifact.record.
                        cooked_hash.value()) ||
                !writer.write_u64(
                    0ULL))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "Unable to write the cooked "
                    "artifact binary header.");
            }

            if (writer.offset() !=
                static_cast<std::size_t>(
                    cooked_asset_artifact_header_byte_count))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "The cooked artifact header size "
                    "is inconsistent.");
            }

            if (!writer.write_string(
                    artifact.record.source_path) ||
                !writer.write_string(
                    artifact.record.importer_name) ||
                !writer.write_string(
                    artifact.cooker_name) ||
                !writer.write_string(
                    artifact.
                        compression_codec_name))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "Unable to write cooked artifact "
                    "string metadata.");
            }

            for (const AssetId dependency :
                 artifact.record.dependencies)
            {
                if (!writer.write_asset_id(
                        dependency))
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            internal_failure,
                        "Unable to write cooked "
                        "artifact dependencies.");
                }
            }

            if (!writer.write_bytes(
                    std::span<const std::byte>{
                        artifact.payload_bytes
                    }) ||
                !writer.complete())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "Unable to complete the cooked "
                    "artifact binary payload.");
            }

            return bytes;
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Unable to allocate cooked artifact "
                "serialization storage.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "An unexpected failure occurred while "
                "serializing a cooked artifact.");
        }
    }

    foundation::Result<CookedAssetArtifact>
    deserialize_cooked_asset_artifact(
        const std::span<const std::byte> bytes,
        const CookedAssetArtifactSerializationLimits
            limits) noexcept
    {
        if (!limits.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cooked artifact deserialization "
                "limits are invalid.");
        }

        if (!std::in_range<std::uint64_t>(
                bytes.size()))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "The cooked artifact byte count "
                "cannot be represented by the binary "
                "format.");
        }

        const std::uint64_t artifact_byte_count =
            static_cast<std::uint64_t>(
                bytes.size());

        if (artifact_byte_count <
                cooked_asset_artifact_header_byte_count)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "The cooked artifact is truncated "
                "before the binary header ends.");
        }

        if (artifact_byte_count >
            limits.max_artifact_byte_count)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "The cooked artifact exceeds the "
                "maximum serialized size.");
        }

        ByteReader reader{bytes};

        for (const std::byte expected :
             cooked_asset_artifact_magic)
        {
            std::byte actual{};

            if (!reader.read_byte(actual) ||
                actual != expected)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "The cooked artifact magic value "
                    "is invalid.");
            }
        }

        std::uint32_t format_version{};
        std::uint32_t flags{};

        AssetId asset{};

        std::uint32_t importer_version{};
        std::uint32_t schema_version{};
        std::uint32_t cooker_version{};
        std::uint32_t codec_version{};

        std::uint32_t dependency_count{};
        std::uint32_t source_path_byte_count{};
        std::uint32_t importer_name_byte_count{};
        std::uint32_t cooker_name_byte_count{};
        std::uint32_t codec_name_byte_count{};

        std::uint32_t reserved_u32{};

        std::uint64_t uncompressed_byte_count{};
        std::uint64_t payload_byte_count{};

        ContentHash source_hash{};
        ContentHash cooked_hash{};

        std::uint64_t reserved_u64{};

        if (!reader.read_u32(
                format_version) ||
            !reader.read_u32(
                flags) ||
            !reader.read_asset_id(
                asset) ||
            !reader.read_u32(
                importer_version) ||
            !reader.read_u32(
                schema_version) ||
            !reader.read_u32(
                cooker_version) ||
            !reader.read_u32(
                codec_version) ||
            !reader.read_u32(
                dependency_count) ||
            !reader.read_u32(
                source_path_byte_count) ||
            !reader.read_u32(
                importer_name_byte_count) ||
            !reader.read_u32(
                cooker_name_byte_count) ||
            !reader.read_u32(
                codec_name_byte_count) ||
            !reader.read_u32(
                reserved_u32) ||
            !reader.read_u64(
                uncompressed_byte_count) ||
            !reader.read_u64(
                payload_byte_count) ||
            !reader.read_content_hash(
                source_hash) ||
            !reader.read_content_hash(
                cooked_hash) ||
            !reader.read_u64(
                reserved_u64))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "The cooked artifact binary header "
                "is truncated.");
        }

        if (reader.offset() !=
            static_cast<std::size_t>(
                cooked_asset_artifact_header_byte_count))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "The cooked artifact header reader "
                "has an inconsistent offset.");
        }

        if (format_version !=
            cooked_asset_artifact_format_version)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    unsupported_operation,
                "The cooked artifact format version "
                "is not supported.");
        }

        const std::uint32_t known_flags =
            cooked_asset_artifact_compressed_flag;

        if ((flags & ~known_flags) != 0U)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "The cooked artifact contains unknown "
                "binary flags.");
        }

        if (reserved_u32 != 0U ||
            reserved_u64 != 0ULL)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "The cooked artifact reserved header "
                "fields are not zero.");
        }

        if (source_path_byte_count >
                limits.max_source_path_byte_count ||
            importer_name_byte_count >
                limits.max_name_byte_count ||
            cooker_name_byte_count >
                limits.max_name_byte_count ||
            codec_name_byte_count >
                limits.max_name_byte_count)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "The cooked artifact string metadata "
                "exceeds the deserialization limits.");
        }

        if (dependency_count >
            limits.max_dependency_count)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "The cooked artifact dependency "
                "count exceeds the deserialization "
                "limit.");
        }

        if (payload_byte_count >
                limits.max_payload_byte_count ||
            uncompressed_byte_count >
                limits.max_uncompressed_byte_count)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "The cooked artifact payload exceeds "
                "the deserialization limits.");
        }

        const std::uint64_t dependency_byte_count =
            static_cast<std::uint64_t>(
                dependency_count) *
            serialized_dependency_byte_count;

        std::uint64_t expected_byte_count =
            cooked_asset_artifact_header_byte_count;

        if (!add_byte_count(
                expected_byte_count,
                source_path_byte_count) ||
            !add_byte_count(
                expected_byte_count,
                importer_name_byte_count) ||
            !add_byte_count(
                expected_byte_count,
                cooker_name_byte_count) ||
            !add_byte_count(
                expected_byte_count,
                codec_name_byte_count) ||
            !add_byte_count(
                expected_byte_count,
                dependency_byte_count) ||
            !add_byte_count(
                expected_byte_count,
                payload_byte_count))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "The cooked artifact declared size "
                "would overflow.");
        }

        if (expected_byte_count !=
            artifact_byte_count)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "The cooked artifact declared body "
                "size does not match the supplied "
                "bytes.");
        }

        if (payload_byte_count >
            static_cast<std::uint64_t>(
                std::numeric_limits<
                    std::size_t>::max()))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "The cooked artifact payload cannot "
                "be represented on this platform.");
        }

        try
        {
            CookedAssetArtifact artifact{};

            artifact.format_version =
                format_version;

            artifact.record.id =
                asset;

            artifact.record.importer_version =
                importer_version;

            artifact.record.schema_version =
                schema_version;

            artifact.record.source_hash =
                source_hash;

            artifact.record.cooked_hash =
                cooked_hash;

            artifact.cooker_version =
                cooker_version;

            artifact.compression_codec_version =
                codec_version;

            artifact.uncompressed_byte_count =
                uncompressed_byte_count;

            if (!reader.read_string(
                    source_path_byte_count,
                    artifact.record.source_path) ||
                !reader.read_string(
                    importer_name_byte_count,
                    artifact.record.importer_name) ||
                !reader.read_string(
                    cooker_name_byte_count,
                    artifact.cooker_name) ||
                !reader.read_string(
                    codec_name_byte_count,
                    artifact.
                        compression_codec_name))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "The cooked artifact string body "
                    "is truncated.");
            }

            artifact.record.dependencies.resize(
                static_cast<std::size_t>(
                    dependency_count));

            for (AssetId& dependency :
                 artifact.record.dependencies)
            {
                if (!reader.read_asset_id(
                        dependency))
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "The cooked artifact dependency "
                        "body is truncated.");
                }
            }

            if (!reader.read_payload(
                    payload_byte_count,
                    artifact.payload_bytes) ||
                !reader.complete())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "The cooked artifact payload body "
                    "is truncated or inconsistent.");
            }

            const bool compressed_flag =
                (flags &
                 cooked_asset_artifact_compressed_flag) !=
                0U;

            if (compressed_flag !=
                artifact.is_compressed())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "The cooked artifact compression "
                    "flag does not match its codec "
                    "metadata.");
            }

            if (!artifact.is_valid())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "The cooked artifact contains "
                    "invalid asset metadata.");
            }

            if (!artifact.is_compressed())
            {
                const ContentHash actual_hash =
                    hash_bytes(
                        std::span<const std::byte>{
                            artifact.payload_bytes
                        });

                if (actual_hash !=
                    artifact.record.
                        cooked_hash.value())
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "The uncompressed artifact "
                        "payload does not match its "
                        "cooked content hash.");
                }
            }

            return artifact;
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Unable to allocate cooked artifact "
                "deserialization storage.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "An unexpected failure occurred while "
                "deserializing a cooked artifact.");
        }
    }
}