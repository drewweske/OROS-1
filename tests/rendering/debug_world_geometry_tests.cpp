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

    check(state,
        grid->size() >= 40U &&
        grid->size() <= 64U &&
        grid->size() % 2U == 0U,
        "Near-clipped ground grid generates bounded line pairs");
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

    // Clipping changes the number of visible ground segments when
    // the grid phase changes. Compare the *same* phase, not the
    // unshifted grid from the beginning of the test.
    DebugWorldScene same_grid = scene;
    same_grid.markers = {};
    const auto phased_grid = build_debug_world_lines(
        same_grid, 16.0F / 9.0F);
    check(state,
        phased_grid.has_value() && !phased_grid->empty(),
        "Ground geometry with shifted grid phase builds");
    if (!phased_grid) { return 1; }

    const auto with_npcs = build_debug_world_lines(
        scene, 16.0F / 9.0F);
    check(state,
        with_npcs.has_value() &&
        with_npcs->size() == phased_grid->size() + 12U,
        "Two NPC posts each add three colored line segments");
    if (!with_npcs) { return 1; }

    const auto repeated = build_debug_world_lines(scene, 16.0F / 9.0F);
    check(state,
        repeated.has_value() &&
        repeated->size() == with_npcs->size() &&
        repeated.value()[phased_grid->size()].x ==
            with_npcs.value()[phased_grid->size()].x,
        "World projection is deterministic for identical inputs");

    DebugWorldScene moved = scene;
    moved.grid_phase_x = 3.0F;
    const auto shifted = build_debug_world_lines(moved, 16.0F / 9.0F);
    check(state,
        shifted.has_value() &&
        shifted.value()[0].x != with_npcs.value()[0].x,
        "Grid visibly shifts with World-relative player motion");

    DebugWorldScene rotated = scene;
    rotated.camera_yaw_radians = 0.7F;
    const auto turned = build_debug_world_lines(
        rotated, 16.0F / 9.0F);
    check(state,
        turned.has_value() && !turned->empty() &&
        turned.value()[0].x != with_npcs.value()[0].x,
        "Rotating camera turns the visible World grid");

    DebugWorldScene invalid_yaw = scene;
    invalid_yaw.camera_yaw_radians =
        (std::numeric_limits<float>::infinity)();
    const auto rejected_yaw =
        build_debug_world_lines(invalid_yaw, 1.0F);
    check(state,
        !rejected_yaw.has_value() &&
        rejected_yaw.error().code == ErrorCode::invalid_argument,
        "Non-finite camera yaw is rejected");

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

    // A landmark's 17 wireframe edges are 34 projected vertices
    // when fully ahead of the camera.
    const std::array<DebugWorldLandmark, 2> buildings{{
        {12.0F, 25.0F, 7.0F, 6.0F, 8.0F,
         1.0F, 0.65F, 0.24F},
        {-12.0F, 32.0F, 9.0F, 8.0F, 5.0F,
         0.60F, 0.36F, 1.0F}
    }};

    DebugWorldScene outpost{};
    outpost.landmarks = buildings;

    const auto outpost_vertices =
        build_debug_world_lines(outpost, 16.0F / 9.0F);

    check(state,
        outpost_vertices.has_value() &&
            outpost_vertices->size() == grid->size() + 68U,
        "Two colored wireframe buildings add 17 edges each");

    if (!outpost_vertices) { return 1; }

    const auto again =
        build_debug_world_lines(outpost, 16.0F / 9.0F);

    check(state,
        again.has_value() &&
        again->size() == outpost_vertices->size() &&
        again.value()[grid->size()].x ==
            outpost_vertices.value()[grid->size()].x,
        "Landmark projection is deterministic");

    std::array<DebugWorldLandmark, 2> shifted_buildings{
        buildings
    };
    shifted_buildings[0].relative_x -= 5.0F;
    DebugWorldScene player_moved = outpost;
    player_moved.landmarks = shifted_buildings;

    const auto landmark_shift =
        build_debug_world_lines(
            player_moved, 16.0F / 9.0F);
    check(state,
        landmark_shift.has_value() &&
            landmark_shift.value()[grid->size()].x !=
                outpost_vertices.value()[grid->size()].x,
        "World-fixed structures shift relative to moving player");

    std::array<DebugWorldLandmark, 1> invalid_building{{
        {8.0F, 25.0F, -4.0F, 6.0F, 7.0F,
         1.0F, 0.5F, 0.5F}
    }};
    DebugWorldScene invalid_landmark{};
    invalid_landmark.landmarks = invalid_building;
    const auto invalid_size =
        build_debug_world_lines(invalid_landmark, 1.0F);
    check(state,
        !invalid_size.has_value() &&
            invalid_size.error().code ==
                ErrorCode::invalid_argument,
        "Negative building width is rejected");

    invalid_building[0].width = 6.0F;
    invalid_building[0].height =
        (std::numeric_limits<float>::infinity)();

    const auto invalid_height =
        build_debug_world_lines(invalid_landmark, 1.0F);

    check(state,
        !invalid_height.has_value() &&
            invalid_height.error().code ==
                ErrorCode::invalid_argument,
        "Non-finite landmark height is rejected");

    const auto ground_surface =
        build_debug_world_solids(empty, 16.0F / 9.0F);
    check(state,
        ground_surface.has_value() &&
            ground_surface->size() >= 6U &&
            ground_surface->size() % 3U == 0U,
        "Opaque ground generates clipped triangle-list surfaces");
    if (!ground_surface) { return 1; }

    bool depths_valid = true;
    for (const auto& vertex : ground_surface.value())
    {
        depths_valid = depths_valid &&
            std::isfinite(vertex.x) &&
            std::isfinite(vertex.y) &&
            std::isfinite(vertex.depth) &&
            vertex.depth >= 0.0F &&
            vertex.depth <= 1.0F;
    }
    check(state, depths_valid,
        "Perspective solid vertices have valid GPU depth");

    const auto opaque_outpost =
        build_debug_world_solids(outpost, 16.0F / 9.0F);
    check(state,
        opaque_outpost.has_value() &&
            opaque_outpost->size() >
                ground_surface->size() &&
            opaque_outpost->size() % 3U == 0U,
        "Buildings add solid wall and pitched-roof triangles");

    const auto opaque_repeat =
        build_debug_world_solids(outpost, 16.0F / 9.0F);
    check(state,
        opaque_repeat.has_value() &&
            opaque_outpost.has_value() &&
            opaque_repeat->size() == opaque_outpost->size() &&
            opaque_repeat.value().back().depth ==
                opaque_outpost.value().back().depth,
        "Opaque projection reproduces identical camera depths");

    const auto rejected_solids =
        build_debug_world_solids(invalid_landmark, 1.0F);
    check(state,
        !rejected_solids.has_value() &&
            rejected_solids.error().code ==
                ErrorCode::invalid_argument,
        "Invalid building data fails opaque projection as well");
    std::cout << "Debug world geometry: "
              << state.count - state.failures << "/"
              << state.count << " passed\n";
    return state.failures == 0 ? 0 : 1;
}
