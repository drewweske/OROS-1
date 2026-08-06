#include "world_cell_persistence_format.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <span>
#include <utility>
#include <vector>

namespace oros::streaming::detail
{
    namespace
    {
        inline constexpr std::array<
            std::byte,
            8U>
            snapshot_magic{
                std::byte{0x4F},
                std::byte{0x52},
                std::byte{0x4F},
                std::byte{0x53},
                std::byte{0x53},
                std::byte{0x4E},
                std::byte{0x50},
                std::byte{0x31}
            };

        inline constexpr std::array<
            std::byte,
            8U>
            manifest_magic{
                std::byte{0x4F},
                std::byte{0x52},
                std::byte{0x4F},
                std::byte{0x53},
                std::byte{0x4D},
                std::byte{0x4E},
                std::byte{0x46},
                std::byte{0x31}
            };

        inline constexpr std::array<
            std::byte,
            8U>
            manifest_pointer_magic{
                std::byte{0x4F},
                std::byte{0x52},
                std::byte{0x4F},
                std::byte{0x53},
                std::byte{0x43},
                std::byte{0x55},
                std::byte{0x52},
                std::byte{0x31}
            };

        inline constexpr std::size_t
            snapshot_checksum_offset{
                64U
            };

        inline constexpr std::size_t
            manifest_checksum_offset{
                40U
            };

        inline constexpr std::size_t
            manifest_pointer_checksum_offset{
                32U
            };

        inline constexpr std::size_t
            checksum_byte_count{
                8U
            };

        [[nodiscard]]
        constexpr std::uint64_t mix_hash(
            std::uint64_t value) noexcept
        {
            value +=
                0x9E3779B97F4A7C15ULL;

            value =
                (value ^ (value >> 30U)) *
                0xBF58476D1CE4E5B9ULL;

            value =
                (value ^ (value >> 27U)) *
                0x94D049BB133111EBULL;

            return
                value ^
                (value >> 31U);
        }

        [[nodiscard]]
        std::uint64_t calculate_checksum(
            const std::span<const std::byte> bytes,
            const std::size_t excluded_offset,
            const std::size_t excluded_count)
            noexcept
        {
            std::uint64_t hash{
                14695981039346656037ULL
            };

            for (std::size_t index = 0U;
                 index < bytes.size();
                 ++index)
            {
                const bool excluded =
                    index >= excluded_offset &&
                    index - excluded_offset <
                        excluded_count;

                if (excluded)
                {
                    continue;
                }

                hash ^=
                    static_cast<std::uint64_t>(
                        std::to_integer<
                            std::uint8_t>(
                                bytes[index]));

                hash *=
                    1099511628211ULL;
            }

            hash ^=
                static_cast<std::uint64_t>(
                    bytes.size());

            return mix_hash(hash);
        }

        class ByteWriter final
        {
        public:
            explicit ByteWriter(
                const std::span<std::byte>
                    bytes) noexcept
                : bytes_{
                      bytes
                  }
            {
            }

            [[nodiscard]]
            bool write_bytes(
                const std::span<
                    const std::byte>
                    values) noexcept
            {
                if (!can_write(
                        values.size()))
                {
                    return false;
                }

                for (const std::byte value :
                     values)
                {
                    bytes_[offset_] =
                        value;

                    ++offset_;
                }

                return true;
            }

            [[nodiscard]]
            bool write_u32(
                const std::uint32_t value)
                noexcept
            {
                for (std::size_t index = 0U;
                     index < 4U;
                     ++index)
                {
                    const unsigned int shift =
                        static_cast<
                            unsigned int>(
                                index *
                                8U);

                    const std::uint8_t component =
                        static_cast<
                            std::uint8_t>(
                                (
                                    value >>
                                    shift
                                ) &
                                0xFFU);

                    if (!write_byte(
                            static_cast<
                                std::byte>(
                                    component)))
                    {
                        return false;
                    }
                }

                return true;
            }

            [[nodiscard]]
            bool write_u64(
                const std::uint64_t value)
                noexcept
            {
                for (std::size_t index = 0U;
                     index < 8U;
                     ++index)
                {
                    const unsigned int shift =
                        static_cast<
                            unsigned int>(
                                index *
                                8U);

                    const std::uint8_t component =
                        static_cast<
                            std::uint8_t>(
                                (
                                    value >>
                                    shift
                                ) &
                                0xFFULL);

                    if (!write_byte(
                            static_cast<
                                std::byte>(
                                    component)))
                    {
                        return false;
                    }
                }

                return true;
            }

            [[nodiscard]]
            bool write_i64(
                const std::int64_t value)
                noexcept
            {
                return write_u64(
                    std::bit_cast<
                        std::uint64_t>(
                            value));
            }

            [[nodiscard]]
            bool complete() const noexcept
            {
                return
                    offset_ ==
                    bytes_.size();
            }

        private:
            [[nodiscard]]
            bool write_byte(
                const std::byte value)
                noexcept
            {
                if (!can_write(1U))
                {
                    return false;
                }

                bytes_[offset_] =
                    value;

                ++offset_;

                return true;
            }

            [[nodiscard]]
            bool can_write(
                const std::size_t count)
                const noexcept
            {
                return
                    offset_ <=
                        bytes_.size() &&
                    count <=
                        bytes_.size() -
                            offset_;
            }

            std::span<std::byte>
                bytes_{};

            std::size_t offset_{};
        };

        class ByteReader final
        {
        public:
            explicit ByteReader(
                const std::span<
                    const std::byte>
                    bytes) noexcept
                : bytes_{
                      bytes
                  }
            {
            }

            [[nodiscard]]
            bool read_magic(
                const std::span<
                    const std::byte>
                    expected) noexcept
            {
                if (!can_read(
                        expected.size()))
                {
                    return false;
                }

                for (const std::byte value :
                     expected)
                {
                    if (bytes_[offset_] !=
                        value)
                    {
                        return false;
                    }

                    ++offset_;
                }

                return true;
            }

            [[nodiscard]]
            bool read_u32(
                std::uint32_t& value)
                noexcept
            {
                value = 0U;

                for (std::size_t index = 0U;
                     index < 4U;
                     ++index)
                {
                    std::byte component{};

                    if (!read_byte(
                            component))
                    {
                        return false;
                    }

                    const unsigned int shift =
                        static_cast<
                            unsigned int>(
                                index *
                                8U);

                    value |=
                        static_cast<
                            std::uint32_t>(
                                std::to_integer<
                                    std::uint8_t>(
                                        component))
                        << shift;
                }

                return true;
            }

            [[nodiscard]]
            bool read_u64(
                std::uint64_t& value)
                noexcept
            {
                value = 0ULL;

                for (std::size_t index = 0U;
                     index < 8U;
                     ++index)
                {
                    std::byte component{};

                    if (!read_byte(
                            component))
                    {
                        return false;
                    }

                    const unsigned int shift =
                        static_cast<
                            unsigned int>(
                                index *
                                8U);

                    value |=
                        static_cast<
                            std::uint64_t>(
                                std::to_integer<
                                    std::uint8_t>(
                                        component))
                        << shift;
                }

                return true;
            }

            [[nodiscard]]
            bool read_i64(
                std::int64_t& value)
                noexcept
            {
                std::uint64_t bits{};

                if (!read_u64(bits))
                {
                    return false;
                }

                value =
                    std::bit_cast<
                        std::int64_t>(
                            bits);

                return true;
            }

            [[nodiscard]]
            bool complete() const noexcept
            {
                return
                    offset_ ==
                    bytes_.size();
            }

        private:
            [[nodiscard]]
            bool read_byte(
                std::byte& value)
                noexcept
            {
                if (!can_read(1U))
                {
                    return false;
                }

                value =
                    bytes_[offset_];

                ++offset_;

                return true;
            }

            [[nodiscard]]
            bool can_read(
                const std::size_t count)
                const noexcept
            {
                return
                    offset_ <=
                        bytes_.size() &&
                    count <=
                        bytes_.size() -
                            offset_;
            }

            std::span<
                const std::byte>
                bytes_{};

            std::size_t offset_{};
        };

        [[nodiscard]]
        bool patch_u64(
            const std::span<std::byte> bytes,
            const std::size_t offset,
            const std::uint64_t value)
            noexcept
        {
            if (offset > bytes.size() ||
                checksum_byte_count >
                    bytes.size() -
                        offset)
            {
                return false;
            }

            for (std::size_t index = 0U;
                 index < checksum_byte_count;
                 ++index)
            {
                const unsigned int shift =
                    static_cast<
                        unsigned int>(
                            index *
                            8U);

                const std::uint8_t component =
                    static_cast<
                        std::uint8_t>(
                            (
                                value >>
                                shift
                            ) &
                            0xFFULL);

                bytes[offset + index] =
                    static_cast<std::byte>(
                        component);
            }

            return true;
        }

        [[nodiscard]]
        bool calculate_manifest_byte_count(
            const std::uint64_t entry_count,
            std::uint64_t& byte_count)
            noexcept
        {
            const std::uint64_t maximum =
                (std::numeric_limits<
                    std::uint64_t>::max)();

            if (entry_count >
                (
                    maximum -
                    world_cell_manifest_file_fixed_byte_count
                ) /
                world_cell_manifest_entry_file_byte_count)
            {
                return false;
            }

            byte_count =
                world_cell_manifest_file_fixed_byte_count +
                entry_count *
                    world_cell_manifest_entry_file_byte_count;

            return true;
        }
    }

    foundation::Result<std::vector<std::byte>>
    serialize_world_cell_snapshot(
        const WorldCellSnapshot& snapshot,
        const WorldCellPersistenceLimits&
            limits) noexcept
    {
        if (!limits.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell persistence limits are "
                "invalid.");
        }

        if (!snapshot.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cannot serialize an invalid world "
                "cell snapshot.");
        }

        if (!std::in_range<std::uint64_t>(
                snapshot.byte_count()))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell snapshot payload size "
                "cannot be represented by the file "
                "format.");
        }

        const std::uint64_t payload_byte_count =
            static_cast<std::uint64_t>(
                snapshot.byte_count());

        if (payload_byte_count >
            limits.
                maximum_snapshot_payload_byte_count)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell snapshot payload exceeds "
                "the configured persistence limit.");
        }

        const std::uint64_t maximum =
            (std::numeric_limits<
                std::uint64_t>::max)();

        if (payload_byte_count >
            maximum -
                world_cell_snapshot_file_fixed_byte_count)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell snapshot file size would "
                "overflow.");
        }

        const std::uint64_t file_byte_count =
            world_cell_snapshot_file_fixed_byte_count +
            payload_byte_count;

        if (!std::in_range<std::size_t>(
                file_byte_count))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell snapshot file size cannot "
                "be represented on this platform.");
        }

        try
        {
            std::vector<std::byte> bytes(
                static_cast<std::size_t>(
                    file_byte_count));

            ByteWriter writer{
                std::span<std::byte>{
                    bytes
                }
            };

            const bool wrote_file =
                writer.write_bytes(
                    std::span<
                        const std::byte>{
                            snapshot_magic
                        }) &&
                writer.write_u32(
                    world_cell_snapshot_file_format_version) &&
                writer.write_u32(
                    snapshot.schema_version()) &&
                writer.write_u64(
                    snapshot.key().
                        world_namespace) &&
                writer.write_i64(
                    snapshot.key().
                        cell.x) &&
                writer.write_i64(
                    snapshot.key().
                        cell.y) &&
                writer.write_i64(
                    snapshot.key().
                        cell.z) &&
                writer.write_u64(
                    snapshot.revision()) &&
                writer.write_u64(
                    payload_byte_count) &&
                writer.write_u64(
                    0ULL) &&
                writer.write_bytes(
                    snapshot.payload());

            if (!wrote_file ||
                !writer.complete())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "Unable to construct the complete "
                    "world cell snapshot file.");
            }

            const std::uint64_t checksum =
                calculate_checksum(
                    std::span<
                        const std::byte>{
                            bytes
                        },
                    snapshot_checksum_offset,
                    checksum_byte_count);

            if (!patch_u64(
                    std::span<std::byte>{
                        bytes
                    },
                    snapshot_checksum_offset,
                    checksum))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "Unable to write the world cell "
                    "snapshot checksum.");
            }

            return bytes;
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Unable to allocate world cell "
                "snapshot serialization storage.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "An unexpected failure occurred "
                "while serializing a world cell "
                "snapshot.");
        }
    }

    foundation::Result<WorldCellSnapshot>
    deserialize_world_cell_snapshot(
        const std::span<const std::byte> bytes,
        const WorldCellPersistenceLimits&
            limits) noexcept
    {
        if (!limits.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell persistence limits are "
                "invalid.");
        }

        if (!std::in_range<std::uint64_t>(
                bytes.size()))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell snapshot file size cannot "
                "be represented by the file format.");
        }

        const std::uint64_t file_byte_count =
            static_cast<std::uint64_t>(
                bytes.size());

        if (file_byte_count <
            world_cell_snapshot_file_fixed_byte_count)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell snapshot file is "
                "truncated.");
        }

        if (file_byte_count >
            maximum_snapshot_file_byte_count(
                limits))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell snapshot file exceeds "
                "the configured persistence limit.");
        }

        ByteReader reader{
            bytes
        };

        std::uint32_t file_format_version{};
        std::uint32_t snapshot_schema_version{};
        std::uint64_t world_namespace{};
        std::int64_t cell_x{};
        std::int64_t cell_y{};
        std::int64_t cell_z{};
        std::uint64_t revision{};
        std::uint64_t payload_byte_count{};
        std::uint64_t stored_checksum{};

        const bool read_header =
            reader.read_magic(
                std::span<
                    const std::byte>{
                        snapshot_magic
                    }) &&
            reader.read_u32(
                file_format_version) &&
            reader.read_u32(
                snapshot_schema_version) &&
            reader.read_u64(
                world_namespace) &&
            reader.read_i64(
                cell_x) &&
            reader.read_i64(
                cell_y) &&
            reader.read_i64(
                cell_z) &&
            reader.read_u64(
                revision) &&
            reader.read_u64(
                payload_byte_count) &&
            reader.read_u64(
                stored_checksum);

        if (!read_header)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell snapshot file header is "
                "invalid or truncated.");
        }

        if (file_format_version !=
            world_cell_snapshot_file_format_version)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell snapshot file format "
                "version is unsupported.");
        }

        if (snapshot_schema_version !=
            world_cell_snapshot_schema_version)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell snapshot schema version "
                "is unsupported.");
        }

        if (payload_byte_count >
            limits.
                maximum_snapshot_payload_byte_count)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell snapshot payload exceeds "
                "the configured persistence limit.");
        }

        const std::uint64_t maximum =
            (std::numeric_limits<
                std::uint64_t>::max)();

        if (payload_byte_count >
            maximum -
                world_cell_snapshot_file_fixed_byte_count)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell snapshot payload size "
                "would overflow the file size.");
        }

        const std::uint64_t expected_byte_count =
            world_cell_snapshot_file_fixed_byte_count +
            payload_byte_count;

        if (expected_byte_count !=
            file_byte_count)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell snapshot file size does "
                "not match its declared payload "
                "size.");
        }

        const std::uint64_t actual_checksum =
            calculate_checksum(
                bytes,
                snapshot_checksum_offset,
                checksum_byte_count);

        if (actual_checksum !=
            stored_checksum)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell snapshot file checksum "
                "does not match its contents.");
        }

        if (!std::in_range<std::size_t>(
                payload_byte_count))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell snapshot payload size "
                "cannot be represented on this "
                "platform.");
        }

        const WorldCellKey key{
            world_namespace,
            world::WorldCell{
                cell_x,
                cell_y,
                cell_z
            }
        };

        return WorldCellSnapshot::create(
            key,
            revision,
            bytes.subspan(
                static_cast<std::size_t>(
                    world_cell_snapshot_file_fixed_byte_count),
                static_cast<std::size_t>(
                    payload_byte_count)));
    }

    foundation::Result<std::vector<std::byte>>
    serialize_world_cell_snapshot_manifest(
        const WorldCellSnapshotManifest&
            manifest,
        const WorldCellPersistenceLimits&
            limits) noexcept
    {
        if (!limits.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell persistence limits are "
                "invalid.");
        }

        if (!manifest.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cannot serialize an invalid world "
                "cell snapshot manifest.");
        }

        if (!std::in_range<std::uint64_t>(
                manifest.size()))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell snapshot manifest entry "
                "count cannot be represented by the "
                "file format.");
        }

        const std::uint64_t entry_count =
            static_cast<std::uint64_t>(
                manifest.size());

        if (entry_count >
            limits.maximum_manifest_entry_count)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell snapshot manifest exceeds "
                "the configured entry limit.");
        }

        std::uint64_t file_byte_count{};

        if (!calculate_manifest_byte_count(
                entry_count,
                file_byte_count))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell snapshot manifest file "
                "size would overflow.");
        }

        if (!std::in_range<std::size_t>(
                file_byte_count))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell snapshot manifest file "
                "size cannot be represented on this "
                "platform.");
        }

        try
        {
            std::vector<std::byte> bytes(
                static_cast<std::size_t>(
                    file_byte_count));

            ByteWriter writer{
                std::span<std::byte>{
                    bytes
                }
            };

            bool wrote_file =
                writer.write_bytes(
                    std::span<
                        const std::byte>{
                            manifest_magic
                        }) &&
                writer.write_u32(
                    world_cell_manifest_file_format_version) &&
                writer.write_u32(
                    manifest.schema_version()) &&
                writer.write_u64(
                    manifest.world_namespace()) &&
                writer.write_u64(
                    manifest.manifest_revision()) &&
                writer.write_u64(
                    entry_count) &&
                writer.write_u64(
                    0ULL);

            for (const WorldCellRevisionId&
                     entry :
                 manifest.entries())
            {
                wrote_file =
                    wrote_file &&
                    writer.write_i64(
                        entry.cell_key.cell.x) &&
                    writer.write_i64(
                        entry.cell_key.cell.y) &&
                    writer.write_i64(
                        entry.cell_key.cell.z) &&
                    writer.write_u64(
                        entry.revision);
            }

            if (!wrote_file ||
                !writer.complete())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "Unable to construct the complete "
                    "world cell snapshot manifest "
                    "file.");
            }

            const std::uint64_t checksum =
                calculate_checksum(
                    std::span<
                        const std::byte>{
                            bytes
                        },
                    manifest_checksum_offset,
                    checksum_byte_count);

            if (!patch_u64(
                    std::span<std::byte>{
                        bytes
                    },
                    manifest_checksum_offset,
                    checksum))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "Unable to write the world cell "
                    "snapshot manifest checksum.");
            }

            return bytes;
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Unable to allocate world cell "
                "snapshot manifest serialization "
                "storage.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "An unexpected failure occurred "
                "while serializing a world cell "
                "snapshot manifest.");
        }
    }

    foundation::Result<
        WorldCellSnapshotManifest>
    deserialize_world_cell_snapshot_manifest(
        const std::span<const std::byte> bytes,
        const WorldCellPersistenceLimits&
            limits) noexcept
    {
        if (!limits.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell persistence limits are "
                "invalid.");
        }

        if (!std::in_range<std::uint64_t>(
                bytes.size()))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell snapshot manifest file "
                "size cannot be represented by the "
                "file format.");
        }

        const std::uint64_t file_byte_count =
            static_cast<std::uint64_t>(
                bytes.size());

        if (file_byte_count <
            world_cell_manifest_file_fixed_byte_count)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell snapshot manifest file "
                "is truncated.");
        }

        if (file_byte_count >
            maximum_manifest_file_byte_count(
                limits))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell snapshot manifest file "
                "exceeds the configured persistence "
                "limit.");
        }

        ByteReader reader{
            bytes
        };

        std::uint32_t file_format_version{};
        std::uint32_t manifest_schema_version{};
        std::uint64_t world_namespace{};
        std::uint64_t manifest_revision{};
        std::uint64_t entry_count{};
        std::uint64_t stored_checksum{};

        const bool read_header =
            reader.read_magic(
                std::span<
                    const std::byte>{
                        manifest_magic
                    }) &&
            reader.read_u32(
                file_format_version) &&
            reader.read_u32(
                manifest_schema_version) &&
            reader.read_u64(
                world_namespace) &&
            reader.read_u64(
                manifest_revision) &&
            reader.read_u64(
                entry_count) &&
            reader.read_u64(
                stored_checksum);

        if (!read_header)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell snapshot manifest header "
                "is invalid or truncated.");
        }

        if (file_format_version !=
            world_cell_manifest_file_format_version)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell snapshot manifest file "
                "format version is unsupported.");
        }

        if (manifest_schema_version !=
            world_cell_snapshot_manifest_schema_version)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell snapshot manifest schema "
                "version is unsupported.");
        }

        if (entry_count >
            limits.maximum_manifest_entry_count)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell snapshot manifest exceeds "
                "the configured entry limit.");
        }

        std::uint64_t expected_byte_count{};

        if (!calculate_manifest_byte_count(
                entry_count,
                expected_byte_count))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell snapshot manifest entry "
                "count would overflow the file size.");
        }

        if (expected_byte_count !=
            file_byte_count)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell snapshot manifest file "
                "size does not match its declared "
                "entry count.");
        }

        const std::uint64_t actual_checksum =
            calculate_checksum(
                bytes,
                manifest_checksum_offset,
                checksum_byte_count);

        if (actual_checksum !=
            stored_checksum)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell snapshot manifest "
                "checksum does not match its "
                "contents.");
        }

        if (!std::in_range<std::size_t>(
                entry_count))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell snapshot manifest entry "
                "count cannot be represented on this "
                "platform.");
        }

        try
        {
            std::vector<
                WorldCellRevisionId>
                entries{};

            entries.reserve(
                static_cast<std::size_t>(
                    entry_count));

            WorldCellKey previous_key{};
            bool has_previous_key{};

            for (std::uint64_t index = 0ULL;
                 index < entry_count;
                 ++index)
            {
                std::int64_t cell_x{};
                std::int64_t cell_y{};
                std::int64_t cell_z{};
                std::uint64_t revision{};

                const bool read_entry =
                    reader.read_i64(
                        cell_x) &&
                    reader.read_i64(
                        cell_y) &&
                    reader.read_i64(
                        cell_z) &&
                    reader.read_u64(
                        revision);

                if (!read_entry)
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "World cell snapshot manifest "
                        "entry data is truncated.");
                }

                const WorldCellKey cell_key{
                    world_namespace,
                    world::WorldCell{
                        cell_x,
                        cell_y,
                        cell_z
                    }
                };

                const WorldCellRevisionId entry{
                    cell_key,
                    revision
                };

                if (!entry.is_valid())
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "World cell snapshot manifest "
                        "contains an invalid revision "
                        "identity.");
                }

                if (has_previous_key &&
                    !(previous_key <
                        cell_key))
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "World cell snapshot manifest "
                        "entries are not in canonical "
                        "cell-key order.");
                }

                entries.push_back(
                    entry);

                previous_key =
                    cell_key;

                has_previous_key =
                    true;
            }

            if (!reader.complete())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "World cell snapshot manifest file "
                    "contains unexpected trailing "
                    "bytes.");
            }

            return
                WorldCellSnapshotManifest::create(
                    world_namespace,
                    manifest_revision,
                    std::span<
                        const WorldCellRevisionId>{
                            entries
                        });
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Unable to allocate world cell "
                "snapshot manifest entry storage.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "An unexpected failure occurred "
                "while deserializing a world cell "
                "snapshot manifest.");
        }
    }

    foundation::Result<std::vector<std::byte>>
    serialize_world_cell_manifest_pointer(
        const std::uint64_t world_namespace,
        const std::uint64_t manifest_revision)
        noexcept
    {
        if (world_namespace == 0ULL)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell manifest pointer "
                "namespace must be non-zero.");
        }

        if (manifest_revision == 0ULL)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell manifest pointer revision "
                "must be non-zero.");
        }

        try
        {
            std::vector<std::byte> bytes(
                static_cast<std::size_t>(
                    world_cell_manifest_pointer_file_byte_count));

            ByteWriter writer{
                std::span<std::byte>{
                    bytes
                }
            };

            const bool wrote_file =
                writer.write_bytes(
                    std::span<
                        const std::byte>{
                            manifest_pointer_magic
                        }) &&
                writer.write_u32(
                    world_cell_manifest_pointer_format_version) &&
                writer.write_u32(
                    0U) &&
                writer.write_u64(
                    world_namespace) &&
                writer.write_u64(
                    manifest_revision) &&
                writer.write_u64(
                    0ULL);

            if (!wrote_file ||
                !writer.complete())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "Unable to construct the complete "
                    "world cell manifest pointer "
                    "file.");
            }

            const std::uint64_t checksum =
                calculate_checksum(
                    std::span<
                        const std::byte>{
                            bytes
                        },
                    manifest_pointer_checksum_offset,
                    checksum_byte_count);

            if (!patch_u64(
                    std::span<std::byte>{
                        bytes
                    },
                    manifest_pointer_checksum_offset,
                    checksum))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "Unable to write the world cell "
                    "manifest pointer checksum.");
            }

            return bytes;
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Unable to allocate world cell "
                "manifest pointer serialization "
                "storage.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "An unexpected failure occurred "
                "while serializing a world cell "
                "manifest pointer.");
        }
    }

    foundation::Result<std::uint64_t>
    deserialize_world_cell_manifest_pointer(
        const std::span<const std::byte> bytes,
        const std::uint64_t
            expected_world_namespace)
        noexcept
    {
        if (expected_world_namespace == 0ULL)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Expected world namespace must be "
                "non-zero.");
        }

        if (bytes.size() !=
            static_cast<std::size_t>(
                world_cell_manifest_pointer_file_byte_count))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell manifest pointer file "
                "has an invalid size.");
        }

        ByteReader reader{
            bytes
        };

        std::uint32_t file_format_version{};
        std::uint32_t reserved{};
        std::uint64_t world_namespace{};
        std::uint64_t manifest_revision{};
        std::uint64_t stored_checksum{};

        const bool read_file =
            reader.read_magic(
                std::span<
                    const std::byte>{
                        manifest_pointer_magic
                    }) &&
            reader.read_u32(
                file_format_version) &&
            reader.read_u32(
                reserved) &&
            reader.read_u64(
                world_namespace) &&
            reader.read_u64(
                manifest_revision) &&
            reader.read_u64(
                stored_checksum);

        if (!read_file ||
            !reader.complete())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell manifest pointer file is "
                "invalid or truncated.");
        }

        if (file_format_version !=
            world_cell_manifest_pointer_format_version)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell manifest pointer format "
                "version is unsupported.");
        }

        if (reserved != 0U)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell manifest pointer contains "
                "unsupported reserved data.");
        }

        if (world_namespace !=
            expected_world_namespace)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell manifest pointer belongs "
                "to a different world namespace.");
        }

        if (manifest_revision == 0ULL)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell manifest pointer contains "
                "an invalid manifest revision.");
        }

        const std::uint64_t actual_checksum =
            calculate_checksum(
                bytes,
                manifest_pointer_checksum_offset,
                checksum_byte_count);

        if (actual_checksum !=
            stored_checksum)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell manifest pointer checksum "
                "does not match its contents.");
        }

        return manifest_revision;
    }
}