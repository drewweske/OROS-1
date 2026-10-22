#include "oros/ai/actor_visual_perception_memory_update.hpp"

#include "oros/foundation/error.hpp"
#include "oros/world/entity_id.hpp"
#include "oros/world/world_position.hpp"
#include "oros/world/world_time.hpp"

#include <cstdint>
#include <iostream>
#include <string_view>

namespace
{
    struct TestState final
    {
        int checks{};
        int failures{};
    };

    void check(
        TestState& state,
        const bool condition,
        const std::string_view name)
    {
        ++state.checks;

        if (condition)
        {
            std::cout
                << "[pass] "
                << name
                << '\n';

            return;
        }

        ++state.failures;

        std::cerr
            << "[fail] "
            << name
            << '\n';
    }

    int finish(
        const TestState& state)
    {
        std::cout
            << "\nActor visual perception memory update test summary: "
            << state.checks - state.failures
            << '/'
            << state.checks
            << " passed.\n";

        return
            state.failures == 0
                ? 0
                : 1;
    }
}

int main()
{
    using namespace oros::ai;
    using namespace oros::foundation;
    using namespace oros::world;

    TestState state{};

    check(
        state,
        !is_valid_actor_visual_perception_memory_update_state(
            ActorVisualPerceptionMemoryUpdateState::
                invalid) &&
            is_valid_actor_visual_perception_memory_update_state(
                ActorVisualPerceptionMemoryUpdateState::
                    preserved) &&
            is_valid_actor_visual_perception_memory_update_state(
                ActorVisualPerceptionMemoryUpdateState::
                    remembered),
        "Perception-memory update states expose an explicit invalid sentinel");

    constexpr std::uint64_t
        world_namespace{
            0x4F524F5300000009ULL
        };

    const EntityId observer{
        world_namespace,
        1000ULL
    };

    const EntityId target{
        world_namespace,
        2000ULL
    };

    const EntityId foreign_target{
        world_namespace + 1ULL,
        2000ULL
    };

    const auto old_position_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                10.0,
                0.0,
                0.0
            });

    const auto new_position_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                20.0,
                2.0,
                0.0
            });

    const auto conflict_position_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                25.0,
                -3.0,
                0.0
            });

    const auto old_confidence_result =
        ActorVisualObservationConfidence::
            create(0.4);

    const auto new_confidence_result =
        ActorVisualObservationConfidence::
            create(0.9);

    const auto conflict_confidence_result =
        ActorVisualObservationConfidence::
            create(0.6);

    check(
        state,
        old_position_result.has_value() &&
            new_position_result.has_value() &&
            conflict_position_result.has_value() &&
            old_confidence_result.has_value() &&
            new_confidence_result.has_value() &&
            conflict_confidence_result.has_value(),
        "Perception-memory adapter fixtures are valid");

    if (
        !old_position_result.has_value() ||
        !new_position_result.has_value() ||
        !conflict_position_result.has_value() ||
        !old_confidence_result.has_value() ||
        !new_confidence_result.has_value() ||
        !conflict_confidence_result.has_value())
    {
        return finish(state);
    }

    const WorldTime time_100 =
        WorldTime::
            from_microseconds_since_epoch(
                100ULL);

    const WorldTime time_150 =
        WorldTime::
            from_microseconds_since_epoch(
                150ULL);

    const WorldTime time_200 =
        WorldTime::
            from_microseconds_since_epoch(
                200ULL);

    const auto initial_observation_result =
        ActorVisualObservation::create(
            observer,
            target,
            old_position_result.value(),
            time_100,
            old_confidence_result.value());

    check(
        state,
        initial_observation_result.has_value(),
        "Initial remembered visual observation fixture is valid");

    if (!initial_observation_result.has_value())
    {
        return finish(state);
    }

    const ActorVisualObservation
        initial_observation =
            initial_observation_result.value();

    ActorVisualObservationMemory memory{};

    check(
        state,
        memory.remember(
                initial_observation).
                has_value(),
        "Initial visual observation enters memory");

    const auto memory_equals =
        [&memory, observer, target](
            const ActorVisualObservation&
                expected)
        {
            const auto result =
                memory.find(
                    observer,
                    target);

            return
                result.has_value() &&
                result.value() ==
                    expected;
        };

    const auto invalid_state_result =
        apply_actor_visual_perception_to_memory(
            memory,
            ActorVisualPerceptionState::invalid,
            observer,
            target,
            new_position_result.value(),
            time_200,
            new_confidence_result.value());

    check(
        state,
        !invalid_state_result.has_value() &&
            invalid_state_result.error().code ==
                ErrorCode::invalid_argument &&
            memory_equals(
                initial_observation),
        "Invalid perception state is rejected without mutating memory");

    const auto invalid_observer_result =
        apply_actor_visual_perception_to_memory(
            memory,
            ActorVisualPerceptionState::occluded,
            invalid_entity_id,
            target,
            new_position_result.value(),
            time_200,
            new_confidence_result.value());

    check(
        state,
        !invalid_observer_result.has_value() &&
            invalid_observer_result.error().code ==
                ErrorCode::invalid_argument &&
            memory_equals(
                initial_observation),
        "Invalid observer is rejected without mutating memory");

    const auto foreign_target_result =
        apply_actor_visual_perception_to_memory(
            memory,
            ActorVisualPerceptionState::occluded,
            observer,
            foreign_target,
            new_position_result.value(),
            time_200,
            new_confidence_result.value());

    check(
        state,
        !foreign_target_result.has_value() &&
            foreign_target_result.error().code ==
                ErrorCode::invalid_argument &&
            memory_equals(
                initial_observation),
        "Cross-namespace adapter identity is rejected without mutation");

    const auto self_target_result =
        apply_actor_visual_perception_to_memory(
            memory,
            ActorVisualPerceptionState::occluded,
            observer,
            observer,
            new_position_result.value(),
            time_200,
            new_confidence_result.value());

    check(
        state,
        !self_target_result.has_value() &&
            self_target_result.error().code ==
                ErrorCode::invalid_argument &&
            memory_equals(
                initial_observation),
        "Self-target adapter identity is rejected without mutation");

    const auto out_of_range_result =
        apply_actor_visual_perception_to_memory(
            memory,
            ActorVisualPerceptionState::
                out_of_range,
            observer,
            target,
            new_position_result.value(),
            time_200,
            new_confidence_result.value());

    check(
        state,
        out_of_range_result.has_value() &&
            out_of_range_result.value() ==
                ActorVisualPerceptionMemoryUpdateState::
                    preserved &&
            memory_equals(
                initial_observation),
        "Out-of-range perception preserves exact remembered knowledge");

    const auto outside_fov_result =
        apply_actor_visual_perception_to_memory(
            memory,
            ActorVisualPerceptionState::
                outside_field_of_view,
            observer,
            target,
            new_position_result.value(),
            time_200,
            new_confidence_result.value());

    check(
        state,
        outside_fov_result.has_value() &&
            outside_fov_result.value() ==
                ActorVisualPerceptionMemoryUpdateState::
                    preserved &&
            memory_equals(
                initial_observation),
        "Outside-FOV perception preserves exact remembered knowledge");

    const auto occluded_result =
        apply_actor_visual_perception_to_memory(
            memory,
            ActorVisualPerceptionState::
                occluded,
            observer,
            target,
            new_position_result.value(),
            time_200,
            new_confidence_result.value());

    check(
        state,
        occluded_result.has_value() &&
            occluded_result.value() ==
                ActorVisualPerceptionMemoryUpdateState::
                    preserved &&
            memory_equals(
                initial_observation),
        "Occluded perception preserves exact remembered knowledge");

    const auto unavailable_result =
        apply_actor_visual_perception_to_memory(
            memory,
            ActorVisualPerceptionState::
                unavailable,
            observer,
            target,
            new_position_result.value(),
            time_200,
            new_confidence_result.value());

    check(
        state,
        unavailable_result.has_value() &&
            unavailable_result.value() ==
                ActorVisualPerceptionMemoryUpdateState::
                    preserved &&
            memory_equals(
                initial_observation),
        "Unavailable perception preserves memory as epistemically unknown");

    ActorVisualObservationMemory
        empty_memory{};

    const auto unavailable_empty_result =
        apply_actor_visual_perception_to_memory(
            empty_memory,
            ActorVisualPerceptionState::
                unavailable,
            observer,
            target,
            new_position_result.value(),
            time_200,
            new_confidence_result.value());

    check(
        state,
        unavailable_empty_result.has_value() &&
            unavailable_empty_result.value() ==
                ActorVisualPerceptionMemoryUpdateState::
                    preserved &&
            empty_memory.empty(),
        "Unavailable perception does not manufacture a negative memory fact");

    const auto visible_result =
        apply_actor_visual_perception_to_memory(
            memory,
            ActorVisualPerceptionState::
                visible,
            observer,
            target,
            new_position_result.value(),
            time_200,
            new_confidence_result.value());

    const auto updated_result =
        memory.find(
            observer,
            target);

    check(
        state,
        visible_result.has_value() &&
            visible_result.value() ==
                ActorVisualPerceptionMemoryUpdateState::
                    remembered &&
            updated_result.has_value() &&
            updated_result.value().
                    target_position() ==
                new_position_result.value() &&
            updated_result.value().
                    observed_at() ==
                time_200 &&
            updated_result.value().
                    confidence().value() ==
                0.9,
        "Visible perception remembers position time and supplied confidence");

    if (!updated_result.has_value())
    {
        return finish(state);
    }

    const ActorVisualObservation
        updated_observation =
            updated_result.value();

    const auto stale_visible_result =
        apply_actor_visual_perception_to_memory(
            memory,
            ActorVisualPerceptionState::
                visible,
            observer,
            target,
            old_position_result.value(),
            time_150,
            old_confidence_result.value());

    check(
        state,
        !stale_visible_result.has_value() &&
            stale_visible_result.error().code ==
                ErrorCode::invalid_argument &&
            memory_equals(
                updated_observation),
        "Stale visible perception cannot overwrite newer remembered knowledge");

    const auto conflicting_visible_result =
        apply_actor_visual_perception_to_memory(
            memory,
            ActorVisualPerceptionState::
                visible,
            observer,
            target,
            conflict_position_result.value(),
            time_200,
            conflict_confidence_result.value());

    check(
        state,
        !conflicting_visible_result.has_value() &&
            conflicting_visible_result.error().
                    code ==
                ErrorCode::invalid_argument &&
            memory_equals(
                updated_observation),
        "Conflicting same-time visible perception preserves deterministic memory");

    const auto identical_visible_result =
        apply_actor_visual_perception_to_memory(
            memory,
            ActorVisualPerceptionState::
                visible,
            observer,
            target,
            new_position_result.value(),
            time_200,
            new_confidence_result.value());

    check(
        state,
        identical_visible_result.has_value() &&
            identical_visible_result.value() ==
                ActorVisualPerceptionMemoryUpdateState::
                    remembered &&
            memory_equals(
                updated_observation),
        "Identical same-time visible perception remains idempotent");

    return finish(state);
}