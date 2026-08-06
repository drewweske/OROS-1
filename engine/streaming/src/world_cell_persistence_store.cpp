#include "oros/streaming/world_cell_persistence_store.hpp"

#include "world_cell_persistence_format.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <new>
#include <span>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#endif

namespace oros::streaming
{
    namespace
    {
        inline constexpr std::size_t
            namespace_hex_length{
                16U
            };

        inline constexpr char
            hex_digits[]{
                "0123456789abcdef"
            };

        [[nodiscard]]
        std::string world_namespace_text(
            const std::uint64_t
                world_namespace)
        {
            std::string text(
                namespace_hex_length,
                '0');

            for (std::size_t index = 0U;
                 index < namespace_hex_length;
                 ++index)
            {
                const unsigned int shift =
                    static_cast<unsigned int>(
                        (
                            namespace_hex_length -
                            1U -
                            index
                        ) *
                        4U);

                const std::uint64_t digit =
                    (
                        world_namespace >>
                        shift
                    ) &
                    0xFULL;

                text[index] =
                    hex_digits[
                        static_cast<std::size_t>(
                            digit)];
            }

            return text;
        }

        [[nodiscard]]
        foundation::Result<
            std::vector<std::byte>>
        read_file_bytes(
            const std::filesystem::path& path,
            const std::uint64_t
                maximum_byte_count) noexcept
        {
            try
            {
                std::error_code
                    file_size_error{};

                const std::uintmax_t
                    filesystem_byte_count =
                        std::filesystem::file_size(
                            path,
                            file_size_error);

                if (file_size_error)
                {
                    if (file_size_error ==
                        std::errc::
                            no_such_file_or_directory)
                    {
                        return foundation::fail(
                            foundation::ErrorCode::
                                not_found,
                            "The world cell persistence "
                            "file does not exist.");
                    }

                    return foundation::fail(
                        foundation::ErrorCode::
                            input_output_failure,
                        "Unable to determine the world "
                        "cell persistence file size.");
                }

                if (!std::in_range<
                        std::uint64_t>(
                            filesystem_byte_count))
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "The world cell persistence "
                        "file size cannot be represented "
                        "by the persistence store.");
                }

                const std::uint64_t byte_count =
                    static_cast<std::uint64_t>(
                        filesystem_byte_count);

                if (byte_count >
                    maximum_byte_count)
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "The world cell persistence "
                        "file exceeds the configured "
                        "size limit.");
                }

                if (!std::in_range<std::size_t>(
                        byte_count) ||
                    !std::in_range<
                        std::streamsize>(
                            byte_count))
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "The world cell persistence "
                        "file size cannot be represented "
                        "on this platform.");
                }

                std::vector<std::byte> bytes(
                    static_cast<std::size_t>(
                        byte_count));

                std::ifstream stream{
                    path,
                    std::ios::binary
                };

                if (!stream.is_open())
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            input_output_failure,
                        "Unable to open the world cell "
                        "persistence file for reading.");
                }

                if (!bytes.empty())
                {
                    stream.read(
                        reinterpret_cast<char*>(
                            bytes.data()),
                        static_cast<std::streamsize>(
                            bytes.size()));

                    if (!stream ||
                        stream.gcount() !=
                            static_cast<
                                std::streamsize>(
                                    bytes.size()))
                    {
                        return foundation::fail(
                            foundation::ErrorCode::
                                input_output_failure,
                            "Unable to read the complete "
                            "world cell persistence "
                            "file.");
                    }
                }

                std::error_code
                    final_size_error{};

                const std::uintmax_t
                    final_byte_count =
                        std::filesystem::file_size(
                            path,
                            final_size_error);

                if (final_size_error ||
                    final_byte_count !=
                        filesystem_byte_count)
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            input_output_failure,
                        "The world cell persistence "
                        "file changed while it was "
                        "being read.");
                }

                return bytes;
            }
            catch (const std::bad_alloc&)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        out_of_memory,
                    "Unable to allocate storage while "
                    "reading a world cell persistence "
                    "file.");
            }
            catch (...)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        input_output_failure,
                    "An unexpected failure occurred "
                    "while reading a world cell "
                    "persistence file.");
            }
        }

        [[nodiscard]]
        foundation::Status
        write_file_bytes(
            const std::filesystem::path& path,
            const std::span<const std::byte>
                bytes) noexcept
        {
            try
            {
                if (!std::in_range<
                        std::streamsize>(
                            bytes.size()))
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "The world cell persistence "
                        "byte count cannot be written "
                        "by the file stream.");
                }

                std::ofstream stream{
                    path,
                    std::ios::binary |
                        std::ios::trunc
                };

                if (!stream.is_open())
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            input_output_failure,
                        "Unable to open the temporary "
                        "world cell persistence file "
                        "for writing.");
                }

                if (!bytes.empty())
                {
                    stream.write(
                        reinterpret_cast<
                            const char*>(
                                bytes.data()),
                        static_cast<std::streamsize>(
                            bytes.size()));

                    if (!stream)
                    {
                        stream.close();

                        std::error_code
                            cleanup_error{};

                        std::filesystem::remove(
                            path,
                            cleanup_error);

                        return foundation::fail(
                            foundation::ErrorCode::
                                input_output_failure,
                            "Unable to write the "
                            "complete temporary world "
                            "cell persistence file.");
                    }
                }

                stream.flush();

                if (!stream)
                {
                    stream.close();

                    std::error_code
                        cleanup_error{};

                    std::filesystem::remove(
                        path,
                        cleanup_error);

                    return foundation::fail(
                        foundation::ErrorCode::
                            input_output_failure,
                        "Unable to flush the temporary "
                        "world cell persistence file.");
                }

                stream.close();

                if (stream.fail())
                {
                    std::error_code
                        cleanup_error{};

                    std::filesystem::remove(
                        path,
                        cleanup_error);

                    return foundation::fail(
                        foundation::ErrorCode::
                            input_output_failure,
                        "Unable to close the temporary "
                        "world cell persistence file.");
                }

                return foundation::Status{};
            }
            catch (const std::bad_alloc&)
            {
                std::error_code
                    cleanup_error{};

                std::filesystem::remove(
                    path,
                    cleanup_error);

                return foundation::fail(
                    foundation::ErrorCode::
                        out_of_memory,
                    "Unable to allocate storage while "
                    "writing a world cell persistence "
                    "file.");
            }
            catch (...)
            {
                std::error_code
                    cleanup_error{};

                std::filesystem::remove(
                    path,
                    cleanup_error);

                return foundation::fail(
                    foundation::ErrorCode::
                        input_output_failure,
                    "An unexpected failure occurred "
                    "while writing a world cell "
                    "persistence file.");
            }
        }

        [[nodiscard]]
        std::filesystem::path
        make_temporary_path(
            const std::filesystem::path&
                destination)
        {
            static std::atomic<std::uint64_t>
                sequence{
                    0ULL
                };

            const auto timestamp =
                std::chrono::steady_clock::now().
                    time_since_epoch().
                    count();

            const std::uint64_t suffix =
                sequence.fetch_add(
                    1ULL,
                    std::memory_order_relaxed);

            std::filesystem::path temporary =
                destination;

            temporary +=
                ".tmp-";

            temporary +=
                std::to_string(
                    timestamp);

            temporary +=
                "-";

            temporary +=
                std::to_string(
                    suffix);

            return temporary;
        }

        [[nodiscard]]
        foundation::Status
        publish_immutable_file(
            const std::filesystem::path&
                destination,
            const std::span<const std::byte>
                bytes) noexcept
        {
            try
            {
                if (!std::in_range<
                        std::uint64_t>(
                            bytes.size()))
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "The immutable persistence "
                        "file size cannot be represented "
                        "by the store.");
                }

                const std::uint64_t
                    maximum_existing_byte_count =
                        static_cast<std::uint64_t>(
                            bytes.size());

                std::error_code
                    exists_error{};

                const bool destination_exists =
                    std::filesystem::exists(
                        destination,
                        exists_error);

                if (exists_error)
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            input_output_failure,
                        "Unable to inspect the world "
                        "cell persistence destination.");
                }

                if (destination_exists)
                {
                    foundation::Result<
                        std::vector<std::byte>>
                        existing_result =
                            read_file_bytes(
                                destination,
                                maximum_existing_byte_count);

                    if (!existing_result.has_value())
                    {
                        return foundation::fail(
                            existing_result.error().
                                code,
                            existing_result.error().
                                message);
                    }

                    if (!std::ranges::equal(
                            existing_result.value(),
                            bytes))
                    {
                        return foundation::fail(
                            foundation::ErrorCode::
                                invalid_state,
                            "An immutable world cell "
                            "persistence path already "
                            "contains different bytes.");
                    }

                    return foundation::Status{};
                }

                std::error_code
                    directory_error{};

                std::filesystem::create_directories(
                    destination.parent_path(),
                    directory_error);

                if (directory_error)
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            input_output_failure,
                        "Unable to create the world "
                        "cell persistence directories.");
                }

                const std::filesystem::path
                    temporary =
                        make_temporary_path(
                            destination);

                const foundation::Status
                    write_status =
                        write_file_bytes(
                            temporary,
                            bytes);

                if (!write_status.has_value())
                {
                    return foundation::fail(
                        write_status.error().code,
                        write_status.error().message);
                }

                std::error_code
                    rename_error{};

                std::filesystem::rename(
                    temporary,
                    destination,
                    rename_error);

                if (!rename_error)
                {
                    return foundation::Status{};
                }

                std::error_code
                    race_exists_error{};

                const bool destination_now_exists =
                    std::filesystem::exists(
                        destination,
                        race_exists_error);

                if (!race_exists_error &&
                    destination_now_exists)
                {
                    foundation::Result<
                        std::vector<std::byte>>
                        existing_result =
                            read_file_bytes(
                                destination,
                                maximum_existing_byte_count);

                    std::error_code
                        cleanup_error{};

                    std::filesystem::remove(
                        temporary,
                        cleanup_error);

                    if (!existing_result.has_value())
                    {
                        return foundation::fail(
                            existing_result.error().
                                code,
                            existing_result.error().
                                message);
                    }

                    if (std::ranges::equal(
                            existing_result.value(),
                            bytes))
                    {
                        return foundation::Status{};
                    }

                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_state,
                        "A concurrent immutable world "
                        "cell persistence write "
                        "produced different bytes at "
                        "the same path.");
                }

                std::error_code
                    cleanup_error{};

                std::filesystem::remove(
                    temporary,
                    cleanup_error);

                return foundation::fail(
                    foundation::ErrorCode::
                        input_output_failure,
                    "Unable to atomically publish the "
                    "immutable world cell persistence "
                    "file.");
            }
            catch (const std::bad_alloc&)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        out_of_memory,
                    "Unable to allocate storage while "
                    "publishing an immutable world "
                    "cell persistence file.");
            }
            catch (...)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        input_output_failure,
                    "An unexpected failure occurred "
                    "while publishing an immutable "
                    "world cell persistence file.");
            }
        }

        [[nodiscard]]
        foundation::Status
        replace_file_atomically(
            const std::filesystem::path&
                destination,
            const std::span<const std::byte>
                bytes) noexcept
        {
            try
            {
                std::error_code
                    directory_error{};

                std::filesystem::create_directories(
                    destination.parent_path(),
                    directory_error);

                if (directory_error)
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            input_output_failure,
                        "Unable to create the world "
                        "cell manifest publication "
                        "directories.");
                }

                const std::filesystem::path
                    temporary =
                        make_temporary_path(
                            destination);

                const foundation::Status
                    write_status =
                        write_file_bytes(
                            temporary,
                            bytes);

                if (!write_status.has_value())
                {
                    return foundation::fail(
                        write_status.error().code,
                        write_status.error().message);
                }

#if defined(_WIN32)
                const BOOL move_result =
                    MoveFileExW(
                        temporary.c_str(),
                        destination.c_str(),
                        MOVEFILE_REPLACE_EXISTING |
                            MOVEFILE_WRITE_THROUGH);

                if (move_result == FALSE)
                {
                    std::error_code
                        cleanup_error{};

                    std::filesystem::remove(
                        temporary,
                        cleanup_error);

                    return foundation::fail(
                        foundation::ErrorCode::
                            input_output_failure,
                        "Unable to atomically replace "
                        "the current world cell "
                        "manifest pointer.");
                }
#else
                std::error_code
                    rename_error{};

                std::filesystem::rename(
                    temporary,
                    destination,
                    rename_error);

                if (rename_error)
                {
                    std::error_code
                        cleanup_error{};

                    std::filesystem::remove(
                        temporary,
                        cleanup_error);

                    return foundation::fail(
                        foundation::ErrorCode::
                            input_output_failure,
                        "Unable to atomically replace "
                        "the current world cell "
                        "manifest pointer.");
                }
#endif

                return foundation::Status{};
            }
            catch (const std::bad_alloc&)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        out_of_memory,
                    "Unable to allocate storage while "
                    "publishing the current world cell "
                    "manifest pointer.");
            }
            catch (...)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        input_output_failure,
                    "An unexpected failure occurred "
                    "while publishing the current "
                    "world cell manifest pointer.");
            }
        }
    }

    WorldCellPersistenceStore::
    WorldCellPersistenceStore(
        std::filesystem::path root_directory,
        const WorldCellPersistenceLimits
            limits)
        : root_directory_{
              std::move(
                  root_directory)
          },
          limits_{
              limits
          }
    {
    }

    WorldCellPersistenceStore::
    WorldCellPersistenceStore(
        WorldCellPersistenceStore&& other)
        noexcept
        : root_directory_{
              std::move(
                  other.root_directory_)
          },
          limits_{
              other.limits_
          }
    {
        other.root_directory_.clear();

        other.limits_ =
            WorldCellPersistenceLimits{
                0ULL,
                0ULL
            };
    }

    WorldCellPersistenceStore&
    WorldCellPersistenceStore::operator=(
        WorldCellPersistenceStore&& other)
        noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        root_directory_ =
            std::move(
                other.root_directory_);

        limits_ =
            other.limits_;

        other.root_directory_.clear();

        other.limits_ =
            WorldCellPersistenceLimits{
                0ULL,
                0ULL
            };

        return *this;
    }

    bool
    WorldCellPersistenceStore::is_valid()
        const noexcept
    {
        return
            !root_directory_.empty() &&
            limits_.is_valid();
    }

    const std::filesystem::path&
    WorldCellPersistenceStore::
    root_directory() const noexcept
    {
        return root_directory_;
    }

    const WorldCellPersistenceLimits&
    WorldCellPersistenceStore::limits()
        const noexcept
    {
        return limits_;
    }

    foundation::Result<
        std::filesystem::path>
    WorldCellPersistenceStore::
    snapshot_path_for(
        const WorldCellRevisionId&
            revision_id) const noexcept
    {
        if (!is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "The world cell persistence store is "
                "not valid.");
        }

        if (!revision_id.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cannot build a snapshot path for an "
                "invalid world cell revision "
                "identity.");
        }

        try
        {
            const std::string namespace_text =
                world_namespace_text(
                    revision_id.cell_key.
                        world_namespace);

            const std::string file_name =
                std::to_string(
                    revision_id.cell_key.
                        cell.z) +
                "@" +
                std::to_string(
                    revision_id.revision) +
                ".orossnapshot";

            const std::filesystem::path path =
                root_directory_ /
                "worlds" /
                namespace_text /
                "snapshots" /
                std::to_string(
                    revision_id.cell_key.
                        cell.x) /
                std::to_string(
                    revision_id.cell_key.
                        cell.y) /
                file_name;

            return path;
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Unable to allocate the world cell "
                "snapshot path.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "An unexpected failure occurred "
                "while building the world cell "
                "snapshot path.");
        }
    }

    foundation::Result<
        std::filesystem::path>
    WorldCellPersistenceStore::
    manifest_path_for(
        const std::uint64_t world_namespace,
        const std::uint64_t manifest_revision)
        const noexcept
    {
        if (!is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "The world cell persistence store is "
                "not valid.");
        }

        if (world_namespace == 0ULL)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cannot build a manifest path for a "
                "zero world namespace.");
        }

        if (manifest_revision == 0ULL)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cannot build a manifest path for a "
                "zero manifest revision.");
        }

        try
        {
            const std::string namespace_text =
                world_namespace_text(
                    world_namespace);

            const std::string file_name =
                std::to_string(
                    manifest_revision) +
                ".orosmanifest";

            const std::filesystem::path path =
                root_directory_ /
                "worlds" /
                namespace_text /
                "manifests" /
                file_name;

            return path;
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Unable to allocate the world cell "
                "manifest path.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "An unexpected failure occurred "
                "while building the world cell "
                "manifest path.");
        }
    }

    foundation::Result<
        std::filesystem::path>
    WorldCellPersistenceStore::
    current_manifest_pointer_path_for(
        const std::uint64_t world_namespace)
        const noexcept
    {
        if (!is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "The world cell persistence store is "
                "not valid.");
        }

        if (world_namespace == 0ULL)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cannot build a current manifest "
                "pointer path for a zero world "
                "namespace.");
        }

        try
        {
            const std::string namespace_text =
                world_namespace_text(
                    world_namespace);

            const std::filesystem::path path =
                root_directory_ /
                "worlds" /
                namespace_text /
                "current.orosmanifestref";

            return path;
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Unable to allocate the current world "
                "cell manifest pointer path.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "An unexpected failure occurred "
                "while building the current world "
                "cell manifest pointer path.");
        }
    }

    foundation::Result<
        WorldCellRevisionId>
    WorldCellPersistenceStore::
    store_snapshot(
        const WorldCellSnapshot& snapshot)
        const noexcept
    {
        if (!is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Cannot store a snapshot in an "
                "invalid world cell persistence "
                "store.");
        }

        if (!snapshot.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cannot store an invalid world cell "
                "snapshot.");
        }

        foundation::Result<
            std::vector<std::byte>>
            serialization_result =
                detail::
                    serialize_world_cell_snapshot(
                        snapshot,
                        limits_);

        if (!serialization_result.has_value())
        {
            return foundation::fail(
                serialization_result.error().
                    code,
                serialization_result.error().
                    message);
        }

        const WorldCellRevisionId revision_id{
            snapshot.key(),
            snapshot.revision()
        };

        const foundation::Result<
            std::filesystem::path>
            path_result =
                snapshot_path_for(
                    revision_id);

        if (!path_result.has_value())
        {
            return foundation::fail(
                path_result.error().code,
                path_result.error().message);
        }

        std::vector<std::byte> bytes =
            std::move(
                serialization_result.value());

        const foundation::Status
            publication_status =
                publish_immutable_file(
                    path_result.value(),
                    std::span<
                        const std::byte>{
                            bytes
                        });

        if (!publication_status.has_value())
        {
            return foundation::fail(
                publication_status.error().code,
                publication_status.error().
                    message);
        }

        return revision_id;
    }

    foundation::Result<
        WorldCellSnapshot>
    WorldCellPersistenceStore::
    load_snapshot(
        const WorldCellRevisionId&
            revision_id) const noexcept
    {
        if (!is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Cannot load a snapshot from an "
                "invalid world cell persistence "
                "store.");
        }

        if (!revision_id.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cannot load an invalid world cell "
                "revision identity.");
        }

        const foundation::Result<
            std::filesystem::path>
            path_result =
                snapshot_path_for(
                    revision_id);

        if (!path_result.has_value())
        {
            return foundation::fail(
                path_result.error().code,
                path_result.error().message);
        }

        foundation::Result<
            std::vector<std::byte>>
            read_result =
                read_file_bytes(
                    path_result.value(),
                    detail::
                        maximum_snapshot_file_byte_count(
                            limits_));

        if (!read_result.has_value())
        {
            return foundation::fail(
                read_result.error().code,
                read_result.error().message);
        }

        foundation::Result<
            WorldCellSnapshot>
            snapshot_result =
                detail::
                    deserialize_world_cell_snapshot(
                        std::span<
                            const std::byte>{
                                read_result.value()
                            },
                        limits_);

        if (!snapshot_result.has_value())
        {
            return foundation::fail(
                snapshot_result.error().code,
                snapshot_result.error().
                    message);
        }

        const WorldCellSnapshot& snapshot =
            snapshot_result.value();

        if (snapshot.key() !=
                revision_id.cell_key ||
            snapshot.revision() !=
                revision_id.revision)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "The persisted world cell snapshot "
                "identity does not match its storage "
                "path.");
        }

        return snapshot_result;
    }

    foundation::Result<bool>
    WorldCellPersistenceStore::
    contains_snapshot(
        const WorldCellRevisionId&
            revision_id) const noexcept
    {
        if (!is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Cannot inspect an invalid world cell "
                "persistence store.");
        }

        if (!revision_id.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cannot inspect an invalid world cell "
                "revision identity.");
        }

        const foundation::Result<
            std::filesystem::path>
            path_result =
                snapshot_path_for(
                    revision_id);

        if (!path_result.has_value())
        {
            return foundation::fail(
                path_result.error().code,
                path_result.error().message);
        }

        std::error_code exists_error{};

        const bool exists =
            std::filesystem::exists(
                path_result.value(),
                exists_error);

        if (exists_error)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    input_output_failure,
                "Unable to inspect the world cell "
                "snapshot path.");
        }

        if (!exists)
        {
            return false;
        }

        const foundation::Result<
            WorldCellSnapshot>
            load_result =
                load_snapshot(
                    revision_id);

        if (!load_result.has_value())
        {
            return foundation::fail(
                load_result.error().code,
                load_result.error().message);
        }

        return true;
    }

    foundation::Status
    WorldCellPersistenceStore::
    store_manifest(
        const WorldCellSnapshotManifest&
            manifest) const noexcept
    {
        if (!is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Cannot store a manifest in an "
                "invalid world cell persistence "
                "store.");
        }

        if (!manifest.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cannot store an invalid world cell "
                "snapshot manifest.");
        }

        foundation::Result<
            std::vector<std::byte>>
            serialization_result =
                detail::
                    serialize_world_cell_snapshot_manifest(
                        manifest,
                        limits_);

        if (!serialization_result.has_value())
        {
            return foundation::fail(
                serialization_result.error().
                    code,
                serialization_result.error().
                    message);
        }

        const foundation::Result<
            std::filesystem::path>
            path_result =
                manifest_path_for(
                    manifest.world_namespace(),
                    manifest.manifest_revision());

        if (!path_result.has_value())
        {
            return foundation::fail(
                path_result.error().code,
                path_result.error().message);
        }

        std::vector<std::byte> bytes =
            std::move(
                serialization_result.value());

        return publish_immutable_file(
            path_result.value(),
            std::span<
                const std::byte>{
                    bytes
                });
    }

    foundation::Result<
        WorldCellSnapshotManifest>
    WorldCellPersistenceStore::
    load_manifest(
        const std::uint64_t world_namespace,
        const std::uint64_t manifest_revision)
        const noexcept
    {
        if (!is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Cannot load a manifest from an "
                "invalid world cell persistence "
                "store.");
        }

        if (world_namespace == 0ULL ||
            manifest_revision == 0ULL)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World namespace and manifest "
                "revision must both be non-zero.");
        }

        const foundation::Result<
            std::filesystem::path>
            path_result =
                manifest_path_for(
                    world_namespace,
                    manifest_revision);

        if (!path_result.has_value())
        {
            return foundation::fail(
                path_result.error().code,
                path_result.error().message);
        }

        foundation::Result<
            std::vector<std::byte>>
            read_result =
                read_file_bytes(
                    path_result.value(),
                    detail::
                        maximum_manifest_file_byte_count(
                            limits_));

        if (!read_result.has_value())
        {
            return foundation::fail(
                read_result.error().code,
                read_result.error().message);
        }

        foundation::Result<
            WorldCellSnapshotManifest>
            manifest_result =
                detail::
                    deserialize_world_cell_snapshot_manifest(
                        std::span<
                            const std::byte>{
                                read_result.value()
                            },
                        limits_);

        if (!manifest_result.has_value())
        {
            return foundation::fail(
                manifest_result.error().code,
                manifest_result.error().
                    message);
        }

        const WorldCellSnapshotManifest&
            manifest =
                manifest_result.value();

        if (manifest.world_namespace() !=
                world_namespace ||
            manifest.manifest_revision() !=
                manifest_revision)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "The persisted world cell manifest "
                "identity does not match its storage "
                "path.");
        }

        return manifest_result;
    }

    foundation::Result<bool>
    WorldCellPersistenceStore::
    contains_manifest(
        const std::uint64_t world_namespace,
        const std::uint64_t manifest_revision)
        const noexcept
    {
        if (!is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Cannot inspect an invalid world cell "
                "persistence store.");
        }

        if (world_namespace == 0ULL ||
            manifest_revision == 0ULL)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World namespace and manifest "
                "revision must both be non-zero.");
        }

        const foundation::Result<
            std::filesystem::path>
            path_result =
                manifest_path_for(
                    world_namespace,
                    manifest_revision);

        if (!path_result.has_value())
        {
            return foundation::fail(
                path_result.error().code,
                path_result.error().message);
        }

        std::error_code exists_error{};

        const bool exists =
            std::filesystem::exists(
                path_result.value(),
                exists_error);

        if (exists_error)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    input_output_failure,
                "Unable to inspect the world cell "
                "manifest path.");
        }

        if (!exists)
        {
            return false;
        }

        const foundation::Result<
            WorldCellSnapshotManifest>
            load_result =
                load_manifest(
                    world_namespace,
                    manifest_revision);

        if (!load_result.has_value())
        {
            return foundation::fail(
                load_result.error().code,
                load_result.error().message);
        }

        return true;
    }

    foundation::Status
    WorldCellPersistenceStore::
    publish_manifest(
        const WorldCellSnapshotManifest&
            manifest) const noexcept
    {
        if (!is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Cannot publish a manifest through an "
                "invalid world cell persistence "
                "store.");
        }

        if (!manifest.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cannot publish an invalid world cell "
                "snapshot manifest.");
        }

        for (const WorldCellRevisionId&
                 entry :
             manifest.entries())
        {
            const foundation::Result<bool>
                contains_result =
                    contains_snapshot(
                        entry);

            if (!contains_result.has_value())
            {
                return foundation::fail(
                    contains_result.error().code,
                    contains_result.error().
                        message);
            }

            if (!contains_result.value())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        not_found,
                    "Cannot publish a world cell "
                    "manifest while a referenced "
                    "snapshot is missing.");
            }
        }

        const foundation::Status
            store_status =
                store_manifest(
                    manifest);

        if (!store_status.has_value())
        {
            return foundation::fail(
                store_status.error().code,
                store_status.error().message);
        }

        foundation::Result<
            std::vector<std::byte>>
            pointer_result =
                detail::
                    serialize_world_cell_manifest_pointer(
                        manifest.world_namespace(),
                        manifest.manifest_revision());

        if (!pointer_result.has_value())
        {
            return foundation::fail(
                pointer_result.error().code,
                pointer_result.error().message);
        }

        const foundation::Result<
            std::filesystem::path>
            path_result =
                current_manifest_pointer_path_for(
                    manifest.world_namespace());

        if (!path_result.has_value())
        {
            return foundation::fail(
                path_result.error().code,
                path_result.error().message);
        }

        std::vector<std::byte> pointer_bytes =
            std::move(
                pointer_result.value());

        return replace_file_atomically(
            path_result.value(),
            std::span<
                const std::byte>{
                    pointer_bytes
                });
    }

    foundation::Result<
        WorldCellSnapshotManifest>
    WorldCellPersistenceStore::
    load_current_manifest(
        const std::uint64_t world_namespace)
        const noexcept
    {
        if (!is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Cannot load the current manifest "
                "from an invalid world cell "
                "persistence store.");
        }

        if (world_namespace == 0ULL)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cannot load a current manifest for "
                "a zero world namespace.");
        }

        const foundation::Result<
            std::filesystem::path>
            path_result =
                current_manifest_pointer_path_for(
                    world_namespace);

        if (!path_result.has_value())
        {
            return foundation::fail(
                path_result.error().code,
                path_result.error().message);
        }

        foundation::Result<
            std::vector<std::byte>>
            read_result =
                read_file_bytes(
                    path_result.value(),
                    detail::
                        world_cell_manifest_pointer_file_byte_count);

        if (!read_result.has_value())
        {
            return foundation::fail(
                read_result.error().code,
                read_result.error().message);
        }

        const foundation::Result<std::uint64_t>
            revision_result =
                detail::
                    deserialize_world_cell_manifest_pointer(
                        std::span<
                            const std::byte>{
                                read_result.value()
                            },
                        world_namespace);

        if (!revision_result.has_value())
        {
            return foundation::fail(
                revision_result.error().code,
                revision_result.error().message);
        }

        return load_manifest(
            world_namespace,
            revision_result.value());
    }
}