#include "oros/assets/cooked_asset_cache.hpp"

#include <limits>
#include <new>
#include <span>
#include <utility>

namespace oros::assets
{
    CookedAssetCache::CookedAssetCache(
        CookedAssetCache&& other) noexcept
        : total_byte_count_{
              std::exchange(
                  other.total_byte_count_,
                  0U)
          }
    {
        entries_.swap(
            other.entries_);
    }

    CookedAssetCache&
    CookedAssetCache::operator=(
        CookedAssetCache&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        entries_.clear();

        entries_.swap(
            other.entries_);

        total_byte_count_ =
            std::exchange(
                other.total_byte_count_,
                0U);

        return *this;
    }

    foundation::Result<bool>
    CookedAssetCache::store(
        const CookedAsset& asset)
    {
        if (!asset.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cannot cache an invalid cooked "
                "asset.");
        }

        if (!asset.record.cooked_hash.
                has_value())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "The cooked asset does not contain "
                "a content hash.");
        }

        const ContentHash& expected_hash =
            asset.record.cooked_hash.value();

        const ContentHash actual_hash =
            hash_bytes(
                std::span<const std::byte>{
                    asset.cooked_bytes
                });

        if (actual_hash != expected_hash)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "The cooked asset payload does not "
                "match its content hash.");
        }

        const auto existing_iterator =
            entries_.find(
                expected_hash);

        if (existing_iterator !=
            entries_.end())
        {
            if (existing_iterator->second ==
                asset.cooked_bytes)
            {
                return false;
            }

            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "The cooked asset cache contains "
                "different data for the same content "
                "hash.");
        }

        const std::size_t byte_count =
            asset.cooked_bytes.size();

        if (byte_count >
            std::numeric_limits<
                std::size_t>::max() -
                total_byte_count_)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "The cooked asset cache byte count "
                "would overflow.");
        }

        try
        {
            const auto insertion =
                entries_.emplace(
                    expected_hash,
                    asset.cooked_bytes);

            if (!insertion.second)
            {
                if (insertion.first->second ==
                    asset.cooked_bytes)
                {
                    return false;
                }

                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_state,
                    "The cooked asset cache rejected "
                    "a conflicting content hash.");
            }
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Unable to allocate cooked asset "
                "cache storage.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "An unexpected failure occurred "
                "while storing a cooked asset.");
        }

        total_byte_count_ +=
            byte_count;

        return true;
    }

    foundation::Status
    CookedAssetCache::erase(
        const ContentHash& hash)
    {
        const auto iterator =
            entries_.find(
                hash);

        if (iterator == entries_.end())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    not_found,
                "The cooked asset cache does not "
                "contain the supplied content hash.");
        }

        const std::size_t byte_count =
            iterator->second.size();

        if (byte_count >
            total_byte_count_)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "The cooked asset cache byte count "
                "is inconsistent.");
        }

        entries_.erase(
            iterator);

        total_byte_count_ -=
            byte_count;

        return {};
    }

    void
    CookedAssetCache::clear() noexcept
    {
        entries_.clear();
        total_byte_count_ = 0U;
    }

    const std::vector<std::byte>*
    CookedAssetCache::find(
        const ContentHash& hash)
        const noexcept
    {
        const auto iterator =
            entries_.find(
                hash);

        if (iterator == entries_.end())
        {
            return nullptr;
        }

        return &iterator->second;
    }

    bool
    CookedAssetCache::contains(
        const ContentHash& hash)
        const noexcept
    {
        return find(hash) != nullptr;
    }

    std::size_t
    CookedAssetCache::size()
        const noexcept
    {
        return entries_.size();
    }

    std::size_t
    CookedAssetCache::total_byte_count()
        const noexcept
    {
        return total_byte_count_;
    }

    bool
    CookedAssetCache::empty()
        const noexcept
    {
        return entries_.empty();
    }
}