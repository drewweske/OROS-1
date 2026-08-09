#include "oros/ai/faction_trust.hpp"

#include <new>
#include <utility>

namespace oros::ai
{
    foundation::Result<
        FactionTrust>
    FactionTrust::create(
        const FactionKey& source,
        const FactionKey& target,
        const std::int32_t score)
    {
        if (
            score < minimum_score ||
            score > maximum_score)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Faction trust score must be "
                "within the inclusive supported "
                "trust range.");
        }

        if (source == target)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Faction trust source and target "
                "must be different faction "
                "identities.");
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

            return FactionTrust{
                std::move(
                    owned_source),
                std::move(
                    owned_target),
                score
            };
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Faction trust could not allocate "
                "its owned faction identities.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "Faction trust construction "
                "failed.");
        }
    }

    const FactionKey&
    FactionTrust::
        source()
        const noexcept
    {
        return source_;
    }

    const FactionKey&
    FactionTrust::
        target()
        const noexcept
    {
        return target_;
    }

    std::int32_t
    FactionTrust::
        score()
        const noexcept
    {
        return score_;
    }

    FactionTrust::
        FactionTrust(
            FactionKey source,
            FactionKey target,
            const std::int32_t score)
            noexcept
        : source_{
              std::move(source)
          },
          target_{
              std::move(target)
          },
          score_{score}
    {
    }
}
