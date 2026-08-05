#include "oros/assets/asset_hot_reload_pipeline.hpp"

#include "oros/assets/content_hash.hpp"

#include <new>
#include <span>
#include <type_traits>
#include <utility>

namespace
{
    static_assert(
        std::is_nothrow_move_assignable_v<
            oros::assets::LoadedAsset>);
}

namespace oros::assets
{
    AssetHotReloadPipeline::
        AssetHotReloadPipeline(
            const CookedAssetCache& cache,
            AssetLoadPipeline& loader)
            noexcept
        : cache_{
              &cache
          },
          loader_{
              &loader
          }
    {
    }

    foundation::Result<
        AssetHotReloadResult>
    AssetHotReloadPipeline::reload(
        const AssetRecord& record)
    {
        if (cache_ == nullptr ||
            loader_ == nullptr)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "The asset hot-reload pipeline is "
                "not connected to its required "
                "storage.");
        }

        if (!record.is_valid() ||
            !record.is_cooked())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cannot hot-reload an invalid or "
                "uncooked asset record.");
        }

        const ResourceHandle handle =
            loader_->handle_for(
                record.id);

        if (!handle.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    not_found,
                "The asset is not currently loaded.");
        }

        LoadedAsset* const loaded_asset =
            loader_->find(
                handle);

        if (loaded_asset == nullptr)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "The loaded asset handle could not "
                "be resolved.");
        }

        if (!loaded_asset->is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "The existing runtime asset is "
                "invalid.");
        }

        if (loaded_asset->record.id !=
            record.id)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "The loaded resource identity does "
                "not match the reload request.");
        }

        if (!loaded_asset->record.
                cooked_hash.has_value() ||
            !record.cooked_hash.has_value())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Hot-reload metadata is missing a "
                "cooked content hash.");
        }

        const ContentHash previous_hash =
            loaded_asset->record.
                cooked_hash.value();

        const ContentHash current_hash =
            record.cooked_hash.value();

        const ContentHash
            existing_runtime_hash =
                hash_bytes(
                    std::span<const std::byte>{
                        loaded_asset->
                            runtime_bytes
                    });

        if (existing_runtime_hash !=
            previous_hash)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "The existing runtime payload no "
                "longer matches its cooked content "
                "hash.");
        }

        try
        {
            LoadedAsset replacement{};

            replacement.record =
                record;

            bool runtime_replaced{};

            if (current_hash ==
                previous_hash)
            {
                replacement.runtime_bytes =
                    loaded_asset->
                        runtime_bytes;
            }
            else
            {
                const std::vector<std::byte>*
                    cached_bytes =
                        cache_->find(
                            current_hash);

                if (cached_bytes == nullptr)
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            not_found,
                        "The replacement cooked "
                        "payload is not present in "
                        "the cache.");
                }

                const ContentHash cached_hash =
                    hash_bytes(
                        std::span<
                            const std::byte>{
                            *cached_bytes
                        });

                if (cached_hash !=
                    current_hash)
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_state,
                        "The replacement cached "
                        "payload does not match its "
                        "cooked content hash.");
                }

                replacement.runtime_bytes =
                    *cached_bytes;

                runtime_replaced = true;
            }

            if (!replacement.is_valid())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "The hot-reload pipeline "
                    "produced an invalid runtime "
                    "asset.");
            }

            const ContentHash
                replacement_runtime_hash =
                    hash_bytes(
                        std::span<
                            const std::byte>{
                            replacement.
                                runtime_bytes
                        });

            if (replacement_runtime_hash !=
                current_hash)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "The replacement runtime "
                    "payload does not match the "
                    "requested cooked hash.");
            }

            *loaded_asset =
                std::move(
                    replacement);

            AssetHotReloadResult result{};

            result.handle =
                handle;

            result.previous_hash =
                previous_hash;

            result.current_hash =
                current_hash;

            result.runtime_replaced =
                runtime_replaced;

            if (!result.is_valid())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "The hot-reload pipeline "
                    "produced an invalid result.");
            }

            return result;
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Unable to allocate memory while "
                "hot-reloading an asset.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "An unexpected failure occurred "
                "while hot-reloading an asset.");
        }
    }
}