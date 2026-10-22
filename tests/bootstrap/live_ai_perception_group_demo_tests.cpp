#include "live_ai_perception_group_demo.hpp"
#include "navigation_demo.hpp"

#include "oros/physical_world/world_cell_collider_payload.hpp"
#include "oros/physical_world/world_cell_collider_set.hpp"
#include "oros/streaming/world_cell_residency.hpp"
#include "oros/streaming/world_cell_revision_id.hpp"
#include "oros/streaming/world_cell_snapshot.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <string_view>
#include <utility>

namespace
{
    struct Checks final
    {
        int count{};
        int failed{};
    };

    void check(
        Checks& checks,
        const bool condition,
        const std::string_view name)
    {
        ++checks.count;
        if (condition)
        {
            std::cout << "[pass] " << name << '\n';
        }
        else
        {
            ++checks.failed;
            std::cerr << "[fail] " << name << '\n';
        }
    }

    [[nodiscard]]
    bool activate_empty_cell(
        oros::physical_world::WorldCellColliderRegistry& registry,
        const oros::streaming::WorldCellKey key)
    {
        using namespace oros::physical_world;
        using namespace oros::streaming;
        using namespace oros::physics;

        const auto set = WorldCellColliderSet::create(
            key, std::span<const ColliderGeometry>{});
        if (!set)
        {
            return false;
        }

        const auto bytes =
            serialize_world_cell_collider_payload(*set);
        if (!bytes)
        {
            return false;
        }

        const auto snapshot = WorldCellSnapshot::create(
            key, 1ULL, std::span<const std::byte>{*bytes});
        if (!snapshot)
        {
            return false;
        }

        auto result = WorldCellResidency::create(key);
        if (!result)
        {
            return false;
        }

        WorldCellResidency residency{std::move(result.value())};
        const WorldCellRevisionId revision{key, 1ULL};

        return residency.queue_load(revision, 7ULL).has_value() &&
            residency.begin_load(7ULL).has_value() &&
            residency.complete_load(
                7ULL, snapshot.value()).has_value() &&
            registry.synchronize(residency).has_value();
    }
}

int main()
{
    using namespace oros::ai;
    using namespace oros::bootstrap;
    using namespace oros::foundation;
    using namespace oros::world;
    using namespace oros::physical_world;
    using namespace oros::streaming;

    Checks checks{};
    auto world_result = World::create(0x4F524F53ULL);
    auto navigation_result = create_navigation_demo();
    check(
        checks,
        world_result.has_value() && navigation_result.has_value(),
        "World and navigation construct");
    if (!world_result || !navigation_result)
    {
        return 1;
    }

    World world{std::move(world_result.value())};
    NavigationDemo navigation{std::move(navigation_result.value())};
    auto first_result = create_live_ai_actor_demo(world, navigation);
    auto second_result = create_live_ai_actor_demo(world, navigation);
    check(
        checks,
        first_result.has_value() && second_result.has_value(),
        "Two live NPC compositions succeed");
    if (!first_result || !second_result)
    {
        return 1;
    }

    LiveAiActorDemo first{std::move(first_result.value())};
    LiveAiActorDemo second{std::move(second_result.value())};
    check(
        checks,
        first.actor != second.actor &&
            world.contains(first.actor) &&
            world.contains(second.actor) &&
            first.vision_state.has_value() &&
            second.vision_state.has_value(),
        "Distinct NPCs have World identity and actor-owned vision");
    if (!first.vision_state || !second.vision_state)
    {
        return 1;
    }

    const auto away = ActorVisionDirection::create(
        WorldDisplacement{-1.0, 0.0, 0.0});
    const auto toward = ActorVisionDirection::create(
        WorldDisplacement{1.0, 0.0, 0.0});
    if (!away || !toward)
    {
        return 1;
    }

    check(
        checks,
        second.vision_state->face(away.value()).has_value(),
        "Second actor faces away independently");

    const WorldPosition* first_position =
        world.find_position(first.actor);
    // Copy the cell before adding another World position;
    // ComponentStore growth may invalidate earlier pointers.
    const WorldCell actor_cell =
        first_position != nullptr ? first_position->cell() : WorldCell{};

    const auto near = first_position != nullptr
        ? first_position->translated(
            WorldDisplacement{10.0, 0.0, 0.0})
        : Result<WorldPosition>{fail(
            ErrorCode::invalid_state, "Missing first NPC position")};

    const auto target = world.create_entity();
    check(
        checks,
        near.has_value() && target.has_value() &&
            first_position != nullptr,
        "Target and authoritative nearby position construct");
    if (!near || !target || first_position == nullptr)
    {
        return 1;
    }

    check(
        checks,
        world.add_position(target.value(), near.value()).has_value(),
        "Target position belongs to World");

    WorldCellColliderRegistry colliders{};
    const WorldCellKey key{
        world.world_namespace(), actor_cell
    };
    const bool cell_ready = activate_empty_cell(colliders, key);
    check(
        checks, cell_ready,
        "Resident clear collider cell is authoritative");
    if (!cell_ready)
    {
        return 1;
    }

    std::array<LiveAiActorDemo*, 2> actors{&second, &first};
    const std::array<EntityId, 1> targets{target.value()};
    const WorldTime first_time =
        WorldTime::from_microseconds_since_epoch(1'000'000ULL);

    const auto blocked_budget = observe_live_ai_actor_group(
        world, colliders, actors, targets, first_time, 1U);
    check(
        checks,
        !blocked_budget.has_value() &&
            blocked_budget.error().code ==
                ErrorCode::invalid_argument &&
            first.visual_memory.empty() &&
            second.visual_memory.empty(),
        "Bootstrap over-budget roster rejects without changing NPC memories");

    const auto observed = observe_live_ai_actor_group(
        world, colliders, actors, targets, first_time, 2U);
    check(
        checks,
        observed.has_value() &&
            observed->observations.size() == 2U &&
            observed->observations[0].observer == first.actor &&
            observed->observations[0].perception ==
                ActorVisualPerceptionState::visible &&
            observed->observations[1].observer == second.actor &&
            observed->observations[1].perception ==
                ActorVisualPerceptionState::outside_field_of_view &&
            first.visual_memory.contains(first.actor, target.value()) &&
            first.visual_memory.size() == 1U &&
            second.visual_memory.empty(),
        "Multi-NPC perception sorts actors and isolates their memories");

    const auto repeated = observe_live_ai_actor_group(
        world, colliders, actors, targets, first_time);
    check(
        checks,
        repeated.has_value() &&
            first.visual_memory.size() == 1U &&
            second.visual_memory.empty(),
        "Identical simulation tick is idempotent");

    check(
        checks,
        second.vision_state->face(toward.value()).has_value(),
        "Second NPC turns toward target");

    const WorldTime second_time =
        WorldTime::from_microseconds_since_epoch(2'000'000ULL);
    const auto both_observed = observe_live_ai_actor_group(
        world, colliders, actors, targets, second_time);
    check(
        checks,
        both_observed.has_value() &&
            both_observed->observations.size() == 2U &&
            both_observed->observations[1].perception ==
                ActorVisualPerceptionState::visible &&
            first.visual_memory.size() == 1U &&
            second.visual_memory.size() == 1U &&
            !first.visual_memory.contains(second.actor, target.value()) &&
            !second.visual_memory.contains(first.actor, target.value()),
        "Two facing NPCs independently remember one player");

    const auto demote = first.fidelity_registry.transition(
        first.actor, ActorSimulationFidelity::statistical_distant);
    const auto distant = observe_live_ai_actor_group(
        world, colliders, actors, targets,
        WorldTime::from_microseconds_since_epoch(3'000'000ULL));
    check(
        checks,
        demote.has_value() &&
            distant.has_value() &&
            distant->skipped_distant_pairs == 1U &&
            distant->observations.size() == 1U &&
            distant->observations[0].observer == second.actor &&
            first.visual_memory.find(
                first.actor, target.value()).value().observed_at() ==
                second_time,
        "Demoted NPC retains memory while second keeps observing");

    const auto promote = first.fidelity_registry.transition(
        first.actor, ActorSimulationFidelity::deep_local);
    const WorldTime fourth =
        WorldTime::from_microseconds_since_epoch(4'000'000ULL);
    const auto promoted = observe_live_ai_actor_group(
        world, colliders, actors, targets, fourth);
    check(
        checks,
        promote.has_value() &&
            promoted.has_value() &&
            promoted->observations.size() == 2U &&
            first.visual_memory.find(
                first.actor, target.value()).value().observed_at() ==
                fourth,
        "Promotion restores perception with preserved actor identity");

    const std::array<LiveAiActorDemo*, 1> only_second{&second};
    const WorldTime latest =
        WorldTime::from_microseconds_since_epoch(6'000'000ULL);
    const auto update_second = observe_live_ai_actor_group(
        world, colliders, only_second, targets, latest);
    if (!update_second)
    {
        return 1;
    }

    const auto first_before =
        first.visual_memory.find(first.actor, target.value());
    const auto second_before =
        second.visual_memory.find(second.actor, target.value());
    const auto stale = observe_live_ai_actor_group(
        world, colliders, actors, targets,
        WorldTime::from_microseconds_since_epoch(5'000'000ULL));
    check(
        checks,
        first_before.has_value() &&
            second_before.has_value() &&
            !stale.has_value() &&
            stale.error().code == ErrorCode::invalid_argument &&
            first.visual_memory.find(
                first.actor, target.value()).value() ==
                first_before.value() &&
            second.visual_memory.find(
                second.actor, target.value()).value() ==
                second_before.value(),
        "Later stale pair leaves both NPC memories unchanged");

    const auto duplicate_actors = std::array<LiveAiActorDemo*, 2>{
        &first, &first
    };
    const auto duplicate = observe_live_ai_actor_group(
        world, colliders, duplicate_actors, targets, latest);
    check(
        checks,
        !duplicate.has_value() &&
            duplicate.error().code == ErrorCode::invalid_state &&
            first.visual_memory.size() == 1U &&
            second.visual_memory.size() == 1U,
        "Duplicate NPC identity rejected without memory mutation");

    const std::array<LiveAiActorDemo*, 2> null_actor{
        &first, nullptr
    };
    const auto invalid_roster = observe_live_ai_actor_group(
        world, colliders, null_actor, targets, latest);
    check(
        checks,
        !invalid_roster.has_value() &&
            invalid_roster.error().code == ErrorCode::invalid_argument,
        "Null NPC roster entry is rejected safely");

    const std::array<EntityId, 1> absent_target{
        EntityId{world.world_namespace(), 9999ULL}
    };
    const auto missing_target = observe_live_ai_actor_group(
        world, colliders, actors, absent_target, latest);
    check(
        checks,
        !missing_target.has_value() &&
            missing_target.error().code == ErrorCode::invalid_state &&
            first.visual_memory.size() == 1U &&
            second.visual_memory.size() == 1U,
        "Missing World target rejects batch without losing knowledge");

    // Both distant actors still hold old facts. Unobserved fixed
    // ticks must not rebuild their memory vectors in the bootstrap
    // adapter, even though it assembles an ephemeral batch registry.
    const auto distant_first = first.fidelity_registry.transition(
        first.actor, ActorSimulationFidelity::statistical_distant);
    const auto distant_second = second.fidelity_registry.transition(
        second.actor, ActorSimulationFidelity::statistical_distant);
    const auto* old_first_memory =
        first.visual_memory.observations_in_canonical_order().data();
    const auto* old_second_memory =
        second.visual_memory.observations_in_canonical_order().data();
    bool inactive_memory_stable =
        distant_first.has_value() && distant_second.has_value();
    for (std::size_t i = 0U;
         i < 64U && inactive_memory_stable; ++i)
    {
        const auto idle_tick = observe_live_ai_actor_group(
            world, colliders, actors, targets,
            WorldTime::from_microseconds_since_epoch(
                7'000'000ULL + static_cast<std::uint64_t>(i)));
        inactive_memory_stable =
            idle_tick.has_value() &&
            idle_tick->observations.empty() &&
            idle_tick->skipped_distant_pairs == 2U &&
            !idle_tick->visual_memory_staged &&
            first.visual_memory.observations_in_canonical_order().data() ==
                old_first_memory &&
            second.visual_memory.observations_in_canonical_order().data() ==
                old_second_memory;
    }
    check(checks, inactive_memory_stable &&
        first.visual_memory.size() == 1U &&
        second.visual_memory.size() == 1U,
        "64 distant multi-NPC ticks retain exact memory storage and facts");

    // The all-distant fast path must reject invalid requests before
    // returning its empty report, just as the reusable tick does.
    const auto absent_distant = observe_live_ai_actor_group(
        world, colliders, actors, absent_target,
        WorldTime::from_microseconds_since_epoch(8'000'000ULL));
    check(checks,
        !absent_distant &&
        absent_distant.error().code == ErrorCode::invalid_state &&
        first.visual_memory.size() == 1U &&
        second.visual_memory.size() == 1U,
        "All-distant fast path rejects missing World target");

    const std::array<EntityId, 2U> duplicate_targets{
        target.value(), target.value()
    };
    const auto repeated_target_distant = observe_live_ai_actor_group(
        world, colliders, actors, duplicate_targets,
        WorldTime::from_microseconds_since_epoch(8'000'000ULL));
    check(checks,
        !repeated_target_distant &&
        repeated_target_distant.error().code ==
            ErrorCode::invalid_argument &&
        first.visual_memory.size() == 1U,
        "All-distant fast path rejects duplicate observer-target pairs");

    const std::array<EntityId, 1U> self_target{
        first.actor
    };
    const auto self_distant = observe_live_ai_actor_group(
        world, colliders, actors, self_target,
        WorldTime::from_microseconds_since_epoch(8'000'000ULL));
    check(checks,
        !self_distant &&
        self_distant.error().code == ErrorCode::invalid_argument,
        "All-distant fast path rejects self-observation");

    const auto unpositioned_target = world.create_entity();
    if (!unpositioned_target.has_value())
    {
        return 1;
    }
    const std::array<EntityId, 1U> no_position{
        unpositioned_target.value()
    };
    const auto missing_position_distant =
        observe_live_ai_actor_group(
            world, colliders, actors, no_position,
            WorldTime::from_microseconds_since_epoch(8'000'000ULL));
    check(checks,
        !missing_position_distant &&
        missing_position_distant.error().code ==
            ErrorCode::invalid_state &&
        second.visual_memory.size() == 1U,
        "All-distant fast path rejects a World entity without position");

    const std::array<EntityId, 0U> no_targets{};
    const auto empty_distant = observe_live_ai_actor_group(
        world, colliders, actors, no_targets,
        WorldTime::from_microseconds_since_epoch(8'000'000ULL));
    check(checks,
        empty_distant &&
        empty_distant->skipped_distant_pairs == 0U &&
        empty_distant->observations.empty() &&
        !empty_distant->visual_memory_staged,
        "All-distant empty request set returns an empty successful report");

    // One actor can remember multiple targets. Mixed fidelity
    // still rebuilds the combined memory transactionally, without
    // duplicating or losing unrelated historical facts.
    const auto other_target = world.create_entity();
    if (!other_target.has_value())
    {
        return 1;
    }
    const WorldPosition* const existing_position =
        world.find_position(target.value());
    if (existing_position == nullptr)
    {
        return 1;
    }
    // Copy by value before the component store grows: insertion may
    // invalidate borrowed pointers into World position storage.
    const WorldPosition stable_target_position{*existing_position};
    if (!world.add_position(
            other_target.value(), stable_target_position).has_value())
    {
        return 1;
    }
    const auto historical_first =
        ActorVisualObservation::create(
            first.actor, other_target.value(),
            *world.find_position(other_target.value()),
            WorldTime::from_microseconds_since_epoch(2'500'000ULL),
            first.vision_state->confidence());
    const auto historical_second =
        ActorVisualObservation::create(
            second.actor, other_target.value(),
            *world.find_position(other_target.value()),
            WorldTime::from_microseconds_since_epoch(2'600'000ULL),
            second.vision_state->confidence());
    if (!historical_first || !historical_second ||
        !first.visual_memory.remember(historical_first.value()) ||
        !second.visual_memory.remember(historical_second.value()))
    {
        return 1;
    }

    const auto local_first = first.fidelity_registry.transition(
        first.actor, ActorSimulationFidelity::deep_local);
    const auto mixed_tick = observe_live_ai_actor_group(
        world, colliders, actors, targets,
        WorldTime::from_microseconds_since_epoch(9'000'000ULL));
    check(checks,
        local_first.has_value() &&
        mixed_tick.has_value() &&
        mixed_tick->skipped_distant_pairs == 1U &&
        mixed_tick->observations.size() == 1U &&
        mixed_tick->visual_memory_staged &&
        first.visual_memory.size() == 2U &&
        second.visual_memory.size() == 2U &&
        first.visual_memory.contains(first.actor, other_target.value()) &&
        second.visual_memory.contains(second.actor, other_target.value()) &&
        first.visual_memory.contains(first.actor, target.value()) &&
        second.visual_memory.contains(second.actor, target.value()),
        "Mixed-fidelity batch retains multiple independent canonical facts");

    std::cout << "\nLive actor group: "
              << checks.count - checks.failed << "/"
              << checks.count << " checks passed\n";
    return checks.failed == 0 ? 0 : 1;
}
