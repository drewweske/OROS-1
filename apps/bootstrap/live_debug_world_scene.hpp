#pragma once

#include "oros/foundation/result.hpp"
#include "oros/rendering/debug_world_geometry.hpp"
#include "oros/world/world_position.hpp"

#include <array>

namespace oros::bootstrap
{
    // Owns the storage referenced by the temporary DebugWorldScene.
    // A returned scene() must not outlive this frame. Keeping the
    // arrays here avoids dangling spans from a local render function.
    struct LiveDebugWorldFrame final
    {
        float grid_phase_x{};
        float grid_phase_z{};
        float camera_yaw_radians{};
        std::array<rendering::DebugWorldMarker, 2U> markers{};
        std::array<rendering::DebugWorldLandmark, 3U> landmarks{};

        [[nodiscard]] rendering::DebugWorldScene
        scene() const noexcept
        {
            return rendering::DebugWorldScene{
                grid_phase_x,
                grid_phase_z,
                camera_yaw_radians,
                markers,
                landmarks
            };
        }
    };

    // Pure bootstrap composition: all positions are World-owned and
    // observed by value. Buildings are anchored to the same absolute
    // World origin as the resident collision geometry, NOT player spawn.
    [[nodiscard]]
    foundation::Result<LiveDebugWorldFrame>
    build_live_debug_world_frame(
        const world::WorldPosition& player,
        const std::array<const world::WorldPosition*, 2U>& actors,
        float camera_yaw_radians);
}
