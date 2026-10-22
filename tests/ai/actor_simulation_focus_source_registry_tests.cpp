#include "oros/ai/actor_simulation_focus_source_registry.hpp"

#include <iostream>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>

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
}

int main()
{
    using namespace oros::ai;
    using namespace oros::foundation;
    using namespace oros::world;

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const ActorSimulationFocusSourceRegistry&>().
                        sources_in_entity_order()),
            std::span<
                const ActorSimulationFocusSource>>);

    static_assert(
        std::is_nothrow_move_constructible_v<
            ActorSimulationFocusSource>);

    static_assert(
        std::is_nothrow_move_assignable_v<
            ActorSimulationFocusSource>);

    static_assert(
        std::is_nothrow_move_constructible_v<
            ActorSimulationFocusSourceRegistry>);

    static_assert(
        std::is_nothrow_move_assignable_v<
            ActorSimulationFocusSourceRegistry>);

    TestState state{};

    const Result<WorldPosition>
        origin_result =
            WorldPosition::origin();

    const Result<WorldPosition>
        early_position_result =
            WorldPosition::create(
                WorldCell{},
                LocalPosition{
                    10.0,
                    0.0,
                    0.0
                });

    const Result<WorldPosition>
        middle_position_result =
            WorldPosition::create(
                WorldCell{},
                LocalPosition{
                    20.0,
                    0.0,
                    0.0
                });

    const Result<WorldPosition>
        late_position_result =
            WorldPosition::create(
                WorldCell{},
                LocalPosition{
                    30.0,
                    0.0,
                    0.0
                });

    const Result<WorldPosition>
        updated_position_result =
            WorldPosition::create(
                WorldCell{
                    1'000'000,
                    0,
                    -1'000'000
                },
                LocalPosition{
                    128.0,
                    64.0,
                    -256.0
                });

    check(
        state,
        origin_result.has_value() &&
            early_position_result.has_value() &&
            middle_position_result.has_value() &&
            late_position_result.has_value() &&
            updated_position_result.has_value(),
        "Focus-source WorldPosition fixtures are valid");

    if (
        !origin_result.has_value() ||
        !early_position_result.has_value() ||
        !middle_position_result.has_value() ||
        !late_position_result.has_value() ||
        !updated_position_result.has_value())
    {
        return 1;
    }

    const WorldPosition origin =
        origin_result.value();

    const WorldPosition early_position =
        early_position_result.value();

    const WorldPosition middle_position =
        middle_position_result.value();

    const WorldPosition late_position =
        late_position_result.value();

    const WorldPosition updated_position =
        updated_position_result.value();

    ActorSimulationFocusSourceRegistry
        registry{};

    check(
        state,
        registry.empty(),
        "New focus-source registry is empty");

    check(
        state,
        registry.size() == 0U,
        "New focus-source registry has zero sources");

    check(
        state,
        registry.
            sources_in_entity_order().
            empty(),
        "New focus-source registry exposes an empty ordered view");

    check(
        state,
        !registry.contains(
            invalid_entity_id),
        "Registry does not contain invalid source identity");

    const Result<WorldPosition>
        invalid_lookup =
            registry.position(
                invalid_entity_id);

    check(
        state,
        !invalid_lookup.has_value() &&
            invalid_lookup.error().code ==
                ErrorCode::invalid_argument,
        "Invalid source lookup reports invalid_argument");

    const Status invalid_insert =
        registry.insert(
            invalid_entity_id,
            origin);

    check(
        state,
        !invalid_insert.has_value() &&
            invalid_insert.error().code ==
                ErrorCode::invalid_argument,
        "Invalid source insertion reports invalid_argument");

    check(
        state,
        registry.empty(),
        "Rejected invalid insertion cannot mutate registry");

    const Status invalid_update =
        registry.update_position(
            invalid_entity_id,
            origin);

    check(
        state,
        !invalid_update.has_value() &&
            invalid_update.error().code ==
                ErrorCode::invalid_argument,
        "Invalid source update reports invalid_argument");

    const Status invalid_remove =
        registry.remove(
            invalid_entity_id);

    check(
        state,
        !invalid_remove.has_value() &&
            invalid_remove.error().code ==
                ErrorCode::invalid_argument,
        "Invalid source removal reports invalid_argument");

    const EntityId early_source{
        1ULL,
        2ULL
    };

    const EntityId middle_source{
        1ULL,
        9ULL
    };

    const EntityId late_source{
        2ULL,
        1ULL
    };

    const EntityId missing_source{
        3ULL,
        1ULL
    };

    const Status insert_late =
        registry.insert(
            late_source,
            late_position);

    const Status insert_early =
        registry.insert(
            early_source,
            early_position);

    const Status insert_middle =
        registry.insert(
            middle_source,
            middle_position);

    check(
        state,
        insert_late.has_value() &&
            insert_early.has_value() &&
            insert_middle.has_value(),
        "Focus sources insert successfully out of order");

    check(
        state,
        registry.size() == 3U,
        "Registry owns three focus sources");

    check(
        state,
        !registry.empty(),
        "Populated focus-source registry is not empty");

    const std::span<
        const ActorSimulationFocusSource>
        ordered =
            registry.
                sources_in_entity_order();

    check(
        state,
        ordered.size() == 3U,
        "Ordered focus-source view contains every source");

    check(
        state,
        ordered.size() == 3U &&
            ordered[0].source_entity() ==
                early_source &&
            ordered[1].source_entity() ==
                middle_source &&
            ordered[2].source_entity() ==
                late_source,
        "Focus sources use canonical World EntityId order");

    check(
        state,
        ordered.size() == 3U &&
            ordered[0].position() ==
                early_position &&
            ordered[1].position() ==
                middle_position &&
            ordered[2].position() ==
                late_position,
        "Canonical source order preserves source positions");

    check(
        state,
        registry.contains(
            early_source) &&
            registry.contains(
                middle_source) &&
            registry.contains(
                late_source),
        "Registry resolves every inserted focus source");

    check(
        state,
        !registry.contains(
            missing_source),
        "Registry rejects an absent valid focus source");

    const Result<WorldPosition>
        early_lookup =
            registry.position(
                early_source);

    check(
        state,
        early_lookup.has_value() &&
            early_lookup.value() ==
                early_position,
        "Position lookup resolves the correct focus source");

    const Result<WorldPosition>
        missing_lookup =
            registry.position(
                missing_source);

    check(
        state,
        !missing_lookup.has_value() &&
            missing_lookup.error().code ==
                ErrorCode::not_found,
        "Missing source lookup reports not_found");

    const Status duplicate_insert =
        registry.insert(
            early_source,
            updated_position);

    check(
        state,
        !duplicate_insert.has_value() &&
            duplicate_insert.error().code ==
                ErrorCode::invalid_state,
        "Duplicate focus-source insertion reports invalid_state");

    check(
        state,
        registry.size() == 3U,
        "Duplicate insertion cannot change source count");

    const Result<WorldPosition>
        position_after_duplicate =
            registry.position(
                early_source);

    check(
        state,
        position_after_duplicate.has_value() &&
            position_after_duplicate.value() ==
                early_position,
        "Duplicate insertion cannot overwrite source position");

    const Status update_missing =
        registry.update_position(
            missing_source,
            updated_position);

    check(
        state,
        !update_missing.has_value() &&
            update_missing.error().code ==
                ErrorCode::not_found,
        "Missing source update reports not_found");

    const Status update_middle =
        registry.update_position(
            middle_source,
            updated_position);

    check(
        state,
        update_middle.has_value(),
        "Existing focus-source position updates explicitly");

    const Result<WorldPosition>
        updated_lookup =
            registry.position(
                middle_source);

    check(
        state,
        updated_lookup.has_value() &&
            updated_lookup.value() ==
                updated_position,
        "Updated source preserves the new large-world position");

    const std::span<
        const ActorSimulationFocusSource>
        after_update =
            registry.
                sources_in_entity_order();

    check(
        state,
        after_update.size() == 3U &&
            after_update[0].source_entity() ==
                early_source &&
            after_update[1].source_entity() ==
                middle_source &&
            after_update[2].source_entity() ==
                late_source,
        "Position updates cannot perturb canonical source order");

    const Status remove_middle =
        registry.remove(
            middle_source);

    check(
        state,
        remove_middle.has_value(),
        "Existing focus source removes explicitly");

    check(
        state,
        registry.size() == 2U &&
            !registry.contains(
                middle_source),
        "Removal deletes only the selected focus source");

    const std::span<
        const ActorSimulationFocusSource>
        after_removal =
            registry.
                sources_in_entity_order();

    check(
        state,
        after_removal.size() == 2U &&
            after_removal[0].source_entity() ==
                early_source &&
            after_removal[1].source_entity() ==
                late_source,
        "Removal preserves canonical order of surviving sources");

    const Status remove_missing =
        registry.remove(
            middle_source);

    check(
        state,
        !remove_missing.has_value() &&
            remove_missing.error().code ==
                ErrorCode::not_found,
        "Repeated focus-source removal reports not_found");

    const Status reinsert_middle =
        registry.insert(
            middle_source,
            updated_position);

    check(
        state,
        reinsert_middle.has_value(),
        "Removed source identity may be registered again");

    const std::span<
        const ActorSimulationFocusSource>
        restored =
            registry.
                sources_in_entity_order();

    check(
        state,
        restored.size() == 3U &&
            restored[0].source_entity() ==
                early_source &&
            restored[1].source_entity() ==
                middle_source &&
            restored[2].source_entity() ==
                late_source,
        "Reinsertion restores canonical EntityId order");

    ActorSimulationFocusSourceRegistry
        alternate_registry{};

    const Status alternate_middle =
        alternate_registry.insert(
            middle_source,
            updated_position);

    const Status alternate_early =
        alternate_registry.insert(
            early_source,
            early_position);

    const Status alternate_late =
        alternate_registry.insert(
            late_source,
            late_position);

    check(
        state,
        alternate_middle.has_value() &&
            alternate_early.has_value() &&
            alternate_late.has_value(),
        "Alternate focus-source insertion order succeeds");

    const std::span<
        const ActorSimulationFocusSource>
        canonical_a =
            registry.
                sources_in_entity_order();

    const std::span<
        const ActorSimulationFocusSource>
        canonical_b =
            alternate_registry.
                sources_in_entity_order();

    bool canonical_match =
        canonical_a.size() ==
            canonical_b.size();

    if (canonical_match)
    {
        for (
            std::size_t index = 0U;
            index < canonical_a.size();
            ++index)
        {
            if (
                canonical_a[index].
                    source_entity() !=
                    canonical_b[index].
                        source_entity() ||
                canonical_a[index].
                    position() !=
                    canonical_b[index].
                        position())
            {
                canonical_match = false;
                break;
            }
        }
    }

    check(
        state,
        canonical_match,
        "Canonical focus-source state is independent of insertion history");

    std::cout
        << state.checks
        << " checks, "
        << state.failures
        << " failures\n";

    return state.failures == 0
        ? 0
        : 1;
}