#include "oros/physics/broad_phase.hpp"

#include <algorithm>
#include <vector>

namespace oros::physics
{
    namespace
    {
        [[nodiscard]]
        bool collider_identity_less(
            const BroadPhaseProxy* left,
            const BroadPhaseProxy* right)
            noexcept
        {
            return
                left->collider() <
                right->collider();
        }

        [[nodiscard]]
        bool sweep_order_less(
            const BroadPhaseProxy* left,
            const BroadPhaseProxy* right)
            noexcept
        {
            const PhysicsScalar left_minimum_x =
                left->bounds().minimum().x;

            const PhysicsScalar right_minimum_x =
                right->bounds().minimum().x;

            if (left_minimum_x !=
                right_minimum_x)
            {
                return
                    left_minimum_x <
                    right_minimum_x;
            }

            const PhysicsScalar left_maximum_x =
                left->bounds().maximum().x;

            const PhysicsScalar right_maximum_x =
                right->bounds().maximum().x;

            if (left_maximum_x !=
                right_maximum_x)
            {
                return
                    left_maximum_x <
                    right_maximum_x;
            }

            return
                left->collider() <
                right->collider();
        }
    }

    foundation::Result<
        std::vector<BroadPhasePair>>
    generate_broad_phase_pairs(
        const std::span<
            const BroadPhaseProxy>
            proxies)
    {
        std::vector<
            const BroadPhaseProxy*>
            ordered_proxies;

        ordered_proxies.reserve(
            proxies.size());

        for (const BroadPhaseProxy& proxy :
             proxies)
        {
            ordered_proxies.push_back(
                &proxy);
        }

        std::sort(
            ordered_proxies.begin(),
            ordered_proxies.end(),
            collider_identity_less);

        for (std::size_t index = 1U;
             index < ordered_proxies.size();
             ++index)
        {
            if (ordered_proxies[index - 1U]->
                    collider() ==
                ordered_proxies[index]->
                    collider())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Broad-phase generation "
                    "requires unique persistent "
                    "collider identities.");
            }
        }

        std::sort(
            ordered_proxies.begin(),
            ordered_proxies.end(),
            sweep_order_less);

        std::vector<BroadPhasePair> pairs;

        for (std::size_t first_index = 0U;
             first_index <
                 ordered_proxies.size();
             ++first_index)
        {
            const BroadPhaseProxy& first_proxy =
                *ordered_proxies[
                    first_index];

            const PhysicsScalar first_maximum_x =
                first_proxy.
                    bounds().
                    maximum().
                    x;

            for (std::size_t second_index =
                     first_index + 1U;
                 second_index <
                     ordered_proxies.size();
                 ++second_index)
            {
                const BroadPhaseProxy&
                    second_proxy =
                        *ordered_proxies[
                            second_index];

                if (second_proxy.
                        bounds().
                        minimum().
                        x >
                    first_maximum_x)
                {
                    break;
                }

                if (!first_proxy.
                        bounds().
                        overlaps(
                            second_proxy.
                                bounds()))
                {
                    continue;
                }

                const auto pair_result =
                    BroadPhasePair::create(
                        first_proxy.collider(),
                        second_proxy.collider());

                if (!pair_result.has_value())
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "Broad-phase generation "
                        "could not create a "
                        "canonical collider pair.");
                }

                pairs.push_back(
                    pair_result.value());
            }
        }

        std::sort(
            pairs.begin(),
            pairs.end());

        return pairs;
    }
}