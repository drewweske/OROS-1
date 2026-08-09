#pragma once

#include "oros/ai/faction_trust.hpp"
#include "oros/foundation/result.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>
#include <vector>

namespace oros::ai
{
    static_assert(
        std::is_nothrow_move_constructible_v<
            FactionTrust>);

    static_assert(
        std::is_nothrow_move_assignable_v<
            FactionTrust>);

    class FactionTrustMatrix final
    {
    public:
        FactionTrustMatrix() = default;

        FactionTrustMatrix(
            const FactionTrustMatrix&) =
                default;

        FactionTrustMatrix&
        operator=(
            const FactionTrustMatrix&) =
                default;

        FactionTrustMatrix(
            FactionTrustMatrix&&)
            noexcept = default;

        FactionTrustMatrix&
        operator=(
            FactionTrustMatrix&&)
            noexcept = default;

        [[nodiscard]]
        foundation::Status
        set_trust(
            const FactionTrust& trust);

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
            std::int32_t>
        score(
            const FactionKey& source,
            const FactionKey& target) const;

        [[nodiscard]]
        std::span<
            const FactionTrust>
        trusts_from(
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
            const FactionTrust>
        trusts_in_canonical_order()
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
            FactionTrust>
            trusts_{};
    };
}
