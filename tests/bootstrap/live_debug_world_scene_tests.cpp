#include "live_debug_world_scene.hpp"
#include "outpost_layout.hpp"

#include "oros/foundation/error.hpp"
#include "oros/world/world_position.hpp"

#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <string_view>

namespace
{
    struct Checks { int total{}; int failures{}; };
    void check(Checks& c, bool good, std::string_view reason)
    {
        ++c.total;
        if (!good)
        {
            ++c.failures;
            std::cerr << "[fail] " << reason << '\n';
        }
    }
}

int main()
{
    using namespace oros::bootstrap;
    using namespace oros::foundation;
    using namespace oros::world;

    Checks checks{};
    const auto origin = WorldPosition::origin();
    const auto first = WorldPosition::create(
        WorldCell{}, LocalPosition{-4.0, 0.0, 8.0});
    const auto second = WorldPosition::create(
        WorldCell{}, LocalPosition{5.0, 0.0, 12.0});
    const auto spawned = WorldPosition::create(
        WorldCell{}, LocalPosition{25.0, 0.0, 12.0});
    check(checks, origin && first && second && spawned,
        "Test positions are valid and normalized");
    if (!origin || !first || !second || !spawned) { return 1; }

    const std::array<const WorldPosition*, 2U> actors{
        &first.value(), &second.value()
    };
    const auto frame = build_live_debug_world_frame(
        origin.value(), actors, 0.0F);
    check(checks, frame.has_value(),
        "Visual scene composes World-owned actors");
    if (!frame) { return 1; }

    check(checks,
        frame->landmarks[0].relative_x ==
            static_cast<float>(outpost_buildings[0].center_x) &&
        frame->landmarks[2].relative_z ==
            static_cast<float>(outpost_buildings[2].center_z),
        "Outpost aligns with absolute collision geometry at world origin");
    check(checks,
        frame->markers[0].relative_x == -4.0F &&
        frame->markers[1].relative_z == 12.0F,
        "NPC markers remain independent of each other");

    // Spawn elsewhere: structures MUST NOT follow spawn position.
    const auto nonorigin = build_live_debug_world_frame(
        spawned.value(), actors, 0.4F);
    check(checks, nonorigin.has_value(),
        "Non-origin player scene composes");
    if (!nonorigin) { return 1; }
    check(checks,
        nonorigin->landmarks[0].relative_x == -37.0F &&
        nonorigin->landmarks[2].relative_z == 35.0F,
        "Outpost remains physically anchored when player spawn changes");
    check(checks,
        nonorigin->markers[0].relative_x == -29.0F &&
        nonorigin->markers[1].relative_z == 0.0F,
        "NPC positions are converted from World-relative displacement");
    const auto view = nonorigin->scene();
    check(checks,
        view.markers.size() == 2U &&
        view.landmarks.size() == outpost_buildings.size() &&
        view.camera_yaw_radians == 0.4F,
        "Scene spans point into the owning frame and preserve yaw");

    // Coordinate normalization across a 1024m World-cell edge.
    const auto near_edge = WorldPosition::create(
        WorldCell{}, LocalPosition{511.0, 0.0, 0.0});
    check(checks, near_edge.has_value(), "Near-edge player valid");
    if (!near_edge) { return 1; }
    const auto crossed = near_edge->translated(
        WorldDisplacement{2.0, 0.0, 0.0});
    check(checks,
        crossed.has_value() &&
        crossed->cell().x == 1LL,
        "World position transitions across cell edge");
    if (!crossed) { return 1; }
    const auto before = build_live_debug_world_frame(
        near_edge.value(), actors, 0.0F);
    const auto after = build_live_debug_world_frame(
        crossed.value(), actors, 0.0F);
    check(checks, before && after,
        "Both sides of World-cell boundary compose");
    if (!before || !after) { return 1; }
    const float wrapped = std::fmod(
        before->grid_phase_x + 2.0F, 5.0F);
    check(checks,
        std::fabs(wrapped - after->grid_phase_x) < 0.001F,
        "Grid stays continuous across World-cell boundary");
    check(checks,
        std::fabs(
            (after->landmarks[0].relative_x -
                before->landmarks[0].relative_x) + 2.0F) <
                0.001F,
        "Outpost moves exactly two meters relative to player");

    const std::array<const WorldPosition*, 2U> missing{
        nullptr, &second.value()
    };
    const auto absent = build_live_debug_world_frame(
        origin.value(), missing, 0.0F);
    check(checks,
        !absent &&
        absent.error().code == ErrorCode::invalid_argument,
        "Missing NPC position rejected before rendering");
    const auto bad_yaw = build_live_debug_world_frame(
        origin.value(), actors,
        (std::numeric_limits<float>::infinity)());
    check(checks,
        !bad_yaw &&
        bad_yaw.error().code == ErrorCode::invalid_argument,
        "Invalid camera orientation rejected");

    std::cout << "Live debug world composition: "
              << checks.total - checks.failures << '/'
              << checks.total << " passed\n";
    return checks.failures == 0 ? 0 : 1;
}
