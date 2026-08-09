#include "oros/ai/faction_relationship_matrix.hpp"

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
        relationship_matches_pair(
            const FactionRelationship& relationship,
            const FactionKey& source,
            const FactionKey& target)
            noexcept
        {
            return
                relationship.source() == source &&
                relationship.target() == target;
        }
    }

    foundation::Status
    FactionRelationshipMatrix::
        set_relationship(
            const FactionRelationship& relationship)
    {
        const std::size_t index =
            lower_bound_index(
                relationship.source(),
                relationship.target());

        const bool existing =
            index < relationships_.size() &&
            relationship_matches_pair(
                relationships_[index],
                relationship.source(),
                relationship.target());

        if (
            existing &&
            relationships_[index] ==
                relationship)
        {
            return {};
        }

        using difference_type =
            std::vector<
                FactionRelationship>::
                    difference_type;

        try
        {
            FactionRelationship
                owned_relationship{
                    relationship
                };

            if (existing)
            {
                relationships_[index] =
                    std::move(
                        owned_relationship);

                return {};
            }

            relationships_.insert(
                relationships_.begin() +
                    static_cast<
                        difference_type>(
                            index),
                std::move(
                    owned_relationship));
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Faction relationship matrix "
                "could not allocate storage for "
                "the relationship.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "Faction relationship matrix "
                "failed while setting a "
                "relationship.");
        }

        return {};
    }

    foundation::Status
    FactionRelationshipMatrix::remove(
        const FactionKey& source,
        const FactionKey& target)
    {
        if (source == target)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Faction relationship matrix "
                "removal requires different "
                "source and target factions.");
        }

        const std::size_t index =
            lower_bound_index(
                source,
                target);

        if (
            index >= relationships_.size() ||
            !relationship_matches_pair(
                relationships_[index],
                source,
                target))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    not_found,
                "Faction relationship matrix "
                "does not contain this directed "
                "relationship.");
        }

        using difference_type =
            std::vector<
                FactionRelationship>::
                    difference_type;

        relationships_.erase(
            relationships_.begin() +
                static_cast<
                    difference_type>(
                        index));

        return {};
    }

    bool
    FactionRelationshipMatrix::contains(
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
            index < relationships_.size() &&
            relationship_matches_pair(
                relationships_[index],
                source,
                target);
    }

    foundation::Result<
        FactionRelationshipDisposition>
    FactionRelationshipMatrix::disposition(
        const FactionKey& source,
        const FactionKey& target) const
    {
        if (source == target)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Faction relationship "
                "disposition query requires "
                "different source and target "
                "factions.");
        }

        const std::size_t index =
            lower_bound_index(
                source,
                target);

        if (
            index >= relationships_.size() ||
            !relationship_matches_pair(
                relationships_[index],
                source,
                target))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    not_found,
                "Faction relationship matrix "
                "does not contain this directed "
                "relationship.");
        }

        return relationships_[index].
            disposition();
    }

    std::span<
        const FactionRelationship>
    FactionRelationshipMatrix::
        relationships_from(
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
                const FactionRelationship>{
                    relationships_
                }.
                subspan(
                    first,
                    last - first);
    }

    std::size_t
    FactionRelationshipMatrix::size()
        const noexcept
    {
        return relationships_.size();
    }

    bool
    FactionRelationshipMatrix::empty()
        const noexcept
    {
        return relationships_.empty();
    }

    std::span<
        const FactionRelationship>
    FactionRelationshipMatrix::
        relationships_in_canonical_order()
        const noexcept
    {
        return
            std::span<
                const FactionRelationship>{
                    relationships_
                };
    }

    std::size_t
    FactionRelationshipMatrix::
        lower_bound_index(
            const FactionKey& source,
            const FactionKey& target)
        const noexcept
    {
        const auto iterator =
            std::lower_bound(
                relationships_.begin(),
                relationships_.end(),
                target,
                [&source](
                    const FactionRelationship&
                        relationship,
                    const FactionKey&
                        candidate_target)
                    noexcept
                {
                    if (
                        faction_key_less(
                            relationship.source(),
                            source))
                    {
                        return true;
                    }

                    if (
                        faction_key_less(
                            source,
                            relationship.source()))
                    {
                        return false;
                    }

                    return faction_key_less(
                        relationship.target(),
                        candidate_target);
                });

        return static_cast<std::size_t>(
            iterator -
            relationships_.begin());
    }

    std::size_t
    FactionRelationshipMatrix::
        lower_bound_source_index(
            const FactionKey& source)
        const noexcept
    {
        const auto iterator =
            std::lower_bound(
                relationships_.begin(),
                relationships_.end(),
                source,
                [](
                    const FactionRelationship&
                        relationship,
                    const FactionKey& candidate)
                    noexcept
                {
                    return faction_key_less(
                        relationship.source(),
                        candidate);
                });

        return static_cast<std::size_t>(
            iterator -
            relationships_.begin());
    }

    std::size_t
    FactionRelationshipMatrix::
        upper_bound_source_index(
            const FactionKey& source)
        const noexcept
    {
        const auto iterator =
            std::upper_bound(
                relationships_.begin(),
                relationships_.end(),
                source,
                [](
                    const FactionKey& candidate,
                    const FactionRelationship&
                        relationship)
                    noexcept
                {
                    return faction_key_less(
                        candidate,
                        relationship.source());
                });

        return static_cast<std::size_t>(
            iterator -
            relationships_.begin());
    }
}
