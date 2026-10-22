#include "oros/rendering/debug_world_geometry.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
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

        inline constexpr float near_plane = 0.5F;
        inline constexpr float far_plane = 256.0F;

        [[nodiscard]] float normalized_depth(
            const float distance) noexcept
        {
            return far_plane / (far_plane - near_plane) -
                (far_plane * near_plane) /
                    ((far_plane - near_plane) * distance);
        }

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
            constexpr float near_depth = near_plane;
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
                red, green, blue, 1.0F,
                normalized_depth(a.depth)
            });
            vertices.push_back({
                1.5F * b.x / (aspect * b.depth),
                1.5F * b.vertical / b.depth,
                red, green, blue, 1.0F,
                normalized_depth(b.depth)
            });
        }
        struct LandmarkCorner final
        {
            float x{};
            float y{};
            float z{};
        };

        void solid_triangle(
            std::vector<DebugWorldSolidVertex>& vertices,
            const std::array<LandmarkCorner, 3U>& world_points,
            const float aspect, const float yaw,
            const float red, const float green, const float blue)
        {
            std::array<CameraPoint, 3U> in{{
                to_camera(
                    world_points[0].x, world_points[0].y,
                    world_points[0].z, yaw),
                to_camera(
                    world_points[1].x, world_points[1].y,
                    world_points[1].z, yaw),
                to_camera(
                    world_points[2].x, world_points[2].y,
                    world_points[2].z, yaw)
            }};
            std::array<CameraPoint, 4U> clipped{};
            std::size_t count{};
            for (std::size_t i = 0U; i < in.size(); ++i)
            {
                const CameraPoint& a = in[i];
                const CameraPoint& b = in[(i + 1U) % in.size()];
                const bool inside_a = a.depth >= near_plane;
                const bool inside_b = b.depth >= near_plane;
                if (inside_a != inside_b)
                {
                    const float fraction =
                        (near_plane - a.depth) /
                            (b.depth - a.depth);
                    clipped[count++] = CameraPoint{
                        a.x + fraction * (b.x - a.x),
                        a.vertical + fraction *
                            (b.vertical - a.vertical),
                        near_plane
                    };
                }
                if (inside_b)
                {
                    clipped[count++] = b;
                }
            }

            if (count < 3U)
            {
                return;
            }
            for (std::size_t i = 1U; i + 1U < count; ++i)
            {
                const std::array<CameraPoint, 3U> triangle{{
                    clipped[0U], clipped[i], clipped[i + 1U]
                }};
                for (const CameraPoint& p : triangle)
                {
                    vertices.push_back({
                        1.5F * p.x / (aspect * p.depth),
                        1.5F * p.vertical / p.depth,
                        normalized_depth(p.depth),
                        red, green, blue, 1.0F
                    });
                }
            }
        }

        void solid_quad(
            std::vector<DebugWorldSolidVertex>& vertices,
            const LandmarkCorner& a,
            const LandmarkCorner& b,
            const LandmarkCorner& c,
            const LandmarkCorner& d,
            const float aspect, const float yaw,
            const float red, const float green, const float blue)
        {
            solid_triangle(
                vertices, {{a, b, c}}, aspect, yaw,
                red, green, blue);
            solid_triangle(
                vertices, {{a, c, d}}, aspect, yaw,
                red, green, blue);
        }

        void solid_building(
            std::vector<DebugWorldSolidVertex>& vertices,
            const DebugWorldLandmark& landmark,
            const float aspect, const float yaw)
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
            const float peak = top + landmark.height * 0.40F;
            const float center = landmark.relative_x;

            // Match the same vertices as wireframe_landmark, so
            // opaque silhouettes and physical wall-box bounds agree.
            const std::array<LandmarkCorner, 10U> c{{
                {left, 0.0F, front}, {right, 0.0F, front},
                {right, 0.0F, back}, {left, 0.0F, back},
                {left, top, front}, {right, top, front},
                {right, top, back}, {left, top, back},
                {center, peak, front}, {center, peak, back}
            }};
            const float red = landmark.red;
            const float green = landmark.green;
            const float blue = landmark.blue;
            solid_quad(vertices, c[0], c[1], c[5], c[4],
                aspect, yaw, red * 0.83F, green * 0.83F,
                blue * 0.83F);
            solid_quad(vertices, c[1], c[2], c[6], c[5],
                aspect, yaw, red * 0.61F, green * 0.61F,
                blue * 0.61F);
            solid_quad(vertices, c[2], c[3], c[7], c[6],
                aspect, yaw, red * 0.70F, green * 0.70F,
                blue * 0.70F);
            solid_quad(vertices, c[3], c[0], c[4], c[7],
                aspect, yaw, red * 0.56F, green * 0.56F,
                blue * 0.56F);
            solid_quad(vertices, c[4], c[8], c[9], c[7],
                aspect, yaw, red * 0.96F, green * 0.96F,
                blue * 0.96F);
            solid_quad(vertices, c[8], c[5], c[6], c[9],
                aspect, yaw, red * 0.75F, green * 0.75F,
                blue * 0.75F);
            solid_triangle(vertices, {{c[4], c[5], c[8]}},
                aspect, yaw, red * 0.74F, green * 0.74F,
                blue * 0.74F);
            solid_triangle(vertices, {{c[7], c[9], c[6]}},
                aspect, yaw, red * 0.67F, green * 0.67F,
                blue * 0.67F);
        }



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
    foundation::Result<std::vector<DebugWorldSolidVertex>>
    build_debug_world_solids(
        const DebugWorldScene& scene,
        const float aspect_ratio)
    {
        // Share the strict scene/parameter validation contract with
        // the wireframe path until a dedicated frame composer exists.
        const auto checked = build_debug_world_lines(
            scene, aspect_ratio);
        if (!checked.has_value())
        {
            return foundation::fail(
                checked.error().code,
                checked.error().message);
        }
        try
        {
            std::vector<DebugWorldSolidVertex> vertices{};
            // Ground: 2 triangles. Each gabled building: 14 triangles.
            // Near-plane splits can double some triangles.
            vertices.reserve(
                12U + 90U * scene.landmarks.size());

            // Opaque world-relative ground slab. The grid overlay
            // still uses World-cell phase and is rendered afterward.
            const std::array<LandmarkCorner, 4U> ground{{
                {-40.0F, -0.04F, -35.0F},
                {40.0F, -0.04F, -35.0F},
                {40.0F, -0.04F, 85.0F},
                {-40.0F, -0.04F, 85.0F}
            }};
            solid_quad(
                vertices, ground[0], ground[1],
                ground[2], ground[3], aspect_ratio,
                scene.camera_yaw_radians,
                0.055F, 0.13F, 0.17F);

            for (const DebugWorldLandmark& building :
                 scene.landmarks)
            {
                solid_building(
                    vertices, building,
                    aspect_ratio, scene.camera_yaw_radians);
            }
            return std::move(vertices);
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::out_of_memory,
                "Solid world projection ran out of memory.");
        }
    }
}
