#include "oros/physics/collider_id.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string_view>
#include <type_traits>
#include <unordered_map>
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

    int finish(
        const TestState& state)
    {
        std::cout
            << "\nCollider identity test summary: "
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
    using namespace oros::physics;
    using oros::world::EntityId;

    static_assert(
        std::is_same_v<
            ColliderShapeSlot,
            std::uint32_t>);

    static_assert(
        std::is_trivially_copyable_v<
            ColliderId>);

    static_assert(
        std::is_standard_layout_v<
            ColliderId>);

    static_assert(
        std::is_copy_constructible_v<
            ColliderId>);

    static_assert(
        std::is_move_constructible_v<
            ColliderId>);

    static_assert(
        !std::is_convertible_v<
            ColliderId,
            bool>);

    static_assert(
        invalid_collider_id ==
            ColliderId{});

    static_assert(
        !invalid_collider_id.is_valid());

    static_assert(
        !static_cast<bool>(
            invalid_collider_id));

    constexpr EntityId
        first_owner{
            0x1111111111111111ULL,
            0x2222222222222222ULL
        };

    constexpr EntityId
        second_owner{
            0x1111111111111111ULL,
            0x3333333333333333ULL
        };

    constexpr EntityId
        third_owner{
            0x4444444444444444ULL,
            0x2222222222222222ULL
        };

    constexpr ColliderId
        first_collider{
            first_owner,
            1U
        };

    constexpr ColliderId
        second_shape_collider{
            first_owner,
            2U
        };

    constexpr ColliderId
        second_entity_collider{
            second_owner,
            1U
        };

    constexpr ColliderId
        second_namespace_collider{
            third_owner,
            1U
        };

    static_assert(
        first_collider.is_valid());

    static_assert(
        static_cast<bool>(
            first_collider));

    static_assert(
        first_collider ==
            ColliderId{
                first_owner,
                1U
            });

    static_assert(
        first_collider !=
            second_shape_collider);

    static_assert(
        first_collider !=
            second_entity_collider);

    static_assert(
        first_collider !=
            second_namespace_collider);

    static_assert(
        first_collider <
            second_shape_collider);

    TestState state{};

    check(
        state,
        !invalid_collider_id.is_valid(),
        "Default collider identity is invalid");

    check(
        state,
        !static_cast<bool>(
            invalid_collider_id),
        "Invalid collider identity converts to false");

    const ColliderId
        missing_owner{
            EntityId{},
            1U
        };

    check(
        state,
        !missing_owner.is_valid(),
        "Collider identity rejects an invalid owner");

    check(
        state,
        !static_cast<bool>(
            missing_owner),
        "Collider with invalid owner converts to false");

    const ColliderId
        missing_shape_slot{
            first_owner,
            0U
        };

    check(
        state,
        !missing_shape_slot.is_valid(),
        "Collider identity rejects shape slot zero");

    check(
        state,
        !static_cast<bool>(
            missing_shape_slot),
        "Collider with shape slot zero converts to false");

    check(
        state,
        first_collider.is_valid(),
        "Collider with valid owner and shape slot is valid");

    check(
        state,
        static_cast<bool>(
            first_collider),
        "Valid collider identity converts to true");

    check(
        state,
        second_shape_collider.is_valid(),
        "Second shape on the same owner is valid");

    check(
        state,
        second_entity_collider.is_valid(),
        "Collider on a second entity is valid");

    check(
        state,
        second_namespace_collider.is_valid(),
        "Collider in a second world namespace is valid");

    check(
        state,
        first_collider.owner ==
            first_owner,
        "Collider identity preserves its owner");

    check(
        state,
        first_collider.shape_slot ==
            1U,
        "Collider identity preserves its shape slot");

    check(
        state,
        first_collider ==
            ColliderId{
                first_owner,
                1U
            },
        "Equivalent collider identities compare equal");

    check(
        state,
        first_collider !=
            second_shape_collider,
        "Different shape slots produce different identities");

    check(
        state,
        first_collider !=
            second_entity_collider,
        "Different entity sequences produce different identities");

    check(
        state,
        first_collider !=
            second_namespace_collider,
        "Different world namespaces produce different identities");

    check(
        state,
        first_collider <
            second_shape_collider,
        "Shape slots participate in deterministic ordering");

    check(
        state,
        first_collider <
            second_entity_collider,
        "Entity sequences participate in deterministic ordering");

    check(
        state,
        first_collider <
            second_namespace_collider,
        "World namespaces participate in deterministic ordering");

    ColliderId copied_collider =
        first_collider;

    check(
        state,
        copied_collider ==
            first_collider,
        "Collider identity copy preserves value");

    ColliderId moved_collider =
        std::move(
            copied_collider);

    check(
        state,
        moved_collider ==
            first_collider,
        "Collider identity move preserves value");

    const ColliderIdHash hasher{};

    const std::size_t
        first_hash =
            hasher(
                first_collider);

    const std::size_t
        repeated_first_hash =
            hasher(
                first_collider);

    check(
        state,
        first_hash ==
            repeated_first_hash,
        "Collider hash is deterministic");

    const ColliderId
        reconstructed_first_collider{
            EntityId{
                first_owner.
                    world_namespace,
                first_owner.
                    entity_sequence
            },
            first_collider.
                shape_slot
        };

    check(
        state,
        hasher(
            reconstructed_first_collider) ==
            first_hash,
        "Equivalent identities produce equal hashes");

    const std::size_t
        second_shape_hash =
            hasher(
                second_shape_collider);

    const std::size_t
        second_entity_hash =
            hasher(
                second_entity_collider);

    const std::size_t
        second_namespace_hash =
            hasher(
                second_namespace_collider);

    check(
        state,
        first_hash !=
            second_shape_hash,
        "Shape slot contributes to collider hash");

    check(
        state,
        first_hash !=
            second_entity_hash,
        "Entity sequence contributes to collider hash");

    check(
        state,
        first_hash !=
            second_namespace_hash,
        "World namespace contributes to collider hash");

    check(
        state,
        second_shape_hash !=
            second_entity_hash,
        "Distinct persistent identities produce distinct sample hashes");

    check(
        state,
        second_entity_hash !=
            second_namespace_hash,
        "Entity and namespace changes occupy distinct sample hashes");

    const ColliderId
        maximum_slot_collider{
            first_owner,
            UINT32_MAX
        };

    check(
        state,
        maximum_slot_collider.
            is_valid(),
        "Maximum shape slot is valid");

    check(
        state,
        hasher(
            maximum_slot_collider) !=
            first_hash,
        "Maximum shape slot contributes to hash identity");

    const EntityId
        maximum_owner{
            UINT64_MAX,
            UINT64_MAX
        };

    const ColliderId
        maximum_collider{
            maximum_owner,
            UINT32_MAX
        };

    check(
        state,
        maximum_collider.is_valid(),
        "Maximum persistent identity values are valid");

    check(
        state,
        static_cast<bool>(
            maximum_collider),
        "Maximum persistent identity converts to true");

    check(
        state,
        hasher(
            maximum_collider) ==
            hasher(
                maximum_collider),
        "Maximum persistent identity hashes deterministically");

    std::unordered_map<
        ColliderId,
        int,
        ColliderIdHash>
        collider_values{};

    collider_values.emplace(
        first_collider,
        10);

    collider_values.emplace(
        second_shape_collider,
        20);

    collider_values.emplace(
        second_entity_collider,
        30);

    collider_values.emplace(
        second_namespace_collider,
        40);

    collider_values.emplace(
        maximum_collider,
        50);

    check(
        state,
        collider_values.size() ==
            5U,
        "Collider identities remain distinct in a hash table");

    check(
        state,
        collider_values.at(
            first_collider) ==
            10,
        "Hash table retrieves first collider");

    check(
        state,
        collider_values.at(
            second_shape_collider) ==
            20,
        "Hash table retrieves second shape");

    check(
        state,
        collider_values.at(
            second_entity_collider) ==
            30,
        "Hash table retrieves second entity");

    check(
        state,
        collider_values.at(
            second_namespace_collider) ==
            40,
        "Hash table retrieves second namespace");

    check(
        state,
        collider_values.at(
            maximum_collider) ==
            50,
        "Hash table retrieves maximum collider identity");

    collider_values[
        reconstructed_first_collider] =
            60;

    check(
        state,
        collider_values.size() ==
            5U,
        "Equivalent collider identity updates existing hash entry");

    check(
        state,
        collider_values.at(
            first_collider) ==
            60,
        "Equivalent identity addresses the same collider record");

    check(
        state,
        collider_values.find(
            invalid_collider_id) ==
            collider_values.end(),
        "Invalid collider identity is absent from valid records");

    return finish(state);
}