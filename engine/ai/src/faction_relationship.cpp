#include "oros/ai/faction_relationship.hpp"

#include <new>
#include <utility>

namespace oros::ai
{
    namespace
    {
        [[nodiscard]]
        bool
        is_valid_faction_relationship_disposition(
            const FactionRelationshipDisposition
                disposition)
            noexcept
        {
            switch (disposition)
            {
            case FactionRelationshipDisposition::
                neutral:
            case FactionRelationshipDisposition::
                allied:
            case FactionRelationshipDisposition::
                hostile:
                return true;

            case FactionRelationshipDisposition::
                invalid:
                return false;
            }

            return false;
        }
    }

    foundation::Result<
        FactionRelationship>
    FactionRelationship::create(
        const FactionKey& source,
        const FactionKey& target,
        const FactionRelationshipDisposition
            disposition)
    {
        if (
            !is_valid_faction_relationship_disposition(
                disposition))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Faction relationship requires "
                "a valid categorical "
                "disposition.");
        }

        if (source == target)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Faction relationship source "
                "and target must be different "
                "faction identities.");
        }

        try
        {
            FactionKey
                owned_source{
                    source
                };

            FactionKey
                owned_target{
                    target
                };

            return FactionRelationship{
                std::move(
                    owned_source),
                std::move(
                    owned_target),
                disposition
            };
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Faction relationship could not "
                "allocate its owned faction "
                "identities.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "Faction relationship "
                "construction failed.");
        }
    }

    const FactionKey&
    FactionRelationship::
        source()
        const noexcept
    {
        return source_;
    }

    const FactionKey&
    FactionRelationship::
        target()
        const noexcept
    {
        return target_;
    }

    FactionRelationshipDisposition
    FactionRelationship::
        disposition()
        const noexcept
    {
        return disposition_;
    }

    FactionRelationship::
        FactionRelationship(
            FactionKey source,
            FactionKey target,
            const FactionRelationshipDisposition
                disposition)
            noexcept
        : source_{
              std::move(source)
          },
          target_{
              std::move(target)
          },
          disposition_{disposition}
    {
    }
}
