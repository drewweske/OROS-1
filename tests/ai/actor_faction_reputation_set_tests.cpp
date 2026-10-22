#include "oros/ai/actor_faction_reputation_set.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
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
                ActorFactionReputation>
                    left,
        const std::span<
            const oros::ai::
                ActorFactionReputation>
                    right)
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
            ActorFactionReputation>);

    static_assert(
        std::is_nothrow_move_assignable_v<
            ActorFactionReputation>);

    static_assert(
        std::is_nothrow_move_constructible_v<
            ActorFactionReputationSet>);

    static_assert(
        std::is_nothrow_move_assignable_v<
            ActorFactionReputationSet>);

    static_assert(
        noexcept(
            std::declval<
                const ActorFactionReputationSet&>().
                contains(
                    std::declval<EntityId>(),
                    std::declval<
                        const FactionKey&>())));

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const ActorFactionReputationSet&>().
                    score(
                        std::declval<EntityId>(),
                        std::declval<
                            const FactionKey&>())),
            Result<std::int32_t>>);

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const ActorFactionReputationSet&>().
                    reputations_for_actor(
                        std::declval<EntityId>())),
            Result<
                std::span<
                    const ActorFactionReputation>>>);

    static_assert(
        noexcept(
            std::declval<
                const ActorFactionReputationSet&>().
                reputations_in_canonical_order()));

    static_assert(
        noexcept(
            std::declval<
                const ActorFactionReputationSet&>().
                size()));

    static_assert(
        noexcept(
            std::declval<
                const ActorFactionReputationSet&>().
                empty()));

    TestState state{};

    constexpr EntityId actor_a{
        0x4F524F53ULL,
        101ULL
    };

    constexpr EntityId actor_b{
        0x4F524F53ULL,
        202ULL
    };

    constexpr EntityId actor_c{
        0x4F524F53ULL,
        303ULL
    };

    auto citizens_result =
        FactionKey::create(
            "oros",
            "citizens");

    auto merchants_result =
        FactionKey::create(
            "oros",
            "merchants");

    auto wardens_result =
        FactionKey::create(
            "oros",
            "wardens");

    auto guild_result =
        FactionKey::create(
            "mod.example",
            "merchant-guild");

    if (
        !citizens_result.has_value() ||
        !merchants_result.has_value() ||
        !wardens_result.has_value() ||
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
        wardens =
            wardens_result.value();

    const FactionKey&
        guild =
            guild_result.value();

    auto a_citizens_zero_result =
        ActorFactionReputation::create(
            actor_a,
            citizens,
            ActorFactionReputation::
                neutral_score);

    auto a_citizens_positive_result =
        ActorFactionReputation::create(
            actor_a,
            citizens,
            48192);

    auto a_merchants_positive_result =
        ActorFactionReputation::create(
            actor_a,
            merchants,
            975);

    auto a_wardens_maximum_result =
        ActorFactionReputation::create(
            actor_a,
            wardens,
            std::numeric_limits<
                std::int32_t>::max());

    auto a_guild_minimum_result =
        ActorFactionReputation::create(
            actor_a,
            guild,
            std::numeric_limits<
                std::int32_t>::min());

    auto b_citizens_negative_result =
        ActorFactionReputation::create(
            actor_b,
            citizens,
            -73125);

    if (
        !a_citizens_zero_result.has_value() ||
        !a_citizens_positive_result.has_value() ||
        !a_merchants_positive_result.has_value() ||
        !a_wardens_maximum_result.has_value() ||
        !a_guild_minimum_result.has_value() ||
        !b_citizens_negative_result.has_value())
    {
        return 1;
    }

    const ActorFactionReputation&
        a_citizens_zero =
            a_citizens_zero_result.value();

    const ActorFactionReputation&
        a_citizens_positive =
            a_citizens_positive_result.value();

    const ActorFactionReputation&
        a_merchants_positive =
            a_merchants_positive_result.value();

    const ActorFactionReputation&
        a_wardens_maximum =
            a_wardens_maximum_result.value();

    const ActorFactionReputation&
        a_guild_minimum =
            a_guild_minimum_result.value();

    const ActorFactionReputation&
        b_citizens_negative =
            b_citizens_negative_result.value();

    ActorFactionReputationSet set{};

    check(
        state,
        set.empty() &&
            set.size() == 0U &&
            set.
                reputations_in_canonical_order().
                empty(),
        "Default reputation set is empty");

    const Status first_set =
        set.set_reputation(
            a_citizens_zero);

    check(
        state,
        first_set.has_value() &&
            set.size() == 1U &&
            !set.empty(),
        "First reputation set inserts an actor faction fact");

    const Status identical_set =
        set.set_reputation(
            a_citizens_zero);

    const auto identical_score =
        set.score(
            actor_a,
            citizens);

    check(
        state,
        identical_set.has_value() &&
            set.size() == 1U &&
            identical_score.has_value() &&
            identical_score.value() == 0,
        "Setting the exact same reputation value is a successful no-op");

    const Status replacement =
        set.set_reputation(
            a_citizens_positive);

    const auto replacement_score =
        set.score(
            actor_a,
            citizens);

    check(
        state,
        replacement.has_value() &&
            replacement_score.has_value() &&
            replacement_score.value() ==
                48192,
        "Changed score replaces the existing actor faction reputation");

    check(
        state,
        set.size() == 1U,
        "Reputation replacement preserves set size");

    const Status second_faction =
        set.set_reputation(
            a_merchants_positive);

    check(
        state,
        second_faction.has_value() &&
            set.size() == 2U &&
            set.contains(
                actor_a,
                citizens) &&
            set.contains(
                actor_a,
                merchants),
        "One actor may hold multiple faction reputations");

    const Status second_actor =
        set.set_reputation(
            b_citizens_negative);

    const auto actor_a_score =
        set.score(
            actor_a,
            citizens);

    const auto actor_b_score =
        set.score(
            actor_b,
            citizens);

    check(
        state,
        second_actor.has_value() &&
            set.size() == 3U &&
            actor_a_score.has_value() &&
            actor_a_score.value() ==
                48192 &&
            actor_b_score.has_value() &&
            actor_b_score.value() ==
                -73125,
        "Different actors hold independent faction reputations");

    check(
        state,
        set.contains(
            actor_a,
            citizens),
        "contains finds an exact actor faction reputation");

    check(
        state,
        !set.contains(
            actor_a,
            guild),
        "contains returns false for a missing actor faction reputation");

    check(
        state,
        !set.contains(
            invalid_entity_id,
            citizens),
        "contains returns false for an invalid actor identity");

    ActorFactionReputationSet
        score_set{};

    if (
        !score_set.
            set_reputation(
                a_guild_minimum).
            has_value() ||
        !score_set.
            set_reputation(
                b_citizens_negative).
            has_value() ||
        !score_set.
            set_reputation(
                a_citizens_zero).
            has_value() ||
        !score_set.
            set_reputation(
                a_merchants_positive).
            has_value() ||
        !score_set.
            set_reputation(
                a_wardens_maximum).
            has_value())
    {
        return 1;
    }

    const auto minimum_query =
        score_set.score(
            actor_a,
            guild);

    check(
        state,
        minimum_query.has_value() &&
            minimum_query.value() ==
                std::numeric_limits<
                    std::int32_t>::min(),
        "Score query returns INT32_MIN reputation");

    const auto negative_query =
        score_set.score(
            actor_b,
            citizens);

    check(
        state,
        negative_query.has_value() &&
            negative_query.value() ==
                -73125,
        "Score query returns negative interior reputation");

    const auto zero_query =
        score_set.score(
            actor_a,
            citizens);

    check(
        state,
        zero_query.has_value() &&
            zero_query.value() ==
                ActorFactionReputation::
                    neutral_score,
        "Score query returns explicit zero reputation");

    const auto positive_query =
        score_set.score(
            actor_a,
            merchants);

    check(
        state,
        positive_query.has_value() &&
            positive_query.value() ==
                975,
        "Score query returns positive interior reputation");

    const auto maximum_query =
        score_set.score(
            actor_a,
            wardens);

    check(
        state,
        maximum_query.has_value() &&
            maximum_query.value() ==
                std::numeric_limits<
                    std::int32_t>::max(),
        "Score query returns INT32_MAX reputation");

    const auto absent_score =
        score_set.score(
            actor_b,
            merchants);

    check(
        state,
        !absent_score.has_value() &&
            absent_score.error().code ==
                ErrorCode::not_found,
        "Absent reputation score returns not_found");

    const auto invalid_actor_score =
        score_set.score(
            invalid_entity_id,
            citizens);

    check(
        state,
        !invalid_actor_score.has_value() &&
            invalid_actor_score.error().code ==
                ErrorCode::invalid_argument,
        "Invalid actor reputation score returns invalid_argument");

    check(
        state,
        zero_query.has_value() &&
            zero_query.value() == 0 &&
            !absent_score.has_value() &&
            absent_score.error().code ==
                ErrorCode::not_found,
        "Explicit zero reputation remains distinguishable from absence");

    ActorFactionReputationSet
        removal_set{};

    if (
        !removal_set.
            set_reputation(
                a_citizens_positive).
            has_value() ||
        !removal_set.
            set_reputation(
                a_merchants_positive).
            has_value() ||
        !removal_set.
            set_reputation(
                b_citizens_negative).
            has_value())
    {
        return 1;
    }

    const Status invalid_actor_remove =
        removal_set.remove(
            invalid_entity_id,
            citizens);

    check(
        state,
        !invalid_actor_remove.has_value() &&
            invalid_actor_remove.error().code ==
                ErrorCode::invalid_argument &&
            removal_set.size() == 3U,
        "Invalid actor reputation removal returns invalid_argument");

    const Status missing_remove =
        removal_set.remove(
            actor_a,
            guild);

    check(
        state,
        !missing_remove.has_value() &&
            missing_remove.error().code ==
                ErrorCode::not_found &&
            removal_set.size() == 3U,
        "Missing reputation removal returns not_found");

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
        "Exact actor faction reputation removal succeeds");

    const auto preserved_other_faction =
        removal_set.score(
            actor_a,
            merchants);

    check(
        state,
        preserved_other_faction.has_value() &&
            preserved_other_faction.value() ==
                975 &&
            removal_set.contains(
                actor_b,
                citizens),
        "Removing one faction preserves the actor's other faction and other actors");

    ActorFactionReputationSet
        canonical_set{};

    if (
        !canonical_set.
            set_reputation(
                a_wardens_maximum).
            has_value() ||
        !canonical_set.
            set_reputation(
                b_citizens_negative).
            has_value() ||
        !canonical_set.
            set_reputation(
                a_merchants_positive).
            has_value() ||
        !canonical_set.
            set_reputation(
                a_guild_minimum).
            has_value() ||
        !canonical_set.
            set_reputation(
                a_citizens_positive).
            has_value())
    {
        return 1;
    }

    const auto invalid_actor_range =
        canonical_set.
            reputations_for_actor(
                invalid_entity_id);

    check(
        state,
        !invalid_actor_range.has_value() &&
            invalid_actor_range.error().code ==
                ErrorCode::invalid_argument,
        "Invalid actor reputation range returns invalid_argument");

    const auto missing_actor_range =
        canonical_set.
            reputations_for_actor(
                actor_c);

    check(
        state,
        missing_actor_range.has_value() &&
            missing_actor_range.value().
                empty(),
        "Valid actor with no reputation facts returns an empty range");

    const auto actor_a_range =
        canonical_set.
            reputations_for_actor(
                actor_a);

    check(
        state,
        actor_a_range.has_value() &&
            actor_a_range.value().
                size() == 4U &&
            actor_a_range.value()[0].
                actor() ==
                actor_a &&
            actor_a_range.value()[1].
                actor() ==
                actor_a &&
            actor_a_range.value()[2].
                actor() ==
                actor_a &&
            actor_a_range.value()[3].
                actor() ==
                actor_a,
        "Actor reputation range is contiguous");

    check(
        state,
        actor_a_range.has_value() &&
            actor_a_range.value().
                size() == 4U &&
            actor_a_range.value()[0].
                faction() ==
                guild &&
            actor_a_range.value()[1].
                faction() ==
                citizens &&
            actor_a_range.value()[2].
                faction() ==
                merchants &&
            actor_a_range.value()[3].
                faction() ==
                wardens,
        "Actor reputation range is faction-canonical");

    const auto canonical =
        canonical_set.
            reputations_in_canonical_order();

    check(
        state,
        canonical.size() == 5U &&
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
                actor_a &&
            canonical[3].faction() ==
                wardens &&
            canonical[4].actor() ==
                actor_b &&
            canonical[4].faction() ==
                citizens,
        "Full reputation order is actor first then faction namespace and name");

    ActorFactionReputationSet
        alternate_history{};

    if (
        !alternate_history.
            set_reputation(
                a_citizens_positive).
            has_value() ||
        !alternate_history.
            set_reputation(
                a_guild_minimum).
            has_value() ||
        !alternate_history.
            set_reputation(
                a_wardens_maximum).
            has_value() ||
        !alternate_history.
            set_reputation(
                a_merchants_positive).
            has_value() ||
        !alternate_history.
            set_reputation(
                b_citizens_negative).
            has_value())
    {
        return 1;
    }

    check(
        state,
        same_sequence(
            canonical_set.
                reputations_in_canonical_order(),
            alternate_history.
                reputations_in_canonical_order()),
        "Different insertion histories yield identical reputation canonical order");

    check(
        state,
        canonical_set.size() == 5U &&
            !canonical_set.empty() &&
            removal_set.size() == 2U &&
            !removal_set.empty() &&
            ActorFactionReputationSet{}.
                empty(),
        "Reputation set size and empty semantics remain consistent");

    std::cout
        << state.checks
        << " checks, "
        << state.failures
        << " failures\n";

    return state.failures == 0
        ? 0
        : 1;
}
