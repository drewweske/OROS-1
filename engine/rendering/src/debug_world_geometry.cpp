#include "oros/rendering/debug_world_geometry.hpp"

#include <cmath>
#include <cstddef>
#include <new>
#include <utility>
#include <vector>

namespace oros::rendering
{
    namespace
    {
        struct Projection final
        {
            float x{};
            float y{};
        };

        // Camera is just behind and above a player facing +Z.
        // Simple perspective is sufficient for a visible graybox.
        [[nodiscard]] Projection project(
            const float x,
            const float y,
            const float z,
            const float aspect) noexcept
        {
            constexpr float sine = 0.17F;
            constexpr float cosine = 0.985F;
            const float height = y - 2.8F;
            const float forward = z + 9.0F;
            const float depth =
                -sine * height + cosine * forward;
            const float vertical =
                cosine * height + sine * forward;

            return {
                1.5F * x / (aspect * depth),
                1.5F * vertical / depth
            };
        }

        void line(
            std::vector<DebugWorldVertex>& vertices,
            const float x0, const float y0, const float z0,
            const float x1, const float y1, const float z1,
            const float aspect,
            const float red, const float green, const float blue)
        {
            const Projection a = project(x0, y0, z0, aspect);
            const Projection b = project(x1, y1, z1, aspect);
            vertices.push_back({
                a.x, a.y, red, green, blue, 1.0F
            });
            vertices.push_back({
                b.x, b.y, red, green, blue, 1.0F
            });
        }
    }

    foundation::Result<std::vector<DebugWorldVertex>>
    build_debug_world_lines(
        const DebugWorldScene& scene,
        const float aspect_ratio)
    {
        if (
            !std::isfinite(aspect_ratio) ||
            aspect_ratio <= 0.0F ||
            !std::isfinite(scene.grid_phase_x) ||
            !std::isfinite(scene.grid_phase_z) ||
            scene.grid_phase_x < -5.0F ||
            scene.grid_phase_x > 5.0F ||
            scene.grid_phase_z < -5.0F ||
            scene.grid_phase_z > 5.0F ||
            scene.markers.size() > 32U)
        {
            return foundation::fail(
                foundation::ErrorCode::invalid_argument,
                "Invalid debug-world aspect, grid phase or marker count.");
        }

        for (const DebugWorldMarker& marker : scene.markers)
        {
            if (
                !std::isfinite(marker.relative_x) ||
                !std::isfinite(marker.relative_z) ||
                !std::isfinite(marker.red) ||
                !std::isfinite(marker.green) ||
                !std::isfinite(marker.blue) ||
                marker.red < 0.0F || marker.red > 1.0F ||
                marker.green < 0.0F || marker.green > 1.0F ||
                marker.blue < 0.0F || marker.blue > 1.0F)
            {
                return foundation::fail(
                    foundation::ErrorCode::invalid_argument,
                    "Invalid debug-world marker position or color.");
            }
        }

        try
        {
            std::vector<DebugWorldVertex> vertices{};
            vertices.reserve(
                80U + scene.markers.size() * 8U);

            // Lines intersect on a World-anchored five-meter grid.
            // Fixed geometry bounds avoid infinite or huge draws.
            for (int n = -7; n <= 7; ++n)
            {
                const float offset =
                    static_cast<float>(n) * 5.0F;

                line(
                    vertices,
                    offset - scene.grid_phase_x, 0.0F, -4.0F,
                    offset - scene.grid_phase_x, 0.0F, 68.0F,
                    aspect_ratio,
                    0.20F, 0.39F, 0.39F);

                line(
                    vertices,
                    -35.0F, 0.0F, offset - scene.grid_phase_z,
                    35.0F, 0.0F, offset - scene.grid_phase_z,
                    aspect_ratio,
                    0.20F, 0.39F, 0.39F);
            }

            // A short cyan cross marks the relative player origin.
            line(
                vertices,
                -0.6F, 0.04F, 0.0F,
                0.6F, 0.04F, 0.0F,
                aspect_ratio,
                0.20F, 0.95F, 0.95F);
            line(
                vertices,
                0.0F, 0.04F, -0.6F,
                0.0F, 0.04F, 0.6F,
                aspect_ratio,
                0.20F, 0.95F, 0.95F);

            // Visible NPCs are simple vertical wireframe posts.
            for (const DebugWorldMarker& marker : scene.markers)
            {
                line(
                    vertices,
                    marker.relative_x, 0.0F, marker.relative_z,
                    marker.relative_x, 2.0F, marker.relative_z,
                    aspect_ratio,
                    marker.red, marker.green, marker.blue);
                line(
                    vertices,
                    marker.relative_x - 0.4F, 1.6F,
                    marker.relative_z,
                    marker.relative_x + 0.4F, 1.6F,
                    marker.relative_z,
                    aspect_ratio,
                    marker.red, marker.green, marker.blue);
                line(
                    vertices,
                    marker.relative_x, 1.6F,
                    marker.relative_z - 0.4F,
                    marker.relative_x, 1.6F,
                    marker.relative_z + 0.4F,
                    aspect_ratio,
                    marker.red, marker.green, marker.blue);
            }

            return std::move(vertices);
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::out_of_memory,
                "Debug-world geometry ran out of memory.");
        }
    }
}
