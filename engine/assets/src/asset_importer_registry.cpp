#include "oros/assets/asset_importer_registry.hpp"

#include <cstddef>
#include <new>
#include <string>
#include <string_view>
#include <utility>

namespace oros::assets
{
    AssetImporterRegistry::AssetImporterRegistry(
        AssetImporterRegistry&& other) noexcept
    {
        importers_.swap(
            other.importers_);

        by_name_.swap(
            other.by_name_);
    }

    AssetImporterRegistry&
    AssetImporterRegistry::operator=(
        AssetImporterRegistry&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        importers_.clear();
        by_name_.clear();

        importers_.swap(
            other.importers_);

        by_name_.swap(
            other.by_name_);

        return *this;
    }

    foundation::Status
    AssetImporterRegistry::register_importer(
        std::unique_ptr<AssetImporter> importer)
    {
        if (importer == nullptr)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cannot register a null asset "
                "importer.");
        }

        const std::string_view importer_name =
            importer->name();

        if (importer_name.empty())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "An asset importer must expose a "
                "non-empty stable name.");
        }

        if (importer->version() == 0U)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "An asset importer version must be "
                "nonzero.");
        }

        if (importer->schema_version() == 0U)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "An asset importer schema version "
                "must be nonzero.");
        }

        if (by_name_.contains(
                importer_name))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "The asset importer registry already "
                "contains the supplied importer name.");
        }

        const std::size_t importer_index =
            importers_.size();

        try
        {
            importers_.push_back(
                std::move(importer));
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Unable to allocate asset importer "
                "ownership storage.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "An unexpected failure occurred while "
                "storing an asset importer.");
        }

        try
        {
            const auto insertion =
                by_name_.emplace(
                    std::string{
                        importer_name
                    },
                    importer_index);

            if (!insertion.second)
            {
                importers_.pop_back();

                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_state,
                    "The asset importer registry "
                    "already contains the supplied "
                    "importer name.");
            }
        }
        catch (const std::bad_alloc&)
        {
            importers_.pop_back();

            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Unable to allocate the asset importer "
                "name index.");
        }
        catch (...)
        {
            importers_.pop_back();

            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "An unexpected failure occurred while "
                "indexing an asset importer.");
        }

        return {};
    }

    foundation::Status
    AssetImporterRegistry::unregister_importer(
        const std::string_view importer_name)
    {
        if (importer_name.empty())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cannot unregister an importer with "
                "an empty name.");
        }

        const auto name_iterator =
            by_name_.find(
                importer_name);

        if (name_iterator ==
            by_name_.end())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    not_found,
                "The asset importer registry does not "
                "contain the supplied importer name.");
        }

        const std::size_t importer_index =
            name_iterator->second;

        if (importer_index >=
                importers_.size() ||
            importers_[importer_index] ==
                nullptr ||
            importers_[importer_index]->
                name() != importer_name)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "The asset importer registry name "
                "index is inconsistent.");
        }

        using DifferenceType =
            std::vector<
                std::unique_ptr<AssetImporter>
            >::difference_type;

        importers_.erase(
            importers_.begin() +
            static_cast<DifferenceType>(
                importer_index));

        by_name_.erase(
            name_iterator);

        for (auto& name_entry :
             by_name_)
        {
            if (name_entry.second >
                importer_index)
            {
                --name_entry.second;
            }
        }

        return {};
    }

    const AssetImporter*
    AssetImporterRegistry::find(
        const std::string_view importer_name)
        const noexcept
    {
        const auto name_iterator =
            by_name_.find(
                importer_name);

        if (name_iterator ==
            by_name_.end())
        {
            return nullptr;
        }

        const std::size_t importer_index =
            name_iterator->second;

        if (importer_index >=
            importers_.size())
        {
            return nullptr;
        }

        return
            importers_[importer_index].
                get();
    }

    foundation::Result<
        const AssetImporter*>
    AssetImporterRegistry::resolve(
        const std::string_view source_path)
        const
    {
        if (source_path.empty())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cannot resolve an importer for an "
                "empty source path.");
        }

        const AssetImporter*
            matching_importer = nullptr;

        for (const std::unique_ptr<
                 AssetImporter>& importer :
             importers_)
        {
            if (importer == nullptr)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "The asset importer registry "
                    "contains a null importer.");
            }

            if (!importer->supports(
                    source_path))
            {
                continue;
            }

            if (matching_importer != nullptr)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_state,
                    "More than one asset importer "
                    "supports the supplied source "
                    "path.");
            }

            matching_importer =
                importer.get();
        }

        if (matching_importer == nullptr)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    unsupported_operation,
                "No registered asset importer supports "
                "the supplied source path.");
        }

        return matching_importer;
    }

    std::size_t
    AssetImporterRegistry::size()
        const noexcept
    {
        return importers_.size();
    }

    bool
    AssetImporterRegistry::empty()
        const noexcept
    {
        return importers_.empty();
    }
}