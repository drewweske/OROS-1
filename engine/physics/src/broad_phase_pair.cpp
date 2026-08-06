#include "oros/physics/broad_phase_pair.hpp"

#include <utility>

namespace oros::physics
{
    BroadPhasePair::BroadPhasePair(
        const ColliderId first_collider,
        const ColliderId second_collider)
        noexcept
        : first_collider_{first_collider},
          second_collider_{second_collider}
    {
    }

    foundation::Result<
        BroadPhasePair>
    BroadPhasePair::create(
        ColliderId first_collider,
        ColliderId second_collider)
    {
        if (!first_collider.is_valid() ||
            !second_collider.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Broad-phase pair requires two "
                "valid persistent collider "
                "identities.");
        }

        if (first_collider ==
            second_collider)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Broad-phase pair requires two "
                "distinct collider identities.");
        }

        if (second_collider <
            first_collider)
        {
            std::swap(
                first_collider,
                second_collider);
        }

        return BroadPhasePair{
            first_collider,
            second_collider
        };
    }

    ColliderId
    BroadPhasePair::first_collider()
        const noexcept
    {
        return first_collider_;
    }

    ColliderId
    BroadPhasePair::second_collider()
        const noexcept
    {
        return second_collider_;
    }

    bool
    BroadPhasePair::contains(
        const ColliderId collider)
        const noexcept
    {
        return
            collider ==
                first_collider_ ||
            collider ==
                second_collider_;
    }

    bool
    BroadPhasePair::operator<(
        const BroadPhasePair& other)
        const noexcept
    {
        if (first_collider_ !=
            other.first_collider_)
        {
            return
                first_collider_ <
                other.first_collider_;
        }

        return
            second_collider_ <
            other.second_collider_;
    }
}