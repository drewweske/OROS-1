#include "oros/world/component_store.hpp"

#include <compare>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string_view>
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

    struct Position final
    {
        int x{};
        int y{};
        int z{};

        auto operator<=>(
            const Position&) const noexcept = default;
    };

    struct MoveOnlyComponent final
    {
        explicit MoveOnlyComponent(
            const int initial_value) noexcept
            : value{initial_value}
        {
        }

        MoveOnlyComponent(
            const MoveOnlyComponent&) = delete;

        MoveOnlyComponent&
        operator=(
            const MoveOnlyComponent&) = delete;

        MoveOnlyComponent(
            MoveOnlyComponent&&) noexcept = default;

        MoveOnlyComponent&
        operator=(
            MoveOnlyComponent&&) noexcept = default;

        int value{};
    };
}

int main()
{
    using namespace oros::foundation;
    using namespace oros::world;

    TestState state{};

    const EntityId first_entity{
        1ULL,
        1ULL
    };

    const EntityId second_entity{
        1ULL,
        2ULL
    };

    const EntityId third_entity{
        1ULL,
        3ULL
    };

    ComponentStore<Position> store{};

    check(
        state,
        store.empty(),
        "New component store is empty");

    check(
        state,
        store.size() == 0U,
        "New component store has zero components");

    check(
        state,
        store.entities().empty(),
        "New component store has no entity identities");

    check(
        state,
        store.components().empty(),
        "New component store has no component values");

    check(
        state,
        !store.contains(first_entity),
        "New component store contains no entity");

    check(
        state,
        store.find(first_entity) == nullptr,
        "Mutable lookup returns null for an absent entity");

    const ComponentStore<Position>&
        initially_const_store = store;

    check(
        state,
        initially_const_store.find(first_entity) ==
            nullptr,
        "Const lookup returns null for an absent entity");

    const Status invalid_insert_result =
        store.insert(
            invalid_entity_id,
            Position{
                1,
                2,
                3
            });

    check(
        state,
        !invalid_insert_result.has_value(),
        "Insertion rejects an invalid entity identity");

    check(
        state,
        !invalid_insert_result.has_value() &&
            invalid_insert_result.error().code ==
                ErrorCode::invalid_argument,
        "Invalid insertion reports invalid_argument");

    check(
        state,
        store.empty(),
        "Rejected insertion leaves storage empty");

    const Status first_insert_result =
        store.insert(
            first_entity,
            Position{
                10,
                20,
                30
            });

    check(
        state,
        first_insert_result.has_value(),
        "Storage accepts the first component");

    check(
        state,
        store.size() == 1U,
        "First insertion increases component count");

    check(
        state,
        !store.empty(),
        "Store is not empty after insertion");

    check(
        state,
        store.contains(first_entity),
        "Store contains the inserted entity");

    check(
        state,
        store.find(first_entity) != nullptr &&
            *store.find(first_entity) ==
                Position{
                    10,
                    20,
                    30
                },
        "Inserted component is discoverable by identity");

    Position* first_position =
        store.find(first_entity);

    check(
        state,
        first_position != nullptr,
        "Mutable lookup finds the inserted component");

    if (first_position != nullptr)
    {
        first_position->x = 11;
    }

    check(
        state,
        store.find(first_entity) != nullptr &&
            *store.find(first_entity) ==
                Position{
                    11,
                    20,
                    30
                },
        "Mutation through a fresh lookup is retained");

    const ComponentStore<Position>&
        const_store_after_first = store;

    const Position* const_first_position =
        const_store_after_first.find(first_entity);

    check(
        state,
        const_first_position != nullptr,
        "Const lookup finds the inserted component");

    check(
        state,
        const_first_position != nullptr &&
            *const_first_position ==
                Position{
                    11,
                    20,
                    30
                },
        "Const lookup observes the stored value");

    check(
        state,
        store.entities().size() ==
            store.components().size(),
        "Entity and component arrays remain aligned");

    check(
        state,
        store.entities()[0] == first_entity,
        "First dense entity entry is correct");

    check(
        state,
        store.components()[0] ==
            Position{
                11,
                20,
                30
            },
        "First dense component entry is correct");

    const Status duplicate_insert_result =
        store.insert(
            first_entity,
            Position{
                99,
                99,
                99
            });

    check(
        state,
        !duplicate_insert_result.has_value(),
        "Storage rejects a duplicate component");

    check(
        state,
        !duplicate_insert_result.has_value() &&
            duplicate_insert_result.error().code ==
                ErrorCode::invalid_state,
        "Duplicate insertion reports invalid_state");

    check(
        state,
        store.size() == 1U,
        "Duplicate insertion preserves component count");

    check(
        state,
        store.find(first_entity) != nullptr &&
            *store.find(first_entity) ==
                Position{
                    11,
                    20,
                    30
                },
        "Duplicate insertion preserves the original value");

    const Status second_insert_result =
        store.insert(
            second_entity,
            Position{
                40,
                50,
                60
            });

    check(
        state,
        second_insert_result.has_value(),
        "Storage accepts a second entity component");

    const Status third_insert_result =
        store.insert(
            third_entity,
            Position{
                70,
                80,
                90
            });

    check(
        state,
        third_insert_result.has_value(),
        "Storage accepts a third entity component");

    check(
        state,
        store.size() == 3U,
        "Three successful insertions produce three components");

    check(
        state,
        store.entities()[0] == first_entity &&
            store.entities()[1] == second_entity &&
            store.entities()[2] == third_entity,
        "Dense entity array preserves insertion order");

    check(
        state,
        store.components()[0] ==
                Position{
                    11,
                    20,
                    30
                } &&
            store.components()[1] ==
                Position{
                    40,
                    50,
                    60
                } &&
            store.components()[2] ==
                Position{
                    70,
                    80,
                    90
                },
        "Dense component array preserves insertion order");

    check(
        state,
        store.find(first_entity) != nullptr &&
            *store.find(first_entity) ==
                Position{
                    11,
                    20,
                    30
                },
        "Identity lookup remains valid after packed growth");

    const Status invalid_remove_result =
        store.remove(invalid_entity_id);

    check(
        state,
        !invalid_remove_result.has_value(),
        "Removal rejects an invalid entity identity");

    check(
        state,
        !invalid_remove_result.has_value() &&
            invalid_remove_result.error().code ==
                ErrorCode::invalid_argument,
        "Invalid removal reports invalid_argument");

    check(
        state,
        store.size() == 3U,
        "Invalid removal preserves component count");

    const EntityId absent_entity{
        1ULL,
        999ULL
    };

    const Status absent_remove_result =
        store.remove(absent_entity);

    check(
        state,
        !absent_remove_result.has_value(),
        "Removal rejects an absent component");

    check(
        state,
        !absent_remove_result.has_value() &&
            absent_remove_result.error().code ==
                ErrorCode::not_found,
        "Absent removal reports not_found");

    check(
        state,
        store.size() == 3U,
        "Absent removal preserves component count");

    const Status middle_remove_result =
        store.remove(second_entity);

    check(
        state,
        middle_remove_result.has_value(),
        "Storage removes a middle dense component");

    check(
        state,
        store.size() == 2U,
        "Middle removal decreases component count");

    check(
        state,
        !store.contains(second_entity),
        "Removed middle entity is no longer present");

    check(
        state,
        store.find(second_entity) == nullptr,
        "Removed middle component cannot be found");

    check(
        state,
        store.contains(first_entity),
        "First component survives middle removal");

    check(
        state,
        store.contains(third_entity),
        "Final component survives middle removal");

    check(
        state,
        store.entities()[0] == first_entity,
        "First dense entry remains in place");

    check(
        state,
        store.entities()[1] == third_entity,
        "Final dense entity swaps into the removed slot");

    check(
        state,
        store.components()[0] ==
            Position{
                11,
                20,
                30
            },
        "First dense component remains unchanged");

    check(
        state,
        store.components()[1] ==
            Position{
                70,
                80,
                90
            },
        "Final dense component swaps into the removed slot");

    check(
        state,
        store.find(third_entity) ==
            &store.components()[1],
        "Moved component lookup points to its new dense slot");

    const Status final_remove_result =
        store.remove(third_entity);

    check(
        state,
        final_remove_result.has_value(),
        "Storage removes its final dense entry");

    check(
        state,
        store.size() == 1U,
        "Final-entry removal decreases component count");

    check(
        state,
        !store.contains(third_entity),
        "Removed final entity is absent");

    check(
        state,
        store.contains(first_entity),
        "Remaining entity survives final-entry removal");

    check(
        state,
        store.entities()[0] == first_entity &&
            store.components()[0] ==
                Position{
                    11,
                    20,
                    30
                },
        "Remaining dense entry stays correct");

    const Status first_remove_result =
        store.remove(first_entity);

    check(
        state,
        first_remove_result.has_value(),
        "Storage removes its last component");

    check(
        state,
        store.empty(),
        "Storage becomes empty after removing all components");

    check(
        state,
        store.size() == 0U,
        "Storage count returns to zero");

    check(
        state,
        store.entities().empty() &&
            store.components().empty(),
        "Dense arrays are empty after all removals");

    ComponentStore<Position> growth_store{};

    constexpr std::uint64_t growth_count{
        256ULL
    };

    bool growth_insertions_succeeded = true;

    for (std::uint64_t sequence = 1ULL;
         sequence <= growth_count;
         ++sequence)
    {
        const Status insert_result =
            growth_store.insert(
                EntityId{
                    77ULL,
                    sequence
                },
                Position{
                    static_cast<int>(sequence),
                    static_cast<int>(
                        sequence * 2ULL),
                    static_cast<int>(
                        sequence * 3ULL)
                });

        if (!insert_result.has_value())
        {
            growth_insertions_succeeded = false;
            break;
        }
    }

    check(
        state,
        growth_insertions_succeeded,
        "Storage grows across repeated insertions");

    check(
        state,
        growth_store.size() ==
            static_cast<std::size_t>(growth_count),
        "Growth storage contains every inserted component");

    bool growth_lookup_succeeded = true;

    for (std::uint64_t sequence = 1ULL;
         sequence <= growth_count;
         ++sequence)
    {
        const EntityId entity{
            77ULL,
            sequence
        };

        const Position* position =
            growth_store.find(entity);

        if (position == nullptr ||
            position->x !=
                static_cast<int>(sequence) ||
            position->y !=
                static_cast<int>(
                    sequence * 2ULL) ||
            position->z !=
                static_cast<int>(
                    sequence * 3ULL))
        {
            growth_lookup_succeeded = false;
            break;
        }
    }

    check(
        state,
        growth_lookup_succeeded,
        "Every grown component remains discoverable");

    check(
        state,
        growth_store.entities().size() ==
            growth_store.components().size(),
        "Grown dense arrays remain aligned");

    growth_store.clear();

    check(
        state,
        growth_store.empty(),
        "Clear empties a populated component store");

    check(
        state,
        growth_store.size() == 0U,
        "Clear resets the component count");

    check(
        state,
        growth_store.entities().empty() &&
            growth_store.components().empty(),
        "Clear empties both dense arrays");

    check(
        state,
        growth_store.find(
            EntityId{
                77ULL,
                1ULL
            }) == nullptr,
        "Cleared components are no longer discoverable");

    const Status post_clear_insert_result =
        growth_store.insert(
            EntityId{
                77ULL,
                500ULL
            },
            Position{
                5,
                6,
                7
            });

    check(
        state,
        post_clear_insert_result.has_value(),
        "Storage accepts insertion after clear");

    check(
        state,
        growth_store.size() == 1U &&
            growth_store.contains(
                EntityId{
                    77ULL,
                    500ULL
                }),
        "Post-clear insertion restores normal storage");

    ComponentStore<MoveOnlyComponent>
        move_only_store{};

    const EntityId move_only_first{
        88ULL,
        1ULL
    };

    const EntityId move_only_second{
        88ULL,
        2ULL
    };

    const Status move_only_first_result =
        move_only_store.insert(
            move_only_first,
            MoveOnlyComponent{100});

    check(
        state,
        move_only_first_result.has_value(),
        "Storage accepts a move-only component");

    const Status move_only_second_result =
        move_only_store.insert(
            move_only_second,
            MoveOnlyComponent{200});

    check(
        state,
        move_only_second_result.has_value(),
        "Storage accepts another move-only component");

    check(
        state,
        move_only_store.find(move_only_first) !=
                nullptr &&
            move_only_store.find(move_only_first)->
                value == 100,
        "First move-only component retains its value");

    check(
        state,
        move_only_store.find(move_only_second) !=
                nullptr &&
            move_only_store.find(move_only_second)->
                value == 200,
        "Second move-only component retains its value");

    const Status move_only_remove_result =
        move_only_store.remove(move_only_first);

    check(
        state,
        move_only_remove_result.has_value(),
        "Swap-removal supports move-only components");

    check(
        state,
        !move_only_store.contains(move_only_first),
        "Removed move-only component is absent");

    check(
        state,
        move_only_store.find(move_only_second) !=
                nullptr &&
            move_only_store.find(move_only_second)->
                value == 200,
        "Moved move-only component retains its value");

    std::cout
        << "\nComponent storage test summary: "
        << state.checks - state.failures
        << '/'
        << state.checks
        << " passed.\n";

    return state.failures == 0 ? 0 : 1;
}