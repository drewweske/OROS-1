#include "oros/assets/asset_catalog.hpp"
#include "oros/assets/asset_cook_pipeline.hpp"
#include "oros/assets/asset_cooker_registry.hpp"
#include "oros/assets/asset_hot_reload_pipeline.hpp"
#include "oros/assets/asset_import_pipeline.hpp"
#include "oros/assets/asset_importer_registry.hpp"
#include "oros/assets/asset_load_pipeline.hpp"
#include "oros/assets/binary_asset_cooker.hpp"
#include "oros/assets/binary_asset_importer.hpp"
#include "oros/assets/content_hash.hpp"
#include "oros/assets/cooked_asset_artifact_pipeline.hpp"
#include "oros/assets/cooked_asset_artifact_store.hpp"
#include "oros/assets/cooked_asset_cache.hpp"
#include "oros/assets/run_length_compression_codec.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
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
                (
                    "oros-asset-lifecycle-tests-" +
                    std::to_string(timestamp) +
                    "-" +
                    std::to_string(suffix)
                );

            std::error_code create_error{};

            std::filesystem::create_directories(
                path_,
                create_error);

            valid_ =
                !create_error;
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
    bool bytes_equal(
        const std::vector<std::byte>& left,
        const std::span<const std::byte>
            right) noexcept
    {
        if (left.size() !=
            right.size())
        {
            return false;
        }

        for (std::size_t index = 0U;
             index < left.size();
             ++index)
        {
            if (left[index] !=
                right[index])
            {
                return false;
            }
        }

        return true;
    }

    int finish(
        const TestState& state)
    {
        std::cout
            << "\nAsset lifecycle test summary: "
            << state.checks - state.failures
            << '/'
            << state.checks
            << " passed.\n";

        return
            state.failures == 0
                ? 0
                : 1;
    }
}

int main()
{
    using namespace oros::assets;
    using namespace oros::foundation;

    TestState state{};

    AssetImporterRegistry
        importer_registry{};

    AssetCookerRegistry
        cooker_registry{};

    const Status importer_registration =
        importer_registry.register_importer(
            std::make_unique<
                BinaryAssetImporter>());

    check(
        state,
        importer_registration.has_value(),
        "Binary importer registers for lifecycle testing");

    const Status cooker_registration =
        cooker_registry.register_cooker(
            std::make_unique<
                BinaryAssetCooker>());

    check(
        state,
        cooker_registration.has_value(),
        "Binary cooker registers for lifecycle testing");

    if (!importer_registration.has_value() ||
        !cooker_registration.has_value())
    {
        return finish(state);
    }

    AssetImportPipeline import_pipeline{
        importer_registry
    };

    AssetCookPipeline cook_pipeline{
        cooker_registry
    };

    RunLengthCompressionCodec
        compression_codec{};

    TemporaryDirectory
        temporary_directory{};

    check(
        state,
        temporary_directory.is_valid(),
        "Lifecycle artifact directory is created");

    if (!temporary_directory.is_valid())
    {
        return finish(state);
    }

    CookedAssetArtifactStore artifact_store{
        temporary_directory.path()
    };

    check(
        state,
        artifact_store.is_valid(),
        "Persistent artifact store is valid");

    AssetCatalog catalog{};
    CookedAssetCache cache{};

    ResourceTable<LoadedAsset>
        resources{};

    AssetLoadPipeline loader{
        cache,
        resources
    };

    AssetHotReloadPipeline hot_reload{
        cache,
        loader
    };

    const AssetId asset{
        0x4F524F53ULL,
        800ULL
    };

    const std::string source_path{
        "assets/lifecycle/world-data.orosbin"
    };

    const std::array<std::byte, 32U>
        original_source{
            std::byte{0x11},
            std::byte{0x11},
            std::byte{0x11},
            std::byte{0x11},
            std::byte{0x11},
            std::byte{0x11},
            std::byte{0x11},
            std::byte{0x11},
            std::byte{0x11},
            std::byte{0x11},
            std::byte{0x11},
            std::byte{0x11},
            std::byte{0x11},
            std::byte{0x11},
            std::byte{0x11},
            std::byte{0x11},
            std::byte{0x44},
            std::byte{0x44},
            std::byte{0x44},
            std::byte{0x44},
            std::byte{0x44},
            std::byte{0x44},
            std::byte{0x44},
            std::byte{0x44},
            std::byte{0x44},
            std::byte{0x44},
            std::byte{0x44},
            std::byte{0x44},
            std::byte{0x44},
            std::byte{0x44},
            std::byte{0x44},
            std::byte{0x44}
        };

    const ImportRequest original_request{
        asset,
        source_path,
        std::span<const std::byte>{
            original_source
        }
    };

    check(
        state,
        original_request.is_valid(),
        "Original lifecycle import request is valid");

    Result<ImportedAsset>
        original_import =
            import_pipeline.import(
                original_request);

    check(
        state,
        original_import.has_value(),
        "Original source imports successfully");

    if (!original_import.has_value())
    {
        return finish(state);
    }

    check(
        state,
        original_import.value().is_valid(),
        "Original import produces valid metadata");

    check(
        state,
        bytes_equal(
            original_import.value().
                intermediate_bytes,
            std::span<const std::byte>{
                original_source
            }),
        "Original import preserves every source byte");

    Result<CookedAsset>
        original_cook =
            cook_pipeline.cook(
                original_import.value());

    check(
        state,
        original_cook.has_value(),
        "Original imported asset cooks successfully");

    if (!original_cook.has_value())
    {
        return finish(state);
    }

    check(
        state,
        original_cook.value().is_valid(),
        "Original cook produces valid metadata");

    check(
        state,
        bytes_equal(
            original_cook.value().
                cooked_bytes,
            std::span<const std::byte>{
                original_source
            }),
        "Original cook preserves every binary byte");

    const ContentHash
        original_source_hash =
            hash_bytes(
                std::span<const std::byte>{
                    original_source
                });

    check(
        state,
        original_cook.value().
            record.source_hash ==
                original_source_hash,
        "Original cooked record preserves the source hash");

    check(
        state,
        original_cook.value().
            record.cooked_hash ==
                original_source_hash,
        "Pass-through cooking records the matching cooked hash");

    const CookedAssetArtifactBuildRequest
        original_build_request{
            &original_cook.value(),
            &compression_codec,
            CookedAssetArtifactCompressionPolicy::
                required
        };

    check(
        state,
        original_build_request.is_valid(),
        "Original artifact build request is valid");

    Result<CookedAssetArtifact>
        original_artifact =
            build_cooked_asset_artifact(
                original_build_request);

    check(
        state,
        original_artifact.has_value(),
        "Original cooked asset builds an artifact");

    if (!original_artifact.has_value())
    {
        return finish(state);
    }

    check(
        state,
        original_artifact.value().
            is_valid(),
        "Original artifact is valid");

    check(
        state,
        original_artifact.value().
            is_compressed(),
        "Original artifact uses the compression codec");

    check(
        state,
        original_artifact.value().
            encoded_byte_count() <
                original_artifact.value().
                    uncompressed_byte_count,
        "Original artifact payload is compressed smaller");

    Result<ContentHash>
        original_artifact_hash =
            artifact_store.store(
                original_artifact.value());

    check(
        state,
        original_artifact_hash.has_value(),
        "Original artifact persists successfully");

    if (!original_artifact_hash.has_value())
    {
        return finish(state);
    }

    Result<bool>
        original_contains =
            artifact_store.contains(
                original_artifact_hash.value());

    check(
        state,
        original_contains.has_value() &&
            original_contains.value(),
        "Artifact store contains the original artifact");

    Result<std::filesystem::path>
        original_artifact_path =
            artifact_store.path_for(
                original_artifact_hash.value());

    check(
        state,
        original_artifact_path.has_value(),
        "Original artifact has a deterministic store path");

    check(
        state,
        original_artifact_path.has_value() &&
            std::filesystem::is_regular_file(
                original_artifact_path.value()),
        "Original artifact exists as a persistent file");

    Result<CookedAssetArtifact>
        original_loaded_artifact =
            artifact_store.load(
                original_artifact_hash.value());

    check(
        state,
        original_loaded_artifact.has_value(),
        "Original artifact loads from persistent storage");

    if (!original_loaded_artifact.has_value())
    {
        return finish(state);
    }

    check(
        state,
        original_loaded_artifact.value().
            record ==
                original_artifact.value().
                    record,
        "Stored artifact preserves the complete asset record");

    check(
        state,
        original_loaded_artifact.value().
            payload_bytes ==
                original_artifact.value().
                    payload_bytes,
        "Stored artifact preserves the encoded payload");

    const CookedAssetArtifactMaterializationRequest
        original_materialization_request{
            &original_loaded_artifact.value(),
            &compression_codec
        };

    check(
        state,
        original_materialization_request.
            is_valid(),
        "Original materialization request is valid");

    Result<CookedAsset>
        original_materialized =
            materialize_cooked_asset_artifact(
                original_materialization_request);

    check(
        state,
        original_materialized.has_value(),
        "Original artifact materializes successfully");

    if (!original_materialized.has_value())
    {
        return finish(state);
    }

    check(
        state,
        original_materialized.value().
            record ==
                original_cook.value().
                    record,
        "Original materialization restores complete metadata");

    check(
        state,
        bytes_equal(
            original_materialized.value().
                cooked_bytes,
            std::span<const std::byte>{
                original_source
            }),
        "Original materialization restores every cooked byte");

    Result<bool>
        original_cache_store =
            cache.store(
                original_materialized.value());

    check(
        state,
        original_cache_store.has_value() &&
            original_cache_store.value(),
        "Original cooked payload enters the runtime cache");

    const Status original_catalog_insert =
        catalog.insert(
            original_materialized.value().
                record);

    check(
        state,
        original_catalog_insert.has_value(),
        "Original record enters the asset catalog");

    check(
        state,
        catalog.size() == 1U,
        "Asset catalog contains one lifecycle record");

    Result<ResourceHandle>
        original_load =
            loader.load(
                original_materialized.value().
                    record);

    check(
        state,
        original_load.has_value(),
        "Original cached asset loads into runtime storage");

    if (!original_load.has_value())
    {
        return finish(state);
    }

    const ResourceHandle stable_handle =
        original_load.value();

    check(
        state,
        stable_handle.is_valid(),
        "Original runtime load produces a valid handle");

    check(
        state,
        loader.handle_for(asset) ==
            stable_handle,
        "Stable asset identity resolves to its runtime handle");

    const LoadedAsset*
        original_runtime =
            loader.find(
                stable_handle);

    check(
        state,
        original_runtime != nullptr,
        "Original runtime resource is discoverable");

    check(
        state,
        original_runtime != nullptr &&
            bytes_equal(
                original_runtime->
                    runtime_bytes,
                std::span<const std::byte>{
                    original_source
                }),
        "Original runtime resource contains every source byte");

    const std::array<std::byte, 32U>
        revised_source{
            std::byte{0x22},
            std::byte{0x22},
            std::byte{0x22},
            std::byte{0x22},
            std::byte{0x22},
            std::byte{0x22},
            std::byte{0x22},
            std::byte{0x22},
            std::byte{0x22},
            std::byte{0x22},
            std::byte{0x22},
            std::byte{0x22},
            std::byte{0x22},
            std::byte{0x22},
            std::byte{0x22},
            std::byte{0x22},
            std::byte{0x88},
            std::byte{0x88},
            std::byte{0x88},
            std::byte{0x88},
            std::byte{0x88},
            std::byte{0x88},
            std::byte{0x88},
            std::byte{0x88},
            std::byte{0x88},
            std::byte{0x88},
            std::byte{0x88},
            std::byte{0x88},
            std::byte{0x88},
            std::byte{0x88},
            std::byte{0x88},
            std::byte{0x88}
        };

    const ImportRequest revised_request{
        asset,
        source_path,
        std::span<const std::byte>{
            revised_source
        }
    };

    Result<ImportedAsset>
        revised_import =
            import_pipeline.import(
                revised_request);

    check(
        state,
        revised_import.has_value(),
        "Revised source imports successfully");

    if (!revised_import.has_value())
    {
        return finish(state);
    }

    Result<CookedAsset>
        revised_cook =
            cook_pipeline.cook(
                revised_import.value());

    check(
        state,
        revised_cook.has_value(),
        "Revised imported asset cooks successfully");

    if (!revised_cook.has_value())
    {
        return finish(state);
    }

    const ContentHash revised_source_hash =
        hash_bytes(
            std::span<const std::byte>{
                revised_source
            });

    check(
        state,
        revised_cook.value().
            record.source_hash ==
                revised_source_hash,
        "Revised cooked record receives the new source hash");

    check(
        state,
        revised_cook.value().
            record.cooked_hash ==
                revised_source_hash,
        "Revised cooked record receives the new cooked hash");

    check(
        state,
        revised_cook.value().
            record.cooked_hash !=
                original_cook.value().
                    record.cooked_hash,
        "Revised cooking produces a distinct content revision");

    const CookedAssetArtifactBuildRequest
        revised_build_request{
            &revised_cook.value(),
            &compression_codec,
            CookedAssetArtifactCompressionPolicy::
                required
        };

    Result<CookedAssetArtifact>
        revised_artifact =
            build_cooked_asset_artifact(
                revised_build_request);

    check(
        state,
        revised_artifact.has_value(),
        "Revised cooked asset builds an artifact");

    if (!revised_artifact.has_value())
    {
        return finish(state);
    }

    check(
        state,
        revised_artifact.value().
            is_compressed(),
        "Revised artifact uses the compression codec");

    Result<ContentHash>
        revised_artifact_hash =
            artifact_store.store(
                revised_artifact.value());

    check(
        state,
        revised_artifact_hash.has_value(),
        "Revised artifact persists successfully");

    if (!revised_artifact_hash.has_value())
    {
        return finish(state);
    }

    check(
        state,
        revised_artifact_hash.value() !=
            original_artifact_hash.value(),
        "Revised artifact receives a distinct artifact hash");

    Result<bool>
        revised_contains =
            artifact_store.contains(
                revised_artifact_hash.value());

    check(
        state,
        revised_contains.has_value() &&
            revised_contains.value(),
        "Artifact store contains the revised artifact");

    check(
        state,
        artifact_store.contains(
            original_artifact_hash.value()).
                has_value() &&
            artifact_store.contains(
                original_artifact_hash.value()).
                value(),
        "Artifact store retains the original immutable artifact");

    Result<CookedAssetArtifact>
        revised_loaded_artifact =
            artifact_store.load(
                revised_artifact_hash.value());

    check(
        state,
        revised_loaded_artifact.has_value(),
        "Revised artifact loads from persistent storage");

    if (!revised_loaded_artifact.has_value())
    {
        return finish(state);
    }

    const CookedAssetArtifactMaterializationRequest
        revised_materialization_request{
            &revised_loaded_artifact.value(),
            &compression_codec
        };

    Result<CookedAsset>
        revised_materialized =
            materialize_cooked_asset_artifact(
                revised_materialization_request);

    check(
        state,
        revised_materialized.has_value(),
        "Revised artifact materializes successfully");

    if (!revised_materialized.has_value())
    {
        return finish(state);
    }

    check(
        state,
        bytes_equal(
            revised_materialized.value().
                cooked_bytes,
            std::span<const std::byte>{
                revised_source
            }),
        "Revised materialization restores every cooked byte");

    Result<bool>
        revised_cache_store =
            cache.store(
                revised_materialized.value());

    check(
        state,
        revised_cache_store.has_value() &&
            revised_cache_store.value(),
        "Revised cooked payload enters the runtime cache");

    const Status revised_catalog_replace =
        catalog.replace(
            revised_materialized.value().
                record);

    check(
        state,
        revised_catalog_replace.has_value(),
        "Asset catalog accepts the revised record");

    const AssetRecord*
        catalog_record =
            catalog.find(
                asset);

    check(
        state,
        catalog_record != nullptr &&
            catalog_record->cooked_hash ==
                revised_source_hash,
        "Asset catalog resolves the revised cooked hash");

    Result<AssetHotReloadResult>
        reload_result =
            hot_reload.reload(
                revised_materialized.value().
                    record);

    check(
        state,
        reload_result.has_value(),
        "Runtime hot reload accepts the revised asset");

    if (!reload_result.has_value())
    {
        return finish(state);
    }

    check(
        state,
        reload_result.value().is_valid(),
        "Runtime hot reload produces a valid result");

    check(
        state,
        reload_result.value().handle ==
            stable_handle,
        "Runtime hot reload preserves the stable handle");

    check(
        state,
        reload_result.value().
            runtime_replaced,
        "Runtime hot reload reports byte replacement");

    check(
        state,
        reload_result.value().
            previous_hash ==
                original_source_hash,
        "Runtime hot reload reports the original cooked hash");

    check(
        state,
        reload_result.value().
            current_hash ==
                revised_source_hash,
        "Runtime hot reload reports the revised cooked hash");

    check(
        state,
        loader.loaded_count() == 1U,
        "Runtime hot reload preserves the loaded count");

    check(
        state,
        resources.slot_count() == 1U,
        "Runtime hot reload allocates no additional slot");

    const LoadedAsset*
        revised_runtime =
            loader.find(
                stable_handle);

    check(
        state,
        revised_runtime != nullptr,
        "Revised runtime resource remains discoverable");

    check(
        state,
        revised_runtime != nullptr &&
            revised_runtime->record ==
                revised_materialized.value().
                    record,
        "Revised runtime resource receives complete metadata");

    check(
        state,
        revised_runtime != nullptr &&
            bytes_equal(
                revised_runtime->
                    runtime_bytes,
                std::span<const std::byte>{
                    revised_source
                }),
        "Revised runtime resource contains every new byte");

    check(
        state,
        loader.handle_for(asset) ==
            stable_handle,
        "Revised asset identity still resolves to the stable handle");

    const Status unload_status =
        loader.unload(
            stable_handle);

    check(
        state,
        unload_status.has_value(),
        "Lifecycle runtime resource unloads successfully");

    check(
        state,
        loader.empty(),
        "Runtime loader is empty after lifecycle cleanup");

    check(
        state,
        !loader.contains(
            stable_handle),
        "Unloaded lifecycle handle becomes stale");

    return finish(state);
}