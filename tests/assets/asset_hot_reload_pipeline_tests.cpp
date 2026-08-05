#include "oros/assets/asset_hot_reload_pipeline.hpp"

#include "oros/assets/content_hash.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
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

    [[nodiscard]]
    oros::assets::CookedAsset
    make_cooked_asset(
        const oros::assets::AssetId id,
        std::string source_path,
        std::vector<std::byte> cooked_bytes,
        const std::uint32_t importer_version =
            6U)
    {
        using namespace oros::assets;

        CookedAsset asset{};

        asset.record.id =
            id;

        asset.record.source_path =
            std::move(
                source_path);

        asset.record.importer_name =
            "oros.hot_reload_importer";

        asset.record.importer_version =
            importer_version;

        asset.record.schema_version =
            4U;

        asset.record.source_hash =
            hash_text(
                asset.record.source_path);

        asset.record.cooked_hash =
            hash_bytes(
                std::span<const std::byte>{
                    cooked_bytes
                });

        asset.record.dependencies = {
            AssetId{
                70ULL,
                1ULL
            }
        };

        asset.cooker_name =
            "oros.hot_reload_cooker";

        asset.cooker_version =
            9U;

        asset.cooked_bytes =
            std::move(
                cooked_bytes);

        return asset;
    }

    static_assert(
        !std::is_copy_constructible_v<
            oros::assets::
                AssetHotReloadPipeline>);

    static_assert(
        !std::is_copy_assignable_v<
            oros::assets::
                AssetHotReloadPipeline>);

    static_assert(
        !std::is_move_constructible_v<
            oros::assets::
                AssetHotReloadPipeline>);

    static_assert(
        !std::is_move_assignable_v<
            oros::assets::
                AssetHotReloadPipeline>);
}

int main()
{
    using namespace oros::assets;
    using namespace oros::foundation;

    TestState state{};

    const AssetHotReloadResult
        default_result{};

    check(
        state,
        !default_result.is_valid(),
        "Default hot-reload result is invalid");

    check(
        state,
        !default_result.handle.is_valid(),
        "Default hot-reload result has an invalid handle");

    check(
        state,
        !default_result.runtime_replaced,
        "Default hot-reload result reports no replacement");

    CookedAssetCache cache{};

    ResourceTable<LoadedAsset>
        resources{};

    AssetLoadPipeline loader{
        cache,
        resources
    };

    AssetHotReloadPipeline
        hot_reload{
            cache,
            loader
        };

    const Result<AssetHotReloadResult>
        invalid_record_result =
            hot_reload.reload(
                AssetRecord{});

    check(
        state,
        !invalid_record_result.has_value(),
        "Hot reload rejects a default asset record");

    check(
        state,
        !invalid_record_result.has_value() &&
            invalid_record_result.error().code ==
                ErrorCode::invalid_argument,
        "Default reload record reports invalid_argument");

    CookedAsset original_asset =
        make_cooked_asset(
            AssetId{
                30ULL,
                1ULL
            },
            "runtime/hot-reload/original.asset",
            {
                std::byte{0x10},
                std::byte{0x20},
                std::byte{0x30},
                std::byte{0x40}
            });

    check(
        state,
        original_asset.is_valid(),
        "Original cooked asset is valid");

    AssetRecord uncooked_record =
        original_asset.record;

    uncooked_record.cooked_hash.reset();

    check(
        state,
        uncooked_record.is_valid(),
        "Uncooked reload record remains structurally valid");

    check(
        state,
        !uncooked_record.is_cooked(),
        "Uncooked reload record reports uncooked state");

    const Result<AssetHotReloadResult>
        uncooked_result =
            hot_reload.reload(
                uncooked_record);

    check(
        state,
        !uncooked_result.has_value(),
        "Hot reload rejects uncooked metadata");

    check(
        state,
        !uncooked_result.has_value() &&
            uncooked_result.error().code ==
                ErrorCode::invalid_argument,
        "Uncooked reload metadata reports invalid_argument");

    const Result<AssetHotReloadResult>
        unloaded_result =
            hot_reload.reload(
                original_asset.record);

    check(
        state,
        !unloaded_result.has_value(),
        "Hot reload rejects an asset that is not loaded");

    check(
        state,
        !unloaded_result.has_value() &&
            unloaded_result.error().code ==
                ErrorCode::not_found,
        "Unloaded asset reload reports not_found");

    const Result<bool>
        original_cache_store =
            cache.store(
                original_asset);

    check(
        state,
        original_cache_store.has_value() &&
            original_cache_store.value(),
        "Original cooked payload enters the cache");

    const Result<ResourceHandle>
        original_load =
            loader.load(
                original_asset.record);

    check(
        state,
        original_load.has_value(),
        "Original asset loads successfully");

    ResourceHandle original_handle{};

    if (original_load.has_value())
    {
        original_handle =
            original_load.value();
    }

    check(
        state,
        original_handle.is_valid(),
        "Original load produces a valid handle");

    check(
        state,
        loader.loaded_count() == 1U,
        "Original load creates one runtime asset");

    LoadedAsset* original_loaded =
        loader.find(
            original_handle);

    check(
        state,
        original_loaded != nullptr,
        "Original loaded asset lookup succeeds");

    check(
        state,
        original_loaded != nullptr &&
            original_loaded->record ==
                original_asset.record,
        "Original load preserves complete metadata");

    check(
        state,
        original_loaded != nullptr &&
            original_loaded->runtime_bytes ==
                original_asset.cooked_bytes,
        "Original load preserves every runtime byte");

    const ContentHash original_hash =
        original_asset.record.
            cooked_hash.value();

    const Status original_cache_erase =
        cache.erase(
            original_hash);

    check(
        state,
        original_cache_erase.has_value(),
        "Original cached payload can be erased");

    check(
        state,
        cache.empty(),
        "Cache is empty before same-revision reload");

    const Result<AssetHotReloadResult>
        same_revision_result =
            hot_reload.reload(
                original_asset.record);

    check(
        state,
        same_revision_result.has_value(),
        "Hot reload accepts an unchanged revision");

    check(
        state,
        same_revision_result.has_value() &&
            same_revision_result.value().
                is_valid(),
        "Unchanged revision produces a valid result");

    check(
        state,
        same_revision_result.has_value() &&
            same_revision_result.value().
                handle ==
                original_handle,
        "Unchanged revision preserves the resource handle");

    check(
        state,
        same_revision_result.has_value() &&
            same_revision_result.value().
                previous_hash ==
                original_hash,
        "Unchanged revision reports the previous hash");

    check(
        state,
        same_revision_result.has_value() &&
            same_revision_result.value().
                current_hash ==
                original_hash,
        "Unchanged revision reports the current hash");

    check(
        state,
        same_revision_result.has_value() &&
            !same_revision_result.value().
                runtime_replaced,
        "Unchanged revision reports no runtime replacement");

    check(
        state,
        loader.loaded_count() == 1U,
        "Unchanged revision preserves the loaded count");

    original_loaded =
        loader.find(
            original_handle);

    check(
        state,
        original_loaded != nullptr &&
            original_loaded->runtime_bytes ==
                original_asset.cooked_bytes,
        "Unchanged revision preserves runtime bytes");

    AssetRecord metadata_revision =
        original_asset.record;

    metadata_revision.importer_version =
        7U;

    metadata_revision.source_hash =
        hash_text(
            "updated source metadata");

    metadata_revision.dependencies = {
        AssetId{
            70ULL,
            2ULL
        },
        AssetId{
            70ULL,
            3ULL
        }
    };

    check(
        state,
        metadata_revision.is_valid(),
        "Metadata-only revision is valid");

    check(
        state,
        metadata_revision.cooked_hash ==
            original_asset.record.
                cooked_hash,
        "Metadata-only revision preserves its cooked hash");

    const Result<AssetHotReloadResult>
        metadata_result =
            hot_reload.reload(
                metadata_revision);

    check(
        state,
        metadata_result.has_value(),
        "Hot reload accepts a metadata-only revision");

    check(
        state,
        metadata_result.has_value() &&
            metadata_result.value().
                handle ==
                original_handle,
        "Metadata-only revision preserves the handle");

    check(
        state,
        metadata_result.has_value() &&
            !metadata_result.value().
                runtime_replaced,
        "Metadata-only revision reports no byte replacement");

    original_loaded =
        loader.find(
            original_handle);

    check(
        state,
        original_loaded != nullptr &&
            original_loaded->record ==
                metadata_revision,
        "Metadata-only reload updates the runtime record");

    check(
        state,
        original_loaded != nullptr &&
            original_loaded->runtime_bytes ==
                original_asset.cooked_bytes,
        "Metadata-only reload preserves runtime bytes");

    CookedAsset revised_asset =
        make_cooked_asset(
            original_asset.record.id,
            original_asset.record.source_path,
            {
                std::byte{0xAA},
                std::byte{0xBB},
                std::byte{0xCC}
            },
            8U);

    revised_asset.record.dependencies = {
        AssetId{
            70ULL,
            4ULL
        }
    };

    check(
        state,
        revised_asset.is_valid(),
        "Replacement cooked asset is valid");

    check(
        state,
        revised_asset.record.id ==
            original_asset.record.id,
        "Replacement preserves the stable asset identity");

    check(
        state,
        revised_asset.record.cooked_hash !=
            original_asset.record.cooked_hash,
        "Replacement has a different cooked hash");

    const LoadedAsset
        before_missing_replacement =
            *original_loaded;

    const Result<AssetHotReloadResult>
        missing_replacement_result =
            hot_reload.reload(
                revised_asset.record);

    check(
        state,
        !missing_replacement_result.
            has_value(),
        "Hot reload rejects a missing replacement payload");

    check(
        state,
        !missing_replacement_result.
                has_value() &&
            missing_replacement_result.
                error().code ==
                ErrorCode::not_found,
        "Missing replacement payload reports not_found");

    original_loaded =
        loader.find(
            original_handle);

    check(
        state,
        original_loaded != nullptr &&
            original_loaded->record ==
                before_missing_replacement.
                    record,
        "Missing replacement preserves existing metadata");

    check(
        state,
        original_loaded != nullptr &&
            original_loaded->runtime_bytes ==
                before_missing_replacement.
                    runtime_bytes,
        "Missing replacement preserves existing runtime bytes");

    const Result<bool>
        revised_cache_store =
            cache.store(
                revised_asset);

    check(
        state,
        revised_cache_store.has_value() &&
            revised_cache_store.value(),
        "Replacement payload enters the cache");

    const ContentHash revised_hash =
        revised_asset.record.
            cooked_hash.value();

    const Result<AssetHotReloadResult>
        revised_result =
            hot_reload.reload(
                revised_asset.record);

    check(
        state,
        revised_result.has_value(),
        "Hot reload accepts a cached replacement revision");

    check(
        state,
        revised_result.has_value() &&
            revised_result.value().
                is_valid(),
        "Replacement revision produces a valid result");

    check(
        state,
        revised_result.has_value() &&
            revised_result.value().
                handle ==
                original_handle,
        "Replacement revision preserves the resource handle");

    check(
        state,
        revised_result.has_value() &&
            revised_result.value().
                previous_hash ==
                original_hash,
        "Replacement reports the previous cooked hash");

    check(
        state,
        revised_result.has_value() &&
            revised_result.value().
                current_hash ==
                revised_hash,
        "Replacement reports the current cooked hash");

    check(
        state,
        revised_result.has_value() &&
            revised_result.value().
                runtime_replaced,
        "Different cooked hash reports runtime replacement");

    check(
        state,
        loader.handle_for(
            revised_asset.record.id) ==
            original_handle,
        "Asset identity still resolves to the original handle");

    check(
        state,
        loader.loaded_count() == 1U,
        "Replacement preserves the loaded count");

    check(
        state,
        resources.slot_count() == 1U,
        "Replacement allocates no additional resource slot");

    original_loaded =
        loader.find(
            original_handle);

    check(
        state,
        original_loaded != nullptr &&
            original_loaded->record ==
                revised_asset.record,
        "Replacement updates complete runtime metadata");

    check(
        state,
        original_loaded != nullptr &&
            original_loaded->runtime_bytes ==
                revised_asset.cooked_bytes,
        "Replacement updates every runtime byte");

    check(
        state,
        original_loaded != nullptr &&
            hash_bytes(
                std::span<const std::byte>{
                    original_loaded->
                        runtime_bytes
                }) ==
                revised_hash,
        "Replacement runtime bytes match the new cooked hash");

    const Result<AssetHotReloadResult>
        repeated_revised_result =
            hot_reload.reload(
                revised_asset.record);

    check(
        state,
        repeated_revised_result.has_value(),
        "Hot reload accepts the current replacement revision");

    check(
        state,
        repeated_revised_result.has_value() &&
            repeated_revised_result.value().
                handle ==
                original_handle,
        "Repeated replacement preserves the handle");

    check(
        state,
        repeated_revised_result.has_value() &&
            repeated_revised_result.value().
                previous_hash ==
                revised_hash &&
            repeated_revised_result.value().
                current_hash ==
                revised_hash,
        "Repeated replacement reports identical hashes");

    check(
        state,
        repeated_revised_result.has_value() &&
            !repeated_revised_result.value().
                runtime_replaced,
        "Repeated replacement reports no byte replacement");

    if (original_loaded != nullptr)
    {
        original_loaded->runtime_bytes[0] =
            std::byte{0xFE};
    }

    const LoadedAsset
        corrupted_snapshot =
            *original_loaded;

    const Result<AssetHotReloadResult>
        corrupted_runtime_result =
            hot_reload.reload(
                revised_asset.record);

    check(
        state,
        !corrupted_runtime_result.has_value(),
        "Hot reload detects corrupted existing runtime data");

    check(
        state,
        !corrupted_runtime_result.has_value() &&
            corrupted_runtime_result.error().code ==
                ErrorCode::invalid_state,
        "Existing runtime corruption reports invalid_state");

    original_loaded =
        loader.find(
            original_handle);

    check(
        state,
        original_loaded != nullptr &&
            original_loaded->runtime_bytes ==
                corrupted_snapshot.runtime_bytes,
        "Corruption rejection does not silently replace data");

    if (original_loaded != nullptr)
    {
        original_loaded->runtime_bytes =
            revised_asset.cooked_bytes;
    }

    const Result<AssetHotReloadResult>
        restored_runtime_result =
            hot_reload.reload(
                revised_asset.record);

    check(
        state,
        restored_runtime_result.has_value(),
        "Restored runtime data permits hot reload");

    check(
        state,
        restored_runtime_result.has_value() &&
            !restored_runtime_result.value().
                runtime_replaced,
        "Restored current revision needs no replacement");

    CookedAsset empty_revision =
        make_cooked_asset(
            original_asset.record.id,
            original_asset.record.source_path,
            {},
            9U);

    check(
        state,
        empty_revision.is_valid(),
        "Empty replacement revision is valid");

    check(
        state,
        !empty_revision.has_cooked_data(),
        "Empty replacement revision has no cooked bytes");

    const Result<bool>
        empty_cache_store =
            cache.store(
                empty_revision);

    check(
        state,
        empty_cache_store.has_value() &&
            empty_cache_store.value(),
        "Empty replacement payload enters the cache");

    const ContentHash empty_hash =
        empty_revision.record.
            cooked_hash.value();

    const Result<AssetHotReloadResult>
        empty_reload_result =
            hot_reload.reload(
                empty_revision.record);

    check(
        state,
        empty_reload_result.has_value(),
        "Hot reload accepts an empty replacement payload");

    check(
        state,
        empty_reload_result.has_value() &&
            empty_reload_result.value().
                handle ==
                original_handle,
        "Empty replacement preserves the handle");

    check(
        state,
        empty_reload_result.has_value() &&
            empty_reload_result.value().
                previous_hash ==
                revised_hash &&
            empty_reload_result.value().
                current_hash ==
                empty_hash,
        "Empty replacement reports both revision hashes");

    check(
        state,
        empty_reload_result.has_value() &&
            empty_reload_result.value().
                runtime_replaced,
        "Empty replacement reports runtime replacement");

    original_loaded =
        loader.find(
            original_handle);

    check(
        state,
        original_loaded != nullptr &&
            original_loaded->is_valid(),
        "Empty replacement leaves a valid runtime asset");

    check(
        state,
        original_loaded != nullptr &&
            !original_loaded->
                has_runtime_data(),
        "Empty replacement reports no runtime data");

    check(
        state,
        original_loaded != nullptr &&
            original_loaded->
                runtime_bytes.empty(),
        "Empty replacement stores an empty runtime payload");

    check(
        state,
        original_loaded != nullptr &&
            original_loaded->record ==
                empty_revision.record,
        "Empty replacement updates runtime metadata");

    CookedAsset final_revision =
        make_cooked_asset(
            original_asset.record.id,
            original_asset.record.source_path,
            {
                std::byte{0x01},
                std::byte{0x02}
            },
            10U);

    const Result<bool>
        final_cache_store =
            cache.store(
                final_revision);

    check(
        state,
        final_cache_store.has_value() &&
            final_cache_store.value(),
        "Final non-empty payload enters the cache");

    const Result<AssetHotReloadResult>
        final_reload_result =
            hot_reload.reload(
                final_revision.record);

    check(
        state,
        final_reload_result.has_value(),
        "Hot reload replaces an empty runtime payload");

    check(
        state,
        final_reload_result.has_value() &&
            final_reload_result.value().
                handle ==
                original_handle,
        "Final replacement preserves the original handle");

    check(
        state,
        final_reload_result.has_value() &&
            final_reload_result.value().
                runtime_replaced,
        "Empty-to-nonempty transition reports replacement");

    original_loaded =
        loader.find(
            original_handle);

    check(
        state,
        original_loaded != nullptr &&
            original_loaded->runtime_bytes ==
                final_revision.cooked_bytes,
        "Final replacement restores non-empty runtime data");

    cache.clear();

    check(
        state,
        cache.empty(),
        "Cache can be cleared after replacement");

    const Result<AssetHotReloadResult>
        same_after_clear_result =
            hot_reload.reload(
                final_revision.record);

    check(
        state,
        same_after_clear_result.has_value(),
        "Current revision reload does not require cached data");

    check(
        state,
        same_after_clear_result.has_value() &&
            !same_after_clear_result.value().
                runtime_replaced,
        "Current revision after cache clear reports no replacement");

    CookedAsset missing_final_revision =
        make_cooked_asset(
            original_asset.record.id,
            original_asset.record.source_path,
            {
                std::byte{0x90},
                std::byte{0x91},
                std::byte{0x92}
            },
            11U);

    const LoadedAsset
        before_missing_final =
            *original_loaded;

    const Result<AssetHotReloadResult>
        missing_after_clear_result =
            hot_reload.reload(
                missing_final_revision.record);

    check(
        state,
        !missing_after_clear_result.
            has_value(),
        "Different revision still requires cached replacement data");

    check(
        state,
        !missing_after_clear_result.
                has_value() &&
            missing_after_clear_result.
                error().code ==
                ErrorCode::not_found,
        "Missing revision after cache clear reports not_found");

    original_loaded =
        loader.find(
            original_handle);

    check(
        state,
        original_loaded != nullptr &&
            original_loaded->record ==
                before_missing_final.record &&
            original_loaded->runtime_bytes ==
                before_missing_final.
                    runtime_bytes,
        "Failed reload after cache clear preserves runtime state");

    CookedAsset second_asset =
        make_cooked_asset(
            AssetId{
                30ULL,
                2ULL
            },
            "runtime/hot-reload/second.asset",
            {
                std::byte{0x55}
            });

    const Result<bool>
        second_cache_store =
            cache.store(
                second_asset);

    check(
        state,
        second_cache_store.has_value() &&
            second_cache_store.value(),
        "Second asset payload enters the cache");

    const Result<ResourceHandle>
        second_load =
            loader.load(
                second_asset.record);

    check(
        state,
        second_load.has_value(),
        "Second asset loads successfully");

    ResourceHandle second_handle{};

    if (second_load.has_value())
    {
        second_handle =
            second_load.value();
    }

    check(
        state,
        second_handle.is_valid() &&
            second_handle !=
                original_handle,
        "Second asset receives a distinct handle");

    CookedAsset second_revision =
        make_cooked_asset(
            second_asset.record.id,
            second_asset.record.source_path,
            {
                std::byte{0x66},
                std::byte{0x77}
            },
            7U);

    const Result<bool>
        second_revision_store =
            cache.store(
                second_revision);

    check(
        state,
        second_revision_store.has_value() &&
            second_revision_store.value(),
        "Second asset replacement enters the cache");

    const Result<AssetHotReloadResult>
        second_reload =
            hot_reload.reload(
                second_revision.record);

    check(
        state,
        second_reload.has_value(),
        "Second loaded asset hot-reloads successfully");

    check(
        state,
        second_reload.has_value() &&
            second_reload.value().
                handle ==
                second_handle,
        "Second asset reload preserves its own handle");

    check(
        state,
        loader.handle_for(
            final_revision.record.id) ==
            original_handle,
        "Second reload preserves the first asset handle");

    check(
        state,
        loader.loaded_count() == 2U,
        "Two runtime assets remain loaded");

    const Status original_unload =
        loader.unload(
            original_handle);

    check(
        state,
        original_unload.has_value(),
        "Original hot-reloaded asset unloads successfully");

    const Result<AssetHotReloadResult>
        reload_after_unload =
            hot_reload.reload(
                final_revision.record);

    check(
        state,
        !reload_after_unload.has_value(),
        "Hot reload rejects an unloaded asset");

    check(
        state,
        !reload_after_unload.has_value() &&
            reload_after_unload.error().code ==
                ErrorCode::not_found,
        "Reload after unload reports not_found");

    check(
        state,
        loader.contains(
            second_handle),
        "Unloading the first asset preserves the second");

    const Status second_unload =
        loader.unload(
            second_handle);

    check(
        state,
        second_unload.has_value(),
        "Second hot-reloaded asset unloads successfully");

    check(
        state,
        loader.empty(),
        "Load pipeline finishes empty");

    check(
        state,
        loader.loaded_count() == 0U,
        "Final unload resets the loaded count");

    std::cout
        << "\nAsset hot-reload pipeline test summary: "
        << state.checks - state.failures
        << '/'
        << state.checks
        << " passed.\n";

    return state.failures == 0 ? 0 : 1;
}