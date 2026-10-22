#include "oros/ai/actor_faction_membership.hpp"

#include <iostream>
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

    oros::foundation::Result<
        oros::ai::ActorFactionMembership>
    make_membership_from_ephemeral_faction(
        const oros::world::EntityId actor)
    {
        auto faction_result =
            oros::ai::FactionKey::create(
                "mod.example",
                "merchant-guild");

        if (!faction_result.has_value())
        {
            return oros::foundation::fail(
                oros::foundation::ErrorCode::
                    internal_failure,
                "Faction membership test fixture "
                "could not construct its faction.");
        }

        return oros::ai::
            ActorFactionMembership::create(
                actor,
                faction_result.value());
    }
}

int main()
{
    using namespace oros::ai;
    using namespace oros::foundation;
    using namespace oros::world;

    static_assert(
        !std::is_default_constructible_v<
            ActorFactionMembership>);

    static_assert(
        std::is_copy_constructible_v<
            ActorFactionMembership>);

    static_assert(
        std::is_copy_assignable_v<
            ActorFactionMembership>);

    static_assert(
        std::is_nothrow_move_constructible_v<
            ActorFactionMembership>);

    static_assert(
        std::is_nothrow_move_assignable_v<
            ActorFactionMembership>);

    static_assert(
        std::is_same_v<
            decltype(
                ActorFactionMembership::create(
                    std::declval<EntityId>(),
                    std::declval<
                        const FactionKey&>())),
            Result<ActorFactionMembership>>);

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const ActorFactionMembership&>().
                    actor()),
            EntityId>);

    static_assert(
        noexcept(
            std::declval<
                const ActorFactionMembership&>().
                actor()));

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const ActorFactionMembership&>().
                    faction()),
            const FactionKey&>);

    static_assert(
        noexcept(
            std::declval<
                const ActorFactionMembership&>().
                faction()));

    TestState state{};

    constexpr EntityId actor_a{
        0x4F524F53ULL,
        101ULL
    };

    constexpr EntityId actor_b{
        0x4F524F53ULL,
        202ULL
    };

    auto citizens_result =
        FactionKey::create(
            "oros",
            "citizens");

    auto merchants_result =
        FactionKey::create(
            "oros",
            "merchants");

    if (
        !citizens_result.has_value() ||
        !merchants_result.has_value())
    {
        return 1;
    }

    const FactionKey&
        citizens =
            citizens_result.value();

    const FactionKey&
        merchants =
            merchants_result.value();

    const Result<ActorFactionMembership>
        membership_result =
            ActorFactionMembership::create(
                actor_a,
                citizens);

    check(
        state,
        membership_result.has_value(),
        "Valid actor and faction construct membership successfully");

    if (!membership_result.has_value())
    {
        return 1;
    }

    const ActorFactionMembership&
        membership =
            membership_result.value();

    check(
        state,
        membership.actor() ==
            actor_a,
        "Membership actor accessor preserves input EntityId");

    check(
        state,
        membership.faction() ==
            citizens,
        "Membership faction accessor preserves input faction identity");

    const Result<ActorFactionMembership>
        invalid_actor_result =
            ActorFactionMembership::create(
                invalid_entity_id,
                citizens);

    check(
        state,
        !invalid_actor_result.has_value() &&
            invalid_actor_result.
                error().code ==
                ErrorCode::invalid_argument,
        "Invalid actor identity is rejected with invalid_argument");

    const Result<ActorFactionMembership>
        ephemeral_result =
            make_membership_from_ephemeral_faction(
                actor_a);

    check(
        state,
        ephemeral_result.has_value() &&
            ephemeral_result.value().
                faction().
                faction_namespace() ==
                "mod.example" &&
            ephemeral_result.value().
                faction().
                faction_name() ==
                "merchant-guild",
        "Membership owns faction identity beyond source key lifetime");

    const Result<ActorFactionMembership>
        equal_membership_result =
            ActorFactionMembership::create(
                actor_a,
                citizens);

    check(
        state,
        equal_membership_result.
                has_value() &&
            equal_membership_result.value() ==
                membership,
        "Same actor and same faction produce equal membership facts");

    const Result<ActorFactionMembership>
        different_faction_result =
            ActorFactionMembership::create(
                actor_a,
                merchants);

    check(
        state,
        different_faction_result.
                has_value() &&
            !(
                different_faction_result.
                    value() ==
                membership
            ),
        "Same actor and different factions produce distinct membership facts");

    const Result<ActorFactionMembership>
        different_actor_result =
            ActorFactionMembership::create(
                actor_b,
                citizens);

    check(
        state,
        different_actor_result.
                has_value() &&
            !(
                different_actor_result.
                    value() ==
                membership
            ),
        "Different actors in same faction produce distinct membership facts");

    check(
        state,
        membership_result.has_value() &&
            different_faction_result.
                has_value() &&
            membership_result.value().
                actor() ==
                actor_a &&
            different_faction_result.
                value().
                actor() ==
                actor_a &&
            membership_result.value().
                faction() ==
                citizens &&
            different_faction_result.
                value().
                faction() ==
                merchants,
        "One actor can independently hold memberships in multiple factions");

    const ActorFactionMembership
        copied_membership =
            membership;

    check(
        state,
        copied_membership ==
            membership,
        "Membership copy preserves actor and faction fact");

    ActorFactionMembership
        assigned_membership =
            different_actor_result.value();

    assigned_membership =
        membership;

    check(
        state,
        assigned_membership ==
            membership,
        "Membership copy assignment preserves actor and faction fact");

    std::cout
        << state.checks
        << " checks, "
        << state.failures
        << " failures\n";

    return state.failures == 0
        ? 0
        : 1;
}
