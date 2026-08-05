#include "oros/assets/asset_load_pipeline.hpp"

#include "oros/assets/content_hash.hpp"

#include <new>
#include <span>
#include <utility>

namespace oros::assets
{
    AssetLoadPipeline::AssetLoadPipeline(
        const CookedAssetCache& cache,
        ResourceTable<LoadedAsset>&
            resources) noexcept
        : cache_{
              &cache
          },
          resources_{
              &resources
          }
    {
    }

    foundation::Result<ResourceHandle>
    AssetLoadPipeline::load(
        const AssetRecord& record)
    {
        if (cache_ == nullptr ||
            resources_ == nullptr)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "The asset load pipeline is not "
                "connected to its required storage.");
        }

        if (!record.is_valid() ||
            !record.is_cooked())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cannot load an invalid or uncooked "
                "asset record.");
        }

        const ContentHash& cooked_hash =
            record.cooked_hash.value();

        const ResourceHandle
            existing_handle =
                resources_->handle_for(
                    record.id);

        if (existing_handle.is_valid())
        {
            const LoadedAsset*
                existing_asset =
                    resources_->find(
                        existing_handle);

            if (existing_asset == nullptr)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "The resource table contains an "
                    "inconsistent asset identity "
                    "lookup.");
            }

            if (!existing_asset->is_valid())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_state,
                    "The existing loaded asset is "
                    "invalid.");
            }

            if (existing_asset->record !=
                record)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_state,
                    "A different cooked revision is "
                    "already loaded for this asset "
                    "identity.");
            }

            const ContentHash
                existing_runtime_hash =
                    hash_bytes(
                        std::span<
                            const std::byte>{
                            existing_asset->
                                runtime_bytes
                        });

            if (existing_runtime_hash !=
                cooked_hash)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_state,
                    "The existing runtime payload no "
                    "longer matches its cooked content "
                    "hash.");
            }

            return existing_handle;
        }

        const std::vector<std::byte>*
            cached_bytes =
                cache_->find(
                    cooked_hash);

        if (cached_bytes == nullptr)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    not_found,
                "The cooked asset payload is not "
                "present in the cache.");
        }

        const ContentHash cached_hash =
            hash_bytes(
                std::span<const std::byte>{
                    *cached_bytes
                });

        if (cached_hash != cooked_hash)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "The cached payload does not match "
                "the asset record's cooked content "
                "hash.");
        }

        try
        {
            LoadedAsset loaded_asset{};

            loaded_asset.record =
                record;

            loaded_asset.runtime_bytes =
                *cached_bytes;

            if (!loaded_asset.is_valid())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "The asset load pipeline produced "
                    "an invalid runtime asset.");
            }

            foundation::Result<
                ResourceHandle>
                insertion_result =
                    resources_->insert(
                        record.id,
                        std::move(
                            loaded_asset));

            if (!insertion_result.has_value())
            {
                return foundation::fail(
                    insertion_result.error().code,
                    insertion_result.error().message);
            }

            return insertion_result.value();
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Unable to allocate memory while "
                "loading a cooked asset.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "An unexpected failure occurred while "
                "loading a cooked asset.");
        }
    }

    foundation::Status
    AssetLoadPipeline::unload(
        const ResourceHandle handle)
    {
        if (resources_ == nullptr)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "The asset load pipeline has no "
                "resource table.");
        }

        return resources_->erase(
            handle);
    }

    LoadedAsset*
    AssetLoadPipeline::find(
        const ResourceHandle handle)
        noexcept
    {
        if (resources_ == nullptr)
        {
            return nullptr;
        }

        return resources_->find(
            handle);
    }

    const LoadedAsset*
    AssetLoadPipeline::find(
        const ResourceHandle handle)
        const noexcept
    {
        if (resources_ == nullptr)
        {
            return nullptr;
        }

        const ResourceTable<
            LoadedAsset>& resources =
                *resources_;

        return resources.find(
            handle);
    }

    ResourceHandle
    AssetLoadPipeline::handle_for(
        const AssetId asset)
        const noexcept
    {
        if (resources_ == nullptr)
        {
            return invalid_resource_handle;
        }

        return resources_->handle_for(
            asset);
    }

    bool
    AssetLoadPipeline::contains(
        const ResourceHandle handle)
        const noexcept
    {
        return
            resources_ != nullptr &&
            resources_->contains(
                handle);
    }

    bool
    AssetLoadPipeline::contains(
        const AssetId asset)
        const noexcept
    {
        return
            resources_ != nullptr &&
            resources_->contains(
                asset);
    }

    std::size_t
    AssetLoadPipeline::loaded_count()
        const noexcept
    {
        if (resources_ == nullptr)
        {
            return 0U;
        }

        return resources_->size();
    }

    bool
    AssetLoadPipeline::empty()
        const noexcept
    {
        return
            resources_ == nullptr ||
            resources_->empty();
    }
}