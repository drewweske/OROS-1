#pragma once

#include "oros/foundation/result.hpp"

#include <span>
#include <vector>

namespace oros::rendering
{
    // Small Windows-first visual debug prototype. Distances are
    // in meters RELATIVE to the World-owned player position.
    struct DebugWorldMarker final
    {
        float relative_x{};
        float relative_z{};
        float red{0.90F};
        float green{0.72F};
        float blue{0.24F};
    };

    // Decorative landmark at a caller-provided World-relative offset.
    // Purely visual for now; has NO collider, save-state or AI authority.
    struct DebugWorldLandmark final
    {
        float relative_x{};
        float relative_z{};
        float width{5.0F};
        float depth{5.0F};
        float height{6.0F};
        float red{0.95F};
        float green{0.64F};
        float blue{0.24F};
    };

    struct DebugWorldScene final
    {
        // Ground-grid offset keeps lines fixed in world space as the
        // first-person player moves. Period is exactly five meters.
        float grid_phase_x{};
        float grid_phase_z{};
        // Positive yaw points the camera toward World +X.
        float camera_yaw_radians{};
        std::span<const DebugWorldMarker> markers{};
        std::span<const DebugWorldLandmark> landmarks{};
    };

    // Streamed D3D12 line-list vertex: screen-space coordinate and
    // unlit RGBA color. No renderer-side World or player ownership.
    struct DebugWorldVertex final
    {
        float x{};
        float y{};
        float red{};
        float green{};
        float blue{};
        float alpha{1.0F};
        // D3D12 normalized camera-space depth. Line decorations
        // test against opaque geometry without writing depth.
        float depth{};
    };

    // Drawn as a GPU triangle-list before line decorations. Colors
    // and depths are produced by pure deterministic CPU projection.
    struct DebugWorldSolidVertex final
    {
        float x{};
        float y{};
        float depth{};
        float red{};
        float green{};
        float blue{};
        float alpha{1.0F};
    };

    // Pure deterministic camera projection, testable without a GPU.
    // Scene coordinates remain relative to the player.
    [[nodiscard]]
    foundation::Result<std::vector<DebugWorldVertex>>
    build_debug_world_lines(
        const DebugWorldScene& scene,
        float aspect_ratio);

    // Bounded, near-clipped flat-shaded ground and building faces.
    [[nodiscard]]
    foundation::Result<std::vector<DebugWorldSolidVertex>>
    build_debug_world_solids(
        const DebugWorldScene& scene,
        float aspect_ratio);
}
