#include "oros/ai/actor_visual_perception_tick.hpp"

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
    struct TestResults final
    {
        int checks{};
        int failed{};
    };

    void check(
        TestResults& results,
        const bool success,
        const std::string_view title)
    {
        ++results.checks;
        if (!success)
        {
            ++results.failed;
            std::cerr << "[FAIL] " << title << '\n';
        }
        else
        {
            std::cout << "[PASS] " << title << '\n';
        }
    }

    bool activate_clear_world_cell(
        oros::physical_world::WorldCellColliderRegistry& colliders,
        const oros::streaming::WorldCellKey cell)
    {
        using namespace oros::physical_world;
        using namespace oros::streaming;
        using namespace oros::physics;

        const auto set = WorldCellColliderSet::create(
            cell, std::span<const ColliderGeometry>{});
        if (!set)
        {
            return false;
        }
        const auto payload =
            serialize_world_cell_collider_payload(*set);
        if (!payload)
        {
            return false;
        }
        const auto snapshot = WorldCellSnapshot::create(
            cell,
            1ULL,
            std::span<const std::byte>{*payload});
        if (!snapshot)
        {
            return false;
        }
        auto residency_result = WorldCellResidency::create(cell);
        if (!residency_result)
        {
            return false;
        }
        WorldCellResidency residency{std::move(*residency_result)};
        const WorldCellRevisionId revision{cell, 1ULL};
        return
            residency.queue_load(revision, 41ULL).has_value() &&
            residency.begin_load(41ULL).has_value() &&
            residency.complete_load(
                41ULL, *snapshot).has_value() &&
            colliders.synchronize(residency).has_value();
    }
}

int main()
{
    using namespace oros::ai;
    using namespace oros::world;
    using namespace oros::foundation;
    using namespace oros::physical_world;
    using namespace oros::streaming;

    TestResults test{};
    auto world_result = World::create(0x4F524F53ULL);
    const auto origin = WorldPosition::origin();
    const auto destination = origin ?
        origin->translated(WorldDisplacement{10.0, 0.0, 0.0}) :
        Result<WorldPosition>{fail(
            ErrorCode::invalid_state, "Missing origin")};

    check(test,
        world_result.has_value() &&
        origin.has_value() && destination.has_value(),
        "World and positions construct");
    if (!world_result || !origin || !destination)
    {
        return 1;
    }

    World world{std::move(*world_result)};
    const auto first = world.create_entity();
    const auto second = world.create_entity();
    const auto target = world.create_entity();
    check(test,
        first.has_value() && second.has_value() &&
        target.has_value(),
        "Two World-owned observers and a target construct");
    if (!first || !second || !target)
    {
        return 1;
    }

    check(test,
        world.add_position(*first, *origin).has_value() &&
        world.add_position(*second, *origin).has_value() &&
        world.add_position(*target, *destination).has_value(),
        "World owns three normalized actor/target positions");

    const auto east = ActorVisionDirection::create(
        WorldDisplacement{1.0, 0.0, 0.0});
    const auto west = ActorVisionDirection::create(
        WorldDisplacement{-1.0, 0.0, 0.0});
    const auto profile = ActorVisionProfile::create(
        25.0, 90.0);
    const auto confidence = ActorVisualObservationConfidence::create(
        0.8);
    check(test,
        east.has_value() && west.has_value() &&
        profile.has_value() && confidence.has_value(),
        "Validated vision profiles for two independent actors");
    if (!east || !west || !profile || !confidence)
    {
        return 1;
    }

    const auto first_state = ActorVisionState::create(
        *first, *east, *profile, *confidence);
    const auto second_state = ActorVisionState::create(
        *second, *west, *profile, *confidence);
    if (!first_state || !second_state)
    {
        return 1;
    }

    ActorVisionStateRegistry visions{};
    ActorSimulationFidelityRegistry fidelities{};
    check(test,
        visions.insert(*second_state).has_value() &&
        visions.insert(*first_state).has_value() &&
        fidelities.insert(
            *second,
            ActorSimulationFidelity::deep_local).has_value() &&
        fidelities.insert(
            *first,
            ActorSimulationFidelity::deep_local).has_value(),
        "Reverse actor insertion preserves independent vision/fidelity");

    WorldCellColliderRegistry clear{};
    const WorldCellKey cell{
        world.world_namespace(), origin->cell()
    };
    const bool resident = activate_clear_world_cell(clear, cell);
    check(test, resident, "Authoritative clear cell is resident");
    if (!resident)
    {
        return 1;
    }

    const std::array<ActorVisualPerceptionTickRequest, 2> reversed{{
        {*second, *target},
        {*first, *target}
    }};
    const std::array<ActorVisualPerceptionTickRequest, 2> forward{{
        {*first, *target},
        {*second, *target}
    }};
    ActorVisualObservationMemory memory{};
    const WorldTime first_time =
        WorldTime::from_microseconds_since_epoch(1'000'000ULL);

    const auto first_tick = observe_actor_visual_perception_tick(
        world, clear, visions, fidelities, memory, reversed, first_time);

    check(test,
        first_tick.has_value() &&
        first_tick->observations.size() == 2U &&
        first_tick->observations[0].observer == *first &&
        first_tick->observations[0].perception ==
            ActorVisualPerceptionState::visible &&
        first_tick->observations[1].observer == *second &&
        first_tick->observations[1].perception ==
            ActorVisualPerceptionState::outside_field_of_view &&
        first_tick->skipped_distant_pairs == 0U &&
        memory.contains(*first, *target) &&
        !memory.contains(*second, *target) &&
        memory.size() == 1U,
        "Canonical tick gives each NPC its own orientation and knowledge");

    const auto repeated = observe_actor_visual_perception_tick(
        world, clear, visions, fidelities, memory, forward, first_time);
    check(test,
        repeated.has_value() &&
        repeated->observations.size() == 2U &&
        memory.size() == 1U,
        "Reordered identical tick is deterministic and idempotent");

    const std::array<ActorVisualPerceptionTickRequest, 2> duplicate{{
        {*first, *target}, {*first, *target}
    }};
    const auto invalid_duplicate =
        observe_actor_visual_perception_tick(
            world, clear, visions, fidelities,
            memory, duplicate, first_time);
    check(test,
        !invalid_duplicate.has_value() &&
        invalid_duplicate.error().code ==
            ErrorCode::invalid_argument &&
        memory.size() == 1U,
        "Duplicate request rejects batch without changing memory");

    const auto self = std::array<ActorVisualPerceptionTickRequest, 1>{{
        {*first, *first}
    }};
    const auto invalid_self = observe_actor_visual_perception_tick(
        world, clear, visions, fidelities,
        memory, self, first_time);
    check(test,
        !invalid_self.has_value() &&
        invalid_self.error().code ==
            ErrorCode::invalid_argument,
        "Self observation is rejected");

    const auto missing_actor = world.create_entity();
    check(test, missing_actor.has_value(),
        "Unregistered actor fixture created");
    if (!missing_actor)
    {
        return 1;
    }
    const std::array<ActorVisualPerceptionTickRequest, 1> missing{{
        {*missing_actor, *target}
    }};
    const auto absent = observe_actor_visual_perception_tick(
        world, clear, visions, fidelities,
        memory, missing, first_time);
    check(test,
        !absent.has_value() &&
        absent.error().code == ErrorCode::invalid_state,
        "Missing actor vision and position reject observation");

    check(test,
        fidelities.transition(
            *first,
            ActorSimulationFidelity::statistical_distant).has_value(),
        "First actor demotes without losing its state");

    const auto skipped = observe_actor_visual_perception_tick(
        world, clear, visions, fidelities,
        memory, reversed,
        WorldTime::from_microseconds_since_epoch(2'000'000ULL));
    check(test,
        skipped.has_value() &&
        skipped->skipped_distant_pairs == 1U &&
        skipped->observations.size() == 1U &&
        skipped->observations[0].observer == *second &&
        memory.contains(*first, *target) &&
        memory.size() == 1U,
        "Distant actor skips sight but retains last-known memory");

    check(test,
        fidelities.transition(
            *first,
            ActorSimulationFidelity::deep_local).has_value(),
        "First actor restores deep-local fidelity");
    const WorldTime later =
        WorldTime::from_microseconds_since_epoch(3'000'000ULL);
    const auto after_promotion = observe_actor_visual_perception_tick(
        world, clear, visions, fidelities, memory, forward, later);
    check(test,
        after_promotion.has_value() &&
        after_promotion->observations.size() == 2U &&
        memory.find(*first, *target).value().observed_at() == later,
        "Promoted actor can refresh its remembered target");

    // Change the second observer's direction and give it a newer
    // observation; a later stale batch should fail only after a
    // would-be successful first observer update.
    ActorVisionState* mutable_second = visions.find(*second);
    check(test,
        mutable_second != nullptr &&
        mutable_second->face(*east).has_value(),
        "Second actor turns toward target independently");

    const std::array<ActorVisualPerceptionTickRequest, 1>
        second_only{{{*second, *target}}};
    const WorldTime newest =
        WorldTime::from_microseconds_since_epoch(5'000'000ULL);
    const auto second_seen = observe_actor_visual_perception_tick(
        world, clear, visions, fidelities, memory, second_only, newest);
    const auto before_conflict = memory.find(*first, *target);
    check(test,
        second_seen.has_value() &&
        second_seen->observations.size() == 1U &&
        second_seen->observations[0].perception ==
            ActorVisualPerceptionState::visible &&
        memory.contains(*second, *target) &&
        before_conflict.has_value() &&
        memory.size() == 2U,
        "Independent second NPC remembers newly visible target");

    const auto rollback = observe_actor_visual_perception_tick(
        world, clear, visions, fidelities, memory, reversed,
        WorldTime::from_microseconds_since_epoch(4'000'000ULL));
    check(test,
        !rollback.has_value() &&
        rollback.error().code == ErrorCode::invalid_argument &&
        memory.size() == 2U &&
        memory.find(*first, *target).value() ==
            before_conflict.value() &&
        memory.find(*second, *target).value().observed_at() ==
            newest,
        "Stale later pair rolls back earlier staged memory update");

    WorldCellColliderRegistry unavailable{};
    const auto unobserved = observe_actor_visual_perception_tick(
        world, unavailable, visions, fidelities, memory,
        reversed, WorldTime::from_microseconds_since_epoch(6'000'000ULL));
    check(test,
        unobserved.has_value() &&
        unobserved->observations.size() == 2U &&
        unobserved->observations[0].perception ==
            ActorVisualPerceptionState::unavailable &&
        unobserved->observations[1].perception ==
            ActorVisualPerceptionState::unavailable &&
        memory.size() == 2U,
        "Unloaded collider data reports unavailable; does not invent sight");

    std::cout << "\nPerception batch: "
              << test.checks - test.failed << "/"
              << test.checks << " checks passed\n";
    return test.failed == 0 ? 0 : 1;
}
