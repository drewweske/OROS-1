#pragma once

#include "oros/assets/asset_id.hpp"
#include "oros/assets/resource_handle.hpp"
#include "oros/foundation/result.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <new>
#include <optional>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace oros::assets
{
    template <typename Resource>
    class ResourceTable final
    {
        static_assert(
            std::is_nothrow_move_constructible_v<
                Resource>,
            "ResourceTable resources must be "
            "nothrow move constructible.");

        static_assert(
            std::is_nothrow_destructible_v<
                Resource>,
            "ResourceTable resources must be "
            "nothrow destructible.");

    public:
        ResourceTable() = default;

        ResourceTable(
            const ResourceTable&) = delete;

        ResourceTable&
        operator=(
            const ResourceTable&) = delete;

        ResourceTable(
            ResourceTable&& other) noexcept
            : slots_{
                std::move(other.slots_)
            },
              by_asset_{
                std::move(other.by_asset_)
            },
              first_free_slot_{
                std::exchange(
                    other.first_free_slot_,
                    0U)
            },
              size_{
                std::exchange(
                    other.size_,
                    0U)
            }
        {
            other.slots_.clear();
            other.by_asset_.clear();
        }

        ResourceTable&
        operator=(
            ResourceTable&& other) noexcept
        {
            if (this == &other)
            {
                return *this;
            }

            slots_ =
                std::move(other.slots_);

            by_asset_ =
                std::move(other.by_asset_);

            first_free_slot_ =
                std::exchange(
                    other.first_free_slot_,
                    0U);

            size_ =
                std::exchange(
                    other.size_,
                    0U);

            other.slots_.clear();
            other.by_asset_.clear();

            return *this;
        }

        [[nodiscard]]
        foundation::Result<ResourceHandle>
        insert(
            const AssetId asset,
            Resource resource)
        {
            if (!asset.is_valid())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Cannot insert a resource with an "
                    "invalid asset identity.");
            }

            if (by_asset_.contains(asset))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_state,
                    "A resource is already loaded for "
                    "the supplied asset identity.");
            }

            if (first_free_slot_ != 0U)
            {
                return insert_into_free_slot(
                    asset,
                    std::move(resource));
            }

            return append_slot(
                asset,
                std::move(resource));
        }

        [[nodiscard]]
        foundation::Status erase(
            const ResourceHandle handle)
        {
            Slot* const slot =
                find_slot(handle);

            if (slot == nullptr)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        not_found,
                    "The resource handle is invalid, "
                    "stale, or not loaded.");
            }

            by_asset_.erase(
                slot->asset);

            slot->resource.reset();

            slot->asset =
                invalid_asset_id;

            --size_;

            if (slot->generation ==
                (std::numeric_limits<
                    std::uint32_t>::max)())
            {
                slot->generation = 0U;
                slot->next_free_slot = 0U;
                slot->retired = true;

                return {};
            }

            ++slot->generation;

            slot->next_free_slot =
                first_free_slot_;

            first_free_slot_ =
                handle.slot;

            return {};
        }

        [[nodiscard]]
        Resource* find(
            const ResourceHandle handle) noexcept
        {
            Slot* const slot =
                find_slot(handle);

            if (slot == nullptr)
            {
                return nullptr;
            }

            return std::addressof(
                slot->resource.value());
        }

        [[nodiscard]]
        const Resource* find(
            const ResourceHandle handle)
            const noexcept
        {
            const Slot* const slot =
                find_slot(handle);

            if (slot == nullptr)
            {
                return nullptr;
            }

            return std::addressof(
                slot->resource.value());
        }

        [[nodiscard]]
        ResourceHandle handle_for(
            const AssetId asset) const noexcept
        {
            const auto iterator =
                by_asset_.find(asset);

            if (iterator == by_asset_.end())
            {
                return invalid_resource_handle;
            }

            return iterator->second;
        }

        [[nodiscard]]
        AssetId asset_for(
            const ResourceHandle handle)
            const noexcept
        {
            const Slot* const slot =
                find_slot(handle);

            if (slot == nullptr)
            {
                return invalid_asset_id;
            }

            return slot->asset;
        }

        [[nodiscard]]
        bool contains(
            const ResourceHandle handle)
            const noexcept
        {
            return
                find_slot(handle) != nullptr;
        }

        [[nodiscard]]
        bool contains(
            const AssetId asset) const noexcept
        {
            return by_asset_.contains(asset);
        }

        [[nodiscard]]
        std::size_t size() const noexcept
        {
            return size_;
        }

        [[nodiscard]]
        bool empty() const noexcept
        {
            return size_ == 0U;
        }

        [[nodiscard]]
        std::size_t slot_count() const noexcept
        {
            return slots_.size();
        }

    private:
        struct Slot final
        {
            std::uint32_t generation{1U};
            std::uint32_t next_free_slot{};
            AssetId asset{};
            std::optional<Resource> resource{};
            bool retired{};
        };

        [[nodiscard]]
        foundation::Result<ResourceHandle>
        append_slot(
            const AssetId asset,
            Resource resource)
        {
            if (slots_.size() >=
                static_cast<std::size_t>(
                    (std::numeric_limits<
                        std::uint32_t>::max)()))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_state,
                    "The resource table has exhausted "
                    "its 32-bit slot range.");
            }

            Slot new_slot{};
            new_slot.asset = asset;

            new_slot.resource.emplace(
                std::move(resource));

            try
            {
                slots_.push_back(
                    std::move(new_slot));
            }
            catch (const std::bad_alloc&)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        out_of_memory,
                    "Unable to allocate a new resource "
                    "table slot.");
            }
            catch (...)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "An unexpected failure occurred while "
                    "allocating a resource table slot.");
            }

            const std::uint32_t slot_number =
                static_cast<std::uint32_t>(
                    slots_.size());

            const ResourceHandle handle{
                slot_number,
                slots_.back().generation
            };

            try
            {
                const bool inserted =
                    by_asset_.emplace(
                        asset,
                        handle).second;

                if (!inserted)
                {
                    slots_.pop_back();

                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_state,
                        "A resource is already loaded for "
                        "the supplied asset identity.");
                }
            }
            catch (const std::bad_alloc&)
            {
                slots_.pop_back();

                return foundation::fail(
                    foundation::ErrorCode::
                        out_of_memory,
                    "Unable to allocate the resource "
                    "identity lookup entry.");
            }
            catch (...)
            {
                slots_.pop_back();

                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "An unexpected failure occurred while "
                    "registering a resource identity.");
            }

            ++size_;
            return handle;
        }

        [[nodiscard]]
        foundation::Result<ResourceHandle>
        insert_into_free_slot(
            const AssetId asset,
            Resource resource)
        {
            const std::uint32_t slot_number =
                first_free_slot_;

            Slot& slot =
                slots_[
                    static_cast<std::size_t>(
                        slot_number - 1U)];

            const ResourceHandle handle{
                slot_number,
                slot.generation
            };

            slot.asset = asset;

            slot.resource.emplace(
                std::move(resource));

            try
            {
                const bool inserted =
                    by_asset_.emplace(
                        asset,
                        handle).second;

                if (!inserted)
                {
                    slot.resource.reset();

                    slot.asset =
                        invalid_asset_id;

                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_state,
                        "A resource is already loaded for "
                        "the supplied asset identity.");
                }
            }
            catch (const std::bad_alloc&)
            {
                slot.resource.reset();

                slot.asset =
                    invalid_asset_id;

                return foundation::fail(
                    foundation::ErrorCode::
                        out_of_memory,
                    "Unable to allocate the resource "
                    "identity lookup entry.");
            }
            catch (...)
            {
                slot.resource.reset();

                slot.asset =
                    invalid_asset_id;

                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "An unexpected failure occurred while "
                    "registering a resource identity.");
            }

            first_free_slot_ =
                slot.next_free_slot;

            slot.next_free_slot = 0U;

            ++size_;
            return handle;
        }

        [[nodiscard]]
        Slot* find_slot(
            const ResourceHandle handle) noexcept
        {
            if (!handle.is_valid() ||
                static_cast<std::size_t>(
                    handle.slot) >
                    slots_.size())
            {
                return nullptr;
            }

            Slot& slot =
                slots_[
                    static_cast<std::size_t>(
                        handle.slot - 1U)];

            if (slot.retired ||
                slot.generation !=
                    handle.generation ||
                !slot.resource.has_value())
            {
                return nullptr;
            }

            return std::addressof(slot);
        }

        [[nodiscard]]
        const Slot* find_slot(
            const ResourceHandle handle)
            const noexcept
        {
            if (!handle.is_valid() ||
                static_cast<std::size_t>(
                    handle.slot) >
                    slots_.size())
            {
                return nullptr;
            }

            const Slot& slot =
                slots_[
                    static_cast<std::size_t>(
                        handle.slot - 1U)];

            if (slot.retired ||
                slot.generation !=
                    handle.generation ||
                !slot.resource.has_value())
            {
                return nullptr;
            }

            return std::addressof(slot);
        }

        std::vector<Slot> slots_{};

        std::unordered_map<
            AssetId,
            ResourceHandle,
            AssetIdHash
        > by_asset_{};

        std::uint32_t first_free_slot_{};
        std::size_t size_{};
    };
}