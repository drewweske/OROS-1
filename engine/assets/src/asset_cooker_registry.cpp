#include "oros/assets/asset_cooker_registry.hpp"

#include <cstddef>
#include <new>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace oros::assets
{
    AssetCookerRegistry::AssetCookerRegistry(
        AssetCookerRegistry&& other) noexcept
    {
        cookers_.swap(
            other.cookers_);

        by_name_.swap(
            other.by_name_);
    }

    AssetCookerRegistry&
    AssetCookerRegistry::operator=(
        AssetCookerRegistry&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        cookers_.clear();
        by_name_.clear();

        cookers_.swap(
            other.cookers_);

        by_name_.swap(
            other.by_name_);

        return *this;
    }

    foundation::Status
    AssetCookerRegistry::register_cooker(
        std::unique_ptr<AssetCooker> cooker)
    {
        if (cooker == nullptr)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cannot register a null asset "
                "cooker.");
        }

        const std::string_view cooker_name =
            cooker->name();

        if (cooker_name.empty())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "An asset cooker must expose a "
                "non-empty stable name.");
        }

        if (cooker->version() == 0U)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "An asset cooker version must be "
                "nonzero.");
        }

        if (by_name_.contains(
                cooker_name))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "The asset cooker registry already "
                "contains the supplied cooker name.");
        }

        const std::size_t cooker_index =
            cookers_.size();

        try
        {
            cookers_.push_back(
                std::move(cooker));
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Unable to allocate asset cooker "
                "ownership storage.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "An unexpected failure occurred while "
                "storing an asset cooker.");
        }

        try
        {
            const auto insertion =
                by_name_.emplace(
                    std::string{
                        cooker_name
                    },
                    cooker_index);

            if (!insertion.second)
            {
                cookers_.pop_back();

                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_state,
                    "The asset cooker registry already "
                    "contains the supplied cooker "
                    "name.");
            }
        }
        catch (const std::bad_alloc&)
        {
            cookers_.pop_back();

            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Unable to allocate the asset cooker "
                "name index.");
        }
        catch (...)
        {
            cookers_.pop_back();

            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "An unexpected failure occurred while "
                "indexing an asset cooker.");
        }

        return {};
    }

    foundation::Status
    AssetCookerRegistry::unregister_cooker(
        const std::string_view cooker_name)
    {
        if (cooker_name.empty())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cannot unregister a cooker with an "
                "empty name.");
        }

        const auto name_iterator =
            by_name_.find(
                cooker_name);

        if (name_iterator ==
            by_name_.end())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    not_found,
                "The asset cooker registry does not "
                "contain the supplied cooker name.");
        }

        const std::size_t cooker_index =
            name_iterator->second;

        if (cooker_index >=
                cookers_.size() ||
            cookers_[cooker_index] ==
                nullptr ||
            cookers_[cooker_index]->
                name() != cooker_name)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "The asset cooker registry name "
                "index is inconsistent.");
        }

        using DifferenceType =
            std::vector<
                std::unique_ptr<AssetCooker>
            >::difference_type;

        cookers_.erase(
            cookers_.begin() +
            static_cast<DifferenceType>(
                cooker_index));

        by_name_.erase(
            name_iterator);

        for (auto& name_entry :
             by_name_)
        {
            if (name_entry.second >
                cooker_index)
            {
                --name_entry.second;
            }
        }

        return {};
    }

    const AssetCooker*
    AssetCookerRegistry::find(
        const std::string_view cooker_name)
        const noexcept
    {
        const auto name_iterator =
            by_name_.find(
                cooker_name);

        if (name_iterator ==
            by_name_.end())
        {
            return nullptr;
        }

        const std::size_t cooker_index =
            name_iterator->second;

        if (cooker_index >=
            cookers_.size())
        {
            return nullptr;
        }

        return
            cookers_[cooker_index].
                get();
    }

    foundation::Result<
        const AssetCooker*>
    AssetCookerRegistry::resolve(
        const AssetRecord& record) const
    {
        if (!record.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cannot resolve a cooker for an "
                "invalid asset record.");
        }

        if (record.is_cooked())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cannot resolve a cooker for an "
                "already-cooked asset record.");
        }

        const AssetCooker*
            matching_cooker = nullptr;

        for (const std::unique_ptr<
                 AssetCooker>& cooker :
             cookers_)
        {
            if (cooker == nullptr)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "The asset cooker registry "
                    "contains a null cooker.");
            }

            if (!cooker->supports(
                    record))
            {
                continue;
            }

            if (matching_cooker != nullptr)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_state,
                    "More than one asset cooker "
                    "supports the supplied asset "
                    "record.");
            }

            matching_cooker =
                cooker.get();
        }

        if (matching_cooker == nullptr)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    unsupported_operation,
                "No registered asset cooker supports "
                "the supplied asset record.");
        }

        return matching_cooker;
    }

    std::size_t
    AssetCookerRegistry::size()
        const noexcept
    {
        return cookers_.size();
    }

    bool
    AssetCookerRegistry::empty()
        const noexcept
    {
        return cookers_.empty();
    }
}