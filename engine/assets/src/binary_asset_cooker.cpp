#include "oros/assets/binary_asset_cooker.hpp"

#include <cstddef>
#include <new>
#include <vector>

namespace oros::assets
{
    std::string_view
    BinaryAssetCooker::name()
        const noexcept
    {
        return binary_asset_cooker_name;
    }

    std::uint32_t
    BinaryAssetCooker::version()
        const noexcept
    {
        return binary_asset_cooker_version;
    }

    bool
    BinaryAssetCooker::supports(
        const AssetRecord& record)
        const noexcept
    {
        return
            record.is_valid() &&
            !record.is_cooked() &&
            record.importer_name ==
                binary_asset_importer_name &&
            record.schema_version ==
                binary_asset_schema_version;
    }

    foundation::Result<CookResult>
    BinaryAssetCooker::cook(
        const CookRequest& request)
        const
    {
        if (!request.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cannot cook an invalid binary asset "
                "request.");
        }

        if (!supports(
                request.record))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    unsupported_operation,
                "The binary asset cooker does not "
                "support the requested asset record.");
        }

        try
        {
            CookResult result{};

            result.cooked_bytes =
                std::vector<std::byte>{
                    request.intermediate_bytes.begin(),
                    request.intermediate_bytes.end()
                };

            return result;
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Unable to allocate binary asset "
                "cooked storage.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "An unexpected failure occurred while "
                "cooking a binary asset.");
        }
    }
}