#include "oros/ai/actor_faction_reputation.hpp"

#include <cstdint>
#include <iostream>
#include <limits>
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
        oros::ai::ActorFactionReputation>
    make_reputation_from_ephemeral_faction(
        const oros::world::EntityId actor,
        const std::int32_t score)
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
                "Actor faction reputation test "
                "fixture could not construct "
                "its faction.");
        }

        return oros::ai::
            ActorFactionReputation::create(
                actor,
                faction_result.value(),
                score);
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
                ActorFactionReputation::
                    neutral_score),
            const std::int32_t>);

    static_assert(
        ActorFactionReputation::
            neutral_score == 0);

    static_assert(
        !std::is_default_constructible_v<
            ActorFactionReputation>);

    static_assert(
        std::is_copy_constructible_v<
            ActorFactionReputation>);

    static_assert(
        std::is_copy_assignable_v<
            ActorFactionReputation>);

    static_assert(
        std::is_nothrow_move_constructible_v<
            ActorFactionReputation>);

    static_assert(
        std::is_nothrow_move_assignable_v<
            ActorFactionReputation>);

    static_assert(
        std::is_same_v<
            decltype(
                ActorFactionReputation::create(
                    std::declval<EntityId>(),
                    std::declval<
                        const FactionKey&>(),
                    std::declval<
                        std::int32_t>())),
            Result<
                ActorFactionReputation>>);

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const ActorFactionReputation&>().
                    actor()),
            EntityId>);

    static_assert(
        noexcept(
            std::declval<
                const ActorFactionReputation&>().
                actor()));

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const ActorFactionReputation&>().
                    faction()),
            const FactionKey&>);

    static_assert(
        noexcept(
            std::declval<
                const ActorFactionReputation&>().
                faction()));

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const ActorFactionReputation&>().
                    score()),
            std::int32_t>);

    static_assert(
        noexcept(
            std::declval<
                const ActorFactionReputation&>().
                score()));

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

    const Result<
        ActorFactionReputation>
        invalid_actor_result =
            ActorFactionReputation::create(
                invalid_entity_id,
                citizens,
                1);

    check(
        state,
        !invalid_actor_result.has_value() &&
            invalid_actor_result.
                error().code ==
                ErrorCode::invalid_argument,
        "Invalid actor identity is rejected with invalid_argument");

    constexpr std::int32_t
        minimum_score =
            std::numeric_limits<
                std::int32_t>::min();

    constexpr std::int32_t
        maximum_score =
            std::numeric_limits<
                std::int32_t>::max();

    const Result<
        ActorFactionReputation>
        minimum_result =
            ActorFactionReputation::create(
                actor_a,
                citizens,
                minimum_score);

    check(
        state,
        minimum_result.has_value() &&
            minimum_result.value().
                score() ==
                minimum_score,
        "INT32_MIN is accepted exactly");

    const Result<
        ActorFactionReputation>
        negative_result =
            ActorFactionReputation::create(
                actor_a,
                citizens,
                -73125);

    check(
        state,
        negative_result.has_value() &&
            negative_result.value().
                score() ==
                -73125,
        "Negative interior reputation is accepted exactly");

    const Result<
        ActorFactionReputation>
        neutral_result =
            ActorFactionReputation::create(
                actor_a,
                citizens,
                ActorFactionReputation::
                    neutral_score);

    check(
        state,
        neutral_result.has_value() &&
            neutral_result.value().
                score() ==
                0,
        "Neutral zero reputation is accepted exactly");

    const Result<
        ActorFactionReputation>
        positive_result =
            ActorFactionReputation::create(
                actor_a,
                citizens,
                48192);

    check(
        state,
        positive_result.has_value() &&
            positive_result.value().
                score() ==
                48192,
        "Positive interior reputation is accepted exactly");

    const Result<
        ActorFactionReputation>
        maximum_result =
            ActorFactionReputation::create(
                actor_a,
                citizens,
                maximum_score);

    check(
        state,
        maximum_result.has_value() &&
            maximum_result.value().
                score() ==
                maximum_score,
        "INT32_MAX is accepted exactly");

    if (!positive_result.has_value())
    {
        return 1;
    }

    const ActorFactionReputation&
        reputation =
            positive_result.value();

    check(
        state,
        reputation.actor() ==
            actor_a,
        "Reputation actor accessor preserves exact EntityId");

    check(
        state,
        reputation.faction() ==
            citizens,
        "Reputation faction accessor preserves exact faction identity");

    check(
        state,
        reputation.score() ==
            48192,
        "Reputation score accessor preserves exact score");

    const Result<
        ActorFactionReputation>
        ephemeral_result =
            make_reputation_from_ephemeral_faction(
                actor_a,
                -915);

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
                "merchant-guild" &&
            ephemeral_result.value().
                score() ==
                -915,
        "Reputation owns faction identity beyond source key lifetime");

    const Result<
        ActorFactionReputation>
        equal_result =
            ActorFactionReputation::create(
                actor_a,
                citizens,
                48192);

    check(
        state,
        equal_result.has_value() &&
            equal_result.value() ==
                reputation,
        "Same actor faction and score produce equal reputation values");

    const Result<
        ActorFactionReputation>
        changed_score_result =
            ActorFactionReputation::create(
                actor_a,
                citizens,
                48193);

    check(
        state,
        changed_score_result.has_value() &&
            !(
                changed_score_result.value() ==
                reputation
            ),
        "Changed score produces a distinct reputation value");

    const Result<
        ActorFactionReputation>
        changed_faction_result =
            ActorFactionReputation::create(
                actor_a,
                merchants,
                48192);

    check(
        state,
        changed_faction_result.has_value() &&
            !(
                changed_faction_result.value() ==
                reputation
            ),
        "Changed faction produces a distinct reputation value");

    const Result<
        ActorFactionReputation>
        changed_actor_result =
            ActorFactionReputation::create(
                actor_b,
                citizens,
                48192);

    check(
        state,
        changed_actor_result.has_value() &&
            !(
                changed_actor_result.value() ==
                reputation
            ),
        "Changed actor produces a distinct reputation value");

    const ActorFactionReputation
        copied_reputation =
            reputation;

    check(
        state,
        copied_reputation ==
            reputation,
        "Reputation copy construction preserves the complete value");

    ActorFactionReputation
        assigned_reputation =
            changed_actor_result.value();

    assigned_reputation =
        reputation;

    check(
        state,
        assigned_reputation ==
            reputation,
        "Reputation copy assignment preserves the complete value");

    std::cout
        << state.checks
        << " checks, "
        << state.failures
        << " failures\n";

    return state.failures == 0
        ? 0
        : 1;
}
