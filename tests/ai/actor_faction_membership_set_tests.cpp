#include "oros/ai/actor_faction_membership_set.hpp"

#include <cstddef>
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

    bool same_sequence(
        const std::span<
            const oros::ai::
                ActorFactionMembership> left,
        const std::span<
            const oros::ai::
                ActorFactionMembership> right)
    {
        if (left.size() != right.size())
        {
            return false;
        }

        for (
            std::size_t index = 0;
            index < left.size();
            ++index)
        {
            if (!(left[index] == right[index]))
            {
                return false;
            }
        }

        return true;
    }
}

int main()
{
    using namespace oros::ai;
    using namespace oros::foundation;
    using namespace oros::world;

    static_assert(
        std::is_nothrow_move_constructible_v<
            ActorFactionMembership>);

    static_assert(
        std::is_nothrow_move_assignable_v<
            ActorFactionMembership>);

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const ActorFactionMembershipSet&>().
                    memberships_for_actor(
                        std::declval<EntityId>())),
            Result<
                std::span<
                    const ActorFactionMembership>>>);

    static_assert(
        noexcept(
            std::declval<
                const ActorFactionMembershipSet&>().
                contains(
                    std::declval<EntityId>(),
                    std::declval<
                        const FactionKey&>())));

    static_assert(
        noexcept(
            std::declval<
                const ActorFactionMembershipSet&>().
                size()));

    static_assert(
        noexcept(
            std::declval<
                const ActorFactionMembershipSet&>().
                empty()));

    static_assert(
        noexcept(
            std::declval<
                const ActorFactionMembershipSet&>().
                memberships_in_canonical_order()));

    TestState state{};

    constexpr EntityId actor_a{
        0x4F524F53ULL,
        100ULL
    };

    constexpr EntityId actor_b{
        0x4F524F53ULL,
        200ULL
    };

    constexpr EntityId actor_c{
        0x4F524F53ULL,
        300ULL
    };

    auto citizens_result =
        FactionKey::create(
            "oros",
            "citizens");

    auto merchants_result =
        FactionKey::create(
            "oros",
            "merchants");

    auto guild_result =
        FactionKey::create(
            "mod.example",
            "merchant-guild");

    if (
        !citizens_result.has_value() ||
        !merchants_result.has_value() ||
        !guild_result.has_value())
    {
        return 1;
    }

    const FactionKey&
        citizens =
            citizens_result.value();

    const FactionKey&
        merchants =
            merchants_result.value();

    const FactionKey&
        guild =
            guild_result.value();

    auto a_citizens_result =
        ActorFactionMembership::create(
            actor_a,
            citizens);

    auto a_merchants_result =
        ActorFactionMembership::create(
            actor_a,
            merchants);

    auto a_guild_result =
        ActorFactionMembership::create(
            actor_a,
            guild);

    auto b_citizens_result =
        ActorFactionMembership::create(
            actor_b,
            citizens);

    if (
        !a_citizens_result.has_value() ||
        !a_merchants_result.has_value() ||
        !a_guild_result.has_value() ||
        !b_citizens_result.has_value())
    {
        return 1;
    }

    const ActorFactionMembership&
        a_citizens =
            a_citizens_result.value();

    const ActorFactionMembership&
        a_merchants =
            a_merchants_result.value();

    const ActorFactionMembership&
        a_guild =
            a_guild_result.value();

    const ActorFactionMembership&
        b_citizens =
            b_citizens_result.value();

    ActorFactionMembershipSet set{};

    check(
        state,
        set.empty() &&
            set.size() == 0U &&
            set.
                memberships_in_canonical_order().
                empty(),
        "Default membership set is empty");

    const Status first_insert =
        set.insert(
            a_citizens);

    check(
        state,
        first_insert.has_value() &&
            !set.empty() &&
            set.size() == 1U,
        "First membership insert succeeds");

    const std::span<
        const ActorFactionMembership>
        before_duplicate =
            set.
                memberships_in_canonical_order();

    const ActorFactionMembership
        duplicate_snapshot =
            before_duplicate.front();

    const Status duplicate_insert =
        set.insert(
            a_citizens);

    check(
        state,
        !duplicate_insert.has_value() &&
            duplicate_insert.error().code ==
                ErrorCode::invalid_state &&
            set.size() == 1U &&
            set.
                memberships_in_canonical_order().
                front() ==
                duplicate_snapshot,
        "Exact duplicate insert is rejected without mutation");

    const Status second_faction_insert =
        set.insert(
            a_merchants);

    check(
        state,
        second_faction_insert.has_value() &&
            set.size() == 2U,
        "Same actor can hold a different faction membership");

    const Status same_faction_other_actor_insert =
        set.insert(
            b_citizens);

    check(
        state,
        same_faction_other_actor_insert.
                has_value() &&
            set.size() == 3U,
        "Different actor can hold the same faction membership");

    check(
        state,
        set.contains(
            actor_a,
            citizens),
        "contains finds an exact actor and faction pair");

    check(
        state,
        !set.contains(
            actor_a,
            guild),
        "contains returns false for an absent valid pair");

    check(
        state,
        !set.contains(
            invalid_entity_id,
            citizens),
        "contains returns false for an invalid actor");

    const Status guild_insert =
        set.insert(
            a_guild);

    check(
        state,
        guild_insert.has_value() &&
            set.size() == 4U,
        "Additional actor membership inserts successfully");

    const auto actor_a_range_result =
        set.memberships_for_actor(
            actor_a);

    check(
        state,
        actor_a_range_result.has_value() &&
            actor_a_range_result.
                value().size() == 3U,
        "Actor membership range is contiguous");

    if (!actor_a_range_result.has_value())
    {
        return 1;
    }

    const std::span<
        const ActorFactionMembership>
        actor_a_range =
            actor_a_range_result.value();

    check(
        state,
        actor_a_range.size() == 3U &&
            actor_a_range[0].faction() ==
                guild &&
            actor_a_range[1].faction() ==
                citizens &&
            actor_a_range[2].faction() ==
                merchants,
        "Actor range is canonically ordered by faction namespace then name");

    const auto missing_actor_range =
        set.memberships_for_actor(
            actor_c);

    check(
        state,
        missing_actor_range.has_value() &&
            missing_actor_range.
                value().empty(),
        "Valid actor with no memberships returns a successful empty span");

    const auto invalid_actor_range =
        set.memberships_for_actor(
            invalid_entity_id);

    check(
        state,
        !invalid_actor_range.has_value() &&
            invalid_actor_range.
                error().code ==
                ErrorCode::invalid_argument,
        "Invalid actor range query is rejected with invalid_argument");

    const std::span<
        const ActorFactionMembership>
        canonical =
            set.
                memberships_in_canonical_order();

    check(
        state,
        canonical.size() == 4U &&
            canonical[0].actor() ==
                actor_a &&
            canonical[0].faction() ==
                guild &&
            canonical[1].actor() ==
                actor_a &&
            canonical[1].faction() ==
                citizens &&
            canonical[2].actor() ==
                actor_a &&
            canonical[2].faction() ==
                merchants &&
            canonical[3].actor() ==
                actor_b &&
            canonical[3].faction() ==
                citizens,
        "Full canonical order is actor then faction namespace then faction name");

    ActorFactionMembershipSet
        reverse_history{};

    if (
        !reverse_history.insert(
            b_citizens).has_value() ||
        !reverse_history.insert(
            a_merchants).has_value() ||
        !reverse_history.insert(
            a_citizens).has_value() ||
        !reverse_history.insert(
            a_guild).has_value())
    {
        return 1;
    }

    check(
        state,
        same_sequence(
            set.
                memberships_in_canonical_order(),
            reverse_history.
                memberships_in_canonical_order()),
        "Opposite insertion history yields identical canonical order");

    ActorFactionMembershipSet
        removal_set{};

    if (
        !removal_set.insert(
            a_citizens).has_value() ||
        !removal_set.insert(
            a_merchants).has_value() ||
        !removal_set.insert(
            b_citizens).has_value())
    {
        return 1;
    }

    const Status invalid_remove =
        removal_set.remove(
            invalid_entity_id,
            citizens);

    check(
        state,
        !invalid_remove.has_value() &&
            invalid_remove.error().code ==
                ErrorCode::invalid_argument &&
            removal_set.size() == 3U,
        "Invalid actor removal is rejected with invalid_argument");

    const Status absent_remove =
        removal_set.remove(
            actor_a,
            guild);

    check(
        state,
        !absent_remove.has_value() &&
            absent_remove.error().code ==
                ErrorCode::not_found &&
            removal_set.size() == 3U,
        "Absent exact membership removal returns not_found");

    const Status exact_remove =
        removal_set.remove(
            actor_a,
            citizens);

    check(
        state,
        exact_remove.has_value() &&
            removal_set.size() == 2U &&
            !removal_set.contains(
                actor_a,
                citizens),
        "Exact membership removal succeeds");

    check(
        state,
        removal_set.contains(
            actor_a,
            merchants),
        "Removal preserves sibling faction membership for the same actor");

    check(
        state,
        removal_set.contains(
            actor_b,
            citizens),
        "Removal preserves same-faction membership for another actor");

    std::cout
        << state.checks
        << " checks, "
        << state.failures
        << " failures\n";

    return state.failures == 0
        ? 0
        : 1;
}
