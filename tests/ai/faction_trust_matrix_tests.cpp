#include "oros/ai/faction_trust_matrix.hpp"

#include <cstddef>
#include <cstdint>
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
            const oros::ai::FactionTrust>
                left,
        const std::span<
            const oros::ai::FactionTrust>
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

    static_assert(
        std::is_nothrow_move_constructible_v<
            FactionTrust>);

    static_assert(
        std::is_nothrow_move_assignable_v<
            FactionTrust>);

    static_assert(
        std::is_nothrow_move_constructible_v<
            FactionTrustMatrix>);

    static_assert(
        std::is_nothrow_move_assignable_v<
            FactionTrustMatrix>);

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const FactionTrustMatrix&>().
                    score(
                        std::declval<
                            const FactionKey&>(),
                        std::declval<
                            const FactionKey&>())),
            Result<std::int32_t>>);

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const FactionTrustMatrix&>().
                    trusts_from(
                        std::declval<
                            const FactionKey&>())),
            std::span<
                const FactionTrust>>);

    static_assert(
        noexcept(
            std::declval<
                const FactionTrustMatrix&>().
                contains(
                    std::declval<
                        const FactionKey&>(),
                    std::declval<
                        const FactionKey&>())));

    static_assert(
        noexcept(
            std::declval<
                const FactionTrustMatrix&>().
                trusts_from(
                    std::declval<
                        const FactionKey&>())));

    static_assert(
        noexcept(
            std::declval<
                const FactionTrustMatrix&>().
                trusts_in_canonical_order()));

    static_assert(
        noexcept(
            std::declval<
                const FactionTrustMatrix&>().
                size()));

    static_assert(
        noexcept(
            std::declval<
                const FactionTrustMatrix&>().
                empty()));

    TestState state{};

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

    auto citizens_merchants_neutral_result =
        FactionTrust::create(
            citizens,
            merchants,
            FactionTrust::neutral_score);

    auto citizens_merchants_positive_result =
        FactionTrust::create(
            citizens,
            merchants,
            2500);

    auto merchants_citizens_negative_result =
        FactionTrust::create(
            merchants,
            citizens,
            -3100);

    auto citizens_guild_minimum_result =
        FactionTrust::create(
            citizens,
            guild,
            FactionTrust::minimum_score);

    auto citizens_wardens_maximum_result =
        FactionTrust::create(
            citizens,
            wardens,
            FactionTrust::maximum_score);

    auto guild_citizens_positive_result =
        FactionTrust::create(
            guild,
            citizens,
            4321);

    if (
        !citizens_merchants_neutral_result.has_value() ||
        !citizens_merchants_positive_result.has_value() ||
        !merchants_citizens_negative_result.has_value() ||
        !citizens_guild_minimum_result.has_value() ||
        !citizens_wardens_maximum_result.has_value() ||
        !guild_citizens_positive_result.has_value())
    {
        return 1;
    }

    const FactionTrust&
        citizens_merchants_neutral =
            citizens_merchants_neutral_result.value();

    const FactionTrust&
        citizens_merchants_positive =
            citizens_merchants_positive_result.value();

    const FactionTrust&
        merchants_citizens_negative =
            merchants_citizens_negative_result.value();

    const FactionTrust&
        citizens_guild_minimum =
            citizens_guild_minimum_result.value();

    const FactionTrust&
        citizens_wardens_maximum =
            citizens_wardens_maximum_result.value();

    const FactionTrust&
        guild_citizens_positive =
            guild_citizens_positive_result.value();

    FactionTrustMatrix matrix{};

    check(
        state,
        matrix.empty() &&
            matrix.size() == 0U &&
            matrix.
                trusts_in_canonical_order().
                empty(),
        "Default trust matrix is empty");

    const Status first_set =
        matrix.set_trust(
            citizens_merchants_neutral);

    check(
        state,
        first_set.has_value() &&
            matrix.size() == 1U &&
            !matrix.empty(),
        "First set inserts a directed trust cell");

    const Status identical_set =
        matrix.set_trust(
            citizens_merchants_neutral);

    const auto identical_score =
        matrix.score(
            citizens,
            merchants);

    check(
        state,
        identical_set.has_value() &&
            matrix.size() == 1U &&
            identical_score.has_value() &&
            identical_score.value() ==
                FactionTrust::neutral_score,
        "Setting the exact same trust value is a successful no-op");

    const Status replacement =
        matrix.set_trust(
            citizens_merchants_positive);

    const auto replacement_score =
        matrix.score(
            citizens,
            merchants);

    check(
        state,
        replacement.has_value() &&
            replacement_score.has_value() &&
            replacement_score.value() ==
                2500,
        "Changed score replaces the existing directed trust cell");

    check(
        state,
        matrix.size() == 1U,
        "Trust replacement preserves matrix size");

    const Status reverse_set =
        matrix.set_trust(
            merchants_citizens_negative);

    const auto forward_after_reverse =
        matrix.score(
            citizens,
            merchants);

    const auto reverse_after_set =
        matrix.score(
            merchants,
            citizens);

    check(
        state,
        reverse_set.has_value() &&
            matrix.size() == 2U &&
            forward_after_reverse.has_value() &&
            forward_after_reverse.value() ==
                2500 &&
            reverse_after_set.has_value() &&
            reverse_after_set.value() ==
                -3100,
        "Reverse trust direction remains an independent cell");

    check(
        state,
        matrix.contains(
            citizens,
            merchants),
        "contains finds an exact directed trust pair");

    check(
        state,
        !matrix.contains(
            citizens,
            guild),
        "contains returns false for a missing directed trust pair");

    check(
        state,
        !matrix.contains(
            citizens,
            citizens),
        "contains returns false for a self pair");

    FactionTrustMatrix
        score_matrix{};

    if (
        !score_matrix.
            set_trust(
                citizens_guild_minimum).
            has_value() ||
        !score_matrix.
            set_trust(
                merchants_citizens_negative).
            has_value() ||
        !score_matrix.
            set_trust(
                citizens_merchants_neutral).
            has_value() ||
        !score_matrix.
            set_trust(
                guild_citizens_positive).
            has_value() ||
        !score_matrix.
            set_trust(
                citizens_wardens_maximum).
            has_value())
    {
        return 1;
    }

    const auto minimum_query =
        score_matrix.score(
            citizens,
            guild);

    check(
        state,
        minimum_query.has_value() &&
            minimum_query.value() ==
                FactionTrust::minimum_score,
        "Score query returns minimum trust");

    const auto negative_query =
        score_matrix.score(
            merchants,
            citizens);

    check(
        state,
        negative_query.has_value() &&
            negative_query.value() ==
                -3100,
        "Score query returns negative interior trust");

    const auto zero_query =
        score_matrix.score(
            citizens,
            merchants);

    check(
        state,
        zero_query.has_value() &&
            zero_query.value() ==
                FactionTrust::neutral_score,
        "Score query returns explicit zero trust");

    const auto positive_query =
        score_matrix.score(
            guild,
            citizens);

    check(
        state,
        positive_query.has_value() &&
            positive_query.value() ==
                4321,
        "Score query returns positive interior trust");

    const auto maximum_query =
        score_matrix.score(
            citizens,
            wardens);

    check(
        state,
        maximum_query.has_value() &&
            maximum_query.value() ==
                FactionTrust::maximum_score,
        "Score query returns maximum trust");

    const auto missing_query =
        score_matrix.score(
            guild,
            merchants);

    check(
        state,
        !missing_query.has_value() &&
            missing_query.error().code ==
                ErrorCode::not_found,
        "Missing directed trust score returns not_found");

    const auto self_query =
        score_matrix.score(
            citizens,
            citizens);

    check(
        state,
        !self_query.has_value() &&
            self_query.error().code ==
                ErrorCode::invalid_argument,
        "Self-pair trust score query returns invalid_argument");

    check(
        state,
        zero_query.has_value() &&
            zero_query.value() == 0 &&
            !missing_query.has_value() &&
            missing_query.error().code ==
                ErrorCode::not_found,
        "Explicit zero trust remains distinguishable from absence");

    FactionTrustMatrix
        removal_matrix{};

    if (
        !removal_matrix.
            set_trust(
                citizens_merchants_positive).
            has_value() ||
        !removal_matrix.
            set_trust(
                merchants_citizens_negative).
            has_value())
    {
        return 1;
    }

    const Status missing_remove =
        removal_matrix.remove(
            citizens,
            guild);

    check(
        state,
        !missing_remove.has_value() &&
            missing_remove.error().code ==
                ErrorCode::not_found &&
            removal_matrix.size() == 2U,
        "Removing a missing directed trust pair returns not_found");

    const Status self_remove =
        removal_matrix.remove(
            citizens,
            citizens);

    check(
        state,
        !self_remove.has_value() &&
            self_remove.error().code ==
                ErrorCode::invalid_argument &&
            removal_matrix.size() == 2U,
        "Removing a self trust pair returns invalid_argument");

    const Status exact_remove =
        removal_matrix.remove(
            citizens,
            merchants);

    check(
        state,
        exact_remove.has_value() &&
            removal_matrix.size() == 1U &&
            !removal_matrix.contains(
                citizens,
                merchants),
        "Removing an exact directed trust pair succeeds");

    const auto reverse_after_remove =
        removal_matrix.score(
            merchants,
            citizens);

    check(
        state,
        removal_matrix.contains(
            merchants,
            citizens) &&
            reverse_after_remove.has_value() &&
            reverse_after_remove.value() ==
                -3100,
        "Removing A-to-B preserves B-to-A trust");

    FactionTrustMatrix
        canonical_matrix{};

    if (
        !canonical_matrix.
            set_trust(
                citizens_wardens_maximum).
            has_value() ||
        !canonical_matrix.
            set_trust(
                guild_citizens_positive).
            has_value() ||
        !canonical_matrix.
            set_trust(
                merchants_citizens_negative).
            has_value() ||
        !canonical_matrix.
            set_trust(
                citizens_guild_minimum).
            has_value() ||
        !canonical_matrix.
            set_trust(
                citizens_merchants_positive).
            has_value())
    {
        return 1;
    }

    const auto missing_source_range =
        canonical_matrix.
            trusts_from(
                wardens);

    check(
        state,
        missing_source_range.empty(),
        "Missing source returns an empty trust span");

    const auto citizens_range =
        canonical_matrix.
            trusts_from(
                citizens);

    check(
        state,
        citizens_range.size() == 3U &&
            citizens_range[0].source() ==
                citizens &&
            citizens_range[1].source() ==
                citizens &&
            citizens_range[2].source() ==
                citizens,
        "trusts_from returns one contiguous source range");

    check(
        state,
        citizens_range.size() == 3U &&
            citizens_range[0].target() ==
                guild &&
            citizens_range[1].target() ==
                merchants &&
            citizens_range[2].target() ==
                wardens,
        "Trust source range is canonically ordered by target namespace then name");

    const auto canonical =
        canonical_matrix.
            trusts_in_canonical_order();

    check(
        state,
        canonical.size() == 5U &&
            canonical[0].source() ==
                guild &&
            canonical[0].target() ==
                citizens &&
            canonical[1].source() ==
                citizens &&
            canonical[1].target() ==
                guild &&
            canonical[2].source() ==
                citizens &&
            canonical[2].target() ==
                merchants &&
            canonical[3].source() ==
                citizens &&
            canonical[3].target() ==
                wardens &&
            canonical[4].source() ==
                merchants &&
            canonical[4].target() ==
                citizens,
        "Full trust canonical order is source namespace/name then target namespace/name");

    FactionTrustMatrix
        alternate_history{};

    if (
        !alternate_history.
            set_trust(
                merchants_citizens_negative).
            has_value() ||
        !alternate_history.
            set_trust(
                citizens_merchants_positive).
            has_value() ||
        !alternate_history.
            set_trust(
                citizens_guild_minimum).
            has_value() ||
        !alternate_history.
            set_trust(
                guild_citizens_positive).
            has_value() ||
        !alternate_history.
            set_trust(
                citizens_wardens_maximum).
            has_value())
    {
        return 1;
    }

    check(
        state,
        same_sequence(
            canonical_matrix.
                trusts_in_canonical_order(),
            alternate_history.
                trusts_in_canonical_order()),
        "Different insertion histories yield identical trust canonical order");

    check(
        state,
        canonical_matrix.size() == 5U &&
            !canonical_matrix.empty() &&
            removal_matrix.size() == 1U &&
            !removal_matrix.empty() &&
            FactionTrustMatrix{}.empty(),
        "Trust matrix size and empty semantics remain consistent");

    std::cout
        << state.checks
        << " checks, "
        << state.failures
        << " failures\n";

    return state.failures == 0
        ? 0
        : 1;
}
