#include "oros/assets/cooked_asset_artifact_store.hpp"

#include "oros/assets/content_hash.hpp"
#include "oros/assets/cooked_asset_artifact_serialization.hpp"
#include "oros/foundation/error.hpp"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
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

    class TemporaryDirectory final
    {
    public:
        TemporaryDirectory()
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

            path_ =
                std::filesystem::
                    temp_directory_path() /
                ("oros-artifact-store-tests-" +
                    std::to_string(timestamp) +
                    "-" +
                    std::to_string(suffix));

            std::error_code create_error{};

            std::filesystem::create_directories(
                path_,
                create_error);

            valid_ = !create_error;
        }

        ~TemporaryDirectory()
        {
            std::error_code remove_error{};

            std::filesystem::remove_all(
                path_,
                remove_error);
        }

        TemporaryDirectory(
            const TemporaryDirectory&) =
                delete;

        TemporaryDirectory&
        operator=(
            const TemporaryDirectory&) =
                delete;

        TemporaryDirectory(
            TemporaryDirectory&&) =
                delete;

        TemporaryDirectory&
        operator=(
            TemporaryDirectory&&) =
                delete;

        [[nodiscard]]
        bool
        is_valid() const noexcept
        {
            return valid_;
        }

        [[nodiscard]]
        const std::filesystem::path&
        path() const noexcept
        {
            return path_;
        }

    private:
        std::filesystem::path path_{};
        bool valid_{};
    };

    [[nodiscard]]
    oros::assets::CookedAssetArtifact
    make_artifact(
        const std::uint64_t asset_sequence,
        const std::string_view source_path)
    {
        using namespace oros::assets;

        CookedAssetArtifact artifact{};

        artifact.payload_bytes = {
            std::byte{0x10},
            std::byte{0x20},
            std::byte{0x30},
            std::byte{0x40},
            std::byte{0x50}
        };

        artifact.record.id =
            AssetId{
                0x4F524F53ULL,
                asset_sequence
            };

        artifact.record.source_path =
            source_path;

        artifact.record.importer_name =
            "oros.texture";

        artifact.record.importer_version = 3U;
        artifact.record.schema_version = 2U;

        artifact.record.source_hash =
            hash_text(
                source_path);

        artifact.record.cooked_hash =
            hash_bytes(
                std::span<const std::byte>{
                    artifact.payload_bytes
                });

        artifact.record.dependencies = {
            AssetId{
                0x4F524F53ULL,
                asset_sequence + 1000ULL
            }
        };

        artifact.cooker_name =
            "oros.texture";

        artifact.cooker_version = 4U;

        artifact.uncompressed_byte_count =
            static_cast<std::uint64_t>(
                artifact.payload_bytes.size());

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

    [[nodiscard]]
    bool write_bytes(
        const std::filesystem::path& path,
        const std::span<const std::byte> bytes)
    {
        std::ofstream stream{
            path,
            std::ios::binary |
                std::ios::trunc
        };

        if (!stream.is_open())
        {
            return false;
        }

        if (!bytes.empty())
        {
            stream.write(
                reinterpret_cast<const char*>(
                    bytes.data()),
                static_cast<std::streamsize>(
                    bytes.size()));
        }

        stream.close();

        return !stream.fail();
    }

    [[nodiscard]]
    bool corrupt_first_byte(
        const std::filesystem::path& path)
    {
        std::fstream stream{
            path,
            std::ios::binary |
                std::ios::in |
                std::ios::out
        };

        if (!stream.is_open())
        {
            return false;
        }

        char value{};

        stream.read(
            &value,
            1);

        if (!stream)
        {
            return false;
        }

        value =
            static_cast<char>(
                static_cast<unsigned char>(
                    value) ^
                0x01U);

        stream.seekp(
            0,
            std::ios::beg);

        stream.write(
            &value,
            1);

        stream.close();

        return !stream.fail();
    }

    [[nodiscard]]
    bool has_temporary_files(
        const std::filesystem::path& root)
    {
        std::error_code exists_error{};

        if (!std::filesystem::exists(
                root,
                exists_error) ||
            exists_error)
        {
            return false;
        }

        std::error_code iterator_error{};

        std::filesystem::
            recursive_directory_iterator iterator{
                root,
                iterator_error
            };

        const std::filesystem::
            recursive_directory_iterator end{};

        while (!iterator_error &&
               iterator != end)
        {
            const std::string filename =
                iterator->path().
                    filename().
                    string();

            if (filename.find(
                    ".tmp-") !=
                std::string::npos)
            {
                return true;
            }

            iterator.increment(
                iterator_error);
        }

        return false;
    }
}

int main()
{
    using namespace oros::assets;
    using oros::foundation::ErrorCode;

    TestState state{};
    TemporaryDirectory temporary_directory{};

    check(
        state,
        temporary_directory.is_valid(),
        "Temporary artifact store directory is created");

    if (!temporary_directory.is_valid())
    {
        std::cout
            << "\nCooked asset artifact store "
               "test summary: "
            << state.checks - state.failures
            << '/'
            << state.checks
            << " passed.\n";

        return 1;
    }

    CookedAssetArtifactStore invalid_store{
        std::filesystem::path{}
    };

    check(
        state,
        !invalid_store.is_valid(),
        "Store with an empty root is invalid");

    const ContentHash arbitrary_hash =
        hash_text(
            "arbitrary artifact address");

    check_failure(
        state,
        invalid_store.path_for(
            arbitrary_hash),
        ErrorCode::invalid_state,
        "Invalid store rejects path generation");

    check_failure(
        state,
        invalid_store.store(
            make_artifact(
                100ULL,
                "textures/invalid.png")),
        ErrorCode::invalid_state,
        "Invalid store rejects persistence");

    check_failure(
        state,
        invalid_store.load(
            arbitrary_hash),
        ErrorCode::invalid_state,
        "Invalid store rejects loading");

    check_failure(
        state,
        invalid_store.contains(
            arbitrary_hash),
        ErrorCode::invalid_state,
        "Invalid store rejects containment checks");

    check_failure(
        state,
        invalid_store.erase(
            arbitrary_hash),
        ErrorCode::invalid_state,
        "Invalid store rejects erasure");

    CookedAssetArtifactSerializationLimits
        invalid_limits{};

    invalid_limits.max_artifact_byte_count =
        cooked_asset_artifact_header_byte_count -
        1ULL;

    CookedAssetArtifactStore
        invalid_limits_store{
            temporary_directory.path() /
                "invalid-limits",
            invalid_limits
        };

    check(
        state,
        !invalid_limits_store.is_valid(),
        "Store with invalid serialization limits is invalid");

    const std::filesystem::path store_root =
        temporary_directory.path() /
        "store";

    CookedAssetArtifactStore store{
        store_root
    };

    check(
        state,
        store.is_valid(),
        "Store with a valid root and limits is valid");

    check(
        state,
        store.root_directory() ==
            store_root,
        "Store preserves its root directory");

    check(
        state,
        store.serialization_limits().
            is_valid(),
        "Store preserves valid serialization limits");

    CookedAssetArtifactStore moved_store{
        std::move(store)
    };

    check(
        state,
        moved_store.is_valid(),
        "Move construction preserves a valid store");

    check(
        state,
        !store.is_valid(),
        "Move construction invalidates the source store");

    CookedAssetArtifactStore
        assigned_store{
            temporary_directory.path() /
                "unused"
        };

    assigned_store =
        std::move(moved_store);

    check(
        state,
        assigned_store.is_valid(),
        "Move assignment preserves a valid store");

    check(
        state,
        !moved_store.is_valid(),
        "Move assignment invalidates the source store");

    assigned_store =
        std::move(assigned_store);

    check(
        state,
        assigned_store.is_valid(),
        "Self move assignment preserves the store");

    const auto arbitrary_path_result =
        assigned_store.path_for(
            arbitrary_hash);

    check(
        state,
        arbitrary_path_result.has_value(),
        "Store creates a path for a content hash");

    if (arbitrary_path_result.has_value())
    {
        const std::string hash_string =
            to_string(
                arbitrary_hash);

        const std::filesystem::path
            expected_path =
                store_root /
                hash_string.substr(
                    0U,
                    2U) /
                hash_string.substr(
                    2U,
                    2U) /
                (hash_string +
                    ".orosartifact");

        check(
            state,
            arbitrary_path_result.value() ==
                expected_path,
            "Artifact path uses deterministic two-level hash sharding");
    }
    else
    {
        check(
            state,
            false,
            "Artifact path uses deterministic two-level hash sharding");
    }

    const auto missing_contains =
        assigned_store.contains(
            arbitrary_hash);

    check(
        state,
        missing_contains.has_value() &&
            !missing_contains.value(),
        "Missing artifact is not contained");

    check_failure(
        state,
        assigned_store.load(
            arbitrary_hash),
        ErrorCode::not_found,
        "Loading a missing artifact reports not found");

    const auto erase_missing =
        assigned_store.erase(
            arbitrary_hash);

    check(
        state,
        erase_missing.has_value(),
        "Erasing a missing artifact is idempotent");

    const CookedAssetArtifact artifact_a =
        make_artifact(
            200ULL,
            "textures/terrain/rock.png");

    const auto serialized_a =
        serialize_cooked_asset_artifact(
            artifact_a);

    check(
        state,
        serialized_a.has_value(),
        "Artifact fixture serializes");

    ContentHash expected_hash_a{};

    if (serialized_a.has_value())
    {
        expected_hash_a =
            hash_bytes(
                std::span<const std::byte>{
                    serialized_a.value()
                });
    }

    const auto store_a_result =
        assigned_store.store(
            artifact_a);

    check(
        state,
        store_a_result.has_value(),
        "Valid artifact is persisted");

    check(
        state,
        store_a_result.has_value() &&
            store_a_result.value() ==
                expected_hash_a,
        "Store returns the hash of the complete serialized artifact");

    ContentHash artifact_hash_a{};

    if (store_a_result.has_value())
    {
        artifact_hash_a =
            store_a_result.value();
    }

    const auto path_a_result =
        assigned_store.path_for(
            artifact_hash_a);

    check(
        state,
        path_a_result.has_value(),
        "Stored artifact path is available");

    if (path_a_result.has_value())
    {
        std::error_code file_error{};

        check(
            state,
            std::filesystem::is_regular_file(
                path_a_result.value(),
                file_error) &&
                !file_error,
            "Stored artifact is published as a regular file");
    }
    else
    {
        check(
            state,
            false,
            "Stored artifact is published as a regular file");
    }

    check(
        state,
        !has_temporary_files(
            store_root),
        "Successful publication leaves no temporary files");

    const auto contains_a =
        assigned_store.contains(
            artifact_hash_a);

    check(
        state,
        contains_a.has_value() &&
            contains_a.value(),
        "Store contains the persisted artifact");

    const auto load_a_result =
        assigned_store.load(
            artifact_hash_a);

    check(
        state,
        load_a_result.has_value(),
        "Persisted artifact loads");

    check(
        state,
        load_a_result.has_value() &&
            artifacts_equal(
                load_a_result.value(),
                artifact_a),
        "Loaded artifact exactly matches the stored artifact");

    const auto duplicate_store_result =
        assigned_store.store(
            artifact_a);

    check(
        state,
        duplicate_store_result.has_value() &&
            duplicate_store_result.value() ==
                artifact_hash_a,
        "Storing identical content is idempotent");

    const CookedAssetArtifact artifact_b =
        make_artifact(
            201ULL,
            "textures/terrain/rock-copy.png");

    check(
        state,
        artifact_a.record.cooked_hash ==
            artifact_b.record.cooked_hash,
        "Metadata-distinct fixtures share the same cooked payload hash");

    const auto store_b_result =
        assigned_store.store(
            artifact_b);

    check(
        state,
        store_b_result.has_value(),
        "Metadata-distinct artifact is persisted");

    check(
        state,
        store_b_result.has_value() &&
            store_b_result.value() !=
                artifact_hash_a,
        "Metadata-distinct artifact receives a different artifact hash");

    if (store_b_result.has_value())
    {
        const auto path_b_result =
            assigned_store.path_for(
                store_b_result.value());

        check(
            state,
            path_a_result.has_value() &&
                path_b_result.has_value() &&
                path_a_result.value() !=
                    path_b_result.value(),
            "Metadata-distinct artifacts occupy different paths");
    }
    else
    {
        check(
            state,
            false,
            "Metadata-distinct artifacts occupy different paths");
    }

    check_failure(
        state,
        assigned_store.store(
            CookedAssetArtifact{}),
        ErrorCode::invalid_argument,
        "Store rejects an invalid artifact");

    bool corruption_written{};

    if (path_a_result.has_value())
    {
        corruption_written =
            corrupt_first_byte(
                path_a_result.value());
    }

    check(
        state,
        corruption_written,
        "Stored artifact fixture is corrupted on disk");

    check_failure(
        state,
        assigned_store.load(
            artifact_hash_a),
        ErrorCode::invalid_state,
        "Load rejects bytes that do not match their content address");

    check_failure(
        state,
        assigned_store.store(
            artifact_a),
        ErrorCode::invalid_state,
        "Store rejects different bytes already occupying an address");

    const auto erase_corrupted =
        assigned_store.erase(
            artifact_hash_a);

    check(
        state,
        erase_corrupted.has_value(),
        "Corrupted artifact can be erased");

    const auto contains_after_erase =
        assigned_store.contains(
            artifact_hash_a);

    check(
        state,
        contains_after_erase.has_value() &&
            !contains_after_erase.value(),
        "Erased artifact is no longer contained");

    check_failure(
        state,
        assigned_store.load(
            artifact_hash_a),
        ErrorCode::not_found,
        "Loading an erased artifact reports not found");

    const auto restored_store_result =
        assigned_store.store(
            artifact_a);

    check(
        state,
        restored_store_result.has_value() &&
            restored_store_result.value() ==
                artifact_hash_a,
        "Artifact can be restored after erasure");

    const auto restored_load_result =
        assigned_store.load(
            artifact_hash_a);

    check(
        state,
        restored_load_result.has_value() &&
            artifacts_equal(
                restored_load_result.value(),
                artifact_a),
        "Restored artifact loads correctly");

    const ContentHash directory_hash =
        hash_text(
            "directory instead of artifact");

    const auto directory_path_result =
        assigned_store.path_for(
            directory_hash);

    bool directory_created{};

    if (directory_path_result.has_value())
    {
        std::error_code directory_error{};

        std::filesystem::create_directories(
            directory_path_result.value(),
            directory_error);

        directory_created =
            !directory_error;
    }

    check(
        state,
        directory_created,
        "Directory fixture is created at an artifact path");

    check_failure(
        state,
        assigned_store.contains(
            directory_hash),
        ErrorCode::invalid_state,
        "Containment rejects a non-file artifact path");

    CookedAssetArtifactSerializationLimits
        tight_limits{};

    tight_limits.max_artifact_byte_count =
        cooked_asset_artifact_header_byte_count;

    tight_limits.max_payload_byte_count =
        cooked_asset_artifact_header_byte_count;

    CookedAssetArtifactStore tight_store{
        temporary_directory.path() /
            "tight-store",
        tight_limits
    };

    check(
        state,
        tight_store.is_valid(),
        "Tight but structurally valid store limits are accepted");

    check_failure(
        state,
        tight_store.store(
            artifact_a),
        ErrorCode::invalid_argument,
        "Store propagates serialization size-limit failures");

    const ContentHash oversized_hash =
        hash_text(
            "oversized artifact file");

    const auto oversized_path_result =
        tight_store.path_for(
            oversized_hash);

    bool oversized_file_written{};

    if (oversized_path_result.has_value())
    {
        std::error_code directory_error{};

        std::filesystem::create_directories(
            oversized_path_result.value().
                parent_path(),
            directory_error);

        if (!directory_error)
        {
            const std::vector<std::byte>
                oversized_bytes(
                    static_cast<std::size_t>(
                        cooked_asset_artifact_header_byte_count +
                        1ULL),
                    std::byte{0x00});

            oversized_file_written =
                write_bytes(
                    oversized_path_result.value(),
                    std::span<const std::byte>{
                        oversized_bytes
                    });
        }
    }

    check(
        state,
        oversized_file_written,
        "Oversized artifact fixture is written");

    check_failure(
        state,
        tight_store.load(
            oversized_hash),
        ErrorCode::invalid_argument,
        "Load rejects an artifact file above the configured limit");

    std::cout
        << "\nCooked asset artifact store "
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