#include "oros/ai/faction_relationship.hpp"

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
        oros::ai::FactionRelationship>
    make_relationship_from_ephemeral_keys()
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
                "Faction relationship test "
                "fixture could not create "
                "ephemeral faction keys.");
        }

        return oros::ai::
            FactionRelationship::create(
                source_result.value(),
                target_result.value(),
                oros::ai::
                    FactionRelationshipDisposition::
                        hostile);
    }
}

int main()
{
    using namespace oros::ai;
    using namespace oros::foundation;

    static_assert(
        std::is_same_v<
            std::underlying_type_t<
                FactionRelationshipDisposition>,
            std::uint8_t>);

    static_assert(
        !std::is_default_constructible_v<
            FactionRelationship>);

    static_assert(
        std::is_copy_constructible_v<
            FactionRelationship>);

    static_assert(
        std::is_copy_assignable_v<
            FactionRelationship>);

    static_assert(
        std::is_nothrow_move_constructible_v<
            FactionRelationship>);

    static_assert(
        std::is_nothrow_move_assignable_v<
            FactionRelationship>);

    static_assert(
        std::is_same_v<
            decltype(
                FactionRelationship::create(
                    std::declval<
                        const FactionKey&>(),
                    std::declval<
                        const FactionKey&>(),
                    std::declval<
                        FactionRelationshipDisposition>())),
            Result<FactionRelationship>>);

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const FactionRelationship&>().
                    source()),
            const FactionKey&>);

    static_assert(
        noexcept(
            std::declval<
                const FactionRelationship&>().
                source()));

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const FactionRelationship&>().
                    target()),
            const FactionKey&>);

    static_assert(
        noexcept(
            std::declval<
                const FactionRelationship&>().
                target()));

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const FactionRelationship&>().
                    disposition()),
            FactionRelationshipDisposition>);

    static_assert(
        noexcept(
            std::declval<
                const FactionRelationship&>().
                disposition()));

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

    const auto neutral_result =
        FactionRelationship::create(
            source,
            target,
            FactionRelationshipDisposition::
                neutral);

    check(
        state,
        neutral_result.has_value(),
        "Neutral relationship constructs successfully");

    const auto allied_result =
        FactionRelationship::create(
            source,
            target,
            FactionRelationshipDisposition::
                allied);

    check(
        state,
        allied_result.has_value(),
        "Allied relationship constructs successfully");

    const auto hostile_result =
        FactionRelationship::create(
            source,
            target,
            FactionRelationshipDisposition::
                hostile);

    check(
        state,
        hostile_result.has_value(),
        "Hostile relationship constructs successfully");

    if (
        !neutral_result.has_value() ||
        !allied_result.has_value() ||
        !hostile_result.has_value())
    {
        return 1;
    }

    const FactionRelationship&
        allied =
            allied_result.value();

    check(
        state,
        allied.source() ==
            source,
        "Source accessor preserves relationship source");

    check(
        state,
        allied.target() ==
            target,
        "Target accessor preserves relationship target");

    check(
        state,
        allied.disposition() ==
            FactionRelationshipDisposition::
                allied,
        "Disposition accessor preserves relationship disposition");

    const auto invalid_disposition_result =
        FactionRelationship::create(
            source,
            target,
            FactionRelationshipDisposition::
                invalid);

    check(
        state,
        !invalid_disposition_result.has_value() &&
            invalid_disposition_result.
                error().code ==
                ErrorCode::invalid_argument,
        "Invalid disposition is rejected with invalid_argument");

    const auto unknown_disposition =
        static_cast<
            FactionRelationshipDisposition>(
                0xFFU);

    const auto unknown_disposition_result =
        FactionRelationship::create(
            source,
            target,
            unknown_disposition);

    check(
        state,
        !unknown_disposition_result.has_value() &&
            unknown_disposition_result.
                error().code ==
                ErrorCode::invalid_argument,
        "Unknown cast disposition is rejected with invalid_argument");

    const auto self_relationship_result =
        FactionRelationship::create(
            source,
            source,
            FactionRelationshipDisposition::
                neutral);

    check(
        state,
        !self_relationship_result.has_value() &&
            self_relationship_result.
                error().code ==
                ErrorCode::invalid_argument,
        "Same source and target are rejected with invalid_argument");

    const auto ephemeral_result =
        make_relationship_from_ephemeral_keys();

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
        "Relationship owns source beyond source-key lifetime");

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
        "Relationship owns target beyond target-key lifetime");

    const auto reverse_result =
        FactionRelationship::create(
            target,
            source,
            FactionRelationshipDisposition::
                allied);

    check(
        state,
        reverse_result.has_value() &&
            allied.source() ==
                source &&
            allied.target() ==
                target,
        "A-to-B relationship preserves source-to-target direction");

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
                allied),
        "A-to-B relationship differs from B-to-A relationship");

    const auto equal_result =
        FactionRelationship::create(
            source,
            target,
            FactionRelationshipDisposition::
                allied);

    check(
        state,
        equal_result.has_value() &&
            equal_result.value() ==
                allied,
        "Same endpoints and disposition produce equal relationships");

    check(
        state,
        !(allied ==
            hostile_result.value()),
        "Same endpoints with different disposition are unequal");

    const FactionRelationship
        copied_relationship =
            allied;

    check(
        state,
        copied_relationship ==
            allied,
        "Relationship copy preserves complete value");

    FactionRelationship
        assigned_relationship =
            hostile_result.value();

    assigned_relationship =
        allied;

    check(
        state,
        assigned_relationship ==
            allied,
        "Relationship copy assignment preserves complete value");

    std::cout
        << state.checks
        << " checks, "
        << state.failures
        << " failures\n";

    return state.failures == 0
        ? 0
        : 1;
}
