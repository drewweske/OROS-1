#include "oros/assets/asset_load_pipeline.hpp"

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
        std::vector<std::byte> cooked_bytes)
    {
        using namespace oros::assets;

        CookedAsset asset{};

        asset.record.id = id;

        asset.record.source_path =
            std::move(
                source_path);

        asset.record.importer_name =
            "oros.load_importer";

        asset.record.importer_version =
            6U;

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
                90ULL,
                1ULL
            }
        };

        asset.cooker_name =
            "oros.load_cooker";

        asset.cooker_version =
            8U;

        asset.cooked_bytes =
            std::move(
                cooked_bytes);

        return asset;
    }

    static_assert(
        std::is_nothrow_move_constructible_v<
            oros::assets::LoadedAsset>);

    static_assert(
        std::is_nothrow_destructible_v<
            oros::assets::LoadedAsset>);

    static_assert(
        !std::is_copy_constructible_v<
            oros::assets::
                AssetLoadPipeline>);

    static_assert(
        !std::is_copy_assignable_v<
            oros::assets::
                AssetLoadPipeline>);

    static_assert(
        !std::is_move_constructible_v<
            oros::assets::
                AssetLoadPipeline>);

    static_assert(
        !std::is_move_assignable_v<
            oros::assets::
                AssetLoadPipeline>);
}

int main()
{
    using namespace oros::assets;
    using namespace oros::foundation;

    TestState state{};

    const LoadedAsset default_loaded_asset{};

    check(
        state,
        !default_loaded_asset.is_valid(),
        "Default loaded asset is invalid");

    check(
        state,
        !default_loaded_asset.
            has_runtime_data(),
        "Default loaded asset has no runtime data");

    check(
        state,
        default_loaded_asset.
            runtime_bytes.empty(),
        "Default loaded asset has an empty payload");

    CookedAssetCache cache{};

    ResourceTable<LoadedAsset>
        resources{};

    AssetLoadPipeline pipeline{
        cache,
        resources
    };

    check(
        state,
        pipeline.empty(),
        "New load pipeline is empty");

    check(
        state,
        pipeline.loaded_count() == 0U,
        "New load pipeline has zero loaded assets");

    check(
        state,
        resources.empty(),
        "New resource table is empty");

    check(
        state,
        pipeline.find(
            invalid_resource_handle) ==
            nullptr,
        "Invalid handle lookup returns null");

    check(
        state,
        pipeline.handle_for(
            invalid_asset_id) ==
            invalid_resource_handle,
        "Invalid asset lookup returns an invalid handle");

    check(
        state,
        !pipeline.contains(
            invalid_resource_handle),
        "Pipeline rejects the invalid resource handle");

    check(
        state,
        !pipeline.contains(
            invalid_asset_id),
        "Pipeline rejects the invalid asset identity");

    const Result<ResourceHandle>
        default_record_result =
            pipeline.load(
                AssetRecord{});

    check(
        state,
        !default_record_result.has_value(),
        "Load pipeline rejects a default asset record");

    check(
        state,
        !default_record_result.has_value() &&
            default_record_result.error().code ==
                ErrorCode::invalid_argument,
        "Default record reports invalid_argument");

    CookedAsset first_asset =
        make_cooked_asset(
            AssetId{
                20ULL,
                1ULL
            },
            "runtime/load/first.asset",
            {
                std::byte{0x10},
                std::byte{0x20},
                std::byte{0x30},
                std::byte{0x40}
            });

    check(
        state,
        first_asset.is_valid(),
        "First cooked asset is valid");

    check(
        state,
        first_asset.record.is_cooked(),
        "First asset record is cooked");

    AssetRecord uncooked_record =
        first_asset.record;

    uncooked_record.cooked_hash.reset();

    check(
        state,
        uncooked_record.is_valid(),
        "Uncooked test record remains structurally valid");

    check(
        state,
        !uncooked_record.is_cooked(),
        "Uncooked test record reports uncooked state");

    const Result<ResourceHandle>
        uncooked_result =
            pipeline.load(
                uncooked_record);

    check(
        state,
        !uncooked_result.has_value(),
        "Load pipeline rejects uncooked metadata");

    check(
        state,
        !uncooked_result.has_value() &&
            uncooked_result.error().code ==
                ErrorCode::invalid_argument,
        "Uncooked metadata reports invalid_argument");

    const Result<ResourceHandle>
        missing_cache_result =
            pipeline.load(
                first_asset.record);

    check(
        state,
        !missing_cache_result.has_value(),
        "Load pipeline rejects a missing cached payload");

    check(
        state,
        !missing_cache_result.has_value() &&
            missing_cache_result.error().code ==
                ErrorCode::not_found,
        "Missing cached payload reports not_found");

    check(
        state,
        pipeline.empty(),
        "Rejected loads preserve an empty pipeline");

    const Result<bool>
        first_cache_store =
            cache.store(
                first_asset);

    check(
        state,
        first_cache_store.has_value() &&
            first_cache_store.value(),
        "First cooked payload enters the cache");

    const ContentHash first_hash =
        first_asset.record.cooked_hash.
            value();

    check(
        state,
        cache.contains(
            first_hash),
        "Cache contains the first cooked payload");

    const Result<ResourceHandle>
        first_load_result =
            pipeline.load(
                first_asset.record);

    check(
        state,
        first_load_result.has_value(),
        "Load pipeline accepts cached cooked metadata");

    ResourceHandle first_handle{};

    if (first_load_result.has_value())
    {
        first_handle =
            first_load_result.value();
    }

    check(
        state,
        first_handle.is_valid(),
        "First load produces a valid resource handle");

    check(
        state,
        first_handle ==
            ResourceHandle{
                1U,
                1U
            },
        "First load uses the first resource slot");

    check(
        state,
        !pipeline.empty(),
        "Pipeline is non-empty after loading");

    check(
        state,
        pipeline.loaded_count() == 1U,
        "First load increases the loaded count");

    check(
        state,
        resources.size() == 1U,
        "First load inserts into the resource table");

    check(
        state,
        resources.slot_count() == 1U,
        "First load allocates one resource slot");

    check(
        state,
        pipeline.contains(
            first_handle),
        "Pipeline contains the first handle");

    check(
        state,
        pipeline.contains(
            first_asset.record.id),
        "Pipeline contains the first asset identity");

    check(
        state,
        pipeline.handle_for(
            first_asset.record.id) ==
            first_handle,
        "Asset identity resolves to the first handle");

    LoadedAsset* first_loaded =
        pipeline.find(
            first_handle);

    check(
        state,
        first_loaded != nullptr,
        "Mutable loaded asset lookup succeeds");

    check(
        state,
        first_loaded != nullptr &&
            first_loaded->is_valid(),
        "Loaded runtime asset is valid");

    check(
        state,
        first_loaded != nullptr &&
            first_loaded->
                has_runtime_data(),
        "Loaded runtime asset reports data");

    check(
        state,
        first_loaded != nullptr &&
            first_loaded->record ==
                first_asset.record,
        "Loaded asset preserves its complete record");

    check(
        state,
        first_loaded != nullptr &&
            first_loaded->runtime_bytes ==
                first_asset.cooked_bytes,
        "Loaded asset preserves every cooked byte");

    const AssetLoadPipeline&
        const_pipeline =
            pipeline;

    const LoadedAsset*
        const_first_loaded =
            const_pipeline.find(
                first_handle);

    check(
        state,
        const_first_loaded ==
            first_loaded,
        "Const lookup resolves the same loaded asset");

    const Result<ResourceHandle>
        repeated_load_result =
            pipeline.load(
                first_asset.record);

    check(
        state,
        repeated_load_result.has_value(),
        "Load pipeline accepts an identical repeated load");

    check(
        state,
        repeated_load_result.has_value() &&
            repeated_load_result.value() ==
                first_handle,
        "Repeated load returns the existing handle");

    check(
        state,
        pipeline.loaded_count() == 1U,
        "Repeated load preserves the loaded count");

    check(
        state,
        resources.slot_count() == 1U,
        "Repeated load allocates no additional slot");

    const Status first_cache_erase =
        cache.erase(
            first_hash);

    check(
        state,
        first_cache_erase.has_value(),
        "First cached payload can be erased after loading");

    check(
        state,
        !cache.contains(
            first_hash),
        "Erased payload is absent from the cache");

    check(
        state,
        first_loaded != nullptr &&
            first_loaded->runtime_bytes ==
                first_asset.cooked_bytes,
        "Loaded runtime bytes survive cache eviction");

    const Result<ResourceHandle>
        repeated_after_eviction =
            pipeline.load(
                first_asset.record);

    check(
        state,
        repeated_after_eviction.
            has_value() &&
            repeated_after_eviction.value() ==
                first_handle,
        "Identical loaded revision does not require the cache");

    CookedAsset revised_asset =
        make_cooked_asset(
            first_asset.record.id,
            first_asset.record.source_path,
            {
                std::byte{0xAA},
                std::byte{0xBB},
                std::byte{0xCC}
            });

    check(
        state,
        revised_asset.is_valid(),
        "Revised cooked asset is valid");

    check(
        state,
        revised_asset.record.id ==
            first_asset.record.id,
        "Revised asset preserves the stable asset identity");

    check(
        state,
        revised_asset.record.cooked_hash !=
            first_asset.record.cooked_hash,
        "Revised payload has a different cooked hash");

    const Result<bool>
        revised_cache_store =
            cache.store(
                revised_asset);

    check(
        state,
        revised_cache_store.has_value() &&
            revised_cache_store.value(),
        "Revised cooked payload enters the cache");

    const Result<ResourceHandle>
        conflicting_revision_result =
            pipeline.load(
                revised_asset.record);

    check(
        state,
        !conflicting_revision_result.
            has_value(),
        "Load pipeline rejects a different loaded revision");

    check(
        state,
        !conflicting_revision_result.
                has_value() &&
            conflicting_revision_result.
                error().code ==
                ErrorCode::invalid_state,
        "Conflicting loaded revision reports invalid_state");

    check(
        state,
        pipeline.handle_for(
            first_asset.record.id) ==
            first_handle,
        "Rejected revision preserves the original handle");

    check(
        state,
        pipeline.loaded_count() == 1U,
        "Rejected revision preserves the loaded count");

    const Status invalid_unload =
        pipeline.unload(
            invalid_resource_handle);

    check(
        state,
        !invalid_unload.has_value(),
        "Load pipeline rejects unloading an invalid handle");

    check(
        state,
        !invalid_unload.has_value() &&
            invalid_unload.error().code ==
                ErrorCode::not_found,
        "Invalid unload reports not_found");

    const Status first_unload =
        pipeline.unload(
            first_handle);

    check(
        state,
        first_unload.has_value(),
        "Load pipeline unloads the first runtime asset");

    check(
        state,
        pipeline.empty(),
        "Pipeline becomes empty after unloading");

    check(
        state,
        pipeline.loaded_count() == 0U,
        "Unload decreases the loaded count");

    check(
        state,
        !pipeline.contains(
            first_handle),
        "Unloaded handle is no longer contained");

    check(
        state,
        !pipeline.contains(
            first_asset.record.id),
        "Unloaded asset identity is no longer contained");

    check(
        state,
        pipeline.find(
            first_handle) ==
            nullptr,
        "Unloaded handle lookup returns null");

    check(
        state,
        pipeline.handle_for(
            first_asset.record.id) ==
            invalid_resource_handle,
        "Unloaded asset lookup returns an invalid handle");

    const Status stale_unload =
        pipeline.unload(
            first_handle);

    check(
        state,
        !stale_unload.has_value(),
        "Load pipeline rejects unloading a stale handle");

    check(
        state,
        !stale_unload.has_value() &&
            stale_unload.error().code ==
                ErrorCode::not_found,
        "Stale unload reports not_found");

    const Result<ResourceHandle>
        revised_load_result =
            pipeline.load(
                revised_asset.record);

    check(
        state,
        revised_load_result.has_value(),
        "Revised asset loads after the original is unloaded");

    ResourceHandle revised_handle{};

    if (revised_load_result.has_value())
    {
        revised_handle =
            revised_load_result.value();
    }

    check(
        state,
        revised_handle.is_valid(),
        "Revised load produces a valid handle");

    check(
        state,
        revised_handle.slot ==
            first_handle.slot,
        "Revised load reuses the released slot");

    check(
        state,
        revised_handle.generation >
            first_handle.generation,
        "Revised load advances the slot generation");

    check(
        state,
        revised_handle !=
            first_handle,
        "Revised handle differs from the stale handle");

    check(
        state,
        !pipeline.contains(
            first_handle),
        "Stale generation cannot access the revised asset");

    LoadedAsset* revised_loaded =
        pipeline.find(
            revised_handle);

    check(
        state,
        revised_loaded != nullptr &&
            revised_loaded->record ==
                revised_asset.record,
        "Revised load preserves revised metadata");

    check(
        state,
        revised_loaded != nullptr &&
            revised_loaded->runtime_bytes ==
                revised_asset.cooked_bytes,
        "Revised load preserves revised runtime bytes");

    if (revised_loaded != nullptr)
    {
        revised_loaded->runtime_bytes[0] =
            std::byte{0xFE};
    }

    const Result<ResourceHandle>
        corrupted_runtime_result =
            pipeline.load(
                revised_asset.record);

    check(
        state,
        !corrupted_runtime_result.has_value(),
        "Load pipeline detects corrupted runtime bytes");

    check(
        state,
        !corrupted_runtime_result.has_value() &&
            corrupted_runtime_result.error().code ==
                ErrorCode::invalid_state,
        "Runtime payload corruption reports invalid_state");

    check(
        state,
        pipeline.contains(
            revised_handle),
        "Corruption detection preserves the loaded handle");

    if (revised_loaded != nullptr)
    {
        revised_loaded->runtime_bytes =
            revised_asset.cooked_bytes;
    }

    const Result<ResourceHandle>
        restored_runtime_result =
            pipeline.load(
                revised_asset.record);

    check(
        state,
        restored_runtime_result.has_value() &&
            restored_runtime_result.value() ==
                revised_handle,
        "Restored runtime data permits repeated loading");

    CookedAsset empty_asset =
        make_cooked_asset(
            AssetId{
                20ULL,
                2ULL
            },
            "runtime/load/empty.asset",
            {});

    check(
        state,
        empty_asset.is_valid(),
        "Empty cooked payload forms a valid asset");

    check(
        state,
        !empty_asset.has_cooked_data(),
        "Empty cooked asset reports no cooked data");

    const Result<bool>
        empty_cache_store =
            cache.store(
                empty_asset);

    check(
        state,
        empty_cache_store.has_value() &&
            empty_cache_store.value(),
        "Empty cooked payload enters the cache");

    const Result<ResourceHandle>
        empty_load_result =
            pipeline.load(
                empty_asset.record);

    check(
        state,
        empty_load_result.has_value(),
        "Load pipeline accepts an empty cooked payload");

    ResourceHandle empty_handle{};

    if (empty_load_result.has_value())
    {
        empty_handle =
            empty_load_result.value();
    }

    const LoadedAsset* empty_loaded =
        pipeline.find(
            empty_handle);

    check(
        state,
        empty_loaded != nullptr &&
            empty_loaded->is_valid(),
        "Empty runtime asset remains valid");

    check(
        state,
        empty_loaded != nullptr &&
            !empty_loaded->
                has_runtime_data(),
        "Empty runtime asset reports no runtime data");

    check(
        state,
        empty_loaded != nullptr &&
            empty_loaded->
                runtime_bytes.empty(),
        "Empty runtime asset preserves an empty payload");

    check(
        state,
        pipeline.loaded_count() == 2U,
        "Empty payload increases the loaded count");

    CookedAsset second_empty_asset =
        make_cooked_asset(
            AssetId{
                20ULL,
                3ULL
            },
            "runtime/load/second-empty.asset",
            {});

    check(
        state,
        second_empty_asset.record.
            cooked_hash ==
            empty_asset.record.
                cooked_hash,
        "Two empty payloads share one content hash");

    const Result<bool>
        second_empty_cache_store =
            cache.store(
                second_empty_asset);

    check(
        state,
        second_empty_cache_store.
                has_value() &&
            !second_empty_cache_store.value(),
        "Cache deduplicates the second empty payload");

    const Result<ResourceHandle>
        second_empty_load_result =
            pipeline.load(
                second_empty_asset.record);

    check(
        state,
        second_empty_load_result.has_value(),
        "Deduplicated content loads for another asset identity");

    ResourceHandle second_empty_handle{};

    if (second_empty_load_result.has_value())
    {
        second_empty_handle =
            second_empty_load_result.value();
    }

    check(
        state,
        second_empty_handle.is_valid() &&
            second_empty_handle !=
                empty_handle,
        "Different asset identities receive different handles");

    check(
        state,
        pipeline.loaded_count() == 3U,
        "Three asset identities are loaded");

    check(
        state,
        resources.slot_count() == 3U,
        "Three live identities occupy three slots");

    cache.clear();

    check(
        state,
        cache.empty(),
        "Cooked cache can be cleared while assets are loaded");

    check(
        state,
        pipeline.loaded_count() == 3U,
        "Cache clear preserves loaded runtime assets");

    check(
        state,
        pipeline.find(
            revised_handle) != nullptr &&
            pipeline.find(
                empty_handle) != nullptr &&
            pipeline.find(
                second_empty_handle) !=
                nullptr,
        "Cache clear preserves every loaded handle");

    const Status revised_unload =
        pipeline.unload(
            revised_handle);

    const Status empty_unload =
        pipeline.unload(
            empty_handle);

    const Status second_empty_unload =
        pipeline.unload(
            second_empty_handle);

    check(
        state,
        revised_unload.has_value() &&
            empty_unload.has_value() &&
            second_empty_unload.has_value(),
        "Load pipeline unloads every runtime asset");

    check(
        state,
        pipeline.empty(),
        "Pipeline is empty after all unloads");

    check(
        state,
        pipeline.loaded_count() == 0U,
        "Final unload resets the loaded count");

    check(
        state,
        resources.slot_count() == 3U,
        "Unloading preserves reusable resource slots");

    const Result<ResourceHandle>
        load_after_cache_clear =
            pipeline.load(
                first_asset.record);

    check(
        state,
        !load_after_cache_clear.has_value(),
        "Unloaded asset cannot reload after cache clear");

    check(
        state,
        !load_after_cache_clear.has_value() &&
            load_after_cache_clear.error().code ==
                ErrorCode::not_found,
        "Reload without cached content reports not_found");

    const Result<bool>
        restored_cache_store =
            cache.store(
                first_asset);

    check(
        state,
        restored_cache_store.has_value() &&
            restored_cache_store.value(),
        "First payload can be restored to the cache");

    const Result<ResourceHandle>
        restored_load_result =
            pipeline.load(
                first_asset.record);

    check(
        state,
        restored_load_result.has_value(),
        "Pipeline can load again after cache restoration");

    ResourceHandle restored_handle{};

    if (restored_load_result.has_value())
    {
        restored_handle =
            restored_load_result.value();
    }

    check(
        state,
        restored_handle.is_valid(),
        "Restored load produces a valid handle");

    check(
        state,
        restored_handle !=
            first_handle &&
            restored_handle !=
                revised_handle,
        "Restored load rejects every earlier stale handle");

    check(
        state,
        pipeline.find(
            restored_handle) != nullptr &&
            pipeline.find(
                restored_handle)->
                runtime_bytes ==
                first_asset.cooked_bytes,
        "Restored load recreates the runtime payload");

    const Status final_unload =
        pipeline.unload(
            restored_handle);

    check(
        state,
        final_unload.has_value(),
        "Restored runtime asset unloads successfully");

    check(
        state,
        pipeline.empty(),
        "Load pipeline finishes empty");

    std::cout
        << "\nAsset load pipeline test summary: "
        << state.checks - state.failures
        << '/'
        << state.checks
        << " passed.\n";

    return state.failures == 0 ? 0 : 1;
}