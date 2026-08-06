#pragma once

#include "oros/foundation/result.hpp"
#include "oros/physics/broad_phase_pair.hpp"
#include "oros/physics/broad_phase_proxy.hpp"

#include <span>
#include <vector>

namespace oros::physics
{
    [[nodiscard]]
    foundation::Result<
        std::vector<BroadPhasePair>>
    generate_broad_phase_pairs(
        std::span<
            const BroadPhaseProxy>
            proxies);
}