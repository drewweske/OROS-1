#include "oros/ai/actor_visual_observation.hpp"
#include "oros/ai/actor_visual_observation_memory.hpp"

#include "oros/foundation/error.hpp"
#include "oros/world/entity_id.hpp"
#include "oros/world/world_position.hpp"
#include "oros/world/world_time.hpp"

#include <cstdint>
#include <iostream>
#include <limits>
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
            << "\nActor visual observation memory test summary: "
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

    const auto zero_confidence_result =
        ActorVisualObservationConfidence::
            create(0.0);

    const auto full_confidence_result =
        ActorVisualObservationConfidence::
            create(1.0);

    const auto nominal_confidence_result =
        ActorVisualObservationConfidence::
            create(0.75);

    const auto low_confidence_result =
        ActorVisualObservationConfidence::
            create(0.5);

    const auto below_confidence_result =
        ActorVisualObservationConfidence::
            create(-0.001);

    const auto above_confidence_result =
        ActorVisualObservationConfidence::
            create(1.001);

    const auto nan_confidence_result =
        ActorVisualObservationConfidence::
            create(
                (
                    std::numeric_limits<
                        double>::
                            quiet_NaN
                )());

    const auto infinite_confidence_result =
        ActorVisualObservationConfidence::
            create(
                (
                    std::numeric_limits<
                        double>::
                            infinity
                )());

    check(
        state,
        zero_confidence_result.has_value() &&
            zero_confidence_result.value().
                    is_valid() &&
            zero_confidence_result.value().
                    value() ==
                0.0 &&
            full_confidence_result.has_value() &&
            full_confidence_result.value().
                    value() ==
                1.0 &&
            nominal_confidence_result.
                    has_value() &&
            nominal_confidence_result.value().
                    value() ==
                0.75,
        "Visual observation confidence accepts finite inclusive boundaries");

    check(
        state,
        !below_confidence_result.has_value() &&
            below_confidence_result.error().
                    code ==
                ErrorCode::invalid_argument &&
            !above_confidence_result.has_value() &&
            above_confidence_result.error().
                    code ==
                ErrorCode::invalid_argument &&
            !nan_confidence_result.has_value() &&
            nan_confidence_result.error().
                    code ==
                ErrorCode::invalid_argument &&
            !infinite_confidence_result.
                has_value() &&
            infinite_confidence_result.error().
                    code ==
                ErrorCode::invalid_argument,
        "Visual observation confidence rejects non-finite and out-of-range values");

    if (
        !nominal_confidence_result.has_value() ||
        !low_confidence_result.has_value())
    {
        return finish(state);
    }

    constexpr std::uint64_t
        world_namespace{
            0x4F524F5300000009ULL
        };

    const EntityId observer{
        world_namespace,
        1000ULL
    };

    const EntityId earlier_observer{
        world_namespace,
        500ULL
    };

    const EntityId target{
        world_namespace,
        2000ULL
    };

    const EntityId second_target{
        world_namespace,
        2500ULL
    };

    const EntityId third_target{
        world_namespace,
        4000ULL
    };

    const EntityId foreign_target{
        world_namespace + 1ULL,
        2000ULL
    };

    const auto first_position_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                10.0,
                0.0,
                0.0
            });

    const auto second_position_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                12.0,
                1.0,
                0.0
            });

    const auto third_position_result =
        WorldPosition::create(
            WorldCell{
                1,
                0,
                0
            },
            LocalPosition{
                -500.0,
                0.0,
                0.0
            });

    check(
        state,
        first_position_result.has_value() &&
            second_position_result.has_value() &&
            third_position_result.has_value(),
        "Remembered visual observation positions are valid");

    if (
        !first_position_result.has_value() ||
        !second_position_result.has_value() ||
        !third_position_result.has_value())
    {
        return finish(state);
    }

    const WorldTime time_90 =
        WorldTime::
            from_microseconds_since_epoch(
                90ULL);

    const WorldTime time_99 =
        WorldTime::
            from_microseconds_since_epoch(
                99ULL);

    const WorldTime time_100 =
        WorldTime::
            from_microseconds_since_epoch(
                100ULL);

    const WorldTime time_120 =
        WorldTime::
            from_microseconds_since_epoch(
                120ULL);

    const WorldTime time_130 =
        WorldTime::
            from_microseconds_since_epoch(
                130ULL);

    const WorldTime time_150 =
        WorldTime::
            from_microseconds_since_epoch(
                150ULL);

    const WorldTime time_160 =
        WorldTime::
            from_microseconds_since_epoch(
                160ULL);

    const WorldTime time_200 =
        WorldTime::
            from_microseconds_since_epoch(
                200ULL);

    const auto invalid_observer_result =
        ActorVisualObservation::create(
            invalid_entity_id,
            target,
            first_position_result.value(),
            time_100,
            nominal_confidence_result.value());

    check(
        state,
        !invalid_observer_result.has_value() &&
            invalid_observer_result.error().
                    code ==
                ErrorCode::invalid_argument,
        "Visual observation rejects invalid observer identity");

    const auto invalid_target_result =
        ActorVisualObservation::create(
            observer,
            invalid_entity_id,
            first_position_result.value(),
            time_100,
            nominal_confidence_result.value());

    check(
        state,
        !invalid_target_result.has_value() &&
            invalid_target_result.error().
                    code ==
                ErrorCode::invalid_argument,
        "Visual observation rejects invalid target identity");

    const auto foreign_target_result =
        ActorVisualObservation::create(
            observer,
            foreign_target,
            first_position_result.value(),
            time_100,
            nominal_confidence_result.value());

    check(
        state,
        !foreign_target_result.has_value() &&
            foreign_target_result.error().
                    code ==
                ErrorCode::invalid_argument,
        "Visual observation rejects cross-namespace identity");

    const auto self_observation_result =
        ActorVisualObservation::create(
            observer,
            observer,
            first_position_result.value(),
            time_100,
            nominal_confidence_result.value());

    check(
        state,
        !self_observation_result.has_value() &&
            self_observation_result.error().
                    code ==
                ErrorCode::invalid_argument,
        "Visual observation requires distinct observer and target");

    const auto initial_observation_result =
        ActorVisualObservation::create(
            observer,
            target,
            first_position_result.value(),
            time_100,
            nominal_confidence_result.value());

    check(
        state,
        initial_observation_result.has_value() &&
            initial_observation_result.value().
                    is_valid() &&
            initial_observation_result.value().
                    observer() ==
                observer &&
            initial_observation_result.value().
                    target() ==
                target &&
            initial_observation_result.value().
                    target_position() ==
                first_position_result.value() &&
            initial_observation_result.value().
                    observed_at() ==
                time_100 &&
            initial_observation_result.value().
                    confidence().value() ==
                0.75,
        "Visual observation preserves identity position time and confidence");

    if (!initial_observation_result.has_value())
    {
        return finish(state);
    }

    const ActorVisualObservation
        initial_observation =
            initial_observation_result.value();

    const auto zero_age_result =
        initial_observation.
            age_microseconds_at(
                time_100);

    const auto later_age_result =
        initial_observation.
            age_microseconds_at(
                time_160);

    const auto backwards_age_result =
        initial_observation.
            age_microseconds_at(
                time_99);

    check(
        state,
        zero_age_result.has_value() &&
            zero_age_result.value() ==
                0ULL &&
            later_age_result.has_value() &&
            later_age_result.value() ==
                60ULL,
        "Visual observation age is exact deterministic WorldTime subtraction");

    check(
        state,
        !backwards_age_result.has_value() &&
            backwards_age_result.error().
                    code ==
                ErrorCode::invalid_argument,
        "Visual observation age rejects backwards chronology");

    check(
        state,
        initial_observation.confidence().
                value() ==
            0.75,
        "Age evaluation does not implicitly decay stored confidence");

    ActorVisualObservationMemory memory{};

    check(
        state,
        memory.empty() &&
            memory.size() ==
                0U,
        "Visual observation memory starts empty");

    check(
        state,
        memory.remember(
                initial_observation).
                has_value() &&
            memory.contains(
                observer,
                target) &&
            memory.size() ==
                1U,
        "Visual observation memory remembers first observer-target fact");

    check(
        state,
        memory.remember(
                initial_observation).
                has_value() &&
            memory.size() ==
                1U,
        "Remembering an identical same-time fact is idempotent");

    const auto found_initial_result =
        memory.find(
            observer,
            target);

    check(
        state,
        found_initial_result.has_value() &&
            found_initial_result.value() ==
                initial_observation,
        "Visual observation lookup returns exact remembered fact");

    const auto stale_observation_result =
        ActorVisualObservation::create(
            observer,
            target,
            second_position_result.value(),
            time_90,
            low_confidence_result.value());

    check(
        state,
        stale_observation_result.has_value(),
        "Stale observation fixture is structurally valid");

    if (!stale_observation_result.has_value())
    {
        return finish(state);
    }

    const auto stale_remember_status =
        memory.remember(
            stale_observation_result.value());

    check(
        state,
        !stale_remember_status.has_value() &&
            stale_remember_status.error().
                    code ==
                ErrorCode::invalid_argument &&
            memory.find(
                    observer,
                    target).
                    has_value() &&
            memory.find(
                    observer,
                    target).
                    value() ==
                initial_observation,
        "Memory rejects stale observations and preserves newer knowledge");

    const auto conflicting_observation_result =
        ActorVisualObservation::create(
            observer,
            target,
            second_position_result.value(),
            time_100,
            low_confidence_result.value());

    check(
        state,
        conflicting_observation_result.
            has_value(),
        "Same-time conflicting observation fixture is structurally valid");

    if (!conflicting_observation_result.has_value())
    {
        return finish(state);
    }

    const auto conflict_status =
        memory.remember(
            conflicting_observation_result.
                value());

    check(
        state,
        !conflict_status.has_value() &&
            conflict_status.error().code ==
                ErrorCode::invalid_argument &&
            memory.find(
                    observer,
                    target).
                    has_value() &&
            memory.find(
                    observer,
                    target).
                    value() ==
                initial_observation,
        "Memory rejects conflicting facts at identical WorldTime");

    const auto newer_observation_result =
        ActorVisualObservation::create(
            observer,
            target,
            second_position_result.value(),
            time_150,
            low_confidence_result.value());

    check(
        state,
        newer_observation_result.has_value(),
        "Newer observation fixture is valid");

    if (!newer_observation_result.has_value())
    {
        return finish(state);
    }

    check(
        state,
        memory.remember(
                newer_observation_result.
                    value()).
                has_value() &&
            memory.size() ==
                1U,
        "Newer observation replaces prior observer-target fact");

    const auto updated_result =
        memory.find(
            observer,
            target);

    const auto updated_age_result =
        updated_result.has_value()
            ? updated_result.value().
                age_microseconds_at(
                    time_200)
            : oros::foundation::Result<
                std::uint64_t>{
                    oros::foundation::fail(
                        ErrorCode::not_found,
                        "Updated observation missing.")
                };

    check(
        state,
        updated_result.has_value() &&
            updated_result.value().
                    target_position() ==
                second_position_result.value() &&
            updated_result.value().
                    observed_at() ==
                time_150 &&
            updated_result.value().
                    confidence().value() ==
                0.5 &&
            updated_age_result.has_value() &&
            updated_age_result.value() ==
                50ULL,
        "Latest memory exposes independent confidence and exact age");

    const auto second_target_observation_result =
        ActorVisualObservation::create(
            observer,
            second_target,
            third_position_result.value(),
            time_120,
            nominal_confidence_result.value());

    const auto earlier_observer_observation_result =
        ActorVisualObservation::create(
            earlier_observer,
            third_target,
            first_position_result.value(),
            time_130,
            nominal_confidence_result.value());

    check(
        state,
        second_target_observation_result.
                has_value() &&
            earlier_observer_observation_result.
                has_value(),
        "Canonical-order observation fixtures are valid");

    if (
        !second_target_observation_result.
            has_value() ||
        !earlier_observer_observation_result.
            has_value())
    {
        return finish(state);
    }

    check(
        state,
        memory.remember(
                second_target_observation_result.
                    value()).
                has_value() &&
            memory.remember(
                earlier_observer_observation_result.
                    value()).
                has_value() &&
            memory.size() ==
                3U,
        "Memory stores multiple observer-target facts");

    const auto canonical =
        memory.
            observations_in_canonical_order();

    check(
        state,
        canonical.size() ==
                3U &&
            canonical[0].observer() ==
                earlier_observer &&
            canonical[0].target() ==
                third_target &&
            canonical[1].observer() ==
                observer &&
            canonical[1].target() ==
                target &&
            canonical[2].observer() ==
                observer &&
            canonical[2].target() ==
                second_target,
        "Memory canonical order is observer then target EntityId");

    const auto observer_range_result =
        memory.observations_for_observer(
            observer);

    check(
        state,
        observer_range_result.has_value() &&
            observer_range_result.value().
                    size() ==
                2U &&
            observer_range_result.value()[0].
                    target() ==
                target &&
            observer_range_result.value()[1].
                    target() ==
                second_target,
        "Observer memory range is contiguous and target ordered");

    const auto invalid_range_result =
        memory.observations_for_observer(
            invalid_entity_id);

    check(
        state,
        !invalid_range_result.has_value() &&
            invalid_range_result.error().
                    code ==
                ErrorCode::invalid_argument,
        "Observer memory range rejects invalid identity");

    check(
        state,
        memory.forget(
                observer,
                second_target).
                has_value() &&
            !memory.contains(
                observer,
                second_target) &&
            memory.size() ==
                2U,
        "Visual observation memory forgets one exact observer-target fact");

    const auto missing_result =
        memory.find(
            observer,
            second_target);

    check(
        state,
        !missing_result.has_value() &&
            missing_result.error().code ==
                ErrorCode::not_found,
        "Forgotten visual observation becomes not-found");

    const auto missing_forget_status =
        memory.forget(
            observer,
            second_target);

    check(
        state,
        !missing_forget_status.has_value() &&
            missing_forget_status.error().
                    code ==
                ErrorCode::not_found,
        "Forgetting an absent visual observation reports not-found");

    return finish(state);
}