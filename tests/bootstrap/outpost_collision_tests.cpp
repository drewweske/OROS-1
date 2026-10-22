#include "outpost_layout.hpp"
#include "physical_world_demo.hpp"

#include "oros/ai/actor_visual_perception.hpp"
#include "oros/ai/actor_vision_profile.hpp"
#include "oros/physical_world/world_segment_query.hpp"
#include "oros/physical_world/world_first_person_controller.hpp"
#include "oros/physics/collider_geometry.hpp"
#include "oros/physics/collision_shape.hpp"
#include "oros/world/world_position.hpp"

#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string_view>
#include <variant>
#include <utility>

namespace
{
    struct Checks final
    {
        int total{};
        int failed{};
    };

    void check(
        Checks& checks,
        const bool success,
        std::string_view label)
    {
        ++checks.total;
        if (!success)
        {
            ++checks.failed;
            std::cerr << "[fail] " << label << '\n';
        }
    }
}

int main()
{
    using namespace oros::bootstrap;
    using namespace oros::world;
    using namespace oros::physics;
    using namespace oros::physical_world;
    using namespace oros::foundation;

    Checks tests{};
    constexpr std::uint64_t ns = 0x4F524F53ULL;
    const EntityId player{ns, 41ULL};
    const EntityId environment{ns, 42ULL};

    auto demo_result = create_physical_world_demo(
        ns, player, environment);
    check(tests, demo_result.has_value(),
        "Outpost World-cell collider composition succeeds");
    if (!demo_result)
    {
        return 1;
    }
    PhysicalWorldDemo demo{std::move(demo_result.value())};

    check(tests,
        demo.registry.is_valid() &&
        demo.registry.active_cell_count() == 1U &&
        demo.registry.collider_count() == 4U &&
        demo.registry.contains(demo.floor_collider),
        "One resident cell owns ground and three outpost boxes");

    bool aligned = true;
    for (std::size_t i = 0U; i < outpost_buildings.size(); ++i)
    {
        const OutpostBuildingDescription& authored =
            outpost_buildings[i];
        const ColliderId id{
            environment,
            first_outpost_collider_slot +
                static_cast<std::uint32_t>(i)
        };
        const ColliderGeometry* collider = demo.registry.find(id);
        if (collider == nullptr)
        {
            aligned = false;
            continue;
        }

        const BoxShape* box =
            std::get_if<BoxShape>(&collider->shape());
        const PhysicsVector3 center = collider->center();
        if (box == nullptr)
        {
            aligned = false;
            continue;
        }

        const PhysicsVector3 extent = box->half_extents();
        aligned = aligned &&
            center.x == authored.center_x &&
            center.z == authored.center_z &&
            center.y == authored.height * 0.5 &&
            extent.x == authored.width * 0.5 &&
            extent.y == authored.height * 0.5 &&
            extent.z == authored.depth * 0.5;
    }
    check(tests, aligned,
        "Box colliders exactly match visual outpost dimensions and XZ centers");

    // Move the player toward the central tower. Before adding these
    // colliders the path passed through the rendered building.
    auto starting = WorldPosition::create(
        WorldCell{},
        LocalPosition{0.0, 0.0, 39.0});
    check(tests, starting.has_value(),
        "Player approach position is a valid WorldPosition");
    if (!starting)
    {
        return 1;
    }

    WorldPosition player_position{std::move(starting.value())};
    const WorldFirstPersonControllerCommand command{
        PhysicsVector3{0.0, 0.0, 1.0}
    };
    constexpr auto tick = std::chrono::nanoseconds{16'666'667};

    bool stepped = true;
    for (int n = 0; n < 100; ++n)
    {
        auto next = step_world_first_person_controller(
            demo.registry, ns, demo.player_collider,
            demo.player_capsule, player_position, command,
            demo.traversal_settings,
            demo.controller_settings, tick);
        if (!next.has_value())
        {
            stepped = false;
            break;
        }
        player_position = next.value();
    }
    check(tests, stepped,
        "Controller advances deterministically toward the tower");

    // The green tower's near wall is at Z=43.5. A radius-1
    // player capsule cannot reach the wall center or pass it.
    check(tests,
        stepped && player_position.cell() == WorldCell{} &&
        player_position.local().z < 43.6 &&
        player_position.local().z >= 39.0,
        "The central tower blocks player forward traversal");

    // An authored building must block both player locomotion AND
    // observer sight. The same resident cell is authoritative for
    // collision and AI perception; no graphics-only proxy raycast.
    const auto tower_near = WorldPosition::create(
        WorldCell{}, LocalPosition{0.0, 2.0, 39.0});
    const auto tower_far = WorldPosition::create(
        WorldCell{}, LocalPosition{0.0, 2.0, 55.0});
    const auto lane_near = WorldPosition::create(
        WorldCell{}, LocalPosition{20.0, 2.0, 39.0});
    const auto lane_far = WorldPosition::create(
        WorldCell{}, LocalPosition{20.0, 2.0, 55.0});

    check(tests,
        tower_near.has_value() && tower_far.has_value() &&
        lane_near.has_value() && lane_far.has_value(),
        "AI observation World endpoints are valid");

    if (!tower_near || !tower_far || !lane_near || !lane_far)
    {
        return 1;
    }

    const auto blocked_segment = query_world_segment(
        demo.registry, ns, tower_near.value(), tower_far.value());
    const ColliderId tower_id{
        environment,
        first_outpost_collider_slot + 2U
    };

    check(tests,
        blocked_segment.has_value() &&
        blocked_segment->state() == WorldSegmentQueryState::blocked &&
        blocked_segment->hit().has_value() &&
        blocked_segment->hit()->collider() == tower_id,
        "Tower collider is the blocking authority for straight sight ray");

    const auto clear_segment = query_world_segment(
        demo.registry, ns, lane_near.value(), lane_far.value());

    check(tests,
        clear_segment.has_value() &&
        clear_segment->state() == WorldSegmentQueryState::clear,
        "Parallel sight ray away from all three buildings stays clear");

    const auto forward = oros::ai::ActorVisionDirection::create(
        WorldDisplacement{0.0, 0.0, 1.0});
    const auto profile = oros::ai::ActorVisionProfile::create(
        100.0, 120.0);

    check(tests,
        forward.has_value() && profile.has_value(),
        "Valid forward vision configuration created");

    if (!forward || !profile)
    {
        return 1;
    }

    const EntityId npc_observer{ns, 43ULL};
    const EntityId sight_target{ns, 44ULL};
    const auto hidden = oros::ai::query_actor_visual_perception(
        demo.registry, npc_observer, tower_near.value(),
        forward.value(), profile.value(),
        sight_target, tower_far.value());
    const auto exposed = oros::ai::query_actor_visual_perception(
        demo.registry, npc_observer, lane_near.value(),
        forward.value(), profile.value(),
        sight_target, lane_far.value());

    check(tests,
        hidden.has_value() &&
        hidden.value() ==
            oros::ai::ActorVisualPerceptionState::occluded,
        "NPC sight is occluded by the real loaded tower collider");

    check(tests,
        exposed.has_value() &&
        exposed.value() ==
            oros::ai::ActorVisualPerceptionState::visible,
        "NPC sees unobstructed player across an open lane");

    std::cout << "Outpost collision integration: "
              << tests.total - tests.failed << "/"
              << tests.total << " passed\n";
    return tests.failed == 0 ? 0 : 1;
}
