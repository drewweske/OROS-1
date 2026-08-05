#pragma once

#include "oros/foundation/result.hpp"
#include "oros/world/entity_id.hpp"

#include <cstddef>
#include <new>
#include <span>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace oros::world
{
    template <typename Component>
    class ComponentStore final
    {
        static_assert(
            std::is_nothrow_move_constructible_v<Component>,
            "World components must be nothrow move constructible.");

        static_assert(
            std::is_nothrow_move_assignable_v<Component>,
            "World components must be nothrow move assignable.");

    public:
        using value_type = Component;
        using size_type = std::size_t;

        [[nodiscard]] foundation::Status
        insert(
            const EntityId entity,
            Component component)
        {
            if (!entity.is_valid())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Component storage requires a valid "
                    "entity identity.");
            }

            if (contains(entity))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_state,
                    "The entity already owns this component.");
            }

            const size_type new_size =
                components_.size() + 1U;

            try
            {
                entities_.reserve(new_size);
                components_.reserve(new_size);
                indices_.reserve(new_size);
            }
            catch (const std::bad_alloc&)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        out_of_memory,
                    "Component storage could not reserve "
                    "memory for another component.");
            }
            catch (...)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "Component storage failed while reserving "
                    "capacity.");
            }

            const size_type dense_index =
                components_.size();

            entities_.push_back(entity);

            components_.push_back(
                std::move(component));

            try
            {
                const auto [iterator, inserted] =
                    indices_.emplace(
                        entity,
                        dense_index);

                static_cast<void>(iterator);

                if (!inserted)
                {
                    components_.pop_back();
                    entities_.pop_back();

                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_state,
                        "The entity already owns this "
                        "component.");
                }
            }
            catch (const std::bad_alloc&)
            {
                components_.pop_back();
                entities_.pop_back();

                return foundation::fail(
                    foundation::ErrorCode::
                        out_of_memory,
                    "Component storage could not index "
                    "another component.");
            }
            catch (...)
            {
                components_.pop_back();
                entities_.pop_back();

                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "Component storage failed while indexing "
                    "another component.");
            }

            return {};
        }

        [[nodiscard]] bool
        contains(
            const EntityId entity) const noexcept
        {
            return indices_.contains(entity);
        }

        [[nodiscard]] Component*
        find(
            const EntityId entity) noexcept
        {
            const auto iterator =
                indices_.find(entity);

            if (iterator == indices_.end())
            {
                return nullptr;
            }

            return &components_[
                iterator->second];
        }

        [[nodiscard]] const Component*
        find(
            const EntityId entity) const noexcept
        {
            const auto iterator =
                indices_.find(entity);

            if (iterator == indices_.end())
            {
                return nullptr;
            }

            return &components_[
                iterator->second];
        }

        [[nodiscard]] foundation::Status
        remove(
            const EntityId entity)
        {
            if (!entity.is_valid())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Component removal requires a valid "
                    "entity identity.");
            }

            const auto removed_iterator =
                indices_.find(entity);

            if (removed_iterator == indices_.end())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        not_found,
                    "The entity does not own this component.");
            }

            const size_type removed_index =
                removed_iterator->second;

            const size_type final_index =
                components_.size() - 1U;

            if (removed_index != final_index)
            {
                components_[removed_index] =
                    std::move(
                        components_[final_index]);

                entities_[removed_index] =
                    entities_[final_index];

                const auto moved_iterator =
                    indices_.find(
                        entities_[removed_index]);

                if (moved_iterator ==
                    indices_.end())
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            internal_failure,
                        "Component storage lost the moved "
                        "entity index.");
                }

                moved_iterator->second =
                    removed_index;
            }

            indices_.erase(removed_iterator);
            components_.pop_back();
            entities_.pop_back();

            return {};
        }

        void clear() noexcept
        {
            indices_.clear();
            components_.clear();
            entities_.clear();
        }

        [[nodiscard]] size_type
        size() const noexcept
        {
            return components_.size();
        }

        [[nodiscard]] bool
        empty() const noexcept
        {
            return components_.empty();
        }

        [[nodiscard]] std::span<const EntityId>
        entities() const noexcept
        {
            return entities_;
        }

        [[nodiscard]] std::span<Component>
        components() noexcept
        {
            return components_;
        }

        [[nodiscard]] std::span<const Component>
        components() const noexcept
        {
            return components_;
        }

    private:
        std::vector<EntityId> entities_{};
        std::vector<Component> components_{};

        std::unordered_map<
            EntityId,
            size_type,
            EntityIdHash>
            indices_{};
    };
}