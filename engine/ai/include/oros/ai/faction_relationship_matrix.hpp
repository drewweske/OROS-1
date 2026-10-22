#pragma once

#include "oros/ai/faction_relationship.hpp"
#include "oros/foundation/result.hpp"

#include <cstddef>
#include <span>
#include <type_traits>
#include <vector>

namespace oros::ai
{
    static_assert(
        std::is_nothrow_move_constructible_v<
            FactionRelationship>);

    static_assert(
        std::is_nothrow_move_assignable_v<
            FactionRelationship>);

    class FactionRelationshipMatrix final
    {
    public:
        FactionRelationshipMatrix() = default;

        FactionRelationshipMatrix(
            const FactionRelationshipMatrix&) =
                default;

        FactionRelationshipMatrix&
        operator=(
            const FactionRelationshipMatrix&) =
                default;

        FactionRelationshipMatrix(
            FactionRelationshipMatrix&&)
            noexcept = default;

        FactionRelationshipMatrix&
        operator=(
            FactionRelationshipMatrix&&)
            noexcept = default;

        [[nodiscard]]
        foundation::Status
        set_relationship(
            const FactionRelationship& relationship);

        [[nodiscard]]
        foundation::Status
        remove(
            const FactionKey& source,
            const FactionKey& target);

        [[nodiscard]]
        bool contains(
            const FactionKey& source,
            const FactionKey& target)
            const noexcept;

        [[nodiscard]]
        foundation::Result<
            FactionRelationshipDisposition>
        disposition(
            const FactionKey& source,
            const FactionKey& target) const;

        [[nodiscard]]
        std::span<
            const FactionRelationship>
        relationships_from(
            const FactionKey& source)
            const noexcept;

        [[nodiscard]]
        std::size_t
        size() const noexcept;

        [[nodiscard]]
        bool
        empty() const noexcept;

        [[nodiscard]]
        std::span<
            const FactionRelationship>
        relationships_in_canonical_order()
            const noexcept;

    private:
        [[nodiscard]]
        std::size_t
        lower_bound_index(
            const FactionKey& source,
            const FactionKey& target)
            const noexcept;

        [[nodiscard]]
        std::size_t
        lower_bound_source_index(
            const FactionKey& source)
            const noexcept;

        [[nodiscard]]
        std::size_t
        upper_bound_source_index(
            const FactionKey& source)
            const noexcept;

        std::vector<
            FactionRelationship>
            relationships_{};
    };
}
