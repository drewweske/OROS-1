#include "oros/rendering/debug_world_geometry.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <new>
#include <utility>
#include <vector>

namespace oros::rendering
{
    namespace
    {
        struct CameraPoint final
        {
            float x{};
            float vertical{};
            float depth{};
        };

        // Simple camera behind the player, pitched slightly downward.
        // Transform World-relative points by the input camera yaw.
        [[nodiscard]] CameraPoint to_camera(
            const float x, const float y, const float z,
            const float yaw) noexcept
        {
            constexpr float sine_pitch = 0.17F;
            constexpr float cosine_pitch = 0.985F;
            const float sine_yaw = std::sin(yaw);
            const float cosine_yaw = std::cos(yaw);
            const float sideways =
                cosine_yaw * x - sine_yaw * z;
            const float forward =
                sine_yaw * x + cosine_yaw * z + 9.0F;
            const float height = y - 2.8F;
            return {
                sideways,
                cosine_pitch * height + sine_pitch * forward,
                -sine_pitch * height + cosine_pitch * forward
            };
        }

        void line(
            std::vector<DebugWorldVertex>& vertices,
            const float x0, const float y0, const float z0,
            const float x1, const float y1, const float z1,
            const float aspect,
            const float yaw,
            const float red, const float green, const float blue)
        {
            constexpr float near_depth = 0.5F;
            CameraPoint a = to_camera(x0, y0, z0, yaw);
            CameraPoint b = to_camera(x1, y1, z1, yaw);

            if (a.depth < near_depth && b.depth < near_depth)
            {
                return;
            }

            // Clip the crossing segment to the positive near plane.
            if (a.depth < near_depth || b.depth < near_depth)
            {
                const float fraction =
                    (near_depth - a.depth) / (b.depth - a.depth);
                const CameraPoint clipped{
                    a.x + fraction * (b.x - a.x),
                    a.vertical + fraction *
                        (b.vertical - a.vertical),
                    near_depth
                };
                if (a.depth < near_depth)
                {
                    a = clipped;
                }
                else
                {
                    b = clipped;
                }
            }

            vertices.push_back({
                1.5F * a.x / (aspect * a.depth),
                1.5F * a.vertical / a.depth,
                red, green, blue, 1.0F
            });
            vertices.push_back({
                1.5F * b.x / (aspect * b.depth),
                1.5F * b.vertical / b.depth,
                red, green, blue, 1.0F
            });
        }
        struct LandmarkCorner final
        {
            float x{};
            float y{};
            float z{};
        };

        void wireframe_landmark(
            std::vector<DebugWorldVertex>& vertices,
            const DebugWorldLandmark& landmark,
            const float aspect,
            const float yaw)
        {
            const float left =
                landmark.relative_x - landmark.width * 0.5F;
            const float right =
                landmark.relative_x + landmark.width * 0.5F;
            const float front =
                landmark.relative_z - landmark.depth * 0.5F;
            const float back =
                landmark.relative_z + landmark.depth * 0.5F;
            const float top = landmark.height;
            const float roof = top + 0.40F * landmark.height;
            const float middle = landmark.relative_x;

            // Stable construction order for deterministic debug frames:
            // four lower corners, four upper corners, two roof peaks.
            const std::array<LandmarkCorner, 10U> corners{{
                {left, 0.0F, front}, {right, 0.0F, front},
                {right, 0.0F, back}, {left, 0.0F, back},
                {left, top, front}, {right, top, front},
                {right, top, back}, {left, top, back},
                {middle, roof, front}, {middle, roof, back}
            }};

            // Lower/upper rectangles, vertical posts and pitched roof.
            constexpr std::array<std::array<std::size_t, 2U>, 17U>
                edges{{
                    {{0U, 1U}}, {{1U, 2U}},
                    {{2U, 3U}}, {{3U, 0U}},
                    {{4U, 5U}}, {{5U, 6U}},
                    {{6U, 7U}}, {{7U, 4U}},
                    {{0U, 4U}}, {{1U, 5U}},
                    {{2U, 6U}}, {{3U, 7U}},
                    {{4U, 8U}}, {{5U, 8U}},
                    {{6U, 9U}}, {{7U, 9U}},
                    {{8U, 9U}}
                }};

            for (const auto& edge : edges)
            {
                const auto& a = corners[edge[0]];
                const auto& b = corners[edge[1]];
                line(
                    vertices,
                    a.x, a.y, a.z,
                    b.x, b.y, b.z,
                    aspect, yaw,
                    landmark.red,
                    landmark.green,
                    landmark.blue);
            }
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
            !std::isfinite(scene.camera_yaw_radians) ||
            scene.grid_phase_x < -5.0F ||
            scene.grid_phase_x > 5.0F ||
            scene.grid_phase_z < -5.0F ||
            scene.grid_phase_z > 5.0F ||
            scene.markers.size() > 32U ||
            scene.landmarks.size() > 12U)
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

        for (const DebugWorldLandmark& landmark : scene.landmarks)
        {
            if (
                !std::isfinite(landmark.relative_x) ||
                !std::isfinite(landmark.relative_z) ||
                !std::isfinite(landmark.width) ||
                !std::isfinite(landmark.depth) ||
                !std::isfinite(landmark.height) ||
                !std::isfinite(landmark.red) ||
                !std::isfinite(landmark.green) ||
                !std::isfinite(landmark.blue) ||
                landmark.width < 0.5F ||
                landmark.width > 30.0F ||
                landmark.depth < 0.5F ||
                landmark.depth > 30.0F ||
                landmark.height < 0.5F ||
                landmark.height > 30.0F ||
                landmark.red < 0.0F || landmark.red > 1.0F ||
                landmark.green < 0.0F || landmark.green > 1.0F ||
                landmark.blue < 0.0F || landmark.blue > 1.0F)
            {
                return foundation::fail(
                    foundation::ErrorCode::invalid_argument,
                    "Invalid debug-world landmark dimensions, "
                    "position or color.");
            }
        }

        try
        {
            std::vector<DebugWorldVertex> vertices{};
            vertices.reserve(
                80U + scene.markers.size() * 8U +
                    scene.landmarks.size() * 34U);

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
                    scene.camera_yaw_radians,
                    0.20F, 0.39F, 0.39F);

                line(
                    vertices,
                    -35.0F, 0.0F, offset - scene.grid_phase_z,
                    35.0F, 0.0F, offset - scene.grid_phase_z,
                    aspect_ratio,
                    scene.camera_yaw_radians,
                    0.20F, 0.39F, 0.39F);
            }

            // A short cyan cross marks the relative player origin.
            line(
                vertices,
                -0.6F, 0.04F, 0.0F,
                0.6F, 0.04F, 0.0F,
                aspect_ratio,
                scene.camera_yaw_radians,
                0.20F, 0.95F, 0.95F);
            line(
                vertices,
                0.0F, 0.04F, -0.6F,
                0.0F, 0.04F, 0.6F,
                aspect_ratio,
                scene.camera_yaw_radians,
                0.20F, 0.95F, 0.95F);

            // Visible NPCs are simple vertical wireframe posts.
            for (const DebugWorldMarker& marker : scene.markers)
            {
                line(
                    vertices,
                    marker.relative_x, 0.0F, marker.relative_z,
                    marker.relative_x, 2.0F, marker.relative_z,
                    aspect_ratio,
                    scene.camera_yaw_radians,
                    marker.red, marker.green, marker.blue);
                line(
                    vertices,
                    marker.relative_x - 0.4F, 1.6F,
                    marker.relative_z,
                    marker.relative_x + 0.4F, 1.6F,
                    marker.relative_z,
                    aspect_ratio,
                    scene.camera_yaw_radians,
                    marker.red, marker.green, marker.blue);
                line(
                    vertices,
                    marker.relative_x, 1.6F,
                    marker.relative_z - 0.4F,
                    marker.relative_x, 1.6F,
                    marker.relative_z + 0.4F,
                    aspect_ratio,
                    scene.camera_yaw_radians,
                    marker.red, marker.green, marker.blue);
            }

            // Decorative structures are relative to stable World
            // positions and do not claim collision authority.
            for (const DebugWorldLandmark& landmark : scene.landmarks)
            {
                wireframe_landmark(
                    vertices, landmark,
                    aspect_ratio, scene.camera_yaw_radians);
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
