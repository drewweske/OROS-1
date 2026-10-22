#include "live_ai_actor_lifecycle_demo.hpp"
#include "navigation_demo.hpp"

#include "oros/ai/actor_visual_observation.hpp"
#include "oros/foundation/error.hpp"
#include "oros/world/world.hpp"
#include "oros/world/world_time.hpp"

#include <array>
#include <iostream>
#include <string_view>
#include <utility>

namespace
{
    struct Checks { int count{}; int failed{}; };
    void check(Checks& c, bool ok, std::string_view text)
    {
        ++c.count;
        if (!ok)
        {
            ++c.failed;
            std::cerr << "[fail] " << text << '\n';
        }
    }
}

int main()
{
    using namespace oros::bootstrap;
    using namespace oros::ai;
    using namespace oros::world;
    using namespace oros::foundation;

    Checks c{};
    auto world_result = World::create(0x4F524F53ULL);
    const auto navigation = create_navigation_demo();
    check(c, world_result && navigation,
        "World and navigation exist");
    if (!world_result || !navigation) { return 1; }
    World world{std::move(world_result.value())};
    auto a = create_live_ai_actor_demo(world, navigation.value());
    auto b = create_live_ai_actor_demo(world, navigation.value());
    check(c, a && b, "Two separate live NPCs spawn");
    if (!a || !b) { return 1; }

    LiveAiActorDemo first{std::move(a.value())};
    LiveAiActorDemo second{std::move(b.value())};
    const auto third = world.create_entity();
    const auto confidence =
        ActorVisualObservationConfidence::create(1.0);
    const auto origin = WorldPosition::origin();
    check(c, third && confidence && origin,
        "External target and observation values created");
    if (!third || !confidence || !origin) { return 1; }
    const WorldTime tick =
        WorldTime::from_microseconds_since_epoch(12'000'000ULL);
    const auto second_sees_first =
        ActorVisualObservation::create(
            second.actor, first.actor, origin.value(),
            tick, confidence.value());
    const auto second_sees_target =
        ActorVisualObservation::create(
            second.actor, third.value(), origin.value(),
            tick, confidence.value());
    const auto first_sees_second =
        ActorVisualObservation::create(
            first.actor, second.actor, origin.value(),
            tick, confidence.value());
    check(c,
        second_sees_first && second_sees_target &&
            first_sees_second,
        "Valid reciprocal NPC observations created");
    if (!second_sees_first || !second_sees_target ||
        !first_sees_second) { return 1; }

    check(c,
        first.visual_memory.remember(
            first_sees_second.value()).has_value() &&
        second.visual_memory.remember(
            second_sees_first.value()).has_value() &&
        second.visual_memory.remember(
            second_sees_target.value()).has_value(),
        "Both NPCs remember separate targets");

    const std::array<LiveAiActorDemo*, 2U> invalid{
        &first, nullptr
    };
    const auto invalid_status =
        retire_live_ai_actor(world, first, invalid);
    check(c,
        !invalid_status &&
        invalid_status.error().code == ErrorCode::invalid_state &&
        world.contains(first.actor) &&
        first.fidelity_registry.contains(first.actor) &&
        first.vision_state.has_value() &&
        second.visual_memory.size() == 2U,
        "Invalid roster does not partially retire actor");

    const std::array<LiveAiActorDemo*, 2U> duplicated{
        &first, &first
    };
    const auto duplicate_status =
        retire_live_ai_actor(world, first, duplicated);
    check(c,
        !duplicate_status &&
        duplicate_status.error().code == ErrorCode::invalid_state &&
        world.contains(first.actor),
        "Duplicate retiree fails before World mutation");

    const std::array<LiveAiActorDemo*, 1U> missing{
        &second
    };
    const auto missing_status =
        retire_live_ai_actor(world, first, missing);
    check(c,
        !missing_status &&
        missing_status.error().code == ErrorCode::invalid_state &&
        world.contains(first.actor),
        "Retiree must belong to active roster");

    const WorldPosition* before = world.find_position(first.actor);
    const EntityId first_id = first.actor;
    const EntityId second_id = second.actor;
    const std::array<LiveAiActorDemo*, 2U> roster{
        &second, &first
    };
    const auto retired = retire_live_ai_actor(
        world, first, roster);
    check(c,
        before != nullptr &&
        retired.has_value() &&
        !world.contains(first_id) &&
        world.find_position(first_id) == nullptr &&
        world.contains(second_id) &&
        world.find_position(second_id) != nullptr &&
        first.visual_memory.empty() &&
        second.visual_memory.size() == 1U &&
        second.visual_memory.contains(second_id, third.value()) &&
        !second.visual_memory.contains(second_id, first_id) &&
        first.fidelity_registry.empty() &&
        !first.vision_state.has_value(),
        "NPC despawn purges own/foreign sight facts and state");

    const auto again = retire_live_ai_actor(world, first, roster);
    check(c,
        !again.has_value() &&
        again.error().code == ErrorCode::invalid_state &&
        second.visual_memory.size() == 1U,
        "Double retirement rejected without disturbing survivor");

    const auto new_entity = world.create_entity();
    check(c,
        new_entity.has_value() && new_entity.value() != first_id &&
        !world.contains(first_id),
        "Retired EntityId is not recycled");

    std::cout << "Live NPC retirement: "
              << c.count - c.failed << "/" << c.count
              << " passed\n";
    return c.failed == 0 ? 0 : 1;
}
