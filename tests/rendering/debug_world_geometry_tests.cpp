#include "oros/rendering/debug_world_geometry.hpp"
#include "oros/foundation/error.hpp"

#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <string_view>

namespace
{
    struct Checks final { int count{}; int failures{}; };

    void check(
        Checks& state,
        const bool condition,
        const std::string_view message)
    {
        ++state.count;
        if (!condition)
        {
            ++state.failures;
            std::cerr << "[fail] " << message << '\n';
        }
    }
}

int main()
{
    using namespace oros::rendering;
    using namespace oros::foundation;

    Checks state{};
    const DebugWorldScene empty{};
    const auto grid = build_debug_world_lines(empty, 16.0F / 9.0F);
    check(state, grid.has_value(), "Finite grid geometry builds");
    if (!grid) { return 1; }

    check(state, grid->size() == 64U,
        "Ground grid and player reference generate 64 line vertices");
    bool finite = true;
    for (const auto& vertex : grid.value())
    {
        finite = finite &&
            std::isfinite(vertex.x) &&
            std::isfinite(vertex.y) &&
            std::isfinite(vertex.alpha);
    }
    check(state, finite, "All debug vertices project to finite coordinates");

    const std::array<DebugWorldMarker, 2> markers{{
        {4.0F, 10.0F, 1.0F, 0.4F, 0.2F},
        {-2.0F, 7.0F, 0.4F, 0.9F, 0.3F}
    }};
    DebugWorldScene scene{};
    scene.grid_phase_x = 1.5F;
    scene.grid_phase_z = -2.0F;
    scene.markers = markers;

    const auto with_npcs = build_debug_world_lines(
        scene, 16.0F / 9.0F);
    check(state,
        with_npcs.has_value() && with_npcs->size() == 76U,
        "Two NPC posts each add three colored line segments");
    if (!with_npcs) { return 1; }

    const auto repeated = build_debug_world_lines(scene, 16.0F / 9.0F);
    check(state,
        repeated.has_value() &&
        repeated->size() == with_npcs->size() &&
        repeated.value()[64].x == with_npcs.value()[64].x,
        "World projection is deterministic for identical inputs");

    DebugWorldScene moved = scene;
    moved.grid_phase_x = 3.0F;
    const auto shifted = build_debug_world_lines(moved, 16.0F / 9.0F);
    check(state,
        shifted.has_value() &&
        shifted.value()[0].x != with_npcs.value()[0].x,
        "Grid visibly shifts with World-relative player motion");

    const auto bad_aspect = build_debug_world_lines(empty, 0.0F);
    check(state,
        !bad_aspect.has_value() &&
        bad_aspect.error().code == ErrorCode::invalid_argument,
        "Zero viewport aspect is rejected");

    const auto bad_nan = build_debug_world_lines(
        empty, (std::numeric_limits<float>::quiet_NaN)());
    check(state,
        !bad_nan.has_value() &&
        bad_nan.error().code == ErrorCode::invalid_argument,
        "NaN aspect is rejected");

    DebugWorldScene bad_scene{};
    bad_scene.grid_phase_z =
        (std::numeric_limits<float>::infinity)();
    const auto bad_grid = build_debug_world_lines(bad_scene, 1.0F);
    check(state,
        !bad_grid.has_value() &&
        bad_grid.error().code == ErrorCode::invalid_argument,
        "Non-finite grid offset is rejected");

    std::array<DebugWorldMarker, 1> bad_marker{{
        {0.0F, 0.0F, 2.0F, 0.0F, 0.0F}
    }};
    bad_scene = {};
    bad_scene.markers = bad_marker;
    const auto bad_colors = build_debug_world_lines(bad_scene, 1.0F);
    check(state,
        !bad_colors.has_value() &&
        bad_colors.error().code == ErrorCode::invalid_argument,
        "Invalid NPC colors are rejected");

    std::cout << "Debug world geometry: "
              << state.count - state.failures << "/"
              << state.count << " passed\n";
    return state.failures == 0 ? 0 : 1;
}
