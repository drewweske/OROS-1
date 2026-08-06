#include "oros/assets/binary_asset_importer.hpp"

#include <cstddef>
#include <new>
#include <string_view>
#include <vector>

namespace oros::assets
{
    namespace
    {
        [[nodiscard]]
        constexpr char
        ascii_lowercase(
            const char value) noexcept
        {
            if (value >= 'A' &&
                value <= 'Z')
            {
                return static_cast<char>(
                    value - 'A' + 'a');
            }

            return value;
        }

        [[nodiscard]]
        bool
        ends_with_ascii_case_insensitive(
            const std::string_view value,
            const std::string_view suffix)
            noexcept
        {
            if (value.size() <
                suffix.size())
            {
                return false;
            }

            const std::size_t offset =
                value.size() -
                suffix.size();

            for (std::size_t index = 0U;
                 index < suffix.size();
                 ++index)
            {
                if (ascii_lowercase(
                        value[offset + index]) !=
                    ascii_lowercase(
                        suffix[index]))
                {
                    return false;
                }
            }

            return true;
        }
    }

    std::string_view
    BinaryAssetImporter::name()
        const noexcept
    {
        return binary_asset_importer_name;
    }

    std::uint32_t
    BinaryAssetImporter::version()
        const noexcept
    {
        return binary_asset_importer_version;
    }

    std::uint32_t
    BinaryAssetImporter::schema_version()
        const noexcept
    {
        return binary_asset_schema_version;
    }

    bool
    BinaryAssetImporter::supports(
        const std::string_view source_path)
        const noexcept
    {
        if (source_path.empty())
        {
            return false;
        }

        return
            ends_with_ascii_case_insensitive(
                source_path,
                binary_asset_source_extension);
    }

    foundation::Result<ImportResult>
    BinaryAssetImporter::import(
        const ImportRequest& request)
        const
    {
        if (!request.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cannot import an invalid binary "
                "asset request.");
        }

        if (!supports(
                request.source_path))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    unsupported_operation,
                "The binary asset importer does not "
                "support the requested source path.");
        }

        try
        {
            ImportResult result{};

            result.intermediate_bytes =
                std::vector<std::byte>{
                    request.source_bytes.begin(),
                    request.source_bytes.end()
                };

            return result;
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Unable to allocate binary asset "
                "intermediate storage.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "An unexpected failure occurred while "
                "importing a binary asset.");
        }
    }
}