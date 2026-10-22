#include "live_debug_world_scene.hpp"
#include "outpost_layout.hpp"

#include "oros/foundation/error.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace oros::bootstrap
{
    namespace
    {
        [[nodiscard]] float
        grid_phase(const std::int64_t cell, const double local)
            noexcept
        {
            // 1024 meters per cell: 1024 % 5 == 4.
            // Reduce the cell first to avoid giant imprecise floats.
            const double raw = std::fmod(
                static_cast<double>(cell % 5LL) * 4.0 +
                local, 5.0);
            return static_cast<float>(
                raw < 0.0 ? raw + 5.0 : raw);
        }

        [[nodiscard]] bool
        representable(const double value) noexcept
        {
            return std::isfinite(value) &&
                std::isfinite(static_cast<float>(value));
        }
    }

    foundation::Result<LiveDebugWorldFrame>
    build_live_debug_world_frame(
        const world::WorldPosition& player,
        const std::array<const world::WorldPosition*, 2U>& actors,
        const float camera_yaw_radians)
    {
        if (!player.is_normalized() ||
            !std::isfinite(camera_yaw_radians))
        {
            return foundation::fail(
                foundation::ErrorCode::invalid_argument,
                "Visual scene requires a normalized player "
                "and finite camera yaw.");
        }

        const auto origin = world::WorldPosition::origin();
        if (!origin.has_value())
        {
            return foundation::fail(
                origin.error().code, origin.error().message);
        }

        // Static collision volumes in physical_world_demo.cpp are
        // authored in WorldCell{0,0,0} at these exact XZ offsets.
        const auto outpost = player.displacement_to(origin.value());
        if (!outpost.has_value() ||
            !representable(outpost.value().x) ||
            !representable(outpost.value().z))
        {
            return foundation::fail(
                foundation::ErrorCode::invalid_argument,
                "Outpost anchor is not representable in this view.");
        }

        LiveDebugWorldFrame frame{};
        frame.camera_yaw_radians = camera_yaw_radians;
        frame.grid_phase_x =
            grid_phase(player.cell().x, player.local().x);
        frame.grid_phase_z =
            grid_phase(player.cell().z, player.local().z);

        for (std::size_t i = 0U; i < actors.size(); ++i)
        {
            if (actors[i] == nullptr ||
                !actors[i]->is_normalized())
            {
                return foundation::fail(
                    foundation::ErrorCode::invalid_argument,
                    "Visual scene requires two World-owned actor positions.");
            }
            const auto delta =
                player.displacement_to(*actors[i]);
            if (!delta.has_value() ||
                !representable(delta.value().x) ||
                !representable(delta.value().z))
            {
                return foundation::fail(
                    foundation::ErrorCode::invalid_argument,
                    "NPC position is not representable in this view.");
            }

            frame.markers[i] = rendering::DebugWorldMarker{
                static_cast<float>(delta.value().x),
                static_cast<float>(delta.value().z),
                i == 0U ? 1.0F : 0.45F,
                i == 0U ? 0.66F : 1.0F,
                i == 0U ? 0.20F : 0.42F
            };
        }

        for (std::size_t i = 0U;
             i < outpost_buildings.size(); ++i)
        {
            const OutpostBuildingDescription& b =
                outpost_buildings[i];
            const double x = outpost.value().x + b.center_x;
            const double z = outpost.value().z + b.center_z;
            if (!representable(x) || !representable(z))
            {
                return foundation::fail(
                    foundation::ErrorCode::invalid_argument,
                    "Building exceeds the visual view coordinate range.");
            }
            frame.landmarks[i] = rendering::DebugWorldLandmark{
                static_cast<float>(x),
                static_cast<float>(z),
                static_cast<float>(b.width),
                static_cast<float>(b.depth),
                static_cast<float>(b.height),
                b.red, b.green, b.blue
            };
        }
        return frame;
    }
}
