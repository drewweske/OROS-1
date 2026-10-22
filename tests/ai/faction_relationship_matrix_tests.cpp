#include "oros/ai/faction_relationship_matrix.hpp"

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
                FactionRelationship> left,
        const std::span<
            const oros::ai::
                FactionRelationship> right)
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
            FactionRelationship>);

    static_assert(
        std::is_nothrow_move_assignable_v<
            FactionRelationship>);

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const FactionRelationshipMatrix&>().
                    disposition(
                        std::declval<
                            const FactionKey&>(),
                        std::declval<
                            const FactionKey&>())),
            Result<
                FactionRelationshipDisposition>>);

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const FactionRelationshipMatrix&>().
                    relationships_from(
                        std::declval<
                            const FactionKey&>())),
            std::span<
                const FactionRelationship>>);

    static_assert(
        noexcept(
            std::declval<
                const FactionRelationshipMatrix&>().
                contains(
                    std::declval<
                        const FactionKey&>(),
                    std::declval<
                        const FactionKey&>())));

    static_assert(
        noexcept(
            std::declval<
                const FactionRelationshipMatrix&>().
                relationships_from(
                    std::declval<
                        const FactionKey&>())));

    static_assert(
        noexcept(
            std::declval<
                const FactionRelationshipMatrix&>().
                relationships_in_canonical_order()));

    static_assert(
        noexcept(
            std::declval<
                const FactionRelationshipMatrix&>().
                size()));

    static_assert(
        noexcept(
            std::declval<
                const FactionRelationshipMatrix&>().
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
        FactionRelationship::create(
            citizens,
            merchants,
            FactionRelationshipDisposition::
                neutral);

    auto citizens_merchants_allied_result =
        FactionRelationship::create(
            citizens,
            merchants,
            FactionRelationshipDisposition::
                allied);

    auto citizens_merchants_hostile_result =
        FactionRelationship::create(
            citizens,
            merchants,
            FactionRelationshipDisposition::
                hostile);

    auto merchants_citizens_hostile_result =
        FactionRelationship::create(
            merchants,
            citizens,
            FactionRelationshipDisposition::
                hostile);

    auto citizens_guild_hostile_result =
        FactionRelationship::create(
            citizens,
            guild,
            FactionRelationshipDisposition::
                hostile);

    auto citizens_wardens_allied_result =
        FactionRelationship::create(
            citizens,
            wardens,
            FactionRelationshipDisposition::
                allied);

    auto guild_citizens_neutral_result =
        FactionRelationship::create(
            guild,
            citizens,
            FactionRelationshipDisposition::
                neutral);

    if (
        !citizens_merchants_neutral_result.has_value() ||
        !citizens_merchants_allied_result.has_value() ||
        !citizens_merchants_hostile_result.has_value() ||
        !merchants_citizens_hostile_result.has_value() ||
        !citizens_guild_hostile_result.has_value() ||
        !citizens_wardens_allied_result.has_value() ||
        !guild_citizens_neutral_result.has_value())
    {
        return 1;
    }

    const FactionRelationship&
        citizens_merchants_neutral =
            citizens_merchants_neutral_result.value();

    const FactionRelationship&
        citizens_merchants_allied =
            citizens_merchants_allied_result.value();

    const FactionRelationship&
        citizens_merchants_hostile =
            citizens_merchants_hostile_result.value();

    const FactionRelationship&
        merchants_citizens_hostile =
            merchants_citizens_hostile_result.value();

    const FactionRelationship&
        citizens_guild_hostile =
            citizens_guild_hostile_result.value();

    const FactionRelationship&
        citizens_wardens_allied =
            citizens_wardens_allied_result.value();

    const FactionRelationship&
        guild_citizens_neutral =
            guild_citizens_neutral_result.value();

    FactionRelationshipMatrix matrix{};

    check(
        state,
        matrix.empty() &&
            matrix.size() == 0U &&
            matrix.
                relationships_in_canonical_order().
                empty(),
        "Default relationship matrix is empty");

    const Status first_set =
        matrix.set_relationship(
            citizens_merchants_neutral);

    check(
        state,
        first_set.has_value() &&
            matrix.size() == 1U &&
            !matrix.empty(),
        "First set inserts a directed relationship cell");

    const Status same_set =
        matrix.set_relationship(
            citizens_merchants_neutral);

    check(
        state,
        same_set.has_value() &&
            matrix.size() == 1U &&
            matrix.disposition(
                citizens,
                merchants).has_value() &&
            matrix.disposition(
                citizens,
                merchants).value() ==
                FactionRelationshipDisposition::
                    neutral,
        "Setting the exact same relationship is a successful no-op");

    const Status replacement =
        matrix.set_relationship(
            citizens_merchants_allied);

    check(
        state,
        replacement.has_value() &&
            matrix.size() == 1U &&
            matrix.disposition(
                citizens,
                merchants).has_value() &&
            matrix.disposition(
                citizens,
                merchants).value() ==
                FactionRelationshipDisposition::
                    allied,
        "Changed disposition atomically replaces the existing directed cell");

    check(
        state,
        matrix.size() == 1U,
        "Relationship replacement does not increase matrix size");

    const Status reverse_set =
        matrix.set_relationship(
            merchants_citizens_hostile);

    check(
        state,
        reverse_set.has_value() &&
            matrix.size() == 2U &&
            matrix.disposition(
                citizens,
                merchants).has_value() &&
            matrix.disposition(
                citizens,
                merchants).value() ==
                FactionRelationshipDisposition::
                    allied &&
            matrix.disposition(
                merchants,
                citizens).has_value() &&
            matrix.disposition(
                merchants,
                citizens).value() ==
                FactionRelationshipDisposition::
                    hostile,
        "Reverse direction remains an independent matrix cell");

    check(
        state,
        matrix.contains(
            citizens,
            merchants),
        "contains finds an exact directed relationship pair");

    check(
        state,
        !matrix.contains(
            citizens,
            guild),
        "contains returns false for a missing directed pair");

    check(
        state,
        !matrix.contains(
            citizens,
            citizens),
        "contains returns false for a self pair");

    FactionRelationshipMatrix
        disposition_matrix{};

    if (
        !disposition_matrix.
            set_relationship(
                citizens_merchants_neutral).
            has_value() ||
        !disposition_matrix.
            set_relationship(
                citizens_wardens_allied).
            has_value() ||
        !disposition_matrix.
            set_relationship(
                merchants_citizens_hostile).
            has_value())
    {
        return 1;
    }

    const auto neutral_query =
        disposition_matrix.disposition(
            citizens,
            merchants);

    check(
        state,
        neutral_query.has_value() &&
            neutral_query.value() ==
                FactionRelationshipDisposition::
                    neutral,
        "Disposition query preserves explicit neutral");

    const auto allied_query =
        disposition_matrix.disposition(
            citizens,
            wardens);

    check(
        state,
        allied_query.has_value() &&
            allied_query.value() ==
                FactionRelationshipDisposition::
                    allied,
        "Disposition query returns allied");

    const auto hostile_query =
        disposition_matrix.disposition(
            merchants,
            citizens);

    check(
        state,
        hostile_query.has_value() &&
            hostile_query.value() ==
                FactionRelationshipDisposition::
                    hostile,
        "Disposition query returns hostile");

    const auto missing_query =
        disposition_matrix.disposition(
            guild,
            merchants);

    check(
        state,
        !missing_query.has_value() &&
            missing_query.error().code ==
                ErrorCode::not_found,
        "Missing directed pair disposition returns not_found");

    const auto self_query =
        disposition_matrix.disposition(
            citizens,
            citizens);

    check(
        state,
        !self_query.has_value() &&
            self_query.error().code ==
                ErrorCode::invalid_argument,
        "Self-pair disposition query returns invalid_argument");

    check(
        state,
        neutral_query.has_value() &&
            neutral_query.value() ==
                FactionRelationshipDisposition::
                    neutral &&
            !missing_query.has_value() &&
            missing_query.error().code ==
                ErrorCode::not_found,
        "Explicit neutral remains distinguishable from relationship absence");

    FactionRelationshipMatrix
        removal_matrix{};

    if (
        !removal_matrix.
            set_relationship(
                citizens_merchants_allied).
            has_value() ||
        !removal_matrix.
            set_relationship(
                merchants_citizens_hostile).
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
        "Removing a missing directed pair returns not_found");

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
        "Removing a self pair returns invalid_argument");

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
        "Removing an exact directed pair succeeds");

    check(
        state,
        removal_matrix.contains(
            merchants,
            citizens) &&
            removal_matrix.disposition(
                merchants,
                citizens).has_value() &&
            removal_matrix.disposition(
                merchants,
                citizens).value() ==
                FactionRelationshipDisposition::
                    hostile,
        "Removing A-to-B preserves B-to-A");

    FactionRelationshipMatrix
        canonical_matrix{};

    if (
        !canonical_matrix.
            set_relationship(
                citizens_wardens_allied).
            has_value() ||
        !canonical_matrix.
            set_relationship(
                guild_citizens_neutral).
            has_value() ||
        !canonical_matrix.
            set_relationship(
                merchants_citizens_hostile).
            has_value() ||
        !canonical_matrix.
            set_relationship(
                citizens_guild_hostile).
            has_value() ||
        !canonical_matrix.
            set_relationship(
                citizens_merchants_allied).
            has_value())
    {
        return 1;
    }

    const auto missing_source_range =
        canonical_matrix.
            relationships_from(
                wardens);

    check(
        state,
        missing_source_range.empty(),
        "Missing source returns an empty relationship span");

    const auto citizens_range =
        canonical_matrix.
            relationships_from(
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
        "relationships_from returns one contiguous source range");

    check(
        state,
        citizens_range.size() == 3U &&
            citizens_range[0].target() ==
                guild &&
            citizens_range[1].target() ==
                merchants &&
            citizens_range[2].target() ==
                wardens,
        "Source range is canonically ordered by target namespace then name");

    const auto canonical =
        canonical_matrix.
            relationships_in_canonical_order();

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
        "Full canonical order is source namespace/name then target namespace/name");

    FactionRelationshipMatrix
        reverse_history{};

    if (
        !reverse_history.
            set_relationship(
                merchants_citizens_hostile).
            has_value() ||
        !reverse_history.
            set_relationship(
                citizens_merchants_allied).
            has_value() ||
        !reverse_history.
            set_relationship(
                citizens_guild_hostile).
            has_value() ||
        !reverse_history.
            set_relationship(
                guild_citizens_neutral).
            has_value() ||
        !reverse_history.
            set_relationship(
                citizens_wardens_allied).
            has_value())
    {
        return 1;
    }

    check(
        state,
        same_sequence(
            canonical_matrix.
                relationships_in_canonical_order(),
            reverse_history.
                relationships_in_canonical_order()),
        "Opposite insertion histories yield identical canonical relationship order");

    const Status final_replacement =
        matrix.set_relationship(
            citizens_merchants_hostile);

    check(
        state,
        final_replacement.has_value() &&
            matrix.size() == 2U &&
            matrix.disposition(
                citizens,
                merchants).has_value() &&
            matrix.disposition(
                citizens,
                merchants).value() ==
                FactionRelationshipDisposition::
                    hostile &&
            matrix.disposition(
                merchants,
                citizens).has_value() &&
            matrix.disposition(
                merchants,
                citizens).value() ==
                FactionRelationshipDisposition::
                    hostile,
        "Replacing one directed cell does not mutate its reverse cell");

    std::cout
        << state.checks
        << " checks, "
        << state.failures
        << " failures\n";

    return state.failures == 0
        ? 0
        : 1;
}
