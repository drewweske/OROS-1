#include "oros/assets/asset_cook_pipeline.hpp"

#include "oros/assets/content_hash.hpp"

#include <cstdint>
#include <new>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace oros::assets
{
    AssetCookPipeline::AssetCookPipeline(
        const AssetCookerRegistry&
            registry) noexcept
        : registry_{
              &registry
          }
    {
    }

    foundation::Result<CookedAsset>
    AssetCookPipeline::cook(
        const ImportedAsset& asset) const
    {
        if (registry_ == nullptr)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "The asset cook pipeline has no "
                "cooker registry.");
        }

        if (!asset.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cannot cook an invalid imported "
                "asset.");
        }

        if (asset.record.is_cooked())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cannot cook an imported asset that "
                "already contains cooked metadata.");
        }

        const foundation::Result<
            const AssetCooker*>
            resolution =
                registry_->resolve(
                    asset.record);

        if (!resolution.has_value())
        {
            return foundation::fail(
                resolution.error().code,
                resolution.error().message);
        }

        const AssetCooker* const cooker =
            resolution.value();

        if (cooker == nullptr)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "The cooker registry resolved a null "
                "asset cooker.");
        }

        const std::string_view
            cooker_name_view =
                cooker->name();

        const std::uint32_t
            cooker_version =
                cooker->version();

        if (cooker_name_view.empty())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "The resolved asset cooker exposes an "
                "empty stable name.");
        }

        if (cooker_version == 0U)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "The resolved asset cooker exposes "
                "version zero.");
        }

        if (registry_->find(
                cooker_name_view) !=
            cooker)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "The resolved asset cooker's stable "
                "name no longer matches the registry "
                "index.");
        }

        if (!cooker->supports(
                asset.record))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "The resolved asset cooker no longer "
                "supports the imported asset.");
        }

        try
        {
            const std::string cooker_name{
                cooker_name_view
            };

            const CookRequest request{
                asset.record,
                std::span<const std::byte>{
                    asset.intermediate_bytes
                }
            };

            if (!request.is_valid())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "The asset cook pipeline produced "
                    "an invalid cook request.");
            }

            foundation::Result<CookResult>
                cook_result =
                    cooker->cook(
                        request);

            if (!cook_result.has_value())
            {
                return foundation::fail(
                    cook_result.error().code,
                    cook_result.error().message);
            }

            if (cooker->name() !=
                    cooker_name ||
                cooker->version() !=
                    cooker_version ||
                registry_->find(
                    cooker_name) !=
                    cooker)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_state,
                    "The asset cooker changed its "
                    "registered metadata while "
                    "cooking.");
            }

            if (!cooker->supports(
                    asset.record))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_state,
                    "The asset cooker changed its "
                    "supported schema while cooking.");
            }

            CookedAsset cooked_asset{};

            cooked_asset.record =
                asset.record;

            cooked_asset.cooker_name =
                cooker_name;

            cooked_asset.cooker_version =
                cooker_version;

            cooked_asset.cooked_bytes =
                std::move(
                    cook_result.value().
                        cooked_bytes);

            cooked_asset.record.cooked_hash =
                hash_bytes(
                    std::span<const std::byte>{
                        cooked_asset.cooked_bytes
                    });

            if (!cooked_asset.is_valid())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "The asset cooker produced an "
                    "invalid cooked asset.");
            }

            return cooked_asset;
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Unable to allocate memory while "
                "cooking an asset.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "An unexpected failure occurred while "
                "cooking an asset.");
        }
    }
}