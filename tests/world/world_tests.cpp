#include "oros/world/world.hpp"

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
}

int main()
{
    using namespace oros::foundation;
    using namespace oros::world;

    TestState state{};

    const Result<World> zero_namespace_result =
        World::create(0ULL);

    check(
        state,
        !zero_namespace_result.has_value(),
        "World rejects a zero namespace");

    check(
        state,
        !zero_namespace_result.has_value() &&
            zero_namespace_result.error().code ==
                ErrorCode::invalid_argument,
        "Zero World namespace reports invalid_argument");

    Result<World> world_result =
        World::create(0x1234ULL);

    check(
        state,
        world_result.has_value(),
        "World accepts a non-zero namespace");

    if (!world_result.has_value())
    {
        std::cout
            << "\nWorld test summary: "
            << state.checks - state.failures
            << '/'
            << state.checks
            << " passed.\n";

        return 1;
    }

    World world{
        std::move(world_result.value())
    };

    check(
        state,
        world.world_namespace() == 0x1234ULL,
        "World preserves its namespace");

    check(
        state,
        world.entity_count() == 0U,
        "New World has no entities");

    check(
        state,
        world.position_count() == 0U,
        "New World has no positions");

    check(
        state,
        world.positions().empty(),
        "New World exposes an empty position store");

    check(
        state,
        world.positions().entities().empty(),
        "New World has no positioned entity identities");

    check(
        state,
        world.positions().components().empty(),
        "New World has no packed position values");

    check(
        state,
        !world.contains(invalid_entity_id),
        "World does not contain an invalid identity");

    check(
        state,
        world.find_position(invalid_entity_id) ==
            nullptr,
        "Mutable lookup rejects an invalid identity");

    const World& initially_const_world =
        world;

    check(
        state,
        initially_const_world.find_position(
            invalid_entity_id) == nullptr,
        "Const lookup rejects an invalid identity");

    Result<EntityId> first_entity_result =
        world.create_entity();

    check(
        state,
        first_entity_result.has_value(),
        "World creates its first entity");

    Result<EntityId> second_entity_result =
        world.create_entity();

    check(
        state,
        second_entity_result.has_value(),
        "World creates its second entity");

    Result<EntityId> third_entity_result =
        world.create_entity();

    check(
        state,
        third_entity_result.has_value(),
        "World creates its third entity");

    if (!first_entity_result.has_value() ||
        !second_entity_result.has_value() ||
        !third_entity_result.has_value())
    {
        std::cout
            << "\nWorld test summary: "
            << state.checks - state.failures
            << '/'
            << state.checks
            << " passed.\n";

        return 1;
    }

    const EntityId first_entity =
        first_entity_result.value();

    const EntityId second_entity =
        second_entity_result.value();

    const EntityId third_entity =
        third_entity_result.value();

    check(
        state,
        first_entity ==
            EntityId{
                0x1234ULL,
                1ULL
            },
        "First World entity uses sequence one");

    check(
        state,
        second_entity ==
            EntityId{
                0x1234ULL,
                2ULL
            },
        "Second World entity uses sequence two");

    check(
        state,
        third_entity ==
            EntityId{
                0x1234ULL,
                3ULL
            },
        "Third World entity uses sequence three");

    check(
        state,
        world.entity_count() == 3U,
        "Three creations produce three entities");

    check(
        state,
        world.contains(first_entity),
        "World contains its first entity");

    check(
        state,
        world.contains(second_entity),
        "World contains its second entity");

    check(
        state,
        world.contains(third_entity),
        "World contains its third entity");

    const EntityId absent_entity{
        0x1234ULL,
        999ULL
    };

    const EntityId foreign_entity{
        0x9999ULL,
        1ULL
    };

    check(
        state,
        !world.contains(absent_entity),
        "World rejects an absent identity from its namespace");

    check(
        state,
        !world.contains(foreign_entity),
        "World rejects an identity from another namespace");

    const Result<WorldPosition> origin_result =
        WorldPosition::origin();

    const Result<WorldPosition> second_position_result =
        WorldPosition::create(
            WorldCell{
                10,
                -20,
                30
            },
            LocalPosition{
                100.0,
                -200.0,
                300.0
            });

    const Result<WorldPosition> third_position_result =
        WorldPosition::create(
            WorldCell{
                -5,
                6,
                -7
            },
            LocalPosition{
                -400.0,
                250.0,
                125.0
            });

    check(
        state,
        origin_result.has_value() &&
            second_position_result.has_value() &&
            third_position_result.has_value(),
        "World test positions initialize");

    if (!origin_result.has_value() ||
        !second_position_result.has_value() ||
        !third_position_result.has_value())
    {
        std::cout
            << "\nWorld test summary: "
            << state.checks - state.failures
            << '/'
            << state.checks
            << " passed.\n";

        return 1;
    }

    const Status invalid_add_result =
        world.add_position(
            invalid_entity_id,
            origin_result.value());

    check(
        state,
        !invalid_add_result.has_value(),
        "World rejects a position for an invalid identity");

    check(
        state,
        !invalid_add_result.has_value() &&
            invalid_add_result.error().code ==
                ErrorCode::invalid_argument,
        "Invalid position attachment reports invalid_argument");

    const Status absent_add_result =
        world.add_position(
            absent_entity,
            origin_result.value());

    check(
        state,
        !absent_add_result.has_value(),
        "World rejects a position for an absent entity");

    check(
        state,
        !absent_add_result.has_value() &&
            absent_add_result.error().code ==
                ErrorCode::not_found,
        "Absent position attachment reports not_found");

    const Status foreign_add_result =
        world.add_position(
            foreign_entity,
            origin_result.value());

    check(
        state,
        !foreign_add_result.has_value(),
        "World rejects a position for a foreign entity");

    check(
        state,
        !foreign_add_result.has_value() &&
            foreign_add_result.error().code ==
                ErrorCode::not_found,
        "Foreign position attachment reports not_found");

    check(
        state,
        world.position_count() == 0U,
        "Rejected attachments preserve position count");

    const Status first_add_result =
        world.add_position(
            first_entity,
            origin_result.value());

    check(
        state,
        first_add_result.has_value(),
        "World attaches a position to its first entity");

    const Status second_add_result =
        world.add_position(
            second_entity,
            second_position_result.value());

    check(
        state,
        second_add_result.has_value(),
        "World attaches a position to its second entity");

    const Status third_add_result =
        world.add_position(
            third_entity,
            third_position_result.value());

    check(
        state,
        third_add_result.has_value(),
        "World attaches a position to its third entity");

    check(
        state,
        world.position_count() == 3U,
        "Three attachments produce three positions");

    check(
        state,
        world.positions().size() == 3U,
        "Position store reports three packed components");

    check(
        state,
        world.positions().entities().size() ==
            world.positions().components().size(),
        "Packed position arrays remain aligned");

    check(
        state,
        world.positions().entities()[0] ==
                first_entity &&
            world.positions().entities()[1] ==
                second_entity &&
            world.positions().entities()[2] ==
                third_entity,
        "Packed position identities preserve insertion order");

    check(
        state,
        world.find_position(first_entity) != nullptr &&
            *world.find_position(first_entity) ==
                origin_result.value(),
        "First attachment is discoverable by stable identity");

    check(
        state,
        world.find_position(first_entity) != nullptr &&
            *world.find_position(first_entity) ==
                origin_result.value(),
        "Mutable lookup finds the first position");

    check(
        state,
        world.find_position(second_entity) != nullptr &&
            *world.find_position(second_entity) ==
                second_position_result.value(),
        "Mutable lookup finds the second position");

    check(
        state,
        world.find_position(third_entity) != nullptr &&
            *world.find_position(third_entity) ==
                third_position_result.value(),
        "Mutable lookup finds the third position");

    const World& populated_const_world =
        world;

    check(
        state,
        populated_const_world.find_position(
            second_entity) != nullptr &&
            *populated_const_world.find_position(
                second_entity) ==
                second_position_result.value(),
        "Const lookup finds an attached position");

    const Result<WorldPosition>
        translated_first_result =
            origin_result.value().translated(
                WorldDisplacement{
                    600.0,
                    -700.0,
                    800.0
                });

    check(
        state,
        translated_first_result.has_value(),
        "Replacement position initializes");

    WorldPosition* mutable_first_position =
        world.find_position(first_entity);

    if (translated_first_result.has_value() &&
        mutable_first_position != nullptr)
    {
        *mutable_first_position =
            translated_first_result.value();
    }

    check(
        state,
        translated_first_result.has_value() &&
            world.find_position(first_entity) != nullptr &&
            *world.find_position(first_entity) ==
                translated_first_result.value(),
        "Mutation through a fresh identity lookup is retained");

    const Status duplicate_add_result =
        world.add_position(
            first_entity,
            origin_result.value());

    check(
        state,
        !duplicate_add_result.has_value(),
        "World rejects a duplicate position");

    check(
        state,
        !duplicate_add_result.has_value() &&
            duplicate_add_result.error().code ==
                ErrorCode::invalid_state,
        "Duplicate position reports invalid_state");

    check(
        state,
        world.position_count() == 3U,
        "Duplicate attachment preserves position count");

    const Status invalid_remove_result =
        world.remove_position(
            invalid_entity_id);

    check(
        state,
        !invalid_remove_result.has_value(),
        "World rejects position removal for an invalid identity");

    check(
        state,
        !invalid_remove_result.has_value() &&
            invalid_remove_result.error().code ==
                ErrorCode::invalid_argument,
        "Invalid position removal reports invalid_argument");

    const Status foreign_remove_result =
        world.remove_position(
            foreign_entity);

    check(
        state,
        !foreign_remove_result.has_value(),
        "World rejects position removal for a foreign entity");

    check(
        state,
        !foreign_remove_result.has_value() &&
            foreign_remove_result.error().code ==
                ErrorCode::not_found,
        "Foreign position removal reports not_found");

    const Status second_remove_result =
        world.remove_position(
            second_entity);

    check(
        state,
        second_remove_result.has_value(),
        "World removes a middle packed position");

    check(
        state,
        world.position_count() == 2U,
        "Position removal decreases position count");

    check(
        state,
        world.contains(second_entity),
        "Removing a position preserves its entity");

    check(
        state,
        world.find_position(second_entity) == nullptr,
        "Removed position is no longer discoverable");

    check(
        state,
        world.find_position(first_entity) != nullptr,
        "First position survives middle removal");

    check(
        state,
        world.find_position(third_entity) != nullptr,
        "Third position survives middle removal");

    check(
        state,
        world.positions().entities()[0] ==
                first_entity &&
            world.positions().entities()[1] ==
                third_entity,
        "Final packed position swaps into removed slot");

    check(
        state,
        world.positions().components()[1] ==
            third_position_result.value(),
        "Swapped packed position retains its value");

    const Status second_remove_again_result =
        world.remove_position(
            second_entity);

    check(
        state,
        !second_remove_again_result.has_value(),
        "World rejects removal of an absent position");

    check(
        state,
        !second_remove_again_result.has_value() &&
            second_remove_again_result.error().code ==
                ErrorCode::not_found,
        "Absent position removal reports not_found");

    const Status invalid_destroy_result =
        world.destroy_entity(
            invalid_entity_id);

    check(
        state,
        !invalid_destroy_result.has_value(),
        "World rejects destruction of an invalid identity");

    check(
        state,
        !invalid_destroy_result.has_value() &&
            invalid_destroy_result.error().code ==
                ErrorCode::invalid_argument,
        "Invalid entity destruction reports invalid_argument");

    const Status foreign_destroy_result =
        world.destroy_entity(
            foreign_entity);

    check(
        state,
        !foreign_destroy_result.has_value(),
        "World rejects destruction of a foreign entity");

    check(
        state,
        !foreign_destroy_result.has_value() &&
            foreign_destroy_result.error().code ==
                ErrorCode::not_found,
        "Foreign entity destruction reports not_found");

    const Status first_destroy_result =
        world.destroy_entity(
            first_entity);

    check(
        state,
        first_destroy_result.has_value(),
        "World destroys an entity with a position");

    check(
        state,
        !world.contains(first_entity),
        "Destroyed positioned entity is absent");

    check(
        state,
        world.find_position(first_entity) == nullptr,
        "Destroying an entity removes its position");

    check(
        state,
        world.entity_count() == 2U,
        "Positioned entity destruction decreases entity count");

    check(
        state,
        world.position_count() == 1U,
        "Positioned entity destruction decreases position count");

    check(
        state,
        world.contains(second_entity) &&
            world.contains(third_entity),
        "Other entities survive positioned destruction");

    const Status destroyed_add_result =
        world.add_position(
            first_entity,
            origin_result.value());

    check(
        state,
        !destroyed_add_result.has_value(),
        "World rejects attachment to a destroyed entity");

    check(
        state,
        !destroyed_add_result.has_value() &&
            destroyed_add_result.error().code ==
                ErrorCode::not_found,
        "Destroyed entity attachment reports not_found");

    const Status second_destroy_result =
        world.destroy_entity(
            second_entity);

    check(
        state,
        second_destroy_result.has_value(),
        "World destroys an entity without a position");

    check(
        state,
        !world.contains(second_entity),
        "Destroyed unpositioned entity is absent");

    check(
        state,
        world.entity_count() == 1U,
        "Unpositioned destruction decreases entity count");

    check(
        state,
        world.position_count() == 1U,
        "Unpositioned destruction preserves position count");

    const Status second_destroy_again_result =
        world.destroy_entity(
            second_entity);

    check(
        state,
        !second_destroy_again_result.has_value(),
        "World rejects repeated entity destruction");

    check(
        state,
        !second_destroy_again_result.has_value() &&
            second_destroy_again_result.error().code ==
                ErrorCode::not_found,
        "Repeated entity destruction reports not_found");

    Result<EntityId> fourth_entity_result =
        world.create_entity();

    check(
        state,
        fourth_entity_result.has_value(),
        "World creates an entity after destruction");

    check(
        state,
        fourth_entity_result.has_value() &&
            fourth_entity_result.value() ==
                EntityId{
                    0x1234ULL,
                    4ULL
                },
        "Destroyed identities are never reused");

    if (!fourth_entity_result.has_value())
    {
        std::cout
            << "\nWorld test summary: "
            << state.checks - state.failures
            << '/'
            << state.checks
            << " passed.\n";

        return 1;
    }

    const EntityId fourth_entity =
        fourth_entity_result.value();

    check(
        state,
        world.entity_count() == 2U,
        "Post-destruction creation restores entity count");

    check(
        state,
        world.contains(third_entity) &&
            world.contains(fourth_entity),
        "World contains surviving and newly created entities");

    const Status fourth_add_result =
        world.add_position(
            fourth_entity,
            origin_result.value());

    check(
        state,
        fourth_add_result.has_value(),
        "New entity accepts a position");

    check(
        state,
        world.position_count() == 2U,
        "New position restores packed position count");

    World moved_world{
        std::move(world)
    };

    check(
        state,
        moved_world.world_namespace() == 0x1234ULL,
        "Moved World preserves its namespace");

    check(
        state,
        moved_world.entity_count() == 2U,
        "Moved World preserves entity count");

    check(
        state,
        moved_world.position_count() == 2U,
        "Moved World preserves position count");

    check(
        state,
        moved_world.contains(third_entity) &&
            moved_world.contains(fourth_entity),
        "Moved World preserves entity ownership");

    check(
        state,
        moved_world.find_position(third_entity) != nullptr &&
            moved_world.find_position(fourth_entity) != nullptr,
        "Moved World preserves packed positions");

    const Status third_destroy_result =
        moved_world.destroy_entity(
            third_entity);

    const Status fourth_destroy_result =
        moved_world.destroy_entity(
            fourth_entity);

    check(
        state,
        third_destroy_result.has_value() &&
            fourth_destroy_result.has_value(),
        "Moved World destroys its remaining entities");

    check(
        state,
        moved_world.entity_count() == 0U,
        "Moved World becomes entity-empty");

    check(
        state,
        moved_world.position_count() == 0U,
        "Moved World becomes position-empty");

    check(
        state,
        moved_world.positions().entities().empty() &&
            moved_world.positions().components().empty(),
        "Moved World clears both packed position arrays");

    Result<World> growth_world_result =
        World::create(0x7000ULL);

    check(
        state,
        growth_world_result.has_value(),
        "Growth World initializes");

    if (growth_world_result.has_value())
    {
        World growth_world{
            std::move(
                growth_world_result.value())
        };

        constexpr std::uint64_t growth_count{
            256ULL
        };

        bool growth_creation_succeeded = true;
        bool growth_identity_succeeded = true;
        bool growth_attachment_succeeded = true;

        for (std::uint64_t sequence = 1ULL;
             sequence <= growth_count;
             ++sequence)
        {
            const Result<EntityId> entity_result =
                growth_world.create_entity();

            if (!entity_result.has_value())
            {
                growth_creation_succeeded = false;
                break;
            }

            const EntityId expected_entity{
                0x7000ULL,
                sequence
            };

            if (entity_result.value() !=
                expected_entity)
            {
                growth_identity_succeeded = false;
                break;
            }

            if ((sequence % 2ULL) == 0ULL)
            {
                const Status add_result =
                    growth_world.add_position(
                        entity_result.value(),
                        origin_result.value());

                if (!add_result.has_value())
                {
                    growth_attachment_succeeded =
                        false;
                    break;
                }
            }
        }

        check(
            state,
            growth_creation_succeeded,
            "World grows across repeated entity creation");

        check(
            state,
            growth_identity_succeeded,
            "Growth entities retain deterministic identities");

        check(
            state,
            growth_attachment_succeeded,
            "Growth entities accept repeated positions");

        check(
            state,
            growth_world.entity_count() ==
                static_cast<std::size_t>(
                    growth_count),
            "Growth World contains every created entity");

        check(
            state,
            growth_world.position_count() ==
                static_cast<std::size_t>(
                    growth_count / 2ULL),
            "Growth World contains every even position");

        bool growth_lookup_succeeded = true;

        for (std::uint64_t sequence = 1ULL;
             sequence <= growth_count;
             ++sequence)
        {
            const EntityId entity{
                0x7000ULL,
                sequence
            };

            const bool should_have_position =
                (sequence % 2ULL) == 0ULL;

            if (!growth_world.contains(entity) ||
                (growth_world.find_position(entity) !=
                    nullptr) != should_have_position)
            {
                growth_lookup_succeeded = false;
                break;
            }
        }

        check(
            state,
            growth_lookup_succeeded,
            "Growth World preserves ownership and positions");

        check(
            state,
            growth_world.positions().
                    entities().size() ==
                growth_world.positions().
                    components().size(),
            "Growth packed position arrays remain aligned");

        bool growth_destruction_succeeded = true;

        for (std::uint64_t sequence = 1ULL;
             sequence <= growth_count;
             ++sequence)
        {
            const Status destroy_result =
                growth_world.destroy_entity(
                    EntityId{
                        0x7000ULL,
                        sequence
                    });

            if (!destroy_result.has_value())
            {
                growth_destruction_succeeded = false;
                break;
            }
        }

        check(
            state,
            growth_destruction_succeeded,
            "World destroys a grown entity population");

        check(
            state,
            growth_world.entity_count() == 0U,
            "Growth destruction clears all entities");

        check(
            state,
            growth_world.position_count() == 0U,
            "Growth destruction clears all positions");

        Result<EntityId> post_growth_result =
            growth_world.create_entity();

        check(
            state,
            post_growth_result.has_value(),
            "Growth World creates an entity after clearing");

        check(
            state,
            post_growth_result.has_value() &&
                post_growth_result.value() ==
                    EntityId{
                        0x7000ULL,
                        257ULL
                    },
            "Growth World does not reuse cleared identities");
    }

    std::cout
        << "\nWorld test summary: "
        << state.checks - state.failures
        << '/'
        << state.checks
        << " passed.\n";

    return state.failures == 0 ? 0 : 1;
}