#include "oros/assets/asset_catalog.hpp"

#include <new>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace oros::assets
{
    namespace
    {
        static_assert(
            std::is_nothrow_move_assignable_v<
                AssetRecord>);

        [[nodiscard]]
        constexpr bool is_separator(
            const char character) noexcept
        {
            return
                character == '/' ||
                character == '\\';
        }

        [[nodiscard]]
        constexpr bool is_ascii_letter(
            const char character) noexcept
        {
            return
                (character >= 'A' &&
                 character <= 'Z') ||
                (character >= 'a' &&
                 character <= 'z');
        }

        [[nodiscard]]
        constexpr char to_ascii_lowercase(
            const char character) noexcept
        {
            if (character >= 'A' &&
                character <= 'Z')
            {
                return static_cast<char>(
                    character -
                    'A' +
                    'a');
            }

            return character;
        }

        [[nodiscard]]
        constexpr bool is_invalid_path_character(
            const char character) noexcept
        {
            const unsigned char value =
                static_cast<unsigned char>(
                    character);

            return
                value < 0x20U ||
                character == '"' ||
                character == '<' ||
                character == '>' ||
                character == '|' ||
                character == '?' ||
                character == '*' ||
                character == ':';
        }

        [[nodiscard]]
        foundation::Result<std::string>
        normalize_source_path(
            const std::string_view source_path)
        {
            if (source_path.empty())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "An asset source path cannot be "
                    "empty.");
            }

            if (is_separator(
                    source_path.front()))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "An asset source path must be "
                    "relative.");
            }

            if (source_path.size() >= 2U &&
                is_ascii_letter(
                    source_path[0]) &&
                source_path[1] == ':')
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "An asset source path cannot contain "
                    "a drive prefix.");
            }

            std::string normalized{};

            try
            {
                normalized.reserve(
                    source_path.size());
            }
            catch (const std::bad_alloc&)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        out_of_memory,
                    "Unable to allocate normalized "
                    "source-path storage.");
            }
            catch (...)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "An unexpected failure occurred while "
                    "preparing a source path.");
            }

            std::size_t position = 0U;

            while (position <
                   source_path.size())
            {
                while (position <
                           source_path.size() &&
                       is_separator(
                           source_path[position]))
                {
                    ++position;
                }

                if (position >=
                    source_path.size())
                {
                    break;
                }

                const std::size_t component_begin =
                    position;

                while (position <
                           source_path.size() &&
                       !is_separator(
                           source_path[position]))
                {
                    ++position;
                }

                const std::string_view component =
                    source_path.substr(
                        component_begin,
                        position -
                            component_begin);

                if (component == ".")
                {
                    continue;
                }

                if (component == "..")
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "An asset source path cannot "
                        "traverse to a parent directory.");
                }

                for (const char character :
                     component)
                {
                    if (is_invalid_path_character(
                            character))
                    {
                        return foundation::fail(
                            foundation::ErrorCode::
                                invalid_argument,
                            "An asset source path contains "
                            "an invalid character.");
                    }
                }

                try
                {
                    if (!normalized.empty())
                    {
                        normalized.push_back('/');
                    }

                    for (const char character :
                         component)
                    {
                        normalized.push_back(
                            to_ascii_lowercase(
                                character));
                    }
                }
                catch (const std::bad_alloc&)
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            out_of_memory,
                        "Unable to allocate normalized "
                        "source-path storage.");
                }
                catch (...)
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            internal_failure,
                        "An unexpected failure occurred "
                        "while normalizing a source path.");
                }
            }

            if (normalized.empty())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "An asset source path must contain "
                    "at least one file-name component.");
            }

            return normalized;
        }

        [[nodiscard]]
        std::optional<std::string>
        normalize_source_path_for_lookup(
            const std::string_view source_path)
            noexcept
        {
            try
            {
                foundation::Result<std::string>
                    normalized =
                        normalize_source_path(
                            source_path);

                if (!normalized.has_value())
                {
                    return std::nullopt;
                }

                return std::move(
                    normalized.value());
            }
            catch (...)
            {
                return std::nullopt;
            }
        }
    }

    foundation::Status
    AssetCatalog::insert(
        AssetRecord record)
    {
        if (!record.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cannot insert an invalid asset "
                "record.");
        }

        foundation::Result<std::string>
            normalized_path_result =
                normalize_source_path(
                    record.source_path);

        if (!normalized_path_result.has_value())
        {
            return std::unexpected{
                std::move(
                    normalized_path_result.error())
            };
        }

        record.source_path =
            std::move(
                normalized_path_result.value());

        const AssetId asset =
            record.id;

        if (records_.contains(asset))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "The asset catalog already contains "
                "the supplied asset identity.");
        }

        if (by_source_path_.contains(
                record.source_path))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "The asset catalog already contains "
                "the normalized source path.");
        }

        try
        {
            const auto record_result =
                records_.emplace(
                    asset,
                    std::move(record));

            if (!record_result.second)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_state,
                    "The asset catalog already contains "
                    "the supplied asset identity.");
            }

            try
            {
                const bool path_inserted =
                    by_source_path_.emplace(
                        record_result.first->
                            second.source_path,
                        asset).second;

                if (!path_inserted)
                {
                    records_.erase(
                        record_result.first);

                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_state,
                        "The asset catalog already "
                        "contains the normalized "
                        "source path.");
                }
            }
            catch (const std::bad_alloc&)
            {
                records_.erase(
                    record_result.first);

                return foundation::fail(
                    foundation::ErrorCode::
                        out_of_memory,
                    "Unable to allocate the catalog "
                    "source-path index.");
            }
            catch (...)
            {
                records_.erase(
                    record_result.first);

                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "An unexpected failure occurred "
                    "while indexing an asset source "
                    "path.");
            }
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Unable to allocate the catalog asset "
                "record.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "An unexpected failure occurred while "
                "inserting an asset record.");
        }

        return {};
    }

    foundation::Status
    AssetCatalog::replace(
        AssetRecord record)
    {
        if (!record.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cannot replace an asset with an "
                "invalid record.");
        }

        foundation::Result<std::string>
            normalized_path_result =
                normalize_source_path(
                    record.source_path);

        if (!normalized_path_result.has_value())
        {
            return std::unexpected{
                std::move(
                    normalized_path_result.error())
            };
        }

        record.source_path =
            std::move(
                normalized_path_result.value());

        const AssetId asset =
            record.id;

        const auto record_iterator =
            records_.find(asset);

        if (record_iterator ==
            records_.end())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    not_found,
                "The asset catalog does not contain "
                "the supplied asset identity.");
        }

        const auto path_owner =
            by_source_path_.find(
                record.source_path);

        if (path_owner !=
                by_source_path_.end() &&
            path_owner->second != asset)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Another asset already owns the "
                "normalized source path.");
        }

        const auto old_path_iterator =
            by_source_path_.find(
                record_iterator->
                    second.source_path);

        if (old_path_iterator ==
            by_source_path_.end() ||
            old_path_iterator->second != asset)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "The asset catalog source-path index "
                "is inconsistent.");
        }

        if (old_path_iterator->first ==
            record.source_path)
        {
            record_iterator->second =
                std::move(record);

            return {};
        }

        try
        {
            const auto insertion =
                by_source_path_.emplace(
                    record.source_path,
                    asset);

            if (!insertion.second)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_state,
                    "Another asset already owns the "
                    "normalized source path.");
            }
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Unable to allocate the replacement "
                "source-path index entry.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "An unexpected failure occurred while "
                "indexing the replacement source path.");
        }

        record_iterator->second =
            std::move(record);

        by_source_path_.erase(
            old_path_iterator);

        return {};
    }

    foundation::Status
    AssetCatalog::erase(
        const AssetId asset)
    {
        if (!asset.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cannot erase an invalid asset "
                "identity.");
        }

        const auto record_iterator =
            records_.find(asset);

        if (record_iterator ==
            records_.end())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    not_found,
                "The asset catalog does not contain "
                "the supplied asset identity.");
        }

        const auto path_iterator =
            by_source_path_.find(
                record_iterator->
                    second.source_path);

        if (path_iterator ==
                by_source_path_.end() ||
            path_iterator->second != asset)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "The asset catalog source-path index "
                "is inconsistent.");
        }

        by_source_path_.erase(
            path_iterator);

        records_.erase(
            record_iterator);

        return {};
    }

    const AssetRecord*
    AssetCatalog::find(
        const AssetId asset) const noexcept
    {
        const auto iterator =
            records_.find(asset);

        if (iterator == records_.end())
        {
            return nullptr;
        }

        return &iterator->second;
    }

    const AssetRecord*
    AssetCatalog::find_by_source_path(
        const std::string_view source_path)
        const noexcept
    {
        const std::optional<std::string>
            normalized_path =
                normalize_source_path_for_lookup(
                    source_path);

        if (!normalized_path.has_value())
        {
            return nullptr;
        }

        const auto path_iterator =
            by_source_path_.find(
                normalized_path.value());

        if (path_iterator ==
            by_source_path_.end())
        {
            return nullptr;
        }

        return find(
            path_iterator->second);
    }

    AssetId
    AssetCatalog::id_for_source_path(
        const std::string_view source_path)
        const noexcept
    {
        const AssetRecord* const record =
            find_by_source_path(
                source_path);

        if (record == nullptr)
        {
            return invalid_asset_id;
        }

        return record->id;
    }

    bool
    AssetCatalog::contains(
        const AssetId asset) const noexcept
    {
        return records_.contains(asset);
    }

    bool
    AssetCatalog::contains_source_path(
        const std::string_view source_path)
        const noexcept
    {
        return
            find_by_source_path(
                source_path) != nullptr;
    }

    std::size_t
    AssetCatalog::size() const noexcept
    {
        return records_.size();
    }

    bool
    AssetCatalog::empty() const noexcept
    {
        return records_.empty();
    }
}