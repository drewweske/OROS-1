#include "oros/assets/cooked_asset_artifact_store.hpp"

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

namespace oros::assets
{
    namespace
    {
        [[nodiscard]]
        foundation::Result<std::vector<std::byte>>
        read_file_bytes(
            const std::filesystem::path& path,
            const std::uint64_t
                maximum_byte_count) noexcept
        {
            try
            {
                std::error_code file_size_error{};

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
                            "The cooked artifact file "
                            "does not exist.");
                    }

                    return foundation::fail(
                        foundation::ErrorCode::
                            input_output_failure,
                        "Unable to determine the cooked "
                        "artifact file size.");
                }

                if (!std::in_range<std::uint64_t>(
                        filesystem_byte_count))
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "The cooked artifact file size "
                        "cannot be represented by the "
                        "artifact store.");
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
                        "The cooked artifact file "
                        "exceeds the configured size "
                        "limit.");
                }

                if (!std::in_range<std::size_t>(
                        byte_count) ||
                    !std::in_range<std::streamsize>(
                        byte_count))
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "The cooked artifact file size "
                        "cannot be represented on this "
                        "platform.");
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
                        "Unable to open the cooked "
                        "artifact file for reading.");
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
                            "cooked artifact file.");
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
                        "The cooked artifact file "
                        "changed while it was being "
                        "read.");
                }

                return bytes;
            }
            catch (const std::bad_alloc&)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        out_of_memory,
                    "Unable to allocate storage while "
                    "reading a cooked artifact file.");
            }
            catch (...)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        input_output_failure,
                    "An unexpected failure occurred "
                    "while reading a cooked artifact "
                    "file.");
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
                if (!std::in_range<std::streamsize>(
                        bytes.size()))
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "The cooked artifact byte count "
                        "cannot be written by the file "
                        "stream.");
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
                        "cooked artifact file for "
                        "writing.");
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
                            "Unable to write the complete "
                            "cooked artifact file.");
                    }
                }

                stream.flush();

                if (!stream)
                {
                    stream.close();

                    std::error_code cleanup_error{};

                    std::filesystem::remove(
                        path,
                        cleanup_error);

                    return foundation::fail(
                        foundation::ErrorCode::
                            input_output_failure,
                        "Unable to flush the temporary "
                        "cooked artifact file.");
                }

                stream.close();

                if (stream.fail())
                {
                    std::error_code cleanup_error{};

                    std::filesystem::remove(
                        path,
                        cleanup_error);

                    return foundation::fail(
                        foundation::ErrorCode::
                            input_output_failure,
                        "Unable to close the temporary "
                        "cooked artifact file.");
                }

                return foundation::Status{};
            }
            catch (const std::bad_alloc&)
            {
                std::error_code cleanup_error{};

                std::filesystem::remove(
                    path,
                    cleanup_error);

                return foundation::fail(
                    foundation::ErrorCode::
                        out_of_memory,
                    "Unable to allocate storage while "
                    "writing a cooked artifact file.");
            }
            catch (...)
            {
                std::error_code cleanup_error{};

                std::filesystem::remove(
                    path,
                    cleanup_error);

                return foundation::fail(
                    foundation::ErrorCode::
                        input_output_failure,
                    "An unexpected failure occurred "
                    "while writing a cooked artifact "
                    "file.");
            }
        }

        [[nodiscard]]
        std::filesystem::path
        make_temporary_path(
            const std::filesystem::path&
                destination)
        {
            static std::atomic<std::uint64_t>
                sequence{0ULL};

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

            temporary += ".tmp-";
            temporary +=
                std::to_string(timestamp);
            temporary += "-";
            temporary +=
                std::to_string(suffix);

            return temporary;
        }
    }

    CookedAssetArtifactStore::
        CookedAssetArtifactStore(
            std::filesystem::path root_directory,
            const
                CookedAssetArtifactSerializationLimits
                    serialization_limits)
        : root_directory_{
              std::move(root_directory)
          },
          serialization_limits_{
              serialization_limits
          }
    {
    }

    CookedAssetArtifactStore::
        CookedAssetArtifactStore(
            CookedAssetArtifactStore&& other)
            noexcept
        : root_directory_{
              std::move(
                  other.root_directory_)
          },
          serialization_limits_{
              other.serialization_limits_
          }
    {
        other.root_directory_.clear();
        other.serialization_limits_ = {};
    }

    CookedAssetArtifactStore&
    CookedAssetArtifactStore::operator=(
        CookedAssetArtifactStore&& other)
        noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        root_directory_ =
            std::move(
                other.root_directory_);

        serialization_limits_ =
            other.serialization_limits_;

        other.root_directory_.clear();
        other.serialization_limits_ = {};

        return *this;
    }

    foundation::Result<std::filesystem::path>
    CookedAssetArtifactStore::path_for(
        const ContentHash& artifact_hash)
        const noexcept
    {
        if (!is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "The cooked artifact store is not "
                "valid.");
        }

        try
        {
            const std::string hash_text =
                to_string(
                    artifact_hash);

            if (hash_text.size() !=
                content_hash_text_length)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "The serialized artifact hash has "
                    "an unexpected length.");
            }

            const std::filesystem::path
                artifact_path =
                    root_directory_ /
                    hash_text.substr(
                        0U,
                        2U) /
                    hash_text.substr(
                        2U,
                        2U) /
                    (hash_text +
                        ".orosartifact");

            return artifact_path;
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Unable to allocate the cooked "
                "artifact path.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "An unexpected failure occurred while "
                "building the cooked artifact path.");
        }
    }

    foundation::Result<ContentHash>
    CookedAssetArtifactStore::store(
        const CookedAssetArtifact& artifact)
        const noexcept
    {
        if (!is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Cannot store an artifact in an "
                "invalid cooked artifact store.");
        }

        if (!artifact.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cannot store an invalid cooked "
                "artifact.");
        }

        try
        {
            foundation::Result<
                std::vector<std::byte>>
                serialization_result =
                    serialize_cooked_asset_artifact(
                        artifact,
                        serialization_limits_);

            if (!serialization_result.has_value())
            {
                return foundation::fail(
                    serialization_result.error().
                        code,
                    serialization_result.error().
                        message);
            }

            std::vector<std::byte>
                serialized_bytes =
                    std::move(
                        serialization_result.value());

            const ContentHash artifact_hash =
                hash_bytes(
                    std::span<const std::byte>{
                        serialized_bytes
                    });

            const foundation::Result<
                std::filesystem::path>
                path_result =
                    path_for(
                        artifact_hash);

            if (!path_result.has_value())
            {
                return foundation::fail(
                    path_result.error().code,
                    path_result.error().message);
            }

            const std::filesystem::path
                destination =
                    path_result.value();

            std::error_code exists_error{};

            const bool destination_exists =
                std::filesystem::exists(
                    destination,
                    exists_error);

            if (exists_error)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        input_output_failure,
                    "Unable to inspect the cooked "
                    "artifact destination.");
            }

            if (destination_exists)
            {
                foundation::Result<
                    std::vector<std::byte>>
                    existing_result =
                        read_file_bytes(
                            destination,
                            serialization_limits_.
                                max_artifact_byte_count);

                if (!existing_result.has_value())
                {
                    return foundation::fail(
                        existing_result.error().code,
                        existing_result.error().
                            message);
                }

                if (existing_result.value() !=
                    serialized_bytes)
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_state,
                        "The content-addressed cooked "
                        "artifact path already contains "
                        "different bytes.");
                }

                return artifact_hash;
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
                    "Unable to create the cooked "
                    "artifact store directories.");
            }

            const std::filesystem::path
                temporary =
                    make_temporary_path(
                        destination);

            const foundation::Status
                write_status =
                    write_file_bytes(
                        temporary,
                        std::span<const std::byte>{
                            serialized_bytes
                        });

            if (!write_status.has_value())
            {
                return foundation::fail(
                    write_status.error().code,
                    write_status.error().message);
            }

            std::error_code rename_error{};

            std::filesystem::rename(
                temporary,
                destination,
                rename_error);

            if (!rename_error)
            {
                return artifact_hash;
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
                            serialization_limits_.
                                max_artifact_byte_count);

                std::error_code cleanup_error{};

                std::filesystem::remove(
                    temporary,
                    cleanup_error);

                if (!existing_result.has_value())
                {
                    return foundation::fail(
                        existing_result.error().code,
                        existing_result.error().
                            message);
                }

                if (existing_result.value() ==
                    serialized_bytes)
                {
                    return artifact_hash;
                }

                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_state,
                    "A concurrent cooked artifact "
                    "write produced different bytes at "
                    "the same content address.");
            }

            std::error_code cleanup_error{};

            std::filesystem::remove(
                temporary,
                cleanup_error);

            return foundation::fail(
                foundation::ErrorCode::
                    input_output_failure,
                "Unable to atomically publish the "
                "cooked artifact file.");
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Unable to allocate storage while "
                "persisting a cooked artifact.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    input_output_failure,
                "An unexpected failure occurred while "
                "persisting a cooked artifact.");
        }
    }

    foundation::Result<CookedAssetArtifact>
    CookedAssetArtifactStore::load(
        const ContentHash& artifact_hash)
        const noexcept
    {
        if (!is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Cannot load an artifact from an "
                "invalid cooked artifact store.");
        }

        const foundation::Result<
            std::filesystem::path>
            path_result =
                path_for(
                    artifact_hash);

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
                    serialization_limits_.
                        max_artifact_byte_count);

        if (!read_result.has_value())
        {
            return foundation::fail(
                read_result.error().code,
                read_result.error().message);
        }

        const std::vector<std::byte>& bytes =
            read_result.value();

        const ContentHash actual_hash =
            hash_bytes(
                std::span<const std::byte>{
                    bytes
                });

        if (actual_hash != artifact_hash)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "The cooked artifact file does not "
                "match its content-addressed hash.");
        }

        foundation::Result<
            CookedAssetArtifact>
            deserialization_result =
                deserialize_cooked_asset_artifact(
                    std::span<const std::byte>{
                        bytes
                    },
                    serialization_limits_);

        if (!deserialization_result.has_value())
        {
            return foundation::fail(
                deserialization_result.error().code,
                deserialization_result.error().
                    message);
        }

        return std::move(
            deserialization_result.value());
    }

    foundation::Result<bool>
    CookedAssetArtifactStore::contains(
        const ContentHash& artifact_hash)
        const noexcept
    {
        if (!is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Cannot inspect an invalid cooked "
                "artifact store.");
        }

        const foundation::Result<
            std::filesystem::path>
            path_result =
                path_for(
                    artifact_hash);

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
                "Unable to inspect the cooked artifact "
                "path.");
        }

        if (!exists)
        {
            return false;
        }

        std::error_code type_error{};

        const bool regular_file =
            std::filesystem::is_regular_file(
                path_result.value(),
                type_error);

        if (type_error)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    input_output_failure,
                "Unable to inspect the cooked artifact "
                "file type.");
        }

        if (!regular_file)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "The cooked artifact path is not a "
                "regular file.");
        }

        return true;
    }

    foundation::Status
    CookedAssetArtifactStore::erase(
        const ContentHash& artifact_hash)
        const noexcept
    {
        if (!is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Cannot erase an artifact from an "
                "invalid cooked artifact store.");
        }

        const foundation::Result<
            std::filesystem::path>
            path_result =
                path_for(
                    artifact_hash);

        if (!path_result.has_value())
        {
            return foundation::fail(
                path_result.error().code,
                path_result.error().message);
        }

        std::error_code remove_error{};

        std::filesystem::remove(
            path_result.value(),
            remove_error);

        if (remove_error)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    input_output_failure,
                "Unable to erase the cooked artifact "
                "file.");
        }

        return foundation::Status{};
    }
}