#include "oros/assets/asset_import_pipeline.hpp"

#include "oros/assets/content_hash.hpp"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <new>
#include <string_view>
#include <utility>

namespace oros::assets
{
    AssetImportPipeline::AssetImportPipeline(
        const AssetImporterRegistry&
            importer_registry) noexcept
        : importer_registry_{
              &importer_registry
          }
    {
    }

    foundation::Result<ImportedAsset>
    AssetImportPipeline::import(
        const ImportRequest& request) const
    {
        if (!request.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cannot execute an invalid asset "
                "import request.");
        }

        if (importer_registry_ == nullptr)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "The asset import pipeline has no "
                "importer registry.");
        }

        foundation::Result<
            const AssetImporter*>
            resolution =
                importer_registry_->resolve(
                    request.source_path);

        if (!resolution.has_value())
        {
            return std::unexpected{
                std::move(
                    resolution.error())
            };
        }

        const AssetImporter* const importer =
            resolution.value();

        if (importer == nullptr)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "The importer registry resolved a null "
                "asset importer.");
        }

        const std::string_view importer_name =
            importer->name();

        const std::uint32_t importer_version =
            importer->version();

        const std::uint32_t schema_version =
            importer->schema_version();

        if (importer_name.empty() ||
            importer_version == 0U ||
            schema_version == 0U)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "The resolved importer exposes invalid "
                "identity or version metadata.");
        }

        foundation::Result<ImportResult>
            import_result =
                [&]() ->
                    foundation::Result<
                        ImportResult>
                {
                    try
                    {
                        return importer->import(
                            request);
                    }
                    catch (const std::bad_alloc&)
                    {
                        return foundation::fail(
                            foundation::ErrorCode::
                                out_of_memory,
                            "The asset importer could "
                            "not allocate its output.");
                    }
                    catch (...)
                    {
                        return foundation::fail(
                            foundation::ErrorCode::
                                internal_failure,
                            "The asset importer raised "
                            "an unexpected exception.");
                    }
                }();

        if (!import_result.has_value())
        {
            return std::unexpected{
                std::move(
                    import_result.error())
            };
        }

        ImportResult importer_output =
            std::move(
                import_result.value());

        for (std::size_t index = 0U;
             index <
                 importer_output.dependencies.size();
             ++index)
        {
            const AssetId dependency =
                importer_output.dependencies[index];

            if (!dependency.is_valid())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_state,
                    "The asset importer produced an "
                    "invalid dependency identity.");
            }

            if (dependency == request.asset)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_state,
                    "The asset importer produced a "
                    "self dependency.");
            }

            for (std::size_t earlier = 0U;
                 earlier < index;
                 ++earlier)
            {
                if (importer_output.
                        dependencies[earlier] ==
                    dependency)
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_state,
                        "The asset importer produced "
                        "duplicate dependencies.");
                }
            }
        }

        try
        {
            ImportedAsset imported_asset{};

            imported_asset.record.id =
                request.asset;

            imported_asset.record.source_path =
                request.source_path;

            imported_asset.record.importer_name =
                importer_name;

            imported_asset.record.importer_version =
                importer_version;

            imported_asset.record.schema_version =
                schema_version;

            imported_asset.record.source_hash =
                hash_bytes(
                    request.source_bytes);

            imported_asset.record.dependencies =
                std::move(
                    importer_output.dependencies);

            imported_asset.intermediate_bytes =
                std::move(
                    importer_output.
                        intermediate_bytes);

            if (!imported_asset.is_valid())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "The asset import pipeline produced "
                    "an invalid asset record.");
            }

            return imported_asset;
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Unable to allocate imported asset "
                "metadata.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "An unexpected failure occurred while "
                "assembling an imported asset.");
        }
    }
}