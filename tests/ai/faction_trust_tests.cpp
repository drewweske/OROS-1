#include "oros/ai/faction_trust.hpp"

#include <cstdint>
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
        oros::ai::FactionTrust>
    make_trust_from_ephemeral_keys()
    {
        auto source_result =
            oros::ai::FactionKey::create(
                "mod.example",
                "source-faction");

        auto target_result =
            oros::ai::FactionKey::create(
                "mod.example",
                "target-faction");

        if (
            !source_result.has_value() ||
            !target_result.has_value())
        {
            return oros::foundation::fail(
                oros::foundation::ErrorCode::
                    internal_failure,
                "Faction trust test fixture "
                "could not create ephemeral "
                "faction keys.");
        }

        return oros::ai::
            FactionTrust::create(
                source_result.value(),
                target_result.value(),
                4321);
    }
}

int main()
{
    using namespace oros::ai;
    using namespace oros::foundation;

    static_assert(
        std::is_same_v<
            std::remove_cv_t<
                decltype(
                    FactionTrust::
                        minimum_score)>,
            std::int32_t>);

    static_assert(
        std::is_same_v<
            std::remove_cv_t<
                decltype(
                    FactionTrust::
                        neutral_score)>,
            std::int32_t>);

    static_assert(
        std::is_same_v<
            std::remove_cv_t<
                decltype(
                    FactionTrust::
                        maximum_score)>,
            std::int32_t>);

    static_assert(
        FactionTrust::minimum_score ==
            -10000);

    static_assert(
        FactionTrust::neutral_score ==
            0);

    static_assert(
        FactionTrust::maximum_score ==
            10000);

    static_assert(
        !std::is_default_constructible_v<
            FactionTrust>);

    static_assert(
        std::is_copy_constructible_v<
            FactionTrust>);

    static_assert(
        std::is_copy_assignable_v<
            FactionTrust>);

    static_assert(
        std::is_nothrow_move_constructible_v<
            FactionTrust>);

    static_assert(
        std::is_nothrow_move_assignable_v<
            FactionTrust>);

    static_assert(
        std::is_same_v<
            decltype(
                FactionTrust::create(
                    std::declval<
                        const FactionKey&>(),
                    std::declval<
                        const FactionKey&>(),
                    std::declval<
                        std::int32_t>())),
            Result<FactionTrust>>);

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const FactionTrust&>().
                    source()),
            const FactionKey&>);

    static_assert(
        noexcept(
            std::declval<
                const FactionTrust&>().
                source()));

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const FactionTrust&>().
                    target()),
            const FactionKey&>);

    static_assert(
        noexcept(
            std::declval<
                const FactionTrust&>().
                target()));

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const FactionTrust&>().
                    score()),
            std::int32_t>);

    static_assert(
        noexcept(
            std::declval<
                const FactionTrust&>().
                score()));

    TestState state{};

    auto source_result =
        FactionKey::create(
            "oros",
            "citizens");

    auto target_result =
        FactionKey::create(
            "oros",
            "merchants");

    if (
        !source_result.has_value() ||
        !target_result.has_value())
    {
        return 1;
    }

    const FactionKey&
        source =
            source_result.value();

    const FactionKey&
        target =
            target_result.value();

    const auto minimum_result =
        FactionTrust::create(
            source,
            target,
            FactionTrust::minimum_score);

    check(
        state,
        minimum_result.has_value() &&
            minimum_result.value().score() ==
                FactionTrust::minimum_score,
        "Minimum trust score constructs successfully");

    const auto neutral_result =
        FactionTrust::create(
            source,
            target,
            FactionTrust::neutral_score);

    check(
        state,
        neutral_result.has_value() &&
            neutral_result.value().score() ==
                FactionTrust::neutral_score,
        "Neutral trust score constructs successfully");

    const auto maximum_result =
        FactionTrust::create(
            source,
            target,
            FactionTrust::maximum_score);

    check(
        state,
        maximum_result.has_value() &&
            maximum_result.value().score() ==
                FactionTrust::maximum_score,
        "Maximum trust score constructs successfully");

    const auto negative_result =
        FactionTrust::create(
            source,
            target,
            -4321);

    check(
        state,
        negative_result.has_value() &&
            negative_result.value().score() ==
                -4321,
        "Negative interior trust score constructs successfully");

    const auto positive_result =
        FactionTrust::create(
            source,
            target,
            4321);

    check(
        state,
        positive_result.has_value() &&
            positive_result.value().score() ==
                4321,
        "Positive interior trust score constructs successfully");

    const auto below_minimum_result =
        FactionTrust::create(
            source,
            target,
            FactionTrust::minimum_score - 1);

    check(
        state,
        !below_minimum_result.has_value() &&
            below_minimum_result.
                error().code ==
                ErrorCode::invalid_argument,
        "Trust score below minimum is rejected with invalid_argument");

    const auto above_maximum_result =
        FactionTrust::create(
            source,
            target,
            FactionTrust::maximum_score + 1);

    check(
        state,
        !above_maximum_result.has_value() &&
            above_maximum_result.
                error().code ==
                ErrorCode::invalid_argument,
        "Trust score above maximum is rejected with invalid_argument");

    const auto self_result =
        FactionTrust::create(
            source,
            source,
            FactionTrust::neutral_score);

    check(
        state,
        !self_result.has_value() &&
            self_result.error().code ==
                ErrorCode::invalid_argument,
        "Same source and target are rejected with invalid_argument");

    if (
        !positive_result.has_value() ||
        !negative_result.has_value())
    {
        return 1;
    }

    const FactionTrust&
        positive =
            positive_result.value();

    check(
        state,
        positive.source() ==
            source,
        "Source accessor preserves trust source");

    check(
        state,
        positive.target() ==
            target,
        "Target accessor preserves trust target");

    check(
        state,
        positive.score() ==
            4321,
        "Score accessor preserves exact signed integer");

    const auto ephemeral_result =
        make_trust_from_ephemeral_keys();

    check(
        state,
        ephemeral_result.has_value() &&
            ephemeral_result.value().
                source().
                faction_namespace() ==
                "mod.example" &&
            ephemeral_result.value().
                source().
                faction_name() ==
                "source-faction",
        "Trust owns source beyond source-key lifetime");

    check(
        state,
        ephemeral_result.has_value() &&
            ephemeral_result.value().
                target().
                faction_namespace() ==
                "mod.example" &&
            ephemeral_result.value().
                target().
                faction_name() ==
                "target-faction",
        "Trust owns target beyond target-key lifetime");

    const auto reverse_result =
        FactionTrust::create(
            target,
            source,
            4321);

    check(
        state,
        reverse_result.has_value() &&
            positive.source() ==
                source &&
            positive.target() ==
                target,
        "A-to-B trust preserves source-to-target direction");

    check(
        state,
        reverse_result.has_value() &&
            reverse_result.value().
                source() ==
                target &&
            reverse_result.value().
                target() ==
                source &&
            !(reverse_result.value() ==
                positive),
        "A-to-B trust differs from B-to-A trust");

    const auto equal_result =
        FactionTrust::create(
            source,
            target,
            4321);

    check(
        state,
        equal_result.has_value() &&
            equal_result.value() ==
                positive,
        "Same endpoints and score produce equal trust values");

    check(
        state,
        !(positive ==
            negative_result.value()),
        "Same endpoints with different score produce unequal trust values");

    const FactionTrust
        copied_trust =
            positive;

    check(
        state,
        copied_trust ==
            positive,
        "Trust copy preserves complete value");

    FactionTrust
        assigned_trust =
            negative_result.value();

    assigned_trust =
        positive;

    check(
        state,
        assigned_trust ==
            positive,
        "Trust copy assignment preserves complete value");

    std::cout
        << state.checks
        << " checks, "
        << state.failures
        << " failures\n";

    return state.failures == 0
        ? 0
        : 1;
}
