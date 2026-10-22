#include "outpost_layout.hpp"
#include "physical_world_demo.hpp"

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

    std::cout << "Outpost collision integration: "
              << tests.total - tests.failed << "/"
              << tests.total << " passed\n";
    return tests.failed == 0 ? 0 : 1;
}
