#include "oros/ai/faction_trust_matrix.hpp"

#include <algorithm>
#include <cstddef>
#include <new>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace oros::ai
{
    namespace
    {
        [[nodiscard]]
        bool
        faction_key_less(
            const FactionKey& left,
            const FactionKey& right)
            noexcept
        {
            const std::string_view
                left_namespace =
                    left.faction_namespace();

            const std::string_view
                right_namespace =
                    right.faction_namespace();

            if (left_namespace < right_namespace)
            {
                return true;
            }

            if (right_namespace < left_namespace)
            {
                return false;
            }

            return
                left.faction_name() <
                right.faction_name();
        }

        [[nodiscard]]
        bool
        trust_matches_pair(
            const FactionTrust& trust,
            const FactionKey& source,
            const FactionKey& target)
            noexcept
        {
            return
                trust.source() == source &&
                trust.target() == target;
        }
    }

    foundation::Status
    FactionTrustMatrix::set_trust(
        const FactionTrust& trust)
    {
        const std::size_t index =
            lower_bound_index(
                trust.source(),
                trust.target());

        const bool existing =
            index < trusts_.size() &&
            trust_matches_pair(
                trusts_[index],
                trust.source(),
                trust.target());

        if (
            existing &&
            trusts_[index] ==
                trust)
        {
            return {};
        }

        using difference_type =
            std::vector<
                FactionTrust>::
                    difference_type;

        try
        {
            FactionTrust
                owned_trust{
                    trust
                };

            if (existing)
            {
                trusts_[index] =
                    std::move(
                        owned_trust);

                return {};
            }

            trusts_.insert(
                trusts_.begin() +
                    static_cast<
                        difference_type>(
                            index),
                std::move(
                    owned_trust));
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Faction trust matrix could not "
                "allocate storage for the trust "
                "fact.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "Faction trust matrix failed "
                "while setting a trust fact.");
        }

        return {};
    }

    foundation::Status
    FactionTrustMatrix::remove(
        const FactionKey& source,
        const FactionKey& target)
    {
        if (source == target)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Faction trust matrix removal "
                "requires different source and "
                "target factions.");
        }

        const std::size_t index =
            lower_bound_index(
                source,
                target);

        if (
            index >= trusts_.size() ||
            !trust_matches_pair(
                trusts_[index],
                source,
                target))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    not_found,
                "Faction trust matrix does not "
                "contain this directed trust "
                "fact.");
        }

        using difference_type =
            std::vector<
                FactionTrust>::
                    difference_type;

        trusts_.erase(
            trusts_.begin() +
                static_cast<
                    difference_type>(
                        index));

        return {};
    }

    bool
    FactionTrustMatrix::contains(
        const FactionKey& source,
        const FactionKey& target)
        const noexcept
    {
        if (source == target)
        {
            return false;
        }

        const std::size_t index =
            lower_bound_index(
                source,
                target);

        return
            index < trusts_.size() &&
            trust_matches_pair(
                trusts_[index],
                source,
                target);
    }

    foundation::Result<
        std::int32_t>
    FactionTrustMatrix::score(
        const FactionKey& source,
        const FactionKey& target) const
    {
        if (source == target)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Faction trust score query "
                "requires different source and "
                "target factions.");
        }

        const std::size_t index =
            lower_bound_index(
                source,
                target);

        if (
            index >= trusts_.size() ||
            !trust_matches_pair(
                trusts_[index],
                source,
                target))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    not_found,
                "Faction trust matrix does not "
                "contain this directed trust "
                "fact.");
        }

        return trusts_[index].score();
    }

    std::span<
        const FactionTrust>
    FactionTrustMatrix::trusts_from(
        const FactionKey& source)
        const noexcept
    {
        const std::size_t first =
            lower_bound_source_index(
                source);

        const std::size_t last =
            upper_bound_source_index(
                source);

        return
            std::span<
                const FactionTrust>{
                    trusts_
                }.
                subspan(
                    first,
                    last - first);
    }

    std::size_t
    FactionTrustMatrix::size()
        const noexcept
    {
        return trusts_.size();
    }

    bool
    FactionTrustMatrix::empty()
        const noexcept
    {
        return trusts_.empty();
    }

    std::span<
        const FactionTrust>
    FactionTrustMatrix::
        trusts_in_canonical_order()
        const noexcept
    {
        return
            std::span<
                const FactionTrust>{
                    trusts_
                };
    }

    std::size_t
    FactionTrustMatrix::
        lower_bound_index(
            const FactionKey& source,
            const FactionKey& target)
        const noexcept
    {
        const auto iterator =
            std::lower_bound(
                trusts_.begin(),
                trusts_.end(),
                target,
                [&source](
                    const FactionTrust& trust,
                    const FactionKey&
                        candidate_target)
                    noexcept
                {
                    if (
                        faction_key_less(
                            trust.source(),
                            source))
                    {
                        return true;
                    }

                    if (
                        faction_key_less(
                            source,
                            trust.source()))
                    {
                        return false;
                    }

                    return faction_key_less(
                        trust.target(),
                        candidate_target);
                });

        return static_cast<std::size_t>(
            iterator -
            trusts_.begin());
    }

    std::size_t
    FactionTrustMatrix::
        lower_bound_source_index(
            const FactionKey& source)
        const noexcept
    {
        const auto iterator =
            std::lower_bound(
                trusts_.begin(),
                trusts_.end(),
                source,
                [](
                    const FactionTrust& trust,
                    const FactionKey& candidate)
                    noexcept
                {
                    return faction_key_less(
                        trust.source(),
                        candidate);
                });

        return static_cast<std::size_t>(
            iterator -
            trusts_.begin());
    }

    std::size_t
    FactionTrustMatrix::
        upper_bound_source_index(
            const FactionKey& source)
        const noexcept
    {
        const auto iterator =
            std::upper_bound(
                trusts_.begin(),
                trusts_.end(),
                source,
                [](
                    const FactionKey& candidate,
                    const FactionTrust& trust)
                    noexcept
                {
                    return faction_key_less(
                        candidate,
                        trust.source());
                });

        return static_cast<std::size_t>(
            iterator -
            trusts_.begin());
    }
}
